#include "ards_compiler.hpp"

#include <assert.h>

namespace ards
{

std::unordered_set<std::string> const keywords =
{
    "u8", "i8", "u16", "i16", "u24", "i24", "u32", "i32",
    "void", "bool", "uchar", "char", "uint", "int", "ulong", "long",
    "if", "else", "while", "for", "return", "break", "continue",
    "constexpr", "prog",
};

std::unordered_map<std::string, compiler_type_t> const primitive_types
{
    { "void",   TYPE_VOID },
    { "bool",   TYPE_BOOL },
    { "u8",     TYPE_U8   },
    { "u16",    TYPE_U16  },
    { "u24",    TYPE_U24  },
    { "u32",    TYPE_U32  },
    { "i8",     TYPE_I8   },
    { "i16",    TYPE_I16  },
    { "i24",    TYPE_I24  },
    { "i32",    TYPE_I32  },
    { "uchar",  TYPE_U8   },
    { "uint",   TYPE_U16  },
    { "ulong",  TYPE_U32  },
    { "char",   TYPE_I8   },
    { "int",    TYPE_I16  },
    { "long",   TYPE_I32  },
};

std::unordered_map<sysfunc_t, compiler_func_decl_t> const sysfunc_decls
{
    { SYS_DISPLAY,          { TYPE_VOID, {} } },
    { SYS_DRAW_PIXEL,       { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_U8 } } },
    { SYS_DRAW_FILLED_RECT, { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8, TYPE_U8 } } },
    { SYS_SET_FRAME_RATE,   { TYPE_VOID, { TYPE_U8 } } },
    { SYS_NEXT_FRAME,       { TYPE_BOOL, {} } },
    { SYS_IDLE,             { TYPE_VOID, {} } },
    { SYS_DEBUG_BREAK,      { TYPE_VOID, {} } },
    { SYS_ASSERT,           { TYPE_VOID, { TYPE_BOOL } } },
    { SYS_POLL_BUTTONS,     { TYPE_VOID, {} } },
    { SYS_JUST_PRESSED,     { TYPE_BOOL, { TYPE_U8 } } },
    { SYS_JUST_RELEASED,    { TYPE_BOOL, { TYPE_U8 } } },
    { SYS_PRESSED,          { TYPE_BOOL, { TYPE_U8 } } },
    { SYS_ANY_PRESSED,      { TYPE_BOOL, { TYPE_U8 } } },
    { SYS_NOT_PRESSED,      { TYPE_BOOL, { TYPE_U8 } } },
};

static bool isspace(char c)
{
    switch(c)
    {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
        return true;
    default:
        return false;
    }
}

