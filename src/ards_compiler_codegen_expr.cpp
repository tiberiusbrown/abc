#include "ards_compiler.hpp"

#include <assert.h>

namespace ards
{

template<int32_t C>
static bool is_constant(ast_node_t const& n)
{
    if(n.type == AST::OP_CAST)
        return is_constant<C>(n.children[1]);
    return
        n.type == AST::INT_CONST && n.value == C ||
        n.type == AST::FLOAT_CONST && n.fvalue == double(C);
}

static bool is_power_of_two(int64_t x)
{
    if(x <= 0) return false;
    return ((uint32_t)x & (uint32_t)(x - 1)) == 0;
}

static bool is_power_of_two(ast_node_t const& n)
{
    return n.type == AST::INT_CONST && is_power_of_two(n.value);
}

static uint8_t get_pow2_shift(int64_t x)
{
    if(x == 0) return 0;
    uint8_t n = 0;
    uint32_t t = (uint32_t)x;
    while(!(t & 1))
        t >>= 1, ++n;
    return n;
}

void compiler_t::resolve_format_call(
    ast_node_t const& n, compiler_func_decl_t const& decl,
    std::vector<compiler_type_t>& arg_types, std::string& fmt)
{
    assert(decl.arg_types.size() >= 1);
    assert(n.children.size() >= decl.arg_types.size());
    std::vector<uint8_t> strlit;
    {
        auto const& nc = n.children[decl.arg_types.size() - 1];
        if(!(nc.type == AST::STRING_LITERAL ||
            nc.type == AST::OP_AREF && nc.children[0].type == AST::STRING_LITERAL))
        {
            errs.push_back({
                "Format string must be a string literal",
                nc.line_info });
            return;
        }
        strlit = strlit_data(nc.type == AST::STRING_LITERAL ? nc : nc.children[0]);
    }
    std::string_view d((char const*)strlit.data(), strlit.size());

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
            break;
        case '.':
        {
            if(++it == d.end())
            {
                errs.push_back({
                    "Trailing '.' in format string",
                    n.line_info });
                return;
            }
            c = *it;
            if(!(c >= '0' && c <= '9'))
            {
                errs.push_back({
                    "Precision specifier must be a single digit: 0-9",
                    n.line_info });
                return;
            }
            if(++it == d.end() || *it != 'f')
            {
                errs.push_back({
                    "Format string: precision modifier must be followed by 'f'",
                    n.line_info });
                return;
            }
            fmt.pop_back();
            fmt += 'f';
            fmt += c;
            arg_types.push_back(TYPE_FLOAT);
            break;
        }
        case '0':
        {
            if(++it == d.end())
            {
                errs.push_back({
                    "Trailing '0' in format string",
                    n.line_info });
                return;
            }
            c = *it;
            if(!(c >= '0' && c <= '9'))
            {
                errs.push_back({
                    "Zero-pad width specifier must be a single digit: 0-9",
                    n.line_info });
                return;
            }
            char ct;
            if(++it == d.end() || ((ct = *it) != 'd' && ct != 'u' && ct != 'x'))
            {
                errs.push_back({
                    "Format string: zero-pad width specifier must be followed by 'd', 'u', or 'x'",
                    n.line_info });
                return;
            }
            fmt.pop_back();
            fmt += ct;
            fmt += c;
            if(ct == 'd')
                arg_types.push_back(TYPE_I32);
            else
                arg_types.push_back(TYPE_U32);
            break;
        }
        case 'd': arg_types.push_back(TYPE_I32); fmt += '0'; break;
        case 'u':
        case 'x': arg_types.push_back(TYPE_U32); fmt += '0'; break;
        case 'f': arg_types.push_back(TYPE_FLOAT); fmt += '2'; break;
        case 'c': arg_types.push_back(TYPE_CHAR); break;
        case 's':
        {
            size_t i = arg_types.size();
            auto const& t = n.children[i].comp_type.without_ref();
            bool is_prog = t.is_prog;
            if(t.is_array_ref() && t.children[0].is_prog) is_prog = true;
            // TODO: optionally pop back and replace with specifier for prog str
            if(is_prog)
            {
                arg_types.push_back(TYPE_STR_PROG);
                fmt.pop_back();
                fmt += 'S';
            }
            else
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
    return;
}

void compiler_t::codegen_expr(
    compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a, bool ref)
{
    if(ref && !a.comp_type.is_ref())
    {
        errs.push_back({
            "Cannot create reference to expression",
            a.line_info });
    }
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

        instr_t i = instr_t((a.type == AST::OP_INC_POST ? I_PINC : I_PDEC) + (size - 1));
        if(t.is_float)
            i = (a.type == AST::OP_INC_POST ? I_PINCF : I_PDECF);

        // special instructions for post-inc/dec
        f.instrs.push_back({ i, a.children[0].line() });
        frame.size -= 2;
        frame.size += size;

        return;
    }

    case AST::OP_ASSIGN_COMPOUND:
    {
        auto const& dtype = a.children[0].comp_type.without_ref_single();
        auto size = dtype.prim_size;
        bool non_root = (
            a.parent &&
            a.parent->type != AST::EXPR_STMT &&
            a.parent->type != AST::LIST);

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
        if(!non_root)
        {
            f.instrs.push_back({ I_POPN, a.line(), 2 });
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
        assert(a.children.size() == 2);
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(f, frame, a, a.children[0].comp_type, a.children[1].comp_type);
        return;
    }

    case AST::OP_UNARY:
    {
        assert(a.children.size() == 2);
        auto op = a.children[0].data;
        if(op == "!")
        {
            if(a.children[0].comp_type.is_float || a.children[1].comp_type.is_float)
            {
                errs.push_back({
                    "Logical NOT may not be performed on floating point values",
                    a.line_info });
                return;
            }
            codegen_expr(f, frame, a.children[1], false);
            codegen_convert(f, frame, a, TYPE_BOOL, a.children[1].comp_type);
            f.instrs.push_back({ I_NOT, a.children[1].line() });
        }
        else if(op == "-" && a.comp_type.is_float)
        {
            codegen_expr(f, frame, a.children[1], false);
            codegen_convert(f, frame, a, a.comp_type, a.children[1].comp_type);
            f.instrs.push_back({ I_PUSH, a.children[1].line(), 0x00 });
            f.instrs.push_back({ I_PUSH, a.children[1].line(), 0x00 });
            f.instrs.push_back({ I_PUSH, a.children[1].line(), 0x00 });
            f.instrs.push_back({ I_PUSH, a.children[1].line(), 0x80 });
            f.instrs.push_back({ I_XOR4, a.children[1].line() });
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
            if(a.children[0].comp_type.is_float || a.children[1].comp_type.is_float)
            {
                errs.push_back({
                    "Bitwise NOT may not be performed on floating point values",
                    a.line_info });
                return;
            }
            compiler_type_t t = a.comp_type;
            if(t.prim_size == 3)
                t = TYPE_U32;
            int index =
                t.prim_size == 1 ? 0 :
                t.prim_size == 2 ? 1 : 2;
            codegen_expr(f, frame, a.children[1], false);
            codegen_convert(f, frame, a.children[1], t, a.children[1].comp_type);
            f.instrs.push_back({ instr_t(I_COMP + index), a.children[1].line() });
            codegen_convert(f, frame, a.children[1], a.comp_type, t);
        }
        else
        {
            assert(false);
        }
        return;
    }

    case AST::INT_CONST:
    {
        uint32_t x = (uint32_t)a.value;
        auto size = a.comp_type.prim_size;
        frame.size += size;
        for(size_t i = 0; i < size; ++i, x >>= 8)
            f.instrs.push_back({ I_PUSH, a.line(), (uint8_t)x });
        return;
    }

    case AST::FLOAT_CONST:
    {
        union
        {
            float f;
            uint32_t x;
        } u = { (float)a.fvalue };
        uint32_t x = u.x;
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
        codegen_expr_ident(f, frame, a, 0);
        return;
    }

    case AST::FUNC_CALL:
    {
        assert(a.children.size() == 2);
        assert(a.children[0].type == AST::IDENT);

        auto func = resolve_func(a);
        auto sys = SYS_NONE;
        {
            auto it = sys_names.find(func.name.substr(1));
            if(it != sys_names.end())
                sys = it->second;
        }
        bool is_format = sysfunc_is_format(sys);

        // call to $tilemap_get with constexpr arguments
        if(sys == SYS_TILEMAP_GET &&
            a.children[1].children[0].type == AST::LABEL_REF &&
            a.children[1].children[1].type == AST::INT_CONST &&
            a.children[1].children[2].type == AST::INT_CONST)
        {
            auto it = progdata.find(std::string(a.children[1].children[0].data));
            if(it != progdata.end())
            {
                auto const& data = it->second.data;
                int format = data[0];
                int nrow = data[1] + data[2] * 256;
                int ncol = data[3] + data[4] * 256;
                int x = (int)a.children[1].children[1].value;
                int y = (int)a.children[1].children[2].value;
                uint16_t t = 0;
                if(x >= 0 && y >= 0 && x < ncol && y < nrow)
                {
                    int i = (y * ncol + x) * format + 5;
                    t = data[i];
                    if(format == 2)
                        t = t * 256 + data[i + 1];
                }
                f.instrs.push_back({ I_PUSH, a.line(), uint8_t(t >> 0) });
                f.instrs.push_back({ I_PUSH, a.line(), uint8_t(t >> 8) });
                frame.size += 2;
                return;
            }
        }

        // TODO: test for reference return type (not allowed)

        // system functions don't need space reserved for return value
        if(!func.is_sys)
        {
            // reserve space for return value
            frame.size += func.decl.return_type.prim_size;
            auto n = (uint8_t)func.decl.return_type.prim_size;
            if(n > 8)
            {
                f.instrs.push_back({ I_ALLOC, a.line(), n });
            }
            else
            {
                for(size_t i = 0; i < func.decl.return_type.prim_size; ++i)
                    f.instrs.push_back({ I_PUSH, a.line(), 0 });
            }
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

        size_t prev_size = frame.size;

        // function args in reverse order
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
                //if(!type.is_any_ref())
                    codegen_convert(f, frame, a, type, expr.comp_type);
            }
        }

        switch(sys)
        {
        case SYS_SPRITES_WIDTH:
        case SYS_SPRITES_HEIGHT:
        case SYS_SPRITES_FRAMES:
        case SYS_TILEMAP_WIDTH:
        case SYS_TILEMAP_HEIGHT:
        {
            uint8_t offset, size;
            switch(sys)
            {
            case SYS_SPRITES_WIDTH:  offset = 0; size = 1; break;
            case SYS_SPRITES_HEIGHT: offset = 1; size = 1; break;
            case SYS_SPRITES_FRAMES: offset = 3; size = 2; break;
            case SYS_TILEMAP_WIDTH:  offset = 3; size = 2; break;
            case SYS_TILEMAP_HEIGHT: offset = 1; size = 2; break;
            default: assert(false); return;
            }
            if(offset != 0)
            {
                f.instrs.push_back({ I_PUSH, a.line(), offset });
                f.instrs.push_back({ I_PUSH, a.line(), 0 });
                f.instrs.push_back({ I_PUSH, a.line(), 0 });
                f.instrs.push_back({ I_ADD3, a.line() });
            }
            f.instrs.push_back({ I_GETPN, a.line(), size });
            frame.size -= 3;
            frame.size += size;
            return;
        }
        default: break;
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
                "and thus cannot be assigned to", a.children[0].line_info });
            return;
        }
        bool non_root =
            a.parent &&
            a.parent->type != AST::EXPR_STMT &&
            a.parent->type != AST::LIST;
        auto const& type_noref = a.children[0].comp_type.without_ref();
        auto const& src_node =
            a.children[1].type == AST::OP_CAST ?
            a.children[1].children[1] :
            a.children[1];
        auto const& src_type = src_node.comp_type;

