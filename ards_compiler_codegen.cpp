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
            return { false, uint8_t(offset) };
        }
    }
    auto it = globals.find(name);
    if(it != globals.end())
        return { true, 0, name };
    errs.push_back({ "Undefined variable \"" + name + "\"", n.line_info });
    return {};
}

void compiler_t::codegen_function(compiler_func_t& f)
{
    compiler_frame_t frame{};

    frame.push();
    // TODO: add func args to scope here

    codegen(f, frame, f.block);
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
    assert(false);
}

void compiler_t::codegen_expr(compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a)
{
    if(!errs.empty()) return;
    switch(a.type)
    {
    case AST::OP_ASSIGN:
    {
        assert(a.children.size() == 2);
        auto lvalue = resolve_lvalue(a.children[0], frame);
        codegen_expr(f, frame, a.children[1]);
        codegen_store_lvalue(f, lvalue);
        break;
    }
    case AST::OP_ADDITIVE:
    {
        assert(false);
        break;
    }
    default:
        assert(false);
        errs.push_back({ "(codegen_expr) Unimplemented AST node", a.line_info });
        return;
    }
}

}
