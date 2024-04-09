#include "ards_compiler.hpp"

#include <algorithm>
#include <assert.h>

namespace ards
{

static void implicit_conversion(compiler_type_t& ta, compiler_type_t& tb)
{
    if(ta == tb) return;
    if(ta.is_signed == tb.is_signed)
    {
        if(ta.prim_size < tb.prim_size)
            ta = tb;
    }
    else if(ta.is_signed)
    {
        // a signed, b unsigned
        if(ta.prim_size <= tb.prim_size)
            ta = tb;
        else
            tb = ta;
    }
}

static bool check_prim(
    compiler_type_t const& t, ast_node_t const& a, std::vector<error_t>& errs)
{
    if(!t.is_prim())
    {
        errs.push_back({
            "\"" + std::string(a.data) + "\" is not a primitive type",
            a.line_info });
        return false;
    }
    return true;
}

static bool check_prim(ast_node_t const& a, std::vector<error_t> errs)
{
    return check_prim(a.comp_type.without_ref(), a, errs);
}

static void insert_aref(ast_node_t& a, compiler_type_t const& t)
{
    auto ta = std::move(a);
    a = { a.line_info, AST::OP_AREF };
    if(t.is_array_ref())
        a.comp_type = t;
    else
    {
        a.comp_type.type = compiler_type_t::ARRAY_REF;
        a.comp_type.children.push_back(t);
        a.comp_type.prim_size = t.without_ref().is_prog ? 6 : 4;
    }
    a.children.emplace_back(std::move(ta));
};

void ast_node_t::insert_cast(compiler_type_t const& t)
{
    auto ta = std::move(*this);
    *this = { ta.line_info, AST::OP_CAST };
    parent = ta.parent;
    ta.parent = this;
    comp_type = t;
    children.push_back({});
    children.back().comp_type = t;
    children.emplace_back(std::move(ta));
};

compiler_type_t prim_type_for_dec(uint32_t x, bool is_signed)
{
    size_t prim_size = 1;

    if(is_signed)
    {
        if(x < (1 << 7)) prim_size = 1;
        else if(x < (1 << 15)) prim_size = 2;
        else if(x < (1 << 23)) prim_size = 3;
        else prim_size = 4;
    }
    else
    {
        if(x < (1 << 8)) prim_size = 1;
        else if(x < (1 << 16)) prim_size = 2;
        else if(x < (1 << 24)) prim_size = 3;
        else prim_size = 4;
    }

    compiler_type_t t{};
    t.prim_size = prim_size;
    t.type = compiler_type_t::PRIM;
    t.is_signed = is_signed;
    return t;
}

compiler_type_t prim_type_for_hex(uint32_t x, bool is_signed)
{
    size_t prim_size = 1;

    if(x < (1 << 7)) prim_size = 1;
    else if(x < (1 << 8)) prim_size = 1, is_signed = false;
    else if(x < (1 << 15)) prim_size = 2;
    else if(x < (1 << 16)) prim_size = 2, is_signed = false;
    else if(x < (1 << 23)) prim_size = 3;
    else if(x < (1 << 24)) prim_size = 3, is_signed = false;
    else if(x < (1ll << 31)) prim_size = 4;
    else prim_size = 4, is_signed = false;

    compiler_type_t t{};
    t.prim_size = prim_size;
    t.type = compiler_type_t::PRIM;
    t.is_signed = is_signed;
    return t;
}

void compiler_t::type_annotate_recurse(ast_node_t& a, compiler_frame_t const& frame)
{
    if(!errs.empty()) return;
    switch(a.type)
    {
    case AST::SPRITES:
        a.comp_type = TYPE_SPRITES;
        break;
    case AST::FONT:
        a.comp_type = TYPE_FONT;
        break;
    case AST::TONES:
        a.comp_type = TYPE_TONES;
        break;
    case AST::COMPOUND_LITERAL:
        for(auto& child : a.children)
            type_annotate_recurse(child, frame);
        break;
    case AST::TYPE_ARRAY:
        type_annotate_recurse(a.children[0], frame);
        transform_constexprs(a.children[0], frame);
        break;
    case AST::TYPE_PROG:
        type_annotate_recurse(a.children[0], frame);
        break;
    case AST::OP_CAST:
    {
        assert(a.children.size() == 2);
        type_annotate_recurse(a.children[1], frame);
        a.comp_type = a.children[0].comp_type;
        transform_constexprs(a, frame);
        break;
    }
    case AST::OP_UNARY:
    {
        assert(a.children.size() == 2);
        type_annotate_recurse(a.children[1], frame);
        auto op = a.children[0].data;
        if(!a.children[0].comp_type.without_ref().is_prim())
        {
            errs.push_back({
                "\"" + std::string(a.children[0].data) + "\" is not a primitive type",
                    a.children[0].line_info });
                    break;
        }
        if(op == "!")
            a.comp_type = TYPE_BOOL;
        else if(op == "-" || op == "~")
            a.comp_type = a.children[1].comp_type.without_ref();
        else
        {
            assert(false);
        }
        transform_constexprs(a, frame);
        break;
    }
    case AST::OP_LOGICAL_AND:
    case AST::OP_LOGICAL_OR:
        assert(a.children.size() == 2);
        for(auto& child : a.children)
            type_annotate_recurse(child, frame);
        a.comp_type = TYPE_BOOL;
        transform_constexprs(a, frame);
        break;
    case AST::OP_ASSIGN:
    case AST::OP_ASSIGN_COMPOUND:
    {
        assert(a.children.size() == 2);
        type_annotate_recurse(a.children[0], frame);
        a.comp_type = a.children[0].comp_type;
        type_annotate_recurse(a.children[1], frame);
        if(a.children[1].type != AST::COMPOUND_LITERAL)
        {

            if(a.children[0].comp_type.is_any_ref() &&
                a.children[0].comp_type.is_nonprog_string() &&
                a.children[1].comp_type.is_any_ref() &&
                a.children[1].comp_type.is_string())
            {
                // generate calls to strcpy for char array assignment
                a.comp_type = TYPE_STR;
                ast_node_t func_call{ a.line_info, AST::FUNC_CALL };
                ast_node_t ident{ a.line_info, AST::IDENT };
                ast_node_t func_args{ a.line_info, AST::FUNC_ARGS };
                ident.data = a.children[1].comp_type.is_prog_string() ?
                    "$strcpy_P" : "$strcpy";
                func_args.children.emplace_back(std::move(a.children[0]));
                func_args.children.emplace_back(std::move(a.children[1]));
                func_call.children.emplace_back(std::move(ident));
                func_call.children.emplace_back(std::move(func_args));
                a = std::move(func_call);
                type_annotate_recurse(a, frame);
                return;
            }
            else
            {
                a.children[1].insert_cast(a.comp_type.without_ref());
                transform_constexprs(a.children[1], frame);
            }
        }
        break;
    }
    case AST::OP_COMPOUND_ASSIGNMENT_DEREF:
    {
        assert(a.parent && a.parent->parent);
        a.comp_type = a.parent->parent->children[0].comp_type;
        break;
    }
    case AST::OP_SHIFT:
        assert(a.children.size() == 2);
        for(auto& child : a.children)
            type_annotate_recurse(child, frame);
        if(!check_prim(a.children[0], errs)) break;
        if(!check_prim(a.children[1], errs)) break;
        a.comp_type = a.children[0].comp_type.without_ref();
        transform_constexprs(a, frame);
        break;
    case AST::OP_INC_POST:
    case AST::OP_DEC_POST:
        assert(a.children.size() == 1);
        type_annotate_recurse(a.children[0], frame);
        a.comp_type = a.children[0].comp_type.without_ref();
        break;
    case AST::OP_BITWISE_AND:
    case AST::OP_BITWISE_OR:
    case AST::OP_BITWISE_XOR:
    case AST::OP_EQUALITY:
    case AST::OP_RELATIONAL:
    case AST::OP_ADDITIVE:
    case AST::OP_MULTIPLICATIVE:
    {
        // C-style implicit conversion rules
        assert(a.children.size() == 2);
        for(auto& child : a.children)
            type_annotate_recurse(child, frame);

        // generate calls to strcmp* family of methods
        if(a.type == AST::OP_EQUALITY &&
            a.children[0].comp_type.is_string() &&
            a.children[1].comp_type.is_string())
        {
            ast_node_t logical_op{ a.line_info };
            ast_node_t func_call{ a.line_info,  AST::FUNC_CALL };
            ast_node_t ident{ a.line_info, AST::IDENT };
            ast_node_t func_args{ a.line_info, AST::FUNC_ARGS };
            func_call.line_info = a.line_info;
            ident.line_info = a.line_info;
            func_args.line_info = a.line_info;
            ident.data =
                a.children[0].comp_type.is_prog_string() ?
                (a.children[1].comp_type.is_prog_string() ? "$strcmp_PP" : "$strcmp_P") :
                (a.children[1].comp_type.is_prog_string() ? "$strcmp_P" : "$strcmp");
            {
                auto* arg0 = &a.children[0];
                auto* arg1 = &a.children[1];
                if(arg0->comp_type.is_prog_string())
                    std::swap(arg0, arg1);
                func_args.children.emplace_back(std::move(*arg0));
                func_args.children.emplace_back(std::move(*arg1));
            }
            func_call.children.emplace_back(std::move(ident));
            func_call.children.emplace_back(std::move(func_args));
            if(a.data == "==")
            {
                logical_op.type = AST::OP_UNARY;
                logical_op.children.emplace_back(ast_node_t{ a.line_info, AST::TOKEN, "!" });
            }
            else
            {
                logical_op.type = AST::OP_CAST;
                logical_op.children.emplace_back(ast_node_t{ a.line_info });
                logical_op.children.back().comp_type = TYPE_BOOL;
            }
            logical_op.children.emplace_back(std::move(func_call));
            a = std::move(logical_op);
            type_annotate_recurse(a, frame);
            return;
        }

        auto t0 = a.children[0].comp_type.without_ref();
        auto t1 = a.children[1].comp_type.without_ref();

        if(!check_prim(t0, a.children[0], errs) || !check_prim(t1, a.children[1], errs))
        {
            transform_constexprs(a, frame);
            break;
        }

        if((a.type == AST::OP_BITWISE_AND ||
            a.type == AST::OP_BITWISE_OR ||
            a.type == AST::OP_BITWISE_XOR) &&
            (t0.is_bool || t1.is_bool || t0.is_float || t1.is_float))
        {
            errs.push_back({
                "Bitwise and, or, and xor may not operate on boolean or floating-point types.",
                a.line_info });
            return;
        }

        bool is_float = t0.is_float || t1.is_float;
        bool ref0 = a.children[0].comp_type.is_ref();
        bool ref1 = a.children[1].comp_type.is_ref();
        bool is_divmod = (
            a.type == AST::OP_MULTIPLICATIVE && (
                a.data == "/" || a.data == "%"));

        if((a.type == AST::OP_EQUALITY || a.type == AST::OP_RELATIONAL) &&
            (t0.is_label_ref() && t1.is_label_ref() && t0.type == t1.type))
        {
            a.comp_type = TYPE_BOOL;
            transform_constexprs(a, frame);
            break;
        }

        t0.is_bool = false;
        t1.is_bool = false;
        t0.is_byte = false;
        t1.is_byte = false;

        if(is_float)
        {
            t0 = t1 = TYPE_FLOAT;
        }
        else if(a.type == AST::OP_ADDITIVE)
        {
            t0.prim_size = t1.prim_size = std::min<size_t>(4,
                std::max(t0.prim_size, t1.prim_size) + 1);
            bool t = (t0.is_signed || t1.is_signed);
            t0.is_signed = t1.is_signed = t;
        }
        else if(a.type == AST::OP_MULTIPLICATIVE && a.data == "*")
        {
            t0.prim_size = t1.prim_size = std::min<size_t>(4,
                t0.prim_size + t1.prim_size);
            bool t = (t0.is_signed || t1.is_signed);
            t0.is_signed = t1.is_signed = t;
        }
        else if(is_divmod && t0.is_signed != t1.is_signed)
        {
            t0.prim_size = t1.prim_size = std::min<size_t>(4,
                std::max(t0.prim_size, t1.prim_size) + 1);
        }
        if(t0 != t1 || ref0 || ref1)
        {
            implicit_conversion(t0, t1);
            implicit_conversion(t1, t0);
        }

        //a.comp_type = t0;
        //transform_constexprs(a, frame);
        //if(a.type == AST::INT_CONST || a.type == AST::FLOAT_CONST)
        //    break;

        if(!is_float && is_divmod)
        {
            if(t0.prim_size < 2 && t1.prim_size < 2)
                t0.prim_size = t1.prim_size = 2;
            else if(t0.prim_size < 4 && t1.prim_size < 4)
                t0.prim_size = t1.prim_size = 4;
        }

        if(ref0 || t0 != a.children[0].comp_type)
            a.children[0].insert_cast(t0);
        if(ref1 || t1 != a.children[1].comp_type)
            a.children[1].insert_cast(t1);
        transform_constexprs(a.children[0], frame);
        transform_constexprs(a.children[1], frame);

        if(a.type == AST::OP_EQUALITY || a.type == AST::OP_RELATIONAL)
            a.comp_type = TYPE_BOOL;
        else if(is_float)
            a.comp_type = TYPE_FLOAT;
        else
            a.comp_type = a.children[0].comp_type.without_ref();
        transform_constexprs(a, frame);
        break;
    }
    case AST::INT_CONST:
    case AST::FLOAT_CONST:
        // already done during parsing
        break;
    case AST::IDENT:
    {
        std::string name(a.data);
        for(auto it = frame.scopes.rbegin(); it != frame.scopes.rend(); ++it)
        {
            auto jt = it->locals.find(name);
            if(jt != it->locals.end())
            {
                a.comp_type = jt->second.var.type;
                if(!jt->second.var.is_constexpr)
                    a.comp_type = a.comp_type.with_ref();
                transform_constexprs(a, frame);
                return;
            }
        }
        auto it = globals.find(name);
        if(it != globals.end())
        {
            a.comp_type = it->second.var.type;
            if(!it->second.var.is_constexpr && !it->second.is_constexpr_ref())
                a.comp_type = a.comp_type.with_ref();
            transform_constexprs(a, frame);
            return;
        }
        errs.push_back({ "Undefined variable \"" + name + "\"", a.line_info });
        break;
    }
    case AST::FUNC_CALL:
    {
        assert(a.children.size() == 2);
        for(auto& c : a.children[1].children)
            type_annotate_recurse(c, frame);
        if(a.children[0].data == "len")
        {
            transform_array_len(a);
            break;
        }
        auto f = resolve_func(a.children[0]);
        bool is_format = sysfunc_is_format(f.name);
        if(f.name.empty()) break;

        auto* arg_types = &f.decl.arg_types;
        std::vector<compiler_type_t> format_types;
        if(is_format)
        {
            std::string format_str;
            resolve_format_call(a.children[1], f.decl, format_types, format_str);
            arg_types = &format_types;
        }

        size_t used_args = a.children[1].children.size();
        size_t func_args = arg_types->size();
        if(used_args > func_args)
        {
            errs.push_back({
                "Too many arguments to function \"" + f.name + "\"",
                a.line_info });
            return;
        }
        if(used_args < func_args)
        {
            errs.push_back({
                "Too few arguments to function \"" + f.name + "\"",
                a.line_info });
            return;
        }

        for(size_t i = 0; i < used_args; ++i)
        {
            auto& c = a.children[1].children[i];
            auto const& t = (*arg_types)[i];
            if(is_format &&
                i == f.decl.arg_types.size() - 1 &&
                c.type != AST::STRING_LITERAL)
            {
                errs.push_back({
                    "Format string must be a string literal",
                    c.line_info });
                return;
            }
            if(t.is_array_ref())
            {
                auto const& ct = c.comp_type.without_ref();
                if(ct.is_array_ref())
                {
                    if(t.children[0] != ct.children[0])
                    {
                        errs.push_back({
                            "Incompatible array element types for UARs",
                            c.line_info });
                        return;
                    }
                    continue;
                }
                if(t.children[0].is_prog != ct.is_prog)
                {
                    errs.push_back({
                        "Mismatched prog for UAR conversion",
                        c.line_info });
                    return;
                }
                insert_aref(c, t);
            }
            else if(c.comp_type != t && c.type != AST::COMPOUND_LITERAL)
            {
                c.insert_cast(t);
                transform_constexprs(c, frame);
            }
        }
        a.comp_type = f.decl.return_type;
        break;
    }
    case AST::ARRAY_INDEX:
    {
        assert(a.children.size() == 2);
        type_annotate_recurse(a.children[0], frame);
        type_annotate_recurse(a.children[1], frame);
        auto t0 = a.children[0].comp_type.without_ref();
        if(!t0.is_array() && !t0.is_array_ref())
        {
            errs.push_back({
                "\"" + std::string(a.children[0].data) +
                "\" is not an array", a.line_info });
            break;
        }
        a.comp_type = t0.children[0].with_ref();
        break;
    }
    case AST::ARRAY_SLICE:
    case AST::ARRAY_SLICE_LEN:
    {
        assert(a.children.size() == 3);
        type_annotate_recurse(a.children[0], frame);
        type_annotate_recurse(a.children[1], frame);
        type_annotate_recurse(a.children[2], frame);
        if(a.children[1].comp_type.is_float || a.children[2].comp_type.is_float)
        {
            errs.push_back({
                "Array slice indices may not be floating point values",
                a.line_info });
            break;
        }
        auto t0 = a.children[0].comp_type.without_ref();
        if(!t0.is_array() && !t0.is_array_ref())
        {
            errs.push_back({
                "\"" + std::string(a.children[0].data) +
                "\" is not an array", a.line_info });
            break;
        }
        if(
            (a.children[1].type == AST::INT_CONST || a.type == AST::ARRAY_SLICE_LEN) &&
            a.children[2].type == AST::INT_CONST)
        {
            int64_t n = (
                a.type == AST::ARRAY_SLICE_LEN ?
                a.children[2].value :
                a.children[2].value - a.children[1].value);
            if(n < 0)
            {
                errs.push_back({"Array slice has negative length", a.line_info});
                break;
            }
            a.comp_type = t0.children[0].with_array((size_t)n).with_ref();
        }
        else
        {
            a.comp_type = t0.children[0].with_array_ref();
        }
        break;
    }
    case AST::STRUCT_MEMBER:
    {
        assert(a.children.size() == 2);
        type_annotate_recurse(a.children[0], frame);
        if(!a.children[0].comp_type.without_ref().is_struct())
        {
            errs.push_back({
                "\"" + std::string(a.children[0].data) +
                "\" is not a struct", a.children[0].line_info
            });
            break;
        }
        std::string name = std::string(a.children[1].data);
        auto* t0 = resolve_member(a.children[0], name);
        if(!t0) break;
        a.comp_type = t0->with_ref();
        break;
    }
    case AST::STRING_LITERAL:
    {
        a.comp_type = strlit_type(strlit_data(a).size());
        break;
    }
    default:
        break;
    }
}

void compiler_t::type_reduce_recurse(ast_node_t& a, size_t size)
{
    if(!errs.empty()) return;
    auto min_size = std::min(a.comp_type.prim_size, size);
    if(a.comp_type.is_float)
        min_size = a.comp_type.prim_size;
    switch(a.type)
    {
    case AST::ARRAY_INDEX:
        type_reduce_recurse(
            a.children[1], a.children[0].comp_type.is_prog_array() ? 3 : 2);
        break;
    case AST::OP_CAST:
        if(a.children[0].comp_type.is_bool || !a.children[0].comp_type.is_prim())
            break;
        min_size = std::min(min_size, a.children[0].comp_type.prim_size);
        a.comp_type.prim_size = min_size;
        a.children[0].comp_type.prim_size = min_size;
        type_reduce_recurse(a.children[1], min_size);
        break;
    case AST::OP_SHIFT:
        type_reduce_recurse(a.children[0], 4);
        type_reduce_recurse(a.children[1], 1);
        break;
    case AST::OP_BITWISE_AND:
    case AST::OP_BITWISE_OR:
    case AST::OP_BITWISE_XOR:
    case AST::OP_ADDITIVE:
    case AST::OP_MULTIPLICATIVE:
        if(a.type == AST::OP_MULTIPLICATIVE && a.data != "*")
            break;
        a.comp_type.prim_size = min_size;
        type_reduce_recurse(a.children[0], min_size);
        type_reduce_recurse(a.children[1], min_size);
        if(a.children[0].comp_type.is_float || a.children[1].comp_type.is_float)
            a.comp_type = TYPE_FLOAT;
        break;
    case AST::OP_LOGICAL_AND:
    case AST::OP_LOGICAL_OR:
    case AST::OP_EQUALITY:
    case AST::OP_RELATIONAL:
        assert(a.children.size() == 2);
        for(auto& child : a.children)
            type_reduce_recurse(child, 4);
        break;
    case AST::OP_ASSIGN:
    case AST::OP_ASSIGN_COMPOUND:
        type_reduce_recurse(a.children[0], size);
        type_reduce_recurse(a.children[1], a.children[0].comp_type.without_ref().prim_size);
        break;
    case AST::OP_INC_POST:
    case AST::OP_DEC_POST:
        a.comp_type.prim_size = min_size;
        type_reduce_recurse(a.children[0], min_size);
        if(a.children[0].comp_type.is_float)
            a.comp_type = TYPE_FLOAT;
        break;
    case AST::FUNC_CALL:
    {
        auto func = resolve_func(a.children[0]);
        for(size_t i = 0; i < func.decl.arg_types.size(); ++i)
        {
            auto const& type = func.decl.arg_types[i];
            auto& expr = a.children[1].children[i];
            if(!type.is_prim())
                continue;
            type_reduce_recurse(expr, std::min(size, type.prim_size));
        }
        break;
    }
    case AST::IDENT:
    case AST::INT_CONST:
        if(a.comp_type.type == compiler_type_t::PRIM && min_size != a.comp_type.prim_size)
            a.insert_cast(a.comp_type.sized_to(min_size));
        break;
    default:
        break;
    }
}

void compiler_t::type_annotate(ast_node_t& a, compiler_frame_t const& frame, size_t size)
{
    a.recurse([](ast_node_t& n) {
        for(auto& child : n.children)
            child.parent = &n;
    });

    type_annotate_recurse(a, frame);
    if(errs.empty())
        type_reduce_recurse(a, size);

    a.recurse([](ast_node_t& n) {
        for(auto& child : n.children)
            child.parent = &n;
    });
}

}