compiler_type_t compiler_t::resolve_type(ast_node_t const& n)
{

    if(n.type == AST::TYPE_PROG)
    {
        assert(n.children.size() == 1);
        compiler_type_t t = resolve_type(n.children[0]);
        t.is_prog = true;
        return t;
    }

    if(n.type == AST::TYPE)
    {
        std::string name(n.data);
        auto it = primitive_types.find(name);
        if(it == primitive_types.end())
        {
            errs.push_back({
                "Unknown type \"" + name + "\"",
                n.line_info });
            return TYPE_NONE;
        }
        compiler_type_t t{};
        if(n.children.empty())
        {
            t = it->second;
            t.is_constexpr = n.comp_type.is_constexpr;
            return t;
        }
        // array
        auto num = n.children[0].value;
        if(num <= 0)
        {
            errs.push_back({
                   "Array type \"" + name + "\" does not have a positive size",
                   n.line_info });
            return TYPE_NONE;
        }
        t.prim_size = it->second.prim_size * num;
        t.type = compiler_type_t::ARRAY;
        t.children.push_back(it->second);
        t.is_constexpr = n.comp_type.is_constexpr;
        return t;
    }

    if(n.comp_type.is_constexpr)
    {
        errs.push_back({ "Only primitive types may be declared 'constexpr'", n.line_info });
        return TYPE_NONE;
    }

    if(n.type == AST::TYPE_REF)
    {
        assert(n.children.size() == 1);
        if(n.children[0].type == AST::TYPE_REF || n.children[0].type == AST::TYPE_AREF)
        {
            errs.push_back({ "Cannot create reference to reference", n.line_info });
            return TYPE_NONE;
        }
        compiler_type_t t{};
        t.type = compiler_type_t::REF;
        t.prim_size = 2;
        t.children.push_back(resolve_type(n.children[0]));
        return t;
    }
    if(n.type == AST::TYPE_AREF)
    {
        assert(n.children.size() == 1);
        if(n.children[0].type == AST::TYPE_REF || n.children[0].type == AST::TYPE_AREF)
        {
            errs.push_back({ "Cannot create reference to reference", n.line_info });
            return TYPE_NONE;
        }
        compiler_type_t t{};
        t.type = compiler_type_t::ARRAY_REF;
        t.prim_size = 4;
        t.children.push_back(resolve_type(n.children[0]));
        return t;
    }
    if(n.type == AST::TYPE_ARRAY)
    {
        assert(n.children.size() == 2);
        if(n.children[0].type != AST::INT_CONST)
        {
            errs.push_back({
                "\"" + std::string(n.children[0].data) +
                "\" is not a constant expression",
                n.children[0].line_info });
            return TYPE_NONE;
        }
        if(n.children[0].value <= 0)
        {
            errs.push_back({
                "Array dimensions must be greater than zero",
                n.children[0].line_info });
            return TYPE_NONE;
        }
        compiler_type_t t{};
        t.type = compiler_type_t::ARRAY;
        t.children.push_back(resolve_type(n.children[1]));
        t.prim_size = size_t(n.children[0].value) * t.children[0].prim_size;
        return t;
    }

    return TYPE_NONE;
}

compiler_func_t compiler_t::resolve_func(ast_node_t const& n)
{
    assert(n.type == AST::IDENT);
    std::string name(n.data);
    assert(!name.empty());

    if(name[0] == '$')
    {
        auto it = sys_names.find(name.substr(1));
        if(it != sys_names.end())
        {
            compiler_func_t f{};
            f.sys = it->second;
            f.is_sys = true;
            f.name = name;
            auto jt = sysfunc_decls.find(it->second);
            assert(jt != sysfunc_decls.end());
            f.decl = jt->second;
            return f;
        }
        else
        {
            errs.push_back({ "Undefined system function: \"" + name + "\"", n.line_info });
            return {};
        }
    }

    {
        auto it = funcs.find(name);
        if(it != funcs.end())
        {
            return it->second;
        }
    }

    errs.push_back({ "Undefined function: \"" + name + "\"", n.line_info });
    return {};
}

bool compiler_t::check_identifier(ast_node_t const& n)
{
    assert(n.type == AST::IDENT);
    std::string ident(n.data);
    if(keywords.count(ident) != 0)
    {
        errs.push_back({
            "\"" + ident + "\" is a keyword and cannot be used as an identifier",
            n.line_info });
        return false;
    }
    return true;
}