        // optimize assignment as call to memcpy, memcpy_P, or memset
        if(type_noref.is_copyable() &&
            type_noref.prim_size >= MIN_MEMCPY_SIZE)
        {
            bool prog = true;
            bool srcref = false;
            bool use_memset = false;
            std::vector<uint8_t> pd;
            std::string pd_label;

            if(src_type.is_any_ref() &&
                src_type.children[0].is_copyable())
            {
                prog = src_type.children[0].is_prog;
                srcref = true;
            }
            else if(!progdata_expr_valid_memcpy(
                a.children[1], a.children[0].comp_type.without_ref(), pd) ||
                pd.empty() || pd.size() != type_noref.prim_size)
            {
                goto no_memcpy_optimization;
            }
            
            if(non_root)
                codegen_expr(f, frame, a.children[0], true);

            if(srcref)
            {
                codegen_expr(f, frame, src_node, false);
                codegen_convert(
                    f, frame, src_node,
                    prog ? TYPE_BYTE_PROG_AREF : TYPE_BYTE_AREF,
                    src_type);
            }
            else
            {
                use_memset = true;
                for(auto b : pd)
                    if(b != pd[0])
                    {
                        use_memset = false;
                        break;
                    }
                if(use_memset)
                {
                    f.instrs.push_back({ I_PUSH, a.children[1].line(), pd[0] });
                    frame.size += 1;
                }
                else
                {
                    auto n = pd.size();
                    pd_label = progdata_label();
                    add_custom_progdata(pd_label, pd);
                    f.instrs.push_back({ I_PUSHL, a.children[1].line(), 0, 0, pd_label });
                    f.instrs.push_back({ I_PUSH, a.children[1].line(), uint8_t(n >> 0) });
                    f.instrs.push_back({ I_PUSH, a.children[1].line(), uint8_t(n >> 8) });
                    f.instrs.push_back({ I_PUSH, a.children[1].line(), uint8_t(n >> 16) });
                    frame.size += 6;
                }
            }

            if(non_root)
            {
                auto line = a.children[0].line();
                auto size = a.children[0].comp_type.prim_size;
                f.instrs.push_back({ I_PUSH, line, (uint8_t)size });
                f.instrs.push_back({ I_GETLN, line, (uint8_t)(size + (prog ? 6 : 4)) });
                frame.size += size;
                codegen_convert(
                    f, frame, a.children[0],
                    TYPE_BYTE_AREF,
                    a.children[0].comp_type);
            }
            else
            {
                codegen_expr(f, frame, a.children[0], true);
                codegen_convert(
                    f, frame, a.children[0],
                    TYPE_BYTE_AREF,
                    a.children[0].comp_type);
            }

            f.instrs.push_back({ I_SYS, a.line(),
                use_memset ? SYS_MEMSET : prog ? SYS_MEMCPY_P : SYS_MEMCPY });
            frame.size -= 4; // dst aref
            frame.size -= (use_memset ? 1 : prog ? 6 : 4); // val or src aref
            return;
        }
no_memcpy_optimization:

        size_t ref_offset = frame.size;
        if(non_root)
        {
            // codegen reference now
            codegen_expr(f, frame, a.children[0], true);
        }

        if(type_noref.is_prim() || type_noref.is_label_ref())
        {
            codegen_expr(f, frame, a.children[1], false);
            codegen_convert(
                f, frame, a.children[1],
                a.children[0].comp_type.without_ref(),
                a.children[1].comp_type);
        }
        else if(type_noref.is_array())
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
        else if(type_noref.is_struct())
        {
            if(a.children[1].type == AST::COMPOUND_LITERAL)
                codegen_expr_compound(f, frame, a.children[1], type_noref);
            else
            {
                codegen_expr(f, frame, a.children[1], false);
                codegen_convert(f, frame, a, type_noref, a.children[1].comp_type);
            }
        }
        else if(!type_noref.is_copyable())
        {
            errs.push_back({
                "Type is not assignable",
                a.line_info });
            return;
        }
        else
        {
            assert(false);
        }

        // get reference for storing
        if(non_root)
        {
            ref_offset = frame.size - ref_offset;
            auto line = a.children[0].line();
            auto size = a.children[0].comp_type.prim_size;
            assert(ref_offset == a.children[1].comp_type.prim_size + size);
            f.instrs.push_back({ I_PUSH, line, (uint8_t)size });
            f.instrs.push_back({ I_GETLN, line, (uint8_t)ref_offset });
            frame.size += size;
        }
        else
        {
            codegen_expr(f, frame, a.children[0], true);
        }

