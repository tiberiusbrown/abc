#include "ards_compiler.hpp"

#include <algorithm>
#include <assert.h>

namespace ards
{

void compiler_t::type_annotate(ast_node_t& a, compiler_frame_t const& frame)
{
    if(!errs.empty()) return;
    switch(a.type)
    {
    case AST::OP_ASSIGN:
    {
        for(auto& child : a.children)
            type_annotate(child, frame);
        assert(a.children.size() == 2);
        a.comp_type = a.children[0].comp_type;
        break;
    }
    case AST::OP_ADDITIVE:
    {
        assert(a.children.size() == 2);
        for(auto& child : a.children)
            type_annotate(child, frame);
        a.comp_type.prim_signed =
            a.children[0].comp_type.prim_signed ||
            a.children[1].comp_type.prim_signed;
        a.comp_type.prim_size = std::max(
            a.children[0].comp_type.prim_size,
            a.children[1].comp_type.prim_size);
        break;
    }
    case AST::INT_CONST:
    {
        int64_t* vp = std::get_if<int64_t>(&a.value);
        assert(vp);
        int64_t v = *vp;
        uint8_t prim_size = 1;
        a.comp_type.prim_signed = (v < 0);
        if(v < 0)
        {
            if(v < -(1 << 23)) prim_size = 4;
            if(v < -(1 << 15)) prim_size = 3;
            if(v < -(1 << 7)) prim_size = 2;
        }
        else
        {
            if(v >= (1 << 24)) prim_size = 4;
            if(v >= (1 << 16)) prim_size = 3;
            if(v >= (1 << 8)) prim_size = 2;
        }
        a.comp_type.prim_size = prim_size;
        break;
    }
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
        assert(a.children.size() >= 1);
        for(size_t i = 1; i < a.children.size(); ++i)
            type_annotate(a.children[i], frame);
        auto const& ident = a.children[0];
        assert(ident.type == AST::IDENT);
        std::string name(ident.data);
        {
            auto it = sys_names.find(name);
            if(it != sys_names.end())
            {
                auto jt = sysfunc_decls.find(it->second);
                assert(jt != sysfunc_decls.end());
                a.comp_type = jt->second.return_type;
                return;
            }
        }
        {
            auto it = funcs.find(name);
            if(it != funcs.end())
            {
                a.comp_type = it->second.decl.return_type;
                return;
            }
        }
        errs.push_back({ "Undefined function \"" + name + "\"", a.line_info });
        break;
    }
    default:
        break;
    }
}

}
