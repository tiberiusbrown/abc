#include "ards_compiler.hpp"

#include <cassert>
#include <cmath>

namespace ards
{

void compiler_t::transform_array_len(ast_node_t& n)
{
    if(!errs.empty()) return;
    assert(n.type == AST::FUNC_CALL);
    assert(n.children.size() == 2);
    assert(n.children[1].children.size() == 1);
    assert(n.children[0].data == "len");
    if(n.type != AST::FUNC_CALL ||
        n.children.size() != 2 ||
        n.children[1].children.size() != 1 ||
        n.children[0].data != "len")
    {
        errs.push_back({
            "len() must be given exactly one argument",
            n.line_info });
        return;
    }

    auto const& type = n.children[1].children[0].comp_type;
    auto const& rtype = type.without_ref();
    if(rtype.is_array())
    {
        n.type = AST::INT_CONST;
        n.value = rtype.array_size();
        n.comp_type = prim_type_for_hex((uint32_t)n.value, false);
        return;
    }
    else if(rtype.is_array_ref())
    {
        n.type = AST::ARRAY_LEN;
        n.comp_type = rtype.children[0].is_prog ? TYPE_U24 : TYPE_U16;
        return;
    }
    /*
    else if(rtype.is_sprites())
    {
        n.type = AST::SPRITES_LEN;
        n.comp_type = TYPE_U16;
        return;
    }
    */
    else
    {
        errs.push_back({
            "len() operates on arrays only",
            n.line_info });
        return;
    }
}

void compiler_t::transform_constexprs(ast_node_t& n, compiler_frame_t const& frame)
{
    if(!errs.empty()) return;
    {
        bool child1_prim = n.children.size() >= 2 && (
            n.children[1].type == AST::INT_CONST ||
            n.children[1].type == AST::FLOAT_CONST);
        if(!(
            n.type == AST::OP_UNARY && child1_prim ||
            n.type == AST::OP_CAST && child1_prim))
        {
            for(auto const& child : n.children)
            {
                if((child.type != AST::INT_CONST && child.type != AST::FLOAT_CONST) ||
                    child.comp_type.type != compiler_type_t::PRIM)
                    return;
            }
        }
    }
    
    bool is_float = n.comp_type.is_float;
    switch(n.type)
    {
    case AST::IDENT:
        if(auto* l = resolve_local(frame, n))
        {
            if(l && l->var.is_constexpr)
            {
                n.comp_type = l->var.type;
                n.comp_type.is_constexpr = false;
                if(l->var.type.is_label_ref())
                {
                    n.type = AST::LABEL_REF;
                    n.data = l->var.label_ref;
                    return;
                }
                n.value = l->var.value;
                break;
            }
            return;
        }
        if(auto* g = resolve_global(n); g && g->var.is_constexpr)
        {
            n.comp_type = g->var.type;
            n.comp_type.is_constexpr = false;
            if(g->var.type.is_label_ref())
            {
                n.type = AST::LABEL_REF;
                n.data = g->var.label_ref;
                return;
            }
            n.value = g->var.value;
            break;
        }
        return;
    case AST::OP_CAST:
        assert(n.children.size() == 2);
        if(n.children[0].comp_type.without_ref().is_label_ref() &&
            n.children[1].comp_type.without_ref() != n.children[0].comp_type.without_ref())
        {
            errs.push_back({
                "Incompatible conversion to asset handle type",
                n.line_info });
            return;
        }
        if(is_float == n.children[1].comp_type.is_float)
            n.value = n.children[1].value;
        else if(is_float)
            n.fvalue = (double)n.children[1].value;
        else
            n.value = (int64_t)n.children[1].fvalue;
        break;
    case AST::OP_ADDITIVE:
        assert(n.children.size() == 2);
        assert(is_float == n.children[0].comp_type.is_float);
        assert(is_float == n.children[1].comp_type.is_float);
        if(is_float)
        {
            if(n.data == "+")
                n.fvalue = n.children[0].fvalue + n.children[1].fvalue;
            else if(n.data == "-")
                n.fvalue = n.children[0].fvalue - n.children[1].fvalue;
            else
                assert(false);
        }
        else
        {
            if(n.data == "+")
                n.value = n.children[0].value + n.children[1].value;
            else if(n.data == "-")
                n.value = n.children[0].value - n.children[1].value;
            else
                assert(false);
        }
        break;
    case AST::OP_MULTIPLICATIVE:
        assert(n.children.size() == 2);
        assert(is_float == n.children[0].comp_type.is_float);
        assert(is_float == n.children[1].comp_type.is_float);
        if(is_float)
        {
            if(n.data == "*")
                n.fvalue = n.children[0].fvalue * n.children[1].fvalue;
            else
            {
                if(n.children[1].fvalue == 0)
                {
                    errs.push_back({ "Division by zero in constant expression", n.line_info });
                    return;
                }
                if(n.data == "/")
                    n.fvalue = n.children[0].fvalue / n.children[1].fvalue;
                else if(n.data == "%")
                {
                    errs.push_back({
                        "The modulo operator may not be applied to floating point values",
                        n.line_info });
                    return;
                }
                else
                    assert(false);
            }
        }
        else
        {
            if(n.data == "*")
                n.value = n.children[0].value * n.children[1].value;
            else
            {
                if(n.children[1].value == 0)
                {
                    errs.push_back({ "Division by zero in constant expression", n.line_info });
                    return;
                }
                if(n.data == "/")
                    n.value = n.children[0].value / n.children[1].value;
                else if(n.data == "%")
                    n.value = n.children[0].value % n.children[1].value;
                else
                    assert(false);
            }
        }
        break;
    case AST::OP_TERNARY:
    {
        assert(n.children.size() == 3);
        if(is_float)
        {
            n.value = n.children[0].value != 0 ? n.children[1].value : n.children[2].value;
        }
        else
        {
            n.fvalue = n.children[0].value != 0 ? n.children[1].fvalue : n.children[2].fvalue;
        }
        break;
    }
    case AST::OP_SHIFT:
        assert(n.children.size() == 2);
        if(is_float || n.children[0].comp_type.is_float)
        {
            errs.push_back({
                "Floating point values may not be shifted",
                n.line_info });
            return;
        }
        if(n.data == "<<")
        {
            uint8_t shift = (uint8_t)n.children[1].value;
            if(shift > 32) shift = 32;
            n.value = n.children[0].value << shift;
        }
        else if(n.data == ">>")
        {
            uint8_t shift = (uint8_t)n.children[1].value;
            if(shift > 32) shift = 32;
            if(n.children[0].comp_type.is_signed)
                n.value = n.children[0].value >> shift;
            else
                n.value = (uint64_t)n.children[0].value >> shift;
        }
        else
            assert(false);
        break;
    case AST::OP_BITWISE_AND:
        assert(n.children.size() == 2);
        if(is_float || n.children[0].comp_type.is_float || n.children[1].comp_type.is_float)
        {
            errs.push_back({
                "Bitwise AND may not be applied to floating point values",
                n.line_info });
            return;
        }
        n.value = int64_t(uint64_t(n.children[0].value) & uint64_t(n.children[1].value));
        break;
    case AST::OP_BITWISE_OR:
        assert(n.children.size() == 2);
        if(is_float || n.children[0].comp_type.is_float || n.children[1].comp_type.is_float)
        {
            errs.push_back({
                "Bitwise OR may not be applied to floating point values",
                n.line_info });
            return;
        }
        n.value = int64_t(uint64_t(n.children[0].value) | uint64_t(n.children[1].value));
        break;
    case AST::OP_BITWISE_XOR:
        assert(n.children.size() == 2);
        if(is_float || n.children[0].comp_type.is_float || n.children[1].comp_type.is_float)
        {
            errs.push_back({
                "Bitwise XOR may not be applied to floating point values",
                n.line_info });
            return;
        }
        n.value = int64_t(uint64_t(n.children[0].value) ^ uint64_t(n.children[1].value));
        break;
    case AST::OP_LOGICAL_AND:
        assert(n.children.size() == 2);
        if(is_float || n.children[0].comp_type.is_float || n.children[1].comp_type.is_float)
        {
            errs.push_back({
                "Logical AND may not be applied to floating point values",
                n.line_info });
            return;
        }
        n.value = int64_t(n.children[0].value && n.children[1].value);
        break;
    case AST::OP_LOGICAL_OR:
        assert(n.children.size() == 2);
        if(is_float || n.children[0].comp_type.is_float || n.children[1].comp_type.is_float)
        {
            errs.push_back({
                "Logical OR may not be applied to floating point values",
                n.line_info });
            return;
        }
        n.value = int64_t(n.children[0].value || n.children[1].value);
        break;
    case AST::OP_UNARY:
    {
        assert(n.children.size() == 2);
        assert(is_float == n.children[1].comp_type.is_float);
        auto op = n.children[0].data;
        if(op == "!")
        {
            if(is_float)
            {
                errs.push_back({
                    "Logical NOT may not be applied to floating point values",
                    n.line_info });
                return;
            }
            n.value = int64_t(!n.children[1].value);
        }
        else if(op == "-")
        {
            if(is_float)
                n.fvalue = -n.children[1].fvalue;
            else
                n.value = -n.children[1].value;
        }
        else if(op == "~")
        {
            if(is_float)
            {
                errs.push_back({
                    "Bitwise NOT may not be applied to floating point values",
                    n.line_info });
                return;
            }
            n.value = int64_t(~uint64_t(n.children[1].value));
        }
        else
            assert(false);
        break;
    }
    case AST::OP_EQUALITY:
    case AST::OP_RELATIONAL:
        assert(n.children.size() == 2);
        assert(n.children[0].comp_type.is_float == n.children[1].comp_type.is_float);
        if(n.children[0].comp_type.is_float)
        {
            if(n.data == "==")
                n.value = int64_t(n.children[0].fvalue == n.children[1].fvalue);
            else if(n.data == "!=")
                n.value = int64_t(n.children[0].fvalue != n.children[1].fvalue);
            else if(n.data == "<=")
                n.value = int64_t(n.children[0].fvalue <= n.children[1].fvalue);
            else if(n.data == ">=")
                n.value = int64_t(n.children[0].fvalue >= n.children[1].fvalue);
            else if(n.data == "<")
                n.value = int64_t(n.children[0].fvalue < n.children[1].fvalue);
            else if(n.data == ">")
                n.value = int64_t(n.children[0].fvalue > n.children[1].fvalue);
            else
                assert(false);
        }
        else
        {
            if(n.data == "==")
                n.value = int64_t(n.children[0].value == n.children[1].value);
            else if(n.data == "!=")
                n.value = int64_t(n.children[0].value != n.children[1].value);
            else if(n.data == "<=")
                n.value = int64_t(n.children[0].value <= n.children[1].value);
            else if(n.data == ">=")
                n.value = int64_t(n.children[0].value >= n.children[1].value);
            else if(n.data == "<")
                n.value = int64_t(n.children[0].value < n.children[1].value);
            else if(n.data == ">")
                n.value = int64_t(n.children[0].value > n.children[1].value);
            else
                assert(false);
        }
        break;
    default:
        return;
    }

    assert(!(n.comp_type.is_float&& n.comp_type.is_signed));

    bool preserve_type = (
        n.type == AST::OP_CAST ||
        n.type == AST::IDENT);

    // if we got here, the node was simplified
    n.type = is_float ? AST::FLOAT_CONST : AST::INT_CONST;

    // mask value according to size
    if(!is_float)
    {
        assert(n.comp_type.prim_size >= 1);
        assert(n.comp_type.prim_size <= 4);
        constexpr uint64_t SIGNS[4] =
        {
            0x0000000000000080ull,
            0x0000000000008000ull,
            0x0000000000800000ull,
            0x0000000080000000ull,
        };
        constexpr uint64_t MASKS[4] =
        {
            0xffffffffffffff00ull,
            0xffffffffffff0000ull,
            0xffffffffff000000ull,
            0xffffffff00000000ull,
        };

        if(!preserve_type)
        {
            n.comp_type.prim_size = (n.comp_type.is_signed && n.value < 0) ?
                prim_type_for_dec((uint32_t)(-n.value), n.comp_type.is_signed).prim_size :
                prim_type_for_dec((uint32_t)(+n.value), n.comp_type.is_signed).prim_size;
        }
        else
        {
            uint64_t sign = SIGNS[n.comp_type.prim_size - 1];
            uint64_t mask = MASKS[n.comp_type.prim_size - 1];
            if(n.comp_type.is_bool)
                n.value = uint64_t(n.value != 0);
            if(n.comp_type.is_signed && ((uint64_t)n.value & sign))
                n.value = int64_t(uint64_t(n.value) | mask);
            else
                n.value = int64_t(uint64_t(n.value) & ~mask);
        }
    }

    assert(!(n.comp_type.is_float && n.comp_type.is_signed));

    n.children.clear();
}

void compiler_t::transform_left_assoc_infix(ast_node_t& n)
{
    if(!errs.empty()) return;
    // transform left-associative infix chains into left binary trees
    for(auto& child : n.children)
        transform_left_assoc_infix(child);
    switch(n.type)
    {
    case AST::OP_LOGICAL_AND:
    case AST::OP_LOGICAL_OR:
    case AST::OP_BITWISE_AND:
    case AST::OP_BITWISE_OR:
    case AST::OP_BITWISE_XOR:
    {
        assert(n.children.size() >= 2);
        ast_node_t a = std::move(n.children[0]);
        ast_node_t op{ n.line_info, n.type, n.data };
        for(size_t i = 1; i < n.children.size(); ++i)
        {
            ast_node_t b = std::move(n.children[i]);
            ast_node_t top = op;
            top.children.emplace_back(std::move(a));
            top.children.emplace_back(std::move(b));
            a = std::move(top);
        }
        n = std::move(a);
        break;
    }
    case AST::OP_EQUALITY:
    case AST::OP_RELATIONAL:
    case AST::OP_SHIFT:
    case AST::OP_ADDITIVE:
    case AST::OP_MULTIPLICATIVE:
    {
        assert(n.children.size() >= 3);
        assert(n.children.size() % 2 == 1);
        ast_node_t a = std::move(n.children[0]);
        for(size_t i = 1; i < n.children.size(); i += 2)
        {
            ast_node_t op = std::move(n.children[i]);
            ast_node_t b = std::move(n.children[i + 1]);
            op.type = n.type;
            op.children.emplace_back(std::move(a));
            op.children.emplace_back(std::move(b));
            a = std::move(op);
        }
        n = std::move(a);
        break;
    }
    default:
        break;
    }
}

}