        codegen_store(f, frame, a.children[0]);
        return;
    }

    case AST::OP_EQUALITY:
    case AST::OP_RELATIONAL:
    {
        assert(a.children.size() == 2);
        if( a.children[0].comp_type.without_prog().without_ref().without_prog() !=
            a.children[1].comp_type.without_prog().without_ref().without_prog())
        {
            errs.push_back({ "Incompatible types in comparison", a.line_info });
            return;
        }
        assert(a.comp_type == TYPE_BOOL);

        // a <  b  ->   (a < b)
        // a >  b  ->   (b < a)
        // a <= b  ->  !(b < a)
        // a >= b  ->  !(a < b)

        bool negated = (a.data == "!=" || a.data == "<=" || a.data == ">=");
        bool equality = (a.data == "==" || a.data == "!=");
        size_t i0 = 0, i1 = 1;
        if(a.data == ">" || a.data == "<=")
            std::swap(i0, i1);

        codegen_expr(f, frame, a.children[i0], false);
        codegen_expr(f, frame, a.children[i1], false);

        auto size = a.children[0].comp_type.prim_size;
        assert(size >= 1 && size <= 4);
        frame.size -= size;       // comparison
        frame.size -= (size - 1); // conversion to bool
        if(a.children[0].comp_type.is_float || a.children[1].comp_type.is_float)
        {
            assert(a.children[0].comp_type.is_float);
            assert(a.children[1].comp_type.is_float);

            instr_t instr = equality ? I_CFEQ : I_CFLT;

            // optimize to an integer comparison if inequality comparison against zero
            if(!equality && (is_constant<0>(a.children[0]) || is_constant<0>(a.children[1])))
                instr = I_CSLT4;

            f.instrs.push_back({ instr, a.line() });
        }
        else
        {
            if(equality)
            {
                f.instrs.push_back({ instr_t(I_SUB + size - 1), a.line() });
                f.instrs.push_back({ instr_t(I_BOOL + size - 1), a.line() });
                f.instrs.push_back({ I_NOT, a.line() });
            }
            else
            {
                instr_t i = (a.children[0].comp_type.is_signed ? I_CSLT : I_CULT);
                f.instrs.push_back({ instr_t(i + size - 1), a.line() });
            }
        }
        if(negated)
            f.instrs.push_back({ I_NOT, a.line() });
        return;
    }

    case AST::OP_ADDITIVE:
    {
        assert(a.data == "+" || a.data == "-");
        assert(a.children.size() == 2);

        bool z0 = a.data == "+" && is_constant<0>(a.children[0]);
        bool z1 = is_constant<0>(a.children[1]);

        if(!z0 || (z0 && z1))
        {
            codegen_expr(f, frame, a.children[0], false);
            codegen_convert(f, frame, a, a.comp_type, a.children[0].comp_type);
        }
        if(!z1)
        {
            codegen_expr(f, frame, a.children[1], false);
            codegen_convert(f, frame, a, a.comp_type, a.children[1].comp_type);
        }
        if(z0 || z1)
            return;
        if(a.comp_type.is_float)
        {
            frame.size -= 4;
            f.instrs.push_back({ instr_t(a.data == "+" ? I_FADD : I_FSUB), a.line()});
        }
        else
        {
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
        }
        return;
    }

    case AST::OP_MULTIPLICATIVE:
    {
        assert(a.children.size() == 2);

        bool c0, c1;

        c0 = is_constant<0>(a.children[0]);
        c1 = a.data == "*" && is_constant<0>(a.children[1]);

        if(c0 || c1)
        {
            f.instrs.push_back({ I_PUSH, a.line(), 0 });
            frame.size += 1;
            codegen_convert(f, frame, a, a.comp_type, TYPE_U8);
            return;
        }

        // convert division into right shift
        if(a.data == "/" && !a.comp_type.is_float &&
            a.comp_type.prim_size != 3 && !a.comp_type.is_signed &&
            is_power_of_two(a.children[1]))
        {
            uint8_t shift = get_pow2_shift(a.children[1].value);
            instr_t ti = I_LSR;
            if(a.comp_type.prim_size == 2) ti = I_LSR2;
            if(a.comp_type.prim_size == 4) ti = I_LSR4;
            codegen_expr(f, frame, a.children[0], false);
            codegen_convert(f, frame, a, a.comp_type, a.children[0].comp_type);
            f.instrs.push_back({ I_PUSH, a.children[1].line(), shift });
            f.instrs.push_back({ ti, a.line() });
            return;
        }

        c0 = a.data == "*" && is_constant<1>(a.children[0]);
        c1 = is_constant<1>(a.children[1]);

        if(!c0 || (c0 && c1))
        {
            codegen_expr(f, frame, a.children[0], false);
            codegen_convert(f, frame, a, a.comp_type, a.children[0].comp_type);
        }
        if(!c1)
        {
            codegen_expr(f, frame, a.children[1], false);
            codegen_convert(f, frame, a, a.comp_type, a.children[1].comp_type);
        }
        if(c0 || c1)
            return;
        static_assert(I_MUL2 == I_MUL + 1);
        static_assert(I_MUL3 == I_MUL + 2);
        static_assert(I_MUL4 == I_MUL + 3);
        auto size = a.comp_type.prim_size;
        assert(size >= 1 && size <= 4);
        frame.size -= size;
        if(a.comp_type.is_float)
        {
            if(a.data == "*")
                f.instrs.push_back({ I_FMUL, a.line() });
            else if(a.data == "/")
                f.instrs.push_back({ I_FDIV, a.line() });
            else if(a.data == "%")
            {
                errs.push_back({
                    "The modulo operator may not be applied to floating point values",
                    a.line_info });
                return;
            }
            else
                assert(false);
        }
        else
        {
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
        }
        return;
    }

    case AST::OP_SHIFT:
    {
        if(a.children[0].comp_type.is_float || a.children[1].comp_type.is_float)
        {
            errs.push_back({
                "Bit shifts may not be performed with floating point values",
                a.line_info });
            return;
        }
        compiler_type_t t = a.comp_type;
        if(t.prim_size == 3)
            t = TYPE_U32;
        codegen_expr(f, frame, a.children[0], false);
        codegen_convert(f, frame, a, t, a.children[0].comp_type);
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(f, frame, a, TYPE_U8, a.children[1].comp_type);
        frame.size -= 1;
        auto index =
            t.prim_size == 1 ? 0 :
            t.prim_size == 2 ? 1 : 2;
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
        codegen_convert(f, frame, a, a.comp_type, t);
        return;
    }

    case AST::OP_BITWISE_AND:
    case AST::OP_BITWISE_OR:
    case AST::OP_BITWISE_XOR:
    {
        if(a.children[0].comp_type.is_float || a.children[1].comp_type.is_float)
        {
            errs.push_back({
                "Bitwise operations may not be performed on floating point values",
                a.line_info });
            return;
        }
        compiler_type_t t = a.comp_type;
        if(t.prim_size == 3)
            t = TYPE_U32;
        codegen_expr(f, frame, a.children[0], false);
        codegen_convert(f, frame, a, t, a.children[1].comp_type);
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(f, frame, a, t, a.children[1].comp_type);
        frame.size -= t.prim_size;
        int index =
            t.prim_size == 1 ? 0 :
            t.prim_size == 2 ? 1 : 2;
        if(a.type == AST::OP_BITWISE_AND)
            f.instrs.push_back({ instr_t(I_AND + index), a.line() });
        else if(a.type == AST::OP_BITWISE_OR)
            f.instrs.push_back({ instr_t(I_OR + index), a.line() });
        else if(a.type == AST::OP_BITWISE_XOR)
            f.instrs.push_back({ instr_t(I_XOR + index), a.line() });
        codegen_convert(f, frame, a, a.comp_type, t);
        return;
    }

    case AST::ARRAY_INDEX:
    {
        size_t offset = 0;
        codegen_expr_array_index(f, frame, a, offset);
        return;
    }

    case AST::ARRAY_SLICE:
    case AST::ARRAY_SLICE_LEN:
    {
        auto const& t = a.children[0].comp_type.without_ref();
    
        uint32_t elem_size = (uint32_t)t.children[0].prim_size;
        bool prog = t.children[0].is_prog;

        // handle when we can compute a simple reference at compile time 
        if(t.is_array() && a.comp_type.without_ref().is_array())
        {
            codegen_expr(f, frame, a.children[0], true);
            codegen_expr(f, frame, a.children[1], false);
            codegen_convert(f, frame, a.children[1],
                prog ? TYPE_U24 : TYPE_U16, a.children[1].comp_type);
            f.instrs.push_back({ prog ? I_ADD3 : I_ADD2, a.line() });
            frame.size -= (prog ? 3 : 2);
            return;
        }
    
        // generate ref
        codegen_expr(f, frame, a.children[0], true);
        codegen_convert(f, frame, a.children[0],
            t.children[0].with_array_ref(), a.children[0].comp_type);
        
        // generate start
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(f, frame, a.children[1],
            prog ? TYPE_U24 : TYPE_U16, a.children[1].comp_type);

        // generate stop
        if(a.type == AST::ARRAY_SLICE_LEN)
        {
            f.instrs.push_back({ I_PUSH, a.children[2].line(), prog ? 3u : 2u });
            f.instrs.push_back({ I_GETLN, a.children[2].line(), prog ? 3u : 2u });
            frame.size += (prog ? 3 : 2);
            codegen_expr(f, frame, a.children[2], false);
            codegen_convert(f, frame, a.children[2],
                prog ? TYPE_U24 : TYPE_U16, a.children[2].comp_type);
            f.instrs.push_back({ prog ? I_ADD3 : I_ADD2, a.children[2].line() });
            frame.size -= (prog ? 3 : 2);
        }
        else
        {
            codegen_expr(f, frame, a.children[2], false);
            codegen_convert(f, frame, a.children[2],
                prog ? TYPE_U24 : TYPE_U16, a.children[2].comp_type);
        }

        // construct slice ref
        f.instrs.push_back({ prog ? I_PSLC : I_ASLC, a.line(), elem_size });
        frame.size -= (prog ? 6 : 4);

        // pop length if simple reference
        if(a.comp_type.without_ref().is_array())
        {
            f.instrs.push_back({ I_POPN, a.line(), uint8_t(prog ? 3 : 2) });
            frame.size -= (prog ? 3 : 2);
        }
    
        return;
    }

    case AST::STRUCT_MEMBER:
    {
        std::string name = std::string(a.children[1].data);
        size_t offset = 0;
        auto const* t = resolve_member(a.children[0], name, &offset);
        if(!t) return;

        // check for array of structs: a[i].x
        // try to add offset to array reference
        if(offset != 0 && (
            a.children[0].type == AST::ARRAY_INDEX
            ))
        {
            codegen_expr_array_index(f, frame, a.children[0], offset);
        }
        else
            codegen_expr(f, frame, a.children[0], true);

        {
            auto const* reft = &a.children[0].comp_type;
            while(reft->is_ref() && reft->children[0].is_ref())
                reft = &reft->children[0];
            if(reft != &a.children[0].comp_type)
                codegen_dereference(f, frame, a.children[0], *reft);
        }
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
        return;
    }

    case AST::OP_LOGICAL_AND:
    case AST::OP_LOGICAL_OR:
    {
        if(a.children[0].comp_type.is_float || a.children[1].comp_type.is_float)
        {
            errs.push_back({
                "Logical operators may not be performed on floating point values",
                a.line_info });
            return;
        }
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

    case AST::MUSIC:
    {
        std::string label = resolve_label_ref(frame, a, TYPE_MUSIC);
        f.instrs.push_back({ I_PUSHL, a.line(), 0, 0, label });
        frame.size += 3;
        return;
    }

    case AST::TILEMAP:
    {
        std::string label = resolve_label_ref(frame, a, TYPE_TILEMAP);
        f.instrs.push_back({ I_PUSHL, a.line(), 0, 0, label });
        frame.size += 3;
        return;
    }

    case AST::TONES:
    {
        std::string label = resolve_label_ref(frame, a, TYPE_TONES);
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
        errs.push_back({
            "Unable to resolve \"" + std::string(r.data) + "\"",
            r.line_info });
        return;
    }

    /*
    case AST::SPRITES_LEN:
    {
        codegen_expr(f, frame, a.children[1].children[0], false);
        codegen_convert(f, frame, a, TYPE_SPRITES, a.children[1].children[0].comp_type);
        f.instrs.push_back({ I_PUSH, a.line(), 2 });
        f.instrs.push_back({ I_PUSH, a.line(), 0 });
        f.instrs.push_back({ I_PUSH, a.line(), 0 });
        f.instrs.push_back({ I_ADD3, a.line() });
        f.instrs.push_back({ I_GETPN, a.line(), 2 });
        frame.size -= 3;
        frame.size += 2;
        return;
    }
    */

    case AST::OP_AREF:
    {
        codegen_expr(f, frame, a.children[0], true);
        assert(a.children[0].comp_type.is_ref());
        auto const& t = a.children[0].comp_type.without_ref();
        if(t.is_array_ref())
            return;
        if(a.comp_type.children[0].is_byte && !t.is_copyable())
        {
            errs.push_back({
                "Cannot create byte UAR from non-copyable type",
                a.line_info });
            return;
        }
        if(!t.is_array() && !a.comp_type.children[0].is_byte)
        {
            errs.push_back({
                "Only byte UARs may be created from non-array types",
                a.line_info });
            return;
        }
        size_t n = a.comp_type.children[0].is_byte ? t.prim_size : t.array_size();
        auto line = a.children[0].line();
        f.instrs.push_back({ I_PUSH, line, uint8_t(n >> 0) });
        f.instrs.push_back({ I_PUSH, line, uint8_t(n >> 8) });
        if(t.is_prog)
            f.instrs.push_back({ I_PUSH, line, uint8_t(n >> 16) });
        frame.size += t.is_prog ? 3 : 2;
        return;
    }

    case AST::OP_TERNARY:
    {
        assert(a.children.size() == 3);
        assert(a.children[0].comp_type.is_bool);
        assert(a.comp_type == a.children[1].comp_type);
        assert(a.comp_type == a.children[2].comp_type);

        // compile-time 'true' condition
        if(a.children[0].type == AST::INT_CONST && a.children[0].value != 0)
        {
            codegen_expr(f, frame, a.children[1], false);
            codegen_convert(f, frame, a, a.comp_type, a.children[1].comp_type);
            return;
        }

        // compile-time 'false' condition
        if(a.children[0].type == AST::INT_CONST && a.children[0].value == 0)
        {
            codegen_expr(f, frame, a.children[2], false);
            codegen_convert(f, frame, a, a.comp_type, a.children[2].comp_type);
            return;
        }

        // condition
        codegen_expr(f, frame, a.children[0], false);
        codegen_convert(f, frame, a.children[0], TYPE_BOOL, a.children[0].comp_type);

        // branch
        auto secondary_label = new_label(f);
        auto end_label = new_label(f);
        f.instrs.push_back({ I_BZ, a.children[0].line(), 0, 0, secondary_label });
        frame.size -= 1;

        // primary case
        auto prev_size = frame.size;
        codegen_expr(f, frame, a.children[1], false);
        codegen_convert(f, frame, a.children[1], a.comp_type, a.children[1].comp_type);
        f.instrs.push_back({ I_JMP, a.children[1].line(), 0, 0, end_label });
        frame.size = prev_size;

        // secondary case
        codegen_label(f, secondary_label);
        codegen_expr(f, frame, a.children[2], false);
        codegen_convert(f, frame, a.children[2], a.comp_type, a.children[2].comp_type);

        // end label
        codegen_label(f, end_label);

        break;
    }

    default:
        assert(false);
        errs.push_back({ "(codegen_expr) Unimplemented AST node", a.line_info });
        return;
    }
}