void compiler_t::compile(std::istream& fi, std::ostream& fo, std::string const& filename)
{
    assert(sysfunc_decls.size() == SYS_NUM);

    parse(fi);
    if(!errs.empty()) return;

    // trim all token whitespace
    ast.recurse([](ast_node_t& n) {
        size_t size = n.data.size();
        size_t i;
        if(size == 0) return;
        for(i = 0; isspace(n.data[i]); ++i);
        n.data.remove_prefix(i);
        for(i = 0; isspace(n.data[size - i - 1]); ++i);
        n.data.remove_suffix(i);
    });

    //
    // transforms
    //

    // transform left-associative infix exprs into binary trees
    transform_left_assoc_infix(ast);

    // reduce constant expressions
    // this is now done after type annotation
    //transform_constexprs(ast);

    // transforms TODO
    // - remove root ops in expr statements that have no side effects

    // transform function calls with type names to casts
    ast.recurse([&](ast_node_t& a) {
        if(a.type != AST::FUNC_CALL) return;
        std::string name(a.children[0].data);
        auto it = primitive_types.find(name);
        if(it == primitive_types.end()) return;
        a.children[0].comp_type = it->second;
        if(a.children[1].children.size() != 1)
        {
            errs.push_back({ "Multiple arguments to cast", a.line_info });
            return;
        }
        ast_node_t t = std::move(a.children[1].children[0]);
        a.children[1] = std::move(t);
        a.type = AST::OP_CAST;
    });

    // gather all functions and globals and check for duplicates
    assert(ast.type == AST::PROGRAM);
    for(auto& n : ast.children)
    {
        if(!errs.empty()) return;
        assert(n.type == AST::DECL_STMT || n.type == AST::FUNC_STMT);
        if(n.type == AST::DECL_STMT)
        {
            if(n.children.size() > 2 &&
                !n.children[0].comp_type.is_constexpr &&
                !n.children[0].comp_type.is_prog)
            {
                errs.push_back({
                    "Only constexpr and prog global variables can be initialized",
                    n.line_info });
                return;
            }
            assert(n.children[1].type == AST::IDENT);
            if(!check_identifier(n.children[1])) return;
            std::string name(n.children[1].data);
            auto it = globals.find(name);
            if(it != globals.end())
            {
                errs.push_back({
                    "Duplicate global \"" + name + "\"",
                    n.children[1].line_info });
                return;
            }
            if(n.children[0].comp_type.is_constexpr)
                type_annotate(n.children[2], {});
            auto& g = globals[name];
            g.name = name;
            type_annotate(n.children[0], {});
            g.var.type = resolve_type(n.children[0]);
            if(!errs.empty()) return;
            if(g.var.type.prim_size == 0)
            {
                errs.push_back({
                    "Global variable \"" + name + "\" has zero size",
                    n.line_info });
                return;
            }
            if(n.children[0].comp_type.is_constexpr)
            {
                g.var.is_constexpr = true;
                if(n.children[2].type == AST::INT_CONST)
                    g.var.value = n.children[2].value;
                else
                {
                    errs.push_back({
                        "\"" + std::string(n.children[2].data) +
                        "\" is not a constant expression",
                        n.children[2].line_info });
                    return;
                }
            }
        }
        else if(n.type == AST::FUNC_STMT)
        {
            assert(n.children.size() == 4);
            assert(n.children[1].type == AST::IDENT);
            assert(n.children[2].type == AST::BLOCK);
            assert(n.children[3].type == AST::LIST);
            std::string name(n.children[1].data);
            {
                auto it = sys_names.find(name);
                if(it != sys_names.end())
                {
                    errs.push_back({
                        "Function \"" + name + "\" is a system method",
                        n.children[1].line_info });
                    return;
                }
            }
            {
                auto it = funcs.find(name);
                if(it != funcs.end())
                {
                    errs.push_back({
                        "Duplicate function \"" + name + "\"",
                        n.children[1].line_info });
                    return;
                }
            }
            auto& f = funcs[name];
            f.decl.return_type = resolve_type(n.children[0]);
            f.name = name;
            f.filename = filename;
            f.block = std::move(n.children[2]);

            // decl args
            auto const& decls = n.children[3].children;
            assert(decls.size() % 2 == 0);
            for(size_t i = 0; i < decls.size(); i += 2)
            {
                auto const& ttype = decls[i + 0];
                auto const& tname = decls[i + 1];
                assert(tname.type == AST::IDENT);
                f.arg_names.push_back(std::string(tname.data));
                f.decl.arg_types.push_back(resolve_type(ttype));
            }
        }
    }

    // transforms are done: set parent pointers
    for(auto& [k, v] : funcs)
    {
        v.block.recurse([](ast_node_t& a) {
            for(auto& child : a.children)
                child.parent = &a;
        });
    }

    // generate code for all functions
    for(auto& [n, f] : funcs)
    {
        if(!errs.empty()) return;
        codegen_function(f);
    }

    // peephole optimizations
    for(auto& [n, f] : funcs)
    {
        if(!errs.empty()) return;
        peephole(f);
    }

    write(fo);
}

}
