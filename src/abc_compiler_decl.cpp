#include "abc_compiler.hpp"

namespace abc
{

void compiler_t::decl(compiler_func_t& f, compiler_frame_t& frame, ast_node_t& n)
{
    assert(n.type == AST::DECL_STMT);
    assert(n.children.size() == 2 || n.children.size() == 3);
    assert(n.children[1].type == AST::IDENT);

    bool is_global = frame.scopes.empty();

    if(n.children.size() <= 2 &&
    (n.children[0].comp_type.is_constexpr ||
        n.children[0].comp_type.is_prog))
    {
        errs.push_back({
            "Prog and constexpr variables must be initialized",
            n.line_info });
        return;
    }
    assert(n.children[1].type == AST::IDENT);
    if(!check_identifier(n.children[1])) return;
    std::string name(n.children[1].data);
    if(is_global && symbol_exists(name))
    {
        errs.push_back({
            "Duplicate symbol \"" + name + "\"",
            n.children[1].line_info });
        return;
    }
    bool saved = n.children[0].comp_type.is_saved;
    compiler_global_t* g = nullptr;
    compiler_local_t* local = nullptr;
    compiler_var_t* v = nullptr;

    if(is_global)
    {
        // global var
        g = &globals[name];
        g->name = name;
        g->saved = saved;
        v = &g->var;
    }
    else
    {
        // local var
        auto& scope = frame.scopes.back();
        local = &scope.locals[std::string(n.children[1].data)];
        local->frame_offset = frame.size;
        v = &local->var;
    }

    assert(v);
    if(!v) return;

    assert(g || local);
    if(!g && !local) return;

    std::string constexpr_ref;
    type_annotate(n.children[0], frame);
    v->type = resolve_type(n.children[0]);
    if(n.children.size() == 3)
    {
        if(!v->type.is_any_ref() &&
            !n.children[0].comp_type.is_constexpr &&
            n.children[2].type != AST::COMPOUND_LITERAL)
            n.children[2].insert_cast(v->type);
        type_annotate(n.children[2], frame);
    }
    if(saved && (v->type.is_any_ref() || v->type.has_child_ref()))
    {
        errs.push_back({
            "References and objects containing references cannot be declared 'saved'",
            n.line_info });
        return;
    }
    if(!is_global && saved)
    {
        errs.push_back({
            "Local variables may not be declared 'saved'",
            n.line_info });
        return;
    }
    if(!is_global && v->type.is_prog)
    {
        errs.push_back({
            "Prog variables must be global",
            n.line_info });
        return;
    }
    if(n.children.size() <= 2 && v->type.is_any_ref())
    {
        errs.push_back({
            "Uninitialized reference \"" + std::string(n.children[1].data) + "\"",
            n.line_info });
        return;
    }
    if(v->type.is_ref() && v->type.children[0].is_array() &&
        v->type.children[0].children[0].is_byte)
    {
        auto const& t = n.children[2].comp_type.without_ref();
        if(!t.is_copyable())
        {
            errs.push_back({
                "Cannot create byte array reference from non-copyable type",
                n.line_info });
            return;
        }
        if(v->type.children[0].prim_size > t.prim_size)
        {
            errs.push_back({
                "Cannot create byte array reference: array size too large",
                n.line_info });
            return;
        }
    }
    else if(v->type.is_ref() &&
        v->type.without_ref() != n.children[2].comp_type.without_ref())
    {
        errs.push_back({
            "Incorrect type for reference \"" + std::string(n.children[1].data) + "\"",
            n.line_info });
        return;
    }
    if(n.children.size() <= 2 && v->type.has_child_ref())
    {
        errs.push_back({
            "Uninitialized child reference(s) in \"" + std::string(n.children[1].data) + "\"",
            n.line_info });
        return;
    }
    if(!errs.empty()) return;
    if(v->type.prim_size == 0)
    {
        errs.push_back({
            "Variable \"" + name + "\" has zero size",
            n.line_info });
        return;
    }
    if(!is_global && v->type.prim_size >= 256)
    {
        errs.push_back({
            "Local variable \"" + std::string(n.children[1].data) + "\" is too large",
            n.line_info });
        return;
    }

    if(v->type.is_prog)
        add_progdata(name, v->type, n.children[2]);

    // constexpr primitive
    else if(n.children[0].comp_type.is_constexpr)
    {
        v->is_constexpr = true;
        if(n.children[2].type == AST::INT_CONST)
        {
            if(v->type.is_float)
                v->fvalue = (double)n.children[2].value;
            else
                v->value = n.children[2].value;
        }
        else if(n.children[2].type == AST::FLOAT_CONST)
        {
            if(v->type.is_float)
                v->fvalue = n.children[2].fvalue;
            else
                v->value = (int64_t)n.children[2].fvalue;
        }
        else if(v->type.is_label_ref())
        {
            v->label_ref = resolve_label_ref({}, n.children[2], v->type);
        }
        else
        {
            errs.push_back({
                "\"" + std::string(n.children[2].data) +
                "\" is not a constant expression",
                n.children[2].line_info });
            return;
        }
    }

    // constexpr-ify reference
    else if(is_global && v->type.is_ref() && n.children[2].type == AST::IDENT)
    {
        g->constexpr_ref = std::string(n.children[2].data);
    }

    // main codegen
    else if(n.children.size() == 3)
    {
        auto const& src_node =
            n.children[2].type == AST::OP_CAST ?
            n.children[2].children[1] :
            n.children[2];
        auto const& src_type = src_node.comp_type;

        // optimize as memcpy or strcpy
        if(v->type.is_copyable() &&
            src_type.is_any_ref() &&
            src_type.without_ref().is_copyable() &&
            v->type.prim_size >= memcpy_min_bytes)
        {
            bool prog = src_type.without_ref().is_prog || src_type.is_prog_array();

            if(!is_global)
            {
                // allocate space on stack
                uint8_t alloc = uint8_t(v->type.without_ref().prim_size);
                f.instrs.push_back({ I_ALLOC, n.line(), alloc });
                frame.scopes.back().size += v->type.prim_size;
                frame.size += v->type.prim_size;
            }

            codegen_expr(f, frame, src_node, false);
            codegen_convert(
                f, frame, src_node,
                prog ? TYPE_BYTE_PROG_AREF : TYPE_BYTE_AREF,
                src_type);

            type_annotate(n.children[1], frame);
            codegen_expr(f, frame, n.children[1], true);
            codegen_convert(
                f, frame, n.children[1],
                TYPE_BYTE_AREF,
                n.children[1].comp_type);

            bool is_string = v->type.is_string() && src_type.is_string();
            sysfunc_t sys = prog ? SYS_MEMCPY_P : SYS_MEMCPY;
            if(is_string)
                sys = prog ? SYS_STRCPY_P : SYS_STRCPY;
            f.instrs.push_back({ I_SYS, n.line(), sys });
            frame.size -= 4;
            frame.size -= (prog ? 6 : 4);
            if(is_string)
            {
                // pop return value of strcpy
                f.instrs.push_back({ I_POPN, n.line(), 4 });
            }
            return;
        }

        if(is_global)
            frame.push();

        if(v->type.is_any_ref() && n.children[2].type == AST::COMPOUND_LITERAL)
        {
            errs.push_back({
                "Cannot create reference to expression",
                n.line_info });
            return;
        }

        if(!v->type.is_any_ref() && n.children[2].type == AST::COMPOUND_LITERAL)
            codegen_expr_compound(f, frame, n.children[2], v->type);
        else
            codegen_expr(f, frame, n.children[2], v->type.is_ref());
        if(!errs.empty()) return;

        auto const& t0 = v->type.without_ref();
        auto const& t1 = n.children[2].comp_type.without_ref();
        if(v->type.is_ref())
        {
            if(t0 != t1 && !(t0.is_array() && t0.children[0].is_byte))
            {
                errs.push_back({
                    "Incorrect type for reference \"" +
                    std::string(n.children[1].data) + "\"",
                    n.line_info });
                return;
            }
        }
        else if(v->type.is_array_ref())
        {
            codegen_convert(f, frame, n.children[2], v->type, n.children[2].comp_type);
        }
        else if(n.children[2].type != AST::COMPOUND_LITERAL)
        {
            if(v->type.is_array() && t0.children[0].without_prog() != t1.children[0].without_prog())
            {
                errs.push_back({
                    "Incompatible type for assignment to \"" +
                    std::string(n.children[1].data) + "\"",
                    n.line_info });
                return;
            }
            codegen_convert(f, frame, n.children[2], v->type, n.children[2].comp_type);
        }

        if(is_global)
        {
            type_annotate(n.children[1], frame);
            codegen_expr(f, frame, n.children[1], true);
            codegen_store(f, frame, n.children[1]);
            f.instrs.push_back({ I_POPN, n.line(), uint8_t(frame.size) });
            frame.pop();
        }
    }

    else if(!is_global)
    {
        // zero-initialize local
        auto size = v->type.prim_size;
        frame.size += size;
        for(size_t i = 0; i < size; ++i)
            f.instrs.push_back({ I_PUSH, n.line(), 0 });
    }

    if(!is_global && !v->type.is_constexpr)
        frame.scopes.back().size += v->type.prim_size;

}

}