void compiler_t::codegen_expr_ident(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& a, size_t offset)
{
    std::string name(a.data);
    if(auto* local = resolve_local(frame, a))
    {
        assert(!local->var.is_constexpr);
        uint8_t frame_offset = (uint8_t)(frame.size - local->frame_offset - offset);
        f.instrs.push_back({ I_REFL, a.line(), frame_offset });
        frame.size += 2;
    }
    else if(auto* global = resolve_global(a))
    {
        assert(!global->var.is_constexpr);
        bool prog = global->var.type.is_prog;
        if(global->is_constexpr_ref())
        {
            if(prog)
                f.instrs.push_back({ I_PUSHL, a.line(), (uint32_t)offset, 0, global->constexpr_ref });
            else
                f.instrs.push_back({ I_PUSHG, a.line(), (uint32_t)offset, 0, global->constexpr_ref });
            frame.size += (prog ? 3 : 2);
        }
        else if(prog)
        {
            f.instrs.push_back({ I_PUSHL, a.line(), (uint32_t)offset, 0, global->name });
            frame.size += 3;
        }
        else
        {
            f.instrs.push_back({ I_PUSHG, a.line(), (uint32_t)offset, 0, global->name });
            frame.size += 2;
        }
    }
    else
    {
        errs.push_back({ "Undefined variable \"" + name + "\"", a.line_info });
    }
}

