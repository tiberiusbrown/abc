#include "ards_compiler.hpp"

#include <assert.h>

namespace ards
{

compiler_lvalue_t compiler_t::resolve_lvalue(ast_node_t const& n, compiler_frame_t const& frame)
{
    assert(n.type == AST::IDENT);
    std::string name(n.data);
    for(auto it = frame.scopes.rbegin(); it != frame.scopes.rend(); ++it)
    {
        auto jt = it->locals.find(name);
        if(jt != it->locals.end())
        {
            size_t offset = frame.size - jt->second.frame_offset;
            if(offset >= 256)
                errs.push_back({ "Stack frame exceeded 256 bytes", n.line_info });
            return { jt->second.type.prim_size, false, uint8_t(offset) };
        }
    }
    auto it = globals.find(name);
    if(it != globals.end())
        return { it->second.type.prim_size, true, 0, name };
    errs.push_back({ "Undefined variable \"" + name + "\"", n.line_info });
    return {};
}

void compiler_t::codegen_function(compiler_func_t& f)
{
    compiler_frame_t frame{};

    frame.push();

    assert(f.decl.arg_types.size() == 0);
    // TODO: add func args to scope here

    codegen(f, frame, f.block);

    for(size_t i = 0; i < frame.size; ++i)
        f.instrs.push_back({ I_POP });
    frame.pop();
}

void compiler_t::codegen(compiler_func_t& f, compiler_frame_t& frame, ast_node_t& a)
{
    if(!errs.empty()) return;
    switch(a.type)
    {
    case AST::BLOCK:
        frame.push();
        for(auto& child : a.children)
            codegen(f, frame, child);
        for(size_t i = 0; i < frame.scopes.back().size; ++i)
            f.instrs.push_back({ I_POP });
        frame.pop();
        break;
    case AST::EXPR_STMT:
        assert(a.children.size() == 1);
        type_annotate(a.children[0], frame);
        codegen_expr(f, frame, a.children[0]);
        break;
    default:
        assert(false);
        errs.push_back({ "(codegen) Unimplemented AST node", a.line_info });
        return;
    }
}

void compiler_t::codegen_store_lvalue(compiler_func_t& f, compiler_lvalue_t const& lvalue)
{
    if(!errs.empty()) return;
    if(lvalue.is_global)
    {
        assert(lvalue.size < 256);
        f.instrs.push_back({ I_PUSH, (uint8_t)lvalue.size });
        f.instrs.push_back({ I_SETGN, 0, lvalue.global_name });
    }
    else
    {
        f.instrs.push_back({ I_PUSH, (uint8_t)lvalue.size });
        f.instrs.push_back({ I_SETLN, lvalue.stack_index });
    }
}

void compiler_t::codegen_convert(compiler_func_t& f, compiler_type_t const& to, compiler_type_t const& from)
{
    assert(from.prim_size != 0);
    if(to.prim_size == from.prim_size) return;
    if(to.prim_size < from.prim_size)
    {
        int n = from.prim_size - to.prim_size;
        for(int i = 0; i < n; ++i)
            f.instrs.push_back({ I_POP });
    }
    if(to.prim_size > from.prim_size)
    {
        int n = to.prim_size - from.prim_size;
        compiler_instr_t instr;
        if(from.prim_signed)
            instr = { I_SEXT };
        else
            instr = { I_PUSH, 0 };
        for(int i = 0; i < n; ++i)
            f.instrs.push_back(instr);
    }
}

void compiler_t::codegen_expr(compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a)
{
    if(!errs.empty()) return;
    switch(a.type)
    {
    case AST::INT_CONST:
    {
        uint32_t x = (uint32_t)a.value;
        for(size_t i = 0; i < a.comp_type.prim_size; ++i, x >>= 8)
            f.instrs.push_back({ I_PUSH, (uint8_t)x });
        return;
    }
    case AST::IDENT:
    {
        std::string name(a.data);
        for(auto it = frame.scopes.rbegin(); it != frame.scopes.rend(); ++it)
        {
            auto jt = it->locals.find(name);
            if(jt != it->locals.end())
            {
                assert(false);
                return;
            }
        }
        auto it = globals.find(name);
        if(it != globals.end())
        {
            assert(it->second.type.prim_size < 256);
            f.instrs.push_back({ I_PUSH, (uint8_t)it->second.type.prim_size });
            f.instrs.push_back({ I_GETGN, 0, it->second.name });
            return;
        }
        errs.push_back({ "Undefined variable \"" + name + "\"", a.line_info });
        return;
    }
    case AST::FUNC_CALL:
    {
        assert(a.children.size() == 2);
        assert(a.children[0].type == AST::IDENT);

        // reserve space for return value
        for(size_t i = 0; i < a.comp_type.prim_size; ++i)
            f.instrs.push_back({ I_PUSH, 0 });

        auto func = resolve_func(a.children[0]);

        // TODO: push args
        assert(a.children[1].type == AST::FUNC_ARGS);
        if(a.children[1].children.size() != func.decl.arg_types.size())
        {
            errs.push_back({
                "Incorrect number of arguments to function \"" + func.name + "\"",
                a.line_info });
            return;
        }
        for(size_t i = 0; i < func.decl.arg_types.size(); ++i)
        {
            auto const& type = func.decl.arg_types[i];
            auto const& expr = a.children[1].children[i];
            codegen_expr(f, frame, expr);
            codegen_convert(f, type, expr.comp_type);
        }

        if(func.is_sys)
            f.instrs.push_back({ I_SYS, func.sys });
        else
            f.instrs.push_back({ I_CALL, 0, std::string(a.children[0].data) });
        return;
    }
    case AST::OP_ASSIGN:
    {
        assert(a.children.size() == 2);
        assert(a.children[0].comp_type.prim_size != 0);
        auto lvalue = resolve_lvalue(a.children[0], frame);
        codegen_expr(f, frame, a.children[1]);
        codegen_convert(f, a.children[0].comp_type, a.children[1].comp_type);
        codegen_store_lvalue(f, lvalue);
        return;
    }
    case AST::OP_ADDITIVE:
    {
        assert(a.data == "+" || a.data == "-");
        assert(a.children.size() == 2);
        codegen_expr(f, frame, a.children[0]);
        codegen_convert(f, a.comp_type, a.children[0].comp_type);
        codegen_expr(f, frame, a.children[1]);
        codegen_convert(f, a.comp_type, a.children[1].comp_type);
        switch(a.comp_type.prim_size)
        {
        case 1:
            f.instrs.push_back({ a.data == "+" ? I_ADD : I_SUB });
            break;
        case 2:
            f.instrs.push_back({ a.data == "+" ? I_ADD2 : I_SUB2 });
            break;
        default:
            assert(false);
            break;
        }
        return;
    }
    default:
        assert(false);
        errs.push_back({ "(codegen_expr) Unimplemented AST node", a.line_info });
        return;
    }
}

}
