#include "ards_compiler.hpp"

#include <cassert>

namespace ards
{

void compiler_t::transform_constexprs(ast_node_t& n)
{
    if(!errs.empty()) return;
    //if(n.type >= AST::EXPR_BEGIN)
        for(auto& child : n.children)
            transform_constexprs(child);
    if(!(
        n.type == AST::OP_UNARY && n.children[1].type == AST::INT_CONST ||
        n.type == AST::OP_CAST && n.children[1].type == AST::INT_CONST))
    {
        for(auto const& child : n.children)
        {
            if(child.type != AST::INT_CONST || child.comp_type.type != compiler_type_t::PRIM)
                return;
        }
    }
    switch(n.type)
    {
    case AST::OP_CAST:
        assert(n.children.size() == 2);
        n.value = n.children[1].value;
        break;
    case AST::OP_ADDITIVE:
        assert(n.children.size() == 2);
        if(n.data == "+")
            n.value = n.children[0].value + n.children[1].value;
        else if(n.data == "-")
            n.value = n.children[0].value - n.children[1].value;
        else
            assert(false);
        break;
    case AST::OP_MULTIPLICATIVE:
        assert(n.children.size() == 2);
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
        break;
    case AST::OP_SHIFT:
        assert(n.children.size() == 2);
        if(n.data == "<<")
            n.value = n.children[0].value << (uint8_t)n.children[1].value;
        else if(n.data == ">>")
        {
            if(n.children[0].comp_type.is_signed)
                n.value = n.children[0].value >> (uint8_t)n.children[1].value;
            else
                n.value = (uint64_t)n.children[0].value >> (uint8_t)n.children[1].value;
        }
        else
            assert(false);
        break;
    case AST::OP_UNARY:
    {
        assert(n.children.size() == 2);
        auto op = n.children[0].data;
        if(op == "!")
            n.value = int64_t(!n.children[1].value);
        else if(op == "-")
            n.value = -n.children[1].value;
        else
            assert(false);
        break;
    }
    case AST::OP_EQUALITY:
    case AST::OP_RELATIONAL:
        assert(n.children.size() == 2);
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
        break;
    default:
        return;
    }

    // if we got here, the node was simplified
    n.type = AST::INT_CONST;

    // mask value according to size
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
        uint64_t sign = SIGNS[n.comp_type.prim_size - 1];
        uint64_t mask = MASKS[n.comp_type.prim_size - 1];
        if(n.comp_type.is_bool)
            n.value = uint64_t(n.value != 0);
        if(n.comp_type.is_signed && ((uint64_t)n.value & sign))
            n.value = int64_t(uint64_t(n.value) | mask);
        else
            n.value = int64_t(uint64_t(n.value) & ~mask);
    }

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