void compiler_t::codegen_expr_array_index(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& a, size_t& offset)
{
    if(a.children[1].comp_type.is_float)
    {
        errs.push_back({
            "Array indices may not be floating point values",
            a.line_info });
        return;
    }

    auto const& t = a.children[0].comp_type.without_ref();
    bool prog = t.is_array_ref() ? t.children[0].is_prog : t.is_prog;
    size_t elem_size = t.children[0].prim_size;

    if(a.children[0].type == AST::IDENT)
    {
        codegen_expr_ident(f, frame, a.children[0], offset);
        if(!t.is_array_ref() &&a.children[0].comp_type.children[0].is_ref())
            codegen_dereference(f, frame, a.children[0], a.children[0].comp_type.children[0]);
        offset = 0;
    }
    else
        codegen_expr(f, frame, a.children[0], true);
    if(t.is_array_ref())
        codegen_convert(f, frame, a.children[0], t, a.children[0].comp_type);

    // optimize the case where children[1] is an integer constant:
    // directly adjust offset without bounds checking
    if(!t.is_array_ref() && a.children[1].type == AST::INT_CONST)
    {
        size_t array_size = t.array_size();
        int64_t v = a.children[1].value;
        if(v < 0 || (size_t)v >= array_size)
        {
            errs.push_back({
                "Array index out of bounds",
                a.children[1].line_info });
            return;
        }
        else
        {
            uint32_t x = (uint32_t)(v * elem_size);
            auto line = a.children[1].line();
            f.instrs.push_back({ I_PUSH, line, uint8_t(x >> 0) });
            f.instrs.push_back({ I_PUSH, line, uint8_t(x >> 8) });
            if(prog)
                f.instrs.push_back({ I_PUSH, line, uint8_t(x >> 16) });
            f.instrs.push_back({ prog ? I_ADD3 : I_ADD2, line });
            return;
        }
    }

    // construct index
    codegen_expr(f, frame, a.children[1], false);
    codegen_convert(
        f, frame, a,
        prog ? TYPE_U24 : TYPE_U16,
        a.children[1].comp_type);

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
    f.instrs.push_back({
        a.type == AST::OP_LOGICAL_AND ? I_BZP : I_BNZP,
        a.line(), 0, 0, sc_label });
    frame.size -= 1;
    if(a.children[1].type == a.type)
        codegen_expr_logical(f, frame, a.children[1], sc_label);
    else
        codegen_expr(f, frame, a.children[1], false);
    codegen_convert(f, frame, a.children[1], TYPE_BOOL, a.children[1].comp_type);
}

}
