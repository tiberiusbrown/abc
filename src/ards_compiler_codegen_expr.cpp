#include "ards_compiler.hpp"

#include <assert.h>

namespace ards
{

void compiler_t::resolve_format_call(
    ast_node_t const& n, compiler_func_decl_t const& decl,
    std::vector<compiler_type_t>& arg_types, std::string& fmt)
{
    assert(decl.arg_types.size() >= 1);
    assert(n.children.size() >= decl.arg_types.size());
    std::string_view d;
    {
        auto const& nc = n.children[decl.arg_types.size() - 1];
        d = nc.type == AST::STRING_LITERAL ? nc.data : nc.children[0].data;
    }

    fmt.clear();
    arg_types = decl.arg_types;

    for(auto it = d.begin(); it != d.end(); ++it)
    {
        char c = *it;
        fmt += c;
        if(c != '%')
            continue;
        if(++it == d.end())
        {
            errs.push_back({
                "Trailing '%' in format string",
                n.line_info });
            return;
        }
        c = *it;
        fmt += c;
        switch(c)
        {
        case '%':
            fmt += c;
            break;
        case 'd': arg_types.push_back(TYPE_I32); break;
        case 'u':
        case 'x': arg_types.push_back(TYPE_U32); break;
        case 'c': arg_types.push_back(TYPE_CHAR); break;
        case 's':
        {
            size_t i = arg_types.size();
            auto const& t = n.children[i].comp_type.without_ref();
            bool is_prog = t.is_prog;
            if(t.is_array_ref() && t.children[0].is_prog) is_prog = true;
            // TODO: optionally pop back and replace with specifier for prog str
            arg_types.push_back(TYPE_STR);
            break;
        }
        default:
            errs.push_back({
                std::string("Unknown format specifier: '%") + c + "'",
                n.line_info });
            return;
        }
    }
}

void compiler_t::codegen_expr(
    compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a, bool ref)
{
    if(!errs.empty()) return;
    switch(a.type)
    {

    case AST::OP_INC_POST:
    case AST::OP_DEC_POST:
    {
        auto const& t = a.children[0].comp_type.without_ref();
        auto size = t.prim_size;

        if(size < 1 || size > 4 || !t.is_prim())
        {
            errs.push_back({
                "Post-increment and post-decrement can only operate on primitive numeric types",
                a.line_info });
            return;
        }

        // create reference
        codegen_expr(f, frame, a.children[0], true);

        // special instructions for post-inc/dec
        f.instrs.push_back({
            instr_t((a.type == AST::OP_INC_POST ? I_PINC : I_PDEC) + (size - 1)),
            a.children[0].line() });
        frame.size -= 2;
        frame.size += size;

        return;
    }

    case AST::OP_ASSIGN_COMPOUND:
    {
        auto size = a.children[0].comp_type.prim_size;
        bool non_root = a.parent->type != AST::EXPR_STMT && a.parent->type != AST::LIST;

        // create reference
        codegen_expr(f, frame, a.children[0], true);
        // dup the ref
        f.instrs.push_back({ I_PUSH, a.children[0].line(), 2 });
        f.instrs.push_back({ I_GETLN, a.children[0].line(), 2 });
        frame.size += 2;

        // perform operation
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(f, frame, a.children[1], a.comp_type, a.children[1].comp_type);

        // dupe the reference again
        f.instrs.push_back({ I_PUSH, a.children[0].line(), 2 });
        f.instrs.push_back({ I_GETLN, a.children[0].line(), uint8_t(size + 2) });
        frame.size += 2;
        // assign to the reference
        f.instrs.push_back({ I_SETRN, a.line(), (uint8_t)size });
        frame.size -= 2;
        frame.size -= size;

        // if the root op, pop the ref
        // if not the root op, deref
        if(non_root)
        {
            f.instrs.push_back({ I_GETRN, a.line(), (uint8_t)size });
            frame.size -= 2;
            frame.size += size;
        }
        else
        {
            f.instrs.push_back({ I_POP, a.line() });
            f.instrs.push_back({ I_POP, a.line() });
            frame.size -= 2;
        }
        return;
    }

    case AST::OP_COMPOUND_ASSIGNMENT_DEREF:
    {
        // nothing -- already dereferenced in OP_ASSIGN_COMPOUND
        return;
    }

    case AST::OP_CAST:
    {
        if(ref) goto rvalue_error;
        assert(a.children.size() == 2);
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(f, frame, a, a.children[0].comp_type, a.children[1].comp_type);
        return;
    }

    case AST::OP_UNARY:
    {
        if(ref) goto rvalue_error;
        assert(a.children.size() == 2);
        auto op = a.children[0].data;
        if(op == "!")
        {
            codegen_expr(f, frame, a.children[1], false);
            codegen_convert(f, frame, a, TYPE_BOOL, a.children[1].comp_type);
            f.instrs.push_back({ I_NOT, a.children[1].line() });
        }
        else if(op == "-")
        {
            auto size = a.children[1].comp_type.prim_size;
            for(size_t i = 0; i < size; ++i)
                f.instrs.push_back({ I_PUSH, a.children[1].line(), 0 });
            frame.size += size;
            codegen_expr(f, frame, a.children[1], false);
            codegen_convert(f, frame, a, a.comp_type, a.children[1].comp_type);
            f.instrs.push_back({ instr_t(I_SUB + size - 1), a.children[1].line() });
            frame.size -= size;
        }
        else if(op == "~")
        {
            auto size = a.children[1].comp_type.prim_size;
            codegen_expr(f, frame, a.children[1], false);
            f.instrs.push_back({ instr_t(I_COMP + size - 1), a.children[1].line() });
        }
        else
        {
            assert(false);
        }
        return;
    }

    case AST::INT_CONST:
    {
        if(ref) goto rvalue_error;
        uint32_t x = (uint32_t)a.value;
        auto size = a.comp_type.prim_size;
        frame.size += size;
        for(size_t i = 0; i < size; ++i, x >>= 8)
            f.instrs.push_back({ I_PUSH, a.line(), (uint8_t)x });
        return;
    }

    case AST::LABEL_REF:
    {
        f.instrs.push_back({ I_PUSHL, a.line(), 0, 0, std::string(a.data) });
        frame.size += 3;
        return;
    }

    case AST::IDENT:
    {
        std::string name(a.data);
        if(auto* local = resolve_local(frame, a))
        {
            assert(!local->var.is_constexpr);
            uint8_t offset = (uint8_t)(frame.size - local->frame_offset);
            uint8_t size = (uint8_t)local->var.type.prim_size;
            if(ref && !local->var.type.is_ref())
            {
                f.instrs.push_back({ I_REFL, a.line(), offset });
                frame.size += 2;
                return;
            }
            assert(!local->var.type.is_prog);
            f.instrs.push_back({ I_PUSH, a.line(), size });
            f.instrs.push_back({ I_GETLN, a.line(), offset });
            frame.size += (uint8_t)local->var.type.prim_size;
            return;
        }
        if(auto* global = resolve_global(a))
        {
            assert(!global->var.is_constexpr);
            bool prog = global->var.type.is_prog;
            if(global->is_constexpr_ref())
            {
                if(prog)
                    f.instrs.push_back({ I_PUSHL, a.line(), 0, 0, global->constexpr_ref });
                else
                    f.instrs.push_back({ I_PUSHG, a.line(), 0, 0, global->constexpr_ref });
                frame.size += (prog ? 3 : 2);
                return;
            }
            if(ref && prog)
            {
                f.instrs.push_back({ I_PUSHL, a.line(), 0, 0, global->name });
                frame.size += 3;
                return;
            }
            if(ref && !prog)
            {
                f.instrs.push_back({ I_REFG, a.line(), 0, 0, global->name });
                frame.size += 2;
                return;
            }
            assert(global->var.type.prim_size < 256);
            frame.size += (uint8_t)global->var.type.prim_size;
            if(prog)
            {
                f.instrs.push_back({ I_PUSHL, a.line(), 0, 0, global->name });
                f.instrs.push_back({ I_GETPN, a.line(), (uint8_t)global->var.type.prim_size });
            }
            else
            {
                f.instrs.push_back({ I_PUSH, a.line(), (uint8_t)global->var.type.prim_size });
                f.instrs.push_back({ I_GETGN, a.line(), 0, 0, global->name });
            }
            return;
        }
        errs.push_back({ "Undefined variable \"" + name + "\"", a.line_info });
        return;
    }

    case AST::FUNC_CALL:
    {
        if(ref) goto rvalue_error;
        assert(a.children.size() == 2);
        assert(a.children[0].type == AST::IDENT);

        auto func = resolve_func(a.children[0]);
        bool is_format = sysfunc_is_format(func.name);

        // TODO: test for reference return type (not allowed)

        // system functions don't need space reserved for return value
        if(!func.is_sys)
        {
            // reserve space for return value
            frame.size += func.decl.return_type.prim_size;
            for(size_t i = 0; i < func.decl.return_type.prim_size; ++i)
                f.instrs.push_back({ I_PUSH, a.line(), 0 });
        }

        assert(a.children[1].type == AST::FUNC_ARGS);
        if(!is_format && a.children[1].children.size() != func.decl.arg_types.size())
        {
            errs.push_back({
                "Incorrect number of arguments to function \"" + func.name + "\"",
                a.line_info });
            return;
        }

        auto* arg_types = &func.decl.arg_types;
        std::vector<compiler_type_t> format_types;
        std::string format_str;
        if(is_format)
        {
            resolve_format_call(a.children[1], func.decl, format_types, format_str);
            arg_types = &format_types;
        }

        // function args in reverse order
        size_t prev_size = frame.size;
        for(size_t i = arg_types->size(); i != 0; --i)
        {
            if(!errs.empty())
                return;
            auto const& type = (*arg_types)[i - 1];
            auto const& expr = a.children[1].children[i - 1];
            bool tref = type.is_ref();
            if(tref && type.without_ref() != expr.comp_type.without_ref())
            {
                errs.push_back({
                    "Cannot create reference to expression",
                    expr.line_info });
                return;
            }
            if(expr.type == AST::COMPOUND_LITERAL)
                codegen_expr_compound(f, frame, expr, type);
            else if(is_format && i == func.decl.arg_types.size())
            {
                // special handling for format string
                assert(expr.type == AST::OP_AREF);
                assert(expr.children[0].type == AST::STRING_LITERAL);

                std::string label = progdata_label();
                std::vector<uint8_t> data(format_str.begin(), format_str.end());
                add_custom_progdata(label, data);
                f.instrs.push_back({ I_PUSHL, a.line(), 0, 0, label });
                f.instrs.push_back({ I_PUSH, a.line(), uint8_t(format_str.size() >> 0) });
                f.instrs.push_back({ I_PUSH, a.line(), uint8_t(format_str.size() >> 8) });
                f.instrs.push_back({ I_PUSH, a.line(), uint8_t(format_str.size() >> 16) });
                frame.size += 6;
            }
            else
            {
                codegen_expr(f, frame, expr, tref);
                if(!type.is_any_ref())
                    codegen_convert(f, frame, a, type, expr.comp_type);
            }
        }
        // called function should pop stack
        frame.size = prev_size;

        if(func.is_sys)
            f.instrs.push_back({ I_SYS, a.line(), func.sys });
        else
            f.instrs.push_back({ I_CALL, a.line(), 0, 0, std::string(a.children[0].data) });

        // system functions push return value onto stack
        if(func.is_sys)
            frame.size += func.decl.return_type.prim_size;

        return;
    }

    case AST::OP_ASSIGN:
    {
        assert(a.children.size() == 2);
        assert(a.children[0].comp_type.prim_size != 0);
        if(a.children[0].comp_type.has_child_ref())
        {
            errs.push_back({
                "\"" + std::string(a.children[0].data) + "\" contains references "
                "and thus cannot be reassigned", a.children[0].line_info });
            return;
        }
        auto const& type_noref = a.children[0].comp_type.without_ref();
        if(type_noref.is_prim() || type_noref.is_label_ref())
        {
            codegen_expr(f, frame, a.children[1], false);
            codegen_convert(f, frame, a, a.children[0].comp_type, a.children[1].comp_type);
        }
        else if(type_noref.type == compiler_type_t::ARRAY)
        {
            if(a.children[1].type == AST::COMPOUND_LITERAL)
                codegen_expr_compound(f, frame, a.children[1], type_noref);
            else if(
                !a.children[1].comp_type.without_ref().is_array() ||
                type_noref.children[0] !=
                a.children[1].comp_type.without_ref().children[0].without_prog())
            {
                errs.push_back({
                    "Incompatible types in assignment to \"" +
                    std::string(a.children[0].data) + "\"",
                    a.line_info });
                return;
            }
#if 0
            else if(type_noref.children[0] == TYPE_CHAR)
            {
                // TODO: convert char array assignment to strcpy or strcpy_P?
            }
#endif
            else
            {
                codegen_expr(f, frame, a.children[1], false);
                if(a.children[1].comp_type.is_ref())
                    codegen_dereference(f, frame, a, a.children[1].comp_type.without_ref());
                codegen_convert(f, frame, a, type_noref, a.children[1].comp_type.without_ref());
            }
        }

        // dup value if not the root op
        switch(a.parent->type)
        {
        case AST::EXPR_STMT:
        case AST::LIST:
            break;
        default:
            frame.size += (uint8_t)a.children[0].comp_type.prim_size;
            f.instrs.push_back({ I_PUSH, a.line(), (uint8_t)a.children[0].comp_type.prim_size });
            f.instrs.push_back({ I_GETLN, a.line(), (uint8_t)a.children[0].comp_type.prim_size });
            break;
        }

        auto lvalue = resolve_lvalue(f, frame, a.children[0]);
        codegen_store_lvalue(f, frame, lvalue);
        return;
    }

    case AST::OP_EQUALITY:
    case AST::OP_RELATIONAL:
    {
        if(ref) goto rvalue_error;
        assert(a.children.size() == 2);
        if( a.children[0].comp_type.without_prog().without_ref().without_prog() !=
            a.children[1].comp_type.without_prog().without_ref().without_prog())
        {
            errs.push_back({ "Incompatible types in comparison", a.line_info });
            return;
        }
        assert(a.comp_type == TYPE_BOOL);
        size_t i0 = 0, i1 = 1;
        if(a.data == ">" || a.data == ">=")
            std::swap(i0, i1);
        codegen_expr(f, frame, a.children[i0], false);
        codegen_expr(f, frame, a.children[i1], false);

        auto size = a.children[0].comp_type.prim_size;
        assert(size >= 1 && size <= 4);
        frame.size -= size;       // comparison
        frame.size -= (size - 1); // conversion to bool
        if(a.data == "==" || a.data == "!=")
        {
            f.instrs.push_back({ instr_t(I_SUB + size - 1), a.line() });
            f.instrs.push_back({ instr_t(I_BOOL + size - 1), a.line() });
            if(a.data == "==")
                f.instrs.push_back({ I_NOT, a.line() });
        }
        else if(a.data == "<=" || a.data == ">=")
        {
            instr_t i = (a.children[0].comp_type.is_signed ? I_CSLE : I_CULE);
            f.instrs.push_back({ instr_t(i + size - 1), a.line() });
        }
        else if(a.data == "<" || a.data == ">")
        {
            instr_t i = (a.children[0].comp_type.is_signed ? I_CSLT : I_CULT);
            f.instrs.push_back({ instr_t(i + size - 1), a.line() });
        }
        else
            assert(false);
        return;
    }

    case AST::OP_ADDITIVE:
    {
        if(ref) goto rvalue_error;
        assert(a.data == "+" || a.data == "-");
        assert(a.children.size() == 2);
        codegen_expr(f, frame, a.children[0], false);
        codegen_convert(f, frame, a, a.comp_type, a.children[0].comp_type);
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(f, frame, a, a.comp_type, a.children[1].comp_type);
        static_assert(I_ADD2 == I_ADD + 1);
        static_assert(I_ADD3 == I_ADD + 2);
        static_assert(I_ADD4 == I_ADD + 3);
        static_assert(I_SUB2 == I_SUB + 1);
        static_assert(I_SUB3 == I_SUB + 2);
        static_assert(I_SUB4 == I_SUB + 3);
        auto size = a.comp_type.prim_size;
        assert(size >= 1 && size <= 4);
        frame.size -= size;
        f.instrs.push_back({ instr_t((a.data == "+" ? I_ADD : I_SUB) + size - 1), a.line() });
        return;
    }

    case AST::OP_MULTIPLICATIVE:
    {
        if(ref) goto rvalue_error;
        assert(a.children.size() == 2);
        //assert(a.children[0].comp_type == a.comp_type);
        //assert(a.children[1].comp_type == a.comp_type);
        codegen_expr(f, frame, a.children[0], false);
        codegen_convert(f, frame, a, a.comp_type, a.children[0].comp_type);
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(f, frame, a, a.comp_type, a.children[1].comp_type);
        static_assert(I_MUL2 == I_MUL + 1);
        static_assert(I_MUL3 == I_MUL + 2);
        static_assert(I_MUL4 == I_MUL + 3);
        auto size = a.comp_type.prim_size;
        assert(size >= 1 && size <= 4);
        frame.size -= size;
        if(a.data == "*")
            f.instrs.push_back({ instr_t(I_MUL + size - 1), a.line() });
        else if(a.data == "/")
        {
            auto tsize = a.comp_type.prim_size;
            assert(tsize == 2 || tsize == 4);
            if(a.comp_type.is_signed)
                f.instrs.push_back({ tsize == 2 ? I_DIV2 : I_DIV4, a.line() });
            else
                f.instrs.push_back({ tsize == 2 ? I_UDIV2 : I_UDIV4, a.line() });
        }
        else if(a.data == "%")
        {
            auto tsize = a.comp_type.prim_size;
            assert(tsize == 2 || tsize == 4);
            if(a.comp_type.is_signed)
                f.instrs.push_back({ tsize == 2 ? I_MOD2 : I_MOD4, a.line() });
            else
                f.instrs.push_back({ tsize == 2 ? I_UMOD2 : I_UMOD4, a.line() });
        }
        else
            assert(false);
        return;
    }

    case AST::OP_SHIFT:
    {
        codegen_expr(f, frame, a.children[0], false);
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(f, frame, a, TYPE_U8, a.children[1].comp_type);
        frame.size -= 1;
        auto index = a.comp_type.prim_size - 1;
        if(a.data == "<<")
            f.instrs.push_back({ instr_t(I_LSL + index), a.line() });
        else if(a.data == ">>")
        {
            if(a.comp_type.is_signed)
                f.instrs.push_back({ instr_t(I_ASR + index), a.line() });
            else
                f.instrs.push_back({ instr_t(I_LSR + index), a.line() });
        }
        else
            assert(false);
        return;
    }

    case AST::OP_BITWISE_AND:
    case AST::OP_BITWISE_OR:
    case AST::OP_BITWISE_XOR:
    {
        codegen_expr(f, frame, a.children[0], false);
        codegen_convert(f, frame, a, a.comp_type, a.children[1].comp_type);
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(f, frame, a, a.comp_type, a.children[1].comp_type);
        auto size = a.comp_type.prim_size;
        frame.size -= size;
        if(a.type == AST::OP_BITWISE_AND)
            f.instrs.push_back({ instr_t(I_AND + size - 1), a.line() });
        else if(a.type == AST::OP_BITWISE_OR)
            f.instrs.push_back({ instr_t(I_OR + size - 1), a.line() });
        else if(a.type == AST::OP_BITWISE_XOR)
            f.instrs.push_back({ instr_t(I_XOR + size - 1), a.line() });
        return;
    }

    case AST::ARRAY_INDEX:
    {
        // TODO: optimize the case where children[0] is an ident and children[1] is
        //       an integer constant, directly adjust offset given to REFL/REFG
        //       instruction instead of below code path

        auto const& t = a.children[0].comp_type.without_ref();
        bool prog = t.is_array_ref() ? t.children[0].is_prog : t.is_prog;

        // construct [unsized] array reference
        if(t.is_array_ref())
            codegen_expr(f, frame, a.children[0], false);
        else
            codegen_expr(f, frame, a.children[0], true);

        // construct index
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(
            f, frame, a,
            prog ? TYPE_U24 : TYPE_U16,
            a.children[1].comp_type);

        size_t elem_size = t.children[0].prim_size;
        if(t.is_array_ref())
        {
            f.instrs.push_back({
                prog ? I_UPIDX : I_UAIDX, a.line(),
                (uint16_t)elem_size });
            frame.size -= prog ? 6 : 4;
        }
        else
        {
            size_t size = t.prim_size;
            size_t num_elems = size / elem_size;
            f.instrs.push_back({
                prog ? I_PIDX : I_AIDX, a.line(),
                (uint16_t)elem_size, (uint32_t)num_elems });
            frame.size -= prog ? 3 : 2;
        }
        // if the child type is a reference, dereference it now
        // for example, a[i] where a is T&[N]
        if(t.children[0].is_ref())
            codegen_dereference(f, frame, a, t.children[0]);
        return;
    }

    case AST::STRUCT_MEMBER:
    {
        std::string name = std::string(a.children[1].data);
        size_t offset = 0;
        auto const* t = resolve_member(a.children[0], name, &offset);
        if(!t) return;
        codegen_expr(f, frame, a.children[0], true);
        if(offset != 0)
        {
            f.instrs.push_back({ I_PUSH, a.children[1].line(), uint8_t(offset >> 0) });
            f.instrs.push_back({ I_PUSH, a.children[1].line(), uint8_t(offset >> 8) });
            if(t->is_prog)
            {
                f.instrs.push_back({ I_PUSH, a.children[1].line(), uint8_t(offset >> 16) });
                f.instrs.push_back({ I_ADD3, a.children[1].line() });
            }
            else
            {
                f.instrs.push_back({ I_ADD2, a.children[1].line() });
            }
        }
        // if the child type is a reference, dereference it now
        // for example, player.hp where hp is int&
        if(t->is_ref())
            codegen_dereference(f, frame, a, t->children[0]);
        return;
    }

    case AST::OP_LOGICAL_AND:
    case AST::OP_LOGICAL_OR:
    {
        std::string sc_label = new_label(f);
        codegen_expr_logical(f, frame, a, sc_label);
        f.instrs.push_back({ I_NOP, 0, 0, 0, sc_label, true });
        return;
    }

    case AST::SPRITES:
    {
        std::string label = progdata_label();
        add_progdata(label, TYPE_SPRITES, a);
        f.instrs.push_back({ I_PUSHL, a.line(), 0, 0, label });
        frame.size += 3;
        return;
    }

    case AST::FONT:
    {
        std::string label = resolve_label_ref(frame, a, TYPE_FONT);
        f.instrs.push_back({ I_PUSHL, a.line(), 0, 0, label });
        frame.size += 3;
        return;
    }

    case AST::STRING_LITERAL:
    {
        std::string label = progdata_label();
        std::vector<uint8_t> data = strlit_data(a);
        add_custom_progdata(label, data);
        f.instrs.push_back({ I_PUSHL, a.line(), 0, 0, label });
        frame.size += 3;
        return;
    }

    case AST::ARRAY_LEN:
    {
        auto const& r = a.children[1].children[0];
        if(auto* v = resolve_global(r))
        {
            uint8_t size = v->var.type.children[0].is_prog ? 3 : 2;
            if(v->var.type.is_prog)
            {
                f.instrs.push_back({ I_PUSHL, r.line(), size, 0, v->name });
                f.instrs.push_back({ I_GETPN, r.line(), size });
            }
            else
            {
                f.instrs.push_back({ I_PUSH, r.line(), size });
                f.instrs.push_back({ I_GETGN, r.line(), size, 0, v->name });
            }
            frame.size += size;
            return;
        }
        if(auto* v = resolve_local(frame, r))
        {
            assert(!v->var.type.is_prog);
            if(!v->var.type.is_array_ref())
            {
                errs.push_back({
                    "\"" + std::string(r.data) + "\" is not an array or [unsized] array reference",
                    r.line_info });
                return;
            }
            uint8_t size = v->var.type.children[0].is_prog ? 3 : 2;
            uint8_t offset = (uint8_t)(frame.size - v->frame_offset - size);
            f.instrs.push_back({ I_PUSH, r.line(), size });
            f.instrs.push_back({ I_GETLN, r.line(), offset });
            frame.size += size;
            return;
        }
        return;
    }

    case AST::OP_AREF:
    {
        codegen_expr(f, frame, a.children[0], true);
        auto const& t = a.children[0].comp_type.without_ref();
        if(t.is_array_ref())
            return;
        assert(t.is_array());
        auto n = t.array_size();
        auto line = a.children[0].line();
        f.instrs.push_back({ I_PUSH, line, uint8_t(n >> 0) });
        f.instrs.push_back({ I_PUSH, line, uint8_t(n >> 8) });
        if(t.is_prog)
            f.instrs.push_back({ I_PUSH, line, uint8_t(n >> 16) });
        frame.size += t.is_prog ? 3 : 2;
        return;
    }

    default:
        assert(false);
        errs.push_back({ "(codegen_expr) Unimplemented AST node", a.line_info });
        return;
    }

rvalue_error:
    errs.push_back({
        "Cannot create reference to expression",
        a.line_info });
}

void compiler_t::codegen_expr_compound(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& a, compiler_type_t const& type)
{
    assert(a.type == AST::COMPOUND_LITERAL);
    if(type.is_array())
    {
        const auto& t = type.children[0];
        size_t num_elems = type.prim_size / t.prim_size;
        if(num_elems < a.children.size())
        {
            errs.push_back({
                "Too many array elements in initializer",
                a.line_info });
            return;
        }
        bool ref = t.is_any_ref();
        if((ref || t.has_child_ref()) && num_elems > a.children.size())
        {
            errs.push_back({
                "Too few array elements in initializer with reference types",
                a.line_info });
            return;
        }
        for(auto const& child : a.children)
        {
            if(child.type == AST::COMPOUND_LITERAL)
                codegen_expr_compound(f, frame, child, t);
            else
            {
                codegen_expr(f, frame, child, ref);
                codegen_convert(f, frame, child, t, child.comp_type);
            }
        }
        for(size_t i = a.children.size(); i < num_elems; ++i)
        {
            for(size_t j = 0; j < t.prim_size; ++j)
                f.instrs.push_back({ I_PUSH, a.line(), 0 });
            frame.size += t.prim_size;
        }
    }
    else if(type.is_struct())
    {
        if(type.children.size() < a.children.size())
        {
            errs.push_back({
                "Too many elements in struct initializer",
                a.line_info });
            return;
        }
        for(size_t i = 0; i < a.children.size(); ++i)
        {
            auto const& child = a.children[i];
            auto const& t = type.children[i];
            bool ref = t.is_ref();
            if(child.type == AST::COMPOUND_LITERAL)
                codegen_expr_compound(f, frame, child, t);
            else
            {
                codegen_expr(f, frame, child, ref);
                codegen_convert(f, frame, child, t, child.comp_type);
            }
        }
        for(size_t i = a.children.size(); i < type.children.size(); ++i)
        {
            auto const& t = type.children[i];
            if(t.is_ref())
            {
                errs.push_back({
                "Uninitialized reference in struct initializer",
                a.line_info });
                return;
            }
            for(size_t j = 0; j < type.children[i].prim_size; ++j)
            {
                f.instrs.push_back({ I_PUSH, a.line(), 0 });
                frame.size += 1;
            }
        }
    }
    else assert(false);
}

void compiler_t::codegen_expr_logical(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& a, std::string const& sc_label)
{
    if(a.children[0].type == a.type)
        codegen_expr_logical(f, frame, a.children[0], sc_label);
    else
        codegen_expr(f, frame, a.children[0], false);
    codegen_convert(f, frame, a.children[0], TYPE_BOOL, a.children[0].comp_type);
    // TODO: special versions of BZ and BNZ to replace following sequence
    //       of DUP; B[N]Z; POP
    f.instrs.push_back({ I_DUP, a.line() });
    f.instrs.push_back({
        a.type == AST::OP_LOGICAL_AND ? I_BZ : I_BNZ,
        a.line(), 0, 0, sc_label });
    frame.size -= 1;
    f.instrs.push_back({ I_POP });
    if(a.children[1].type == a.type)
        codegen_expr_logical(f, frame, a.children[1], sc_label);
    else
        codegen_expr(f, frame, a.children[1], false);
    codegen_convert(f, frame, a.children[1], TYPE_BOOL, a.children[1].comp_type);
}

}
