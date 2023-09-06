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
    compiler_type_t const& t, ast_node_t const& a, std::vector<error_t> errs)
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

static void insert_cast(ast_node_t& a, compiler_type_t const& t)
{
    auto ta = std::move(a);
    a = { {}, AST::OP_CAST };
    a.comp_type = t;
    a.children.push_back({});
    a.children.back().comp_type = t;
    a.children.emplace_back(std::move(ta));
};

void compiler_t::type_annotate_recurse(ast_node_t& a, compiler_frame_t const& frame)
{
    if(!errs.empty()) return;
    switch(a.type)
    {
    case AST::OP_CAST:
        assert(a.children.size() == 2);
        type_annotate_recurse(a.children[1], frame);
        a.comp_type = a.children[0].comp_type;
        break;
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
        break;
    }
    case AST::OP_LOGICAL_AND:
    case AST::OP_LOGICAL_OR:
        assert(a.children.size() == 2);
        for(auto& child : a.children)
            type_annotate_recurse(child, frame);
        a.comp_type = TYPE_BOOL;
        break;
    case AST::OP_ASSIGN:
    {
        assert(a.children.size() == 2);
        for(auto& child : a.children)
            type_annotate_recurse(child, frame);
        a.comp_type = a.children[0].comp_type.without_ref();
        break;
    }
    case AST::OP_SHIFT:
        assert(a.children.size() == 2);
        for(auto& child : a.children)
            type_annotate_recurse(child, frame);
        if(!check_prim(a.children[0], errs)) break;
        if(!check_prim(a.children[1], errs)) break;
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
        auto t0 = a.children[0].comp_type.without_ref();
        auto t1 = a.children[1].comp_type.without_ref();

        if((a.type == AST::OP_BITWISE_AND ||
            a.type == AST::OP_BITWISE_OR  ||
            a.type == AST::OP_BITWISE_XOR) &&
            (t0.is_bool || t1.is_bool))
        {
            errs.push_back({
                "Bitwise and, or, and xor may not operate on boolean types.",
                a.line_info });
            return;
        }

        bool ref0 = a.children[0].comp_type.type == compiler_type_t::REF;
        bool ref1 = a.children[1].comp_type.type == compiler_type_t::REF;
        bool is_divmod = (
            a.type == AST::OP_MULTIPLICATIVE && (
            a.data == "/" || a.data == "%"));
        if(!check_prim(t0, a.children[0], errs)) break;
        if(!check_prim(t1, a.children[1], errs)) break;

        t0.is_bool = false;
        t1.is_bool = false;

        if(a.type == AST::OP_ADDITIVE)
        {
            t0.prim_size = t1.prim_size = std::min<size_t>(4, 
                std::max(t0.prim_size, t1.prim_size) + 1);
            t0.is_signed = t1.is_signed = (t0.is_signed || t1.is_signed);
        }
        else if(a.type == AST::OP_MULTIPLICATIVE && a.data == "*")
        {
            t0.prim_size = t1.prim_size = std::min<size_t>(4,
                t0.prim_size + t1.prim_size);
            t0.is_signed = t1.is_signed = (t0.is_signed || t1.is_signed);
        }
        else if(is_divmod && t0.is_signed != t1.is_signed)
        {
            t0.prim_size = t1.prim_size = std::max(t0.prim_size, t1.prim_size) + 1;
        }
        if(t0 != t1 || ref0 || ref1)
        {
            implicit_conversion(t0, t1);
            implicit_conversion(t1, t0);
        }

        if(is_divmod)
        {
            if(t0.prim_size < 2 && t1.prim_size < 2)
                t0.prim_size = t1.prim_size = 2;
            else if(t0.prim_size < 4 && t1.prim_size < 4)
                t0.prim_size = t1.prim_size = 4;
        }

        if(ref0 || t0 != a.children[0].comp_type)
            insert_cast(a.children[0], t0);
        if(ref1 || t1 != a.children[1].comp_type)
            insert_cast(a.children[1], t1);

        if(a.type == AST::OP_EQUALITY || a.type == AST::OP_RELATIONAL)
            a.comp_type = TYPE_BOOL;
        else
            a.comp_type = a.children[0].comp_type.without_ref();
        break;
    }
    case AST::INT_CONST:
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
                a.comp_type = jt->second.type;
                return;
            }
        }
        auto it = globals.find(name);
        if(it != globals.end())
        {
            a.comp_type = it->second.type;
            return;
        }
        errs.push_back({ "Undefined variable \"" + name + "\"", a.line_info });
        break;
    }
    case AST::FUNC_CALL:
    {
        assert(a.children.size() == 2);
        auto f = resolve_func(a.children[0]);
        if(f.name.empty()) break;
        for(size_t i = 0; i < a.children[1].children.size(); ++i)
        {
            auto& c = a.children[1].children[i];
            auto const& t = f.decl.arg_types[i];
            type_annotate_recurse(c, frame);
            if(c.comp_type != t)
                insert_cast(c, t);
        }
        a.comp_type = f.decl.return_type;
        break;
    }
    case AST::ARRAY_INDEX:
    {
        assert(a.children.size() == 2);
        type_annotate_recurse(a.children[0], frame);
        type_annotate_recurse(a.children[1], frame);
        auto t0 = a.children[0].comp_type;
        auto t1 = a.children[1].comp_type;
        auto tt = t0.ref_type();
        if(tt != compiler_type_t::ARRAY)
        {
            errs.push_back({
                "\"" + std::string(a.children[0].data) +
                "\" is not an array", a.line_info });
            break;
        }
        a.comp_type.type = compiler_type_t::REF;
        a.comp_type.prim_size = 2;
        a.comp_type.children.push_back(t0.without_ref().children[0]);
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
    switch(a.type)
    {
    case AST::ARRAY_INDEX:
        type_reduce_recurse(a.children[1], std::min<size_t>(2, min_size));
        break;
    case AST::OP_CAST:
        if(a.children[0].comp_type.is_bool)
            break;
        min_size = std::min(min_size, a.children[0].comp_type.prim_size);
        a.comp_type.prim_size = min_size;
        a.children[0].comp_type.prim_size = min_size;
        type_reduce_recurse(a.children[1], min_size);
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
        break;
    case AST::OP_ASSIGN:
        a.comp_type.prim_size = min_size;
        type_reduce_recurse(a.children[1], a.children[0].comp_type.without_ref().prim_size);
        break;
    case AST::FUNC_CALL:
    {
        auto func = resolve_func(a.children[0]);
        for(size_t i = 0; i < func.decl.arg_types.size(); ++i)
        {
            auto const& type = func.decl.arg_types[i];
            auto& expr = a.children[1].children[i];
            if(type.type != compiler_type_t::PRIM)
                continue;
            type_reduce_recurse(expr, std::min(size, type.prim_size));
        }
        break;
    }
    case AST::IDENT:
    case AST::INT_CONST:
        if(a.comp_type.type == compiler_type_t::PRIM && min_size != a.comp_type.prim_size)
            insert_cast(a, a.comp_type.sized_to(min_size));
        break;
    default:
        break;
    }
}

void compiler_t::type_annotate(ast_node_t& a, compiler_frame_t const& frame, size_t size)
{
    type_annotate_recurse(a, frame);
    transform_constexprs(a, frame);
    type_reduce_recurse(a, size);
}

}
