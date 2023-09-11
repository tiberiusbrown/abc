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
    {
        assert(t.children.size() == 1);
        if(n.type != AST::IDENT)
            goto error;
        std::string name(n.data);
        if(t.children[0].is_prog)
        {
            pd.relocs_prog.push_back({ pd.data.size(), name });
            pd.data.push_back(0);
        }
        else
            pd.relocs_glob.push_back({ pd.data.size(), name });
        pd.data.push_back(0);
        pd.data.push_back(0);
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
