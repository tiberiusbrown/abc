#include "ards_compiler.hpp"

#include <sstream>

#include <cassert>

namespace ards
{

std::string compiler_t::progdata_label()
{
    std::ostringstream ss;
    ss << "$PD_" << progdata_label_index;
    ++progdata_label_index;
    return ss.str();
}

void compiler_t::add_custom_progdata(std::string const& label, std::vector<uint8_t>& data)
{
    auto& pdata = progdata[label];
    assert(pdata.data.empty());
    pdata.data = std::move(data);
}

void compiler_t::add_progdata(
    std::string const& label, compiler_type_t const& t, ast_node_t const& n)
{
    auto& pdata = progdata[label];
    assert(pdata.data.empty());
    progdata_expr(n, t, pdata);
}

void compiler_t::progdata_expr(
    ast_node_t const& n, compiler_type_t const& t, compiler_progdata_t& pd)
{
    if(!errs.empty()) return;

    switch(t.type)
    {
    case compiler_type_t::SPRITES:
    {
        encode_sprites(pd.data, n);
        break;
    }
    case compiler_type_t::PRIM:
    {
        if(n.type != AST::INT_CONST)
            goto error;
        uint64_t x = (uint64_t)n.value;
        for(size_t i = 0; i < t.prim_size; ++i)
        {
            pd.data.push_back((uint8_t)x);
            x >>= 8;
        }
        break;
    }
    case compiler_type_t::REF:
    case compiler_type_t::ARRAY_REF:
    {
        assert(t.children.size() == 1);
        std::string name;
        if(n.type == AST::STRING_LITERAL)
        {
            name = progdata_label();
        }
        else if(n.type == AST::IDENT)
            name = n.data;
        else
            goto error;
        if(t.children[0].is_prog)
        {
            pd.relocs_prog.push_back({ pd.data.size(), name });
            pd.data.push_back(0);
        }
        else
            pd.relocs_glob.push_back({ pd.data.size(), name });
        pd.data.push_back(0);
        pd.data.push_back(0);
        if(t.is_array_ref() && n.type == AST::IDENT)
        {
            auto* g = resolve_global(n);
            if(!g)
            {
                errs.push_back({
                    "Unable to resolve global \"" + name + "\"",
                    n.line_info });
                return;
            }
            auto size = g->var.type.array_size();
            if(size <= 0)
            {
                errs.push_back({
                    "Cannot initialize unsized array reference to non-array",
                    n.line_info });
                return;
            }
            pd.data.push_back(uint8_t(size >> 0));
            pd.data.push_back(uint8_t(size >> 8));
            if(t.children[0].is_prog)
                pd.data.push_back(uint8_t(size >> 16));
        }
        if(t.is_array_ref() && n.type == AST::STRING_LITERAL)
        {
            auto data = strlit_data(n);
            auto size = data.size();
            add_custom_progdata(name, data);
            pd.data.push_back(uint8_t(size >> 0));
            pd.data.push_back(uint8_t(size >> 8));
            pd.data.push_back(uint8_t(size >> 16));
            assert(t.children[0].is_prog);
        }
        break;
    }
    case compiler_type_t::ARRAY:
    {
        assert(t.children.size() == 1);
        size_t num_elems = t.prim_size / t.children[0].prim_size;
        if(n.type != AST::COMPOUND_LITERAL)
            goto error;
        if(n.children.size() != num_elems)
        {
            errs.push_back({
                "Incorrect number of elements in prog array initializer",
                n.line_info });
            return;
        }
        for(auto const& child : n.children)
        {
            progdata_expr(child, t.children[0], pd);
        }
        break;
    }
    case compiler_type_t::STRUCT:
    {
        if(n.type != AST::COMPOUND_LITERAL)
            goto error;
        if(n.children.size() != t.children.size())
        {
            errs.push_back({
                "Incorrect number of members in prog struct initializer",
                n.line_info });
            return;
        }
        for(size_t i = 0; i < n.children.size(); ++i)
        {
            auto const& child = n.children[i];
            auto const& tt = t.children[i];
            progdata_expr(child, tt, pd);
        }
        break;
    }
    default:
        errs.push_back({
            "Prog variable not initialized to constant expression",
            n.line_info });
        break;
    }
    return;
error:
    errs.push_back({ "Invalid expression type in prog initializer", n.line_info });
}

}
