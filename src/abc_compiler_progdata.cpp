#include "abc_compiler.hpp"

#include <algorithm>
#include <sstream>

#include <cassert>

#include <rapidjson/document.h>

namespace abc
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
    try_merge_progdata(label, pdata);
}

void compiler_t::add_progdata(
    std::string const& label, compiler_type_t const& t, ast_node_t const& n)
{
    auto& pdata = progdata[label];
    assert(pdata.data.empty());
    progdata_expr(n, t, pdata);
    try_merge_progdata(label, pdata);
}

void compiler_t::try_merge_progdata(
    std::string const& label, compiler_progdata_t& pdata)
{
#if 0
    (void)label;
    (void)pdata;
#else
    auto const& data = pdata.data;
    if(!pdata.relocs_glob.empty()) return;
    if(!pdata.relocs_prog.empty()) return;
    for(auto& [k, pd] : progdata)
    {
        if(k == label)
            continue;
        // search for data inside pd.data
        for(auto it = pd.data.begin();;)
        {
            it = std::search(
                it, pd.data.end(),
                data.begin(), data.end());
            if(it == pd.data.end())
                break;

            // we found a subsequence match: ensure there are no relocs inside it
            size_t index = size_t(it - pd.data.begin());
            bool valid = true;
            for(auto const& p : pd.relocs_glob)
                if(p.first >= index && p.first < index + data.size())
                    valid = false;
            for(auto const& p : pd.relocs_prog)
                if(p.first >= index && p.first < index + data.size())
                    valid = false;
            if(!valid)
                continue;

            // add a intermediate label into pd
            pd.inter_labels.push_back({ index, label });
            std::sort(pd.inter_labels.begin(), pd.inter_labels.end());
            pdata.data.clear();
            pdata.merged = true;
            return;
        }
    }
#endif
}

void compiler_t::progdata_zero(
    ast_node_t const& n, compiler_type_t const& t, compiler_progdata_t& pd)
{
    if(t.is_any_ref() || t.has_child_ref())
    {
        errs.push_back({
            "Too few initializers for reference types",
            n.line_info });
        return;
    }
    for(size_t i = 0; i < t.prim_size; ++i)
        pd.data.push_back(0);
}

bool compiler_t::progdata_encode_prim(
        ast_node_t const& n, compiler_type_t const& t, std::vector<uint8_t>& data)
{
    assert(t.type == compiler_type_t::PRIM);
    uint64_t x;
    if(n.type == AST::INT_CONST)
        x = (uint64_t)n.value;
    else if(n.type == AST::FLOAT_CONST)
    {
        float f = (float)n.fvalue;
        union { float f; uint32_t x; } u;
        u.f = f;
        x = u.x;
    }
    else return false;
    for(size_t i = 0; i < t.prim_size; ++i)
    {
        data.push_back((uint8_t)x);
        x >>= 8;
    }
    return true;
}

bool compiler_t::progdata_expr_valid_memcpy(
        ast_node_t const& n, compiler_type_t const& t, std::vector<uint8_t>& data)
{
    switch(t.type)
    {
    case compiler_type_t::PRIM:
        return progdata_encode_prim(n, t, data);
    case compiler_type_t::ARRAY:
    {
        assert(t.children.size() == 1);
        size_t num_elems = t.prim_size / t.children[0].prim_size;
        if(n.type != AST::COMPOUND_LITERAL)
            return false;
        if(n.children.size() > num_elems)
            return false;
        for(auto const& child : n.children)
            if(!progdata_expr_valid_memcpy(child, t.children[0], data))
                return false;
        size_t rem_bytes = num_elems - n.children.size();
        rem_bytes *= t.children[0].prim_size;
        for(size_t i = 0; i < rem_bytes; ++i)
            data.push_back(0);
        return true;
    }
    case compiler_type_t::STRUCT:
    {
        if(n.type != AST::COMPOUND_LITERAL)
            return false;
        if(n.children.size() > t.children.size())
            return false;
        for(size_t i = 0; i < n.children.size(); ++i)
        {
            auto const& child = n.children[i];
            auto const& tt = t.children[i];
            if(!progdata_expr_valid_memcpy(child, tt, data))
                return false;
        }
        for(size_t i = n.children.size(); i < t.children.size(); ++i)
        {
            if(t.is_any_ref() || t.has_child_ref())
                return false;
            for(size_t j = 0; j < t.children[i].prim_size; ++j)
                data.push_back(0);
        }
        return true;
    }
    default:
        break;
    }
    return false;
}

void compiler_t::progdata_expr(
    ast_node_t const& n, compiler_type_t const& t, compiler_progdata_t& pd)
{
    if(!errs.empty()) return;

    switch(t.type)
    {
    case compiler_type_t::SPRITES:
    case compiler_type_t::FONT:
    case compiler_type_t::TONES:
    case compiler_type_t::MUSIC:
    case compiler_type_t::TILEMAP:
        if(n.type == AST::LABEL_REF)
        {
            std::string name(n.data);
            pd.relocs_prog.push_back({ pd.data.size(), name });
            pd.data.push_back(0);
            pd.data.push_back(0);
            pd.data.push_back(0);
            return;
        }
        break;
    default:
        break;
    }

    switch(t.type)
    {
    case compiler_type_t::SPRITES:
    {
        encode_sprites(pd.data, n);
        break;
    }
    case compiler_type_t::FONT:
    {
        encode_font(pd.data, n);
        break;
    }
    case compiler_type_t::MUSIC:
    {
        encode_tones_midi(pd.data, n.children[0].string_literal(), true);
        break;
    }
    case compiler_type_t::TILEMAP:
    {
        std::string error;
        if(n.children[0].type == AST::STRING_LITERAL)
        {
            std::string layer_name;
            if(n.children.size() == 2)
                layer_name = n.children[1].string_literal();
            error = encode_tilemap_tmx(
                pd.data, n.children[0].string_literal(), layer_name);
        }
        else
        {
            error = encode_tilemap_literal(pd.data, n);
        }
        if(!error.empty())
            errs.push_back({ error, n.line_info });
        break;
    }
    case compiler_type_t::TONES:
    {
        encode_tones(pd.data, n);
        break;
    }
    case compiler_type_t::PRIM:
    {
        if(!progdata_encode_prim(n, t, pd.data))
            goto error;
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
        if(t.children[0].is_char && n.type == AST::STRING_LITERAL)
        {
            auto data = strlit_data(n);            
            for(size_t i = 0; i < num_elems; ++i)
                pd.data.push_back(i < data.size() ? data[i] : 0);
            break;
        }
        if(n.type != AST::COMPOUND_LITERAL)
            goto error;
        if(n.children.size() > num_elems)
        {
            errs.push_back({
                "Too many elements in prog array initializer",
                n.line_info });
            return;
        }
        for(auto const& child : n.children)
            progdata_expr(child, t.children[0], pd);
        for(size_t i = n.children.size(); i < num_elems; ++i)
            progdata_zero(n, t.children[0], pd);
        break;
    }
    case compiler_type_t::STRUCT:
    {
        if(n.type != AST::COMPOUND_LITERAL)
            goto error;
        if(n.children.size() > t.children.size())
        {
            errs.push_back({
                "Too many members in prog struct initializer",
                n.line_info });
            return;
        }
        for(size_t i = 0; i < n.children.size(); ++i)
        {
            auto const& child = n.children[i];
            auto const& tt = t.children[i];
            progdata_expr(child, tt, pd);
        }
        for(size_t i = n.children.size(); i < t.children.size(); ++i)
            progdata_zero(n, t.children[i], pd);
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
