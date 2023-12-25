#include "ards_compiler.hpp"

#include <filesystem>
#include <sstream>

#include <assert.h>

namespace ards
{

compiler_t::compiler_t()
    : progdata_label_index(0)
{}

static std::string const GLOBINIT_FUNC = "$globinit";
static std::string const FILE_INTERNAL = "<internal>";

std::unordered_set<std::string> const keywords =
{
    "u8", "i8", "u16", "i16", "u24", "i24", "u32", "i32",
    "void", "bool", "uchar", "char", "uint", "int", "ulong", "long",
    "if", "else", "while", "for", "return", "break", "continue",
    "constexpr", "saved", "prog", "sprites", "font", "tones",
    "struct", "import", "len",
};

std::unordered_map<std::string, compiler_type_t> const primitive_types
{
    { "void",    TYPE_VOID    },
    { "bool",    TYPE_BOOL    },
    { "char",    TYPE_CHAR    },
    { "u8",      TYPE_U8      },
    { "u16",     TYPE_U16     },
    { "u24",     TYPE_U24     },
    { "u32",     TYPE_U32     },
    { "i8",      TYPE_I8      },
    { "i16",     TYPE_I16     },
    { "i24",     TYPE_I24     },
    { "i32",     TYPE_I32     },
    { "ushort",  TYPE_U8      },
    { "uint",    TYPE_U16     },
    { "ulong",   TYPE_U32     },
    { "short",   TYPE_I8      },
    { "int",     TYPE_I16     },
    { "long",    TYPE_I32     },
    { "sprites", TYPE_SPRITES },
    { "font",    TYPE_FONT    },
    { "tones",   TYPE_TONES   },
};

bool sysfunc_is_format(std::string const& f)
{
    if(f.empty() || f[0] != '$')
        return false;
    auto it = sys_names.find(f.substr(1));
    if(it != sys_names.end())
        return sysfunc_is_format(it->second);
    return false;
}

std::vector<builtin_constexpr_t> const builtin_constexprs
{
    { "WHITE",        TYPE_U8,   1 },
    { "BLACK",        TYPE_U8,   0 },
    { "A_BUTTON",     TYPE_U8,   1 << 3 },
    { "B_BUTTON",     TYPE_U8,   1 << 2 },
    { "UP_BUTTON",    TYPE_U8,   1 << 7 },
    { "DOWN_BUTTON",  TYPE_U8,   1 << 4 },
    { "LEFT_BUTTON",  TYPE_U8,   1 << 5 },
    { "RIGHT_BUTTON", TYPE_U8,   1 << 6 },
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

static void make_prog(compiler_type_t& t)
{
    t.is_prog = true;
    if(t.is_any_ref())
        return;
    for(auto& child : t.children)
        make_prog(child);
}

compiler_type_t compiler_t::resolve_type(ast_node_t const& n)
{

    if(n.type == AST::TYPE_PROG)
    {
        assert(n.children.size() == 1);
        compiler_type_t t = resolve_type(n.children[0]);
        make_prog(t);
        return t;
    }

    if(n.type == AST::TYPE)
    {
        std::string name(n.data);
        compiler_type_t const* pt = nullptr;
        if(auto it = structs.find(name); it != structs.end())
            pt = &it->second;
        else if(auto it2 = primitive_types.find(name); it2 != primitive_types.end())
            pt = &it2->second;
        else
        {
            errs.push_back({
                    "Unknown type \"" + name + "\"",
                    n.line_info });
            return TYPE_NONE;
        }
        assert(pt != nullptr);
        compiler_type_t t{};
        if(n.children.empty())
        {
            t = *pt;
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
        t.prim_size = pt->prim_size * num;
        t.type = compiler_type_t::ARRAY;
        t.children.push_back(*pt);
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
        if( n.children[0].type == AST::TYPE_REF ||
            n.children[0].type == AST::TYPE_AREF ||
            n.children[0].type == AST::TYPE_AREF_PROG)
        {
            errs.push_back({ "Cannot create reference to reference", n.line_info });
            return TYPE_NONE;
        }
        compiler_type_t t{};
        t.type = compiler_type_t::REF;
        t.children.push_back(resolve_type(n.children[0]));
        t.prim_size = t.children[0].is_prog ? 3 : 2;
        return t;
    }
    if(n.type == AST::TYPE_AREF || n.type == AST::TYPE_AREF_PROG)
    {
        assert(n.children.size() == 1);
        if( n.children[0].type == AST::TYPE_REF ||
            n.children[0].type == AST::TYPE_AREF ||
            n.children[0].type == AST::TYPE_AREF_PROG)
        {
            errs.push_back({ "Cannot create reference to reference", n.line_info });
            return TYPE_NONE;
        }
        compiler_type_t t{};
        t.type = compiler_type_t::ARRAY_REF;
        t.children.push_back(resolve_type(n.children[0]));
        if(n.type == AST::TYPE_AREF_PROG)
            t.children[0].is_prog = true;
        t.prim_size = t.children[0].is_prog ? 6 : 4;
        return t;
    }
    if(n.type == AST::TYPE_ARRAY)
    {
        assert(n.children.size() == 2);
        if(n.children[0].type != AST::INT_CONST)
        {
            errs.push_back({
                "Array dimension \"" + std::string(n.children[0].data) +
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
        auto type = resolve_type(n.children[1]);
        if(type.is_prog)
        {
            errs.push_back({
                "Array elements may not be declared 'prog'. "
                "Declare the entire array 'prog' instead: "
                "'T[N] prog' instead of 'T prog[N]'",
                n.line_info });
            return TYPE_NONE;
        }
        t.children.emplace_back(std::move(type));
        t.prim_size = size_t(n.children[0].value) * t.children[0].prim_size;
        t.is_prog = t.children[0].is_prog;
        return t;
    }
    if(n.type == AST::STRUCT_STMT)
    {
        compiler_type_t t{};
        t.type = compiler_type_t::STRUCT;
        size_t size = 0;
        for(size_t i = 1; i < n.children.size(); ++i)
        {
            auto const& decl = n.children[i];
            for(size_t j = 1; j < decl.children.size(); ++j)
            {
                auto type = resolve_type(decl.children[0]);
                if(type.is_prog)
                {
                    errs.push_back({
                        "Struct members cannot be declared 'prog'",
                        decl.children[0].line_info });
                    return {};
                }
                t.children.emplace_back(std::move(type));
                t.members.push_back({ std::string(decl.children[j].data), size });
                size += t.children.back().prim_size;
            }
        }
        t.prim_size = size;
        return t;
    }

    return TYPE_NONE;
}

compiler_func_t compiler_t::resolve_func(ast_node_t const& n)
{
    assert(n.type == AST::IDENT);
    std::string name(n.data);
    assert(!name.empty());
    assert(name != "len");

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

std::string compiler_t::resolve_label_ref(
    compiler_frame_t const& frame, ast_node_t const& n, compiler_type_t const& t)
{
    if(n.type == AST::SPRITES)
    {
        std::string label = progdata_label();
        add_progdata(label, TYPE_SPRITES, n);
        return label;
    }
    else if(n.type == AST::FONT)
    {
        std::string label;
        font_key_t key{};
        assert(n.children.size() == 2);
        assert(n.children[0].type == AST::INT_CONST);
        assert(n.children[1].type == AST::STRING_LITERAL);
        key.first = (int)n.children[0].value;
        key.second = n.children[1].data;
        auto it = font_label_cache.find(key);
        if(it != font_label_cache.end())
            label = it->second;
        else
        {
            label = font_label_cache[key] = progdata_label();
            add_progdata(label, TYPE_FONT, n);
        }
        return label;
    }
    else if(n.type == AST::TONES)
    {
        std::string label = progdata_label();
        add_progdata(label, TYPE_TONES, n);
        return label;
    }
    else if(n.type == AST::IDENT)
    {
        if(auto* local = resolve_local(frame, n))
        {
            if(local->var.type != t)
                goto wrong_type;
            if(!local->var.type.is_label_ref() || local->var.label_ref.empty()) goto non_ref_type;
            return local->var.label_ref;
        }
        if(auto* global = resolve_global(n))
        {
            if(global->var.type != t)
                goto wrong_type;
            if(!global->var.type.is_label_ref() || global->var.label_ref.empty()) goto non_ref_type;
            return global->var.label_ref;
        }
    }

    assert(false);

wrong_type:
    errs.push_back({ "Incompatible types", n.line_info });
    return "";

non_ref_type:
    errs.push_back({ "Not a constant expression", n.line_info });
    return "";
}

void compiler_t::compile(
    std::string const& fpath,
    std::string const& fname,
    std::function<bool(std::string const&, std::vector<char>&)> const& loader,
    std::ostream& fo)
{
    assert(sysfunc_decls.size() == SYS_NUM);
    for(auto const& [k, v] : sysfunc_decls)
        assert(v.arg_types.size() == v.arg_names.size());

    font_label_cache.clear();

    funcs.clear();
    globals.clear();
    structs.clear();

    errs.clear();
    warns.clear();

    compiled_files.clear();
    import_set.clear();
    progdata.clear();
    input_data.clear();

    for(auto const& d : builtin_constexprs)
    {
        assert(globals.count(d.name) == 0);
        auto& g = globals[d.name];
        g.name = d.name;
        g.var.type = d.type;
        g.var.is_constexpr = true;
        g.var.value = d.value;
    }

    // create global constructor
    {
        auto& f = funcs[GLOBINIT_FUNC];
        f.name = GLOBINIT_FUNC;
        f.filename = FILE_INTERNAL;
    }

    file_loader = loader;
    base_path = fpath;
    current_path = fpath;
    current_file = fname;

    compile_recurse(fpath, fname);

    // add final ret to global constructor
    funcs[GLOBINIT_FUNC].instrs.push_back({ I_RET });

    // generate code for all functions
    for(auto& [n, f] : funcs)
    {
        if(!errs.empty()) return;
        if(f.block.type == AST::NONE) continue;
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

void compiler_t::compile_recurse(std::string const& fpath, std::string const& fname)
{
    std::vector<char> input;
    std::string pathbase = fpath.empty() ? "" : fpath + "/";
    std::string filename = pathbase + fname + ".abc";

    if(import_set.count(filename) != 0)
    {
        errs.push_back({ "Import loop detected" });
        return;
    }

    if(compiled_files.count(filename) != 0)
        return;

    auto& compile_data = compiled_files[filename];
    ast_node_t& ast = compile_data.second;
    if(!file_loader || !file_loader(filename, compile_data.first))
    {
        errs.push_back({ "Unable to open module \"" + fname + "\"" });
        return;
    }

    parse(compile_data.first, ast);
    if(!errs.empty()) return;

    // trim all token whitespace
    ast.recurse([](ast_node_t& n) {
        if(n.type == AST::STRING_LITERAL) return;
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

    // transform all multivariable DECL_STMTs into multiple single variable
    ast.recurse([&](ast_node_t& a) {
        for(size_t i = 0; i < a.children.size(); ++i)
        {
            if(a.type == AST::STRUCT_STMT) continue;
            if(a.children[i].type != AST::DECL_STMT) continue;
            auto t = std::move(a.children[i]);
            assert(t.children.size() >= 2);
            a.children.insert(a.children.begin() + i, t.children.size() - 2, {});
            for(size_t j = 1; j < t.children.size(); ++j)
            {
                auto& src = t.children[j];
                assert(src.type == AST::DECL_ITEM);
                if(src.children.size() < 2 && t.children[0].comp_type.is_constexpr)
                {
                    errs.push_back({
                        "Constexpr variable \"" + std::string(src.children[0].data) +
                        "\" must be initialized", t.line_info
                    });
                    return;
                }
                auto& v = a.children[i + j - 1];
                v = {};
                v.type = AST::DECL_STMT;
                v.data = t.data;
                v.line_info = t.line_info;
                v.children.push_back(t.children[0]);
                v.children.emplace_back(std::move(src.children[0]));
                if(src.children.size() >= 2)
                    v.children.emplace_back(std::move(src.children[1]));
            }
            i += (t.children.size() - 2);
        }
    });
    if(!errs.empty()) return;

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

    ast.recurse([](ast_node_t& a) {
        for(auto& child : a.children)
            child.parent = &a;
    });

    // gather all functions and globals and check for duplicates
    assert(ast.type == AST::PROGRAM);
    for(auto& n : ast.children)
    {
        if(!errs.empty()) return;
        assert(
            n.type == AST::DECL_STMT ||
            n.type == AST::FUNC_STMT ||
            n.type == AST::STRUCT_STMT ||
            n.type == AST::IMPORT_STMT);
        if(n.type == AST::DECL_STMT)
        {
            if(n.children.size() <= 2 &&
                (n.children[0].comp_type.is_constexpr ||
                n.children[0].comp_type.is_prog))
            {
                errs.push_back({
                    "Prog and constexpr variables must be initialized",
                    n.line_info });
                return;
            }
            assert(n.children[1].type == AST::IDENT);
            if(!check_identifier(n.children[1])) return;
            std::string name(n.children[1].data);
            if(symbol_exists(name))
            {
                errs.push_back({
                    "Duplicate symbol \"" + name + "\"",
                    n.children[1].line_info });
                return;
            }
            if(n.children.size() == 3)
                type_annotate(n.children[2], {});
            auto& g = globals[name];
            g.name = name;
            g.saved = n.children[0].comp_type.is_saved;
            type_annotate(n.children[0], {});
            g.var.type = resolve_type(n.children[0]);
            if(g.saved && (g.var.type.is_any_ref() || g.var.type.has_child_ref()))
            {
                errs.push_back({
                    "References and objects containing references cannot be declared 'saved'",
                    n.line_info });
                return;
            }
            if(n.children.size() <= 2 && g.var.type.is_any_ref())
            {
                errs.push_back({
                    "Uninitialized global reference \"" + std::string(n.children[1].data) + "\"",
                    n.line_info });
                return;
            }
            if(g.var.type.is_ref() &&
                g.var.type.without_ref() != n.children[2].comp_type.without_ref())
            {
                errs.push_back({
                    "Incorrect type for reference \"" + std::string(n.children[1].data) + "\"",
                    n.line_info });
                return;
            }
            if(n.children.size() <= 2 && g.var.type.has_child_ref())
            {
                errs.push_back({
                    "Uninitialized child reference(s) in \"" + std::string(n.children[1].data) + "\"",
                    n.line_info });
                return;
            }
            if(!errs.empty()) return;
            if(g.var.type.prim_size == 0)
            {
                errs.push_back({
                    "Global variable \"" + name + "\" has zero size",
                    n.line_info });
                return;
            }
            if(g.var.type.is_prog)
                add_progdata(name, g.var.type, n.children[2]);

            // constexpr primitive
            else if(n.children[0].comp_type.is_constexpr)
            {
                g.var.is_constexpr = true;
                if(n.children[2].type == AST::INT_CONST)
                {
                    g.var.value = n.children[2].value;
                }
                else if(g.var.type.is_label_ref())
                {
                    g.var.label_ref = resolve_label_ref({}, n.children[2], g.var.type);
                }
                else
                {
                    errs.push_back({
                        "\"" + std::string(n.children[2].data) +
                        "\" is not a constant expression",
                        n.children[2].line_info });
                    return;
                }
            }

            // constexpr-ify reference
            else if(g.var.type.is_ref() && n.children[2].type == AST::IDENT)
            {
                g.constexpr_ref = std::string(n.children[2].data);
            }

            // add to $globinit
            else if(n.children.size() == 3)
            {
                compiler_frame_t frame{};
                frame.push();
                auto& f = funcs[GLOBINIT_FUNC];
                if(!g.var.type.is_any_ref() && n.children[2].type == AST::COMPOUND_LITERAL)
                    codegen_expr_compound(f, frame, n.children[2], g.var.type);
                else
                    codegen_expr(f, frame, n.children[2], g.var.type.is_any_ref());
                auto lvalue = resolve_lvalue(f, frame, n.children[1]);
                if(n.children[2].type != AST::COMPOUND_LITERAL)
                    codegen_convert(f, frame, n.children[2], lvalue.type, n.children[2].comp_type);
                codegen_store_lvalue(f, frame, lvalue);
                for(size_t i = 0; i < frame.size; ++i)
                    f.instrs.push_back({ I_POP });
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
            if(symbol_exists(name))
            {
                errs.push_back({
                    "Duplicate symbol \"" + name + "\"",
                    n.children[1].line_info });
                return;
            }
            auto& f = funcs[name];
            f.decl.return_type = resolve_type(n.children[0]);
            f.name = name;
            f.filename =
                std::filesystem::path(current_path + "/" + fname + ".abc")
                .lexically_relative(std::filesystem::path(base_path))
                .generic_string();
            f.block = std::move(n.children[2]);

            if(name == "main" && f.decl.return_type != TYPE_VOID)
            {
                errs.push_back({
                    "The 'main' function must have return type 'void'",
                    n.children[0].line_info });
                return;
            }

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
                f.decl.arg_names.push_back(std::string(tname.data));
            }

            if(name == "main" && !f.arg_names.empty())
            {
                errs.push_back({
                    "The 'main' function must take no arguments",
                    n.children[3].line_info });
                return;
            }
        }
        else if(n.type == AST::STRUCT_STMT)
        {
            std::string name(n.children[0].data);
            if(symbol_exists(name))
            {
                errs.push_back({
                    "Duplicate symbol \"" + name + "\"",
                    n.line_info });
                return;
            }
            structs[name] = resolve_type(n);
        }
        else if(n.type == AST::IMPORT_STMT)
        {
            std::string new_path = fpath;
            for(size_t i = 0; i + 1 < n.children.size(); ++i)
            {
                new_path += "/";
                new_path += n.children[i].data;
            }
            std::string new_file = std::string(n.children.back().data);
            import_set.insert(filename);
            std::string old_path = std::move(current_path);
            std::string old_file = std::move(current_file);
            current_path = new_path;
            current_file = new_file;
            compile_recurse(new_path, new_file);
            if(errs.empty())
            {
                current_path = std::move(old_path);
                current_file = std::move(old_file);
            }
            import_set.erase(filename);
        }
    }
}

static uint8_t hex2val(char hex)
{
    if(hex >= '0' && hex <= '9') return uint8_t(hex - '0');
    if(hex >= 'a' && hex <= 'f') return uint8_t(hex - 'a' + 10);
    if(hex >= 'A' && hex <= 'F') return uint8_t(hex - 'A' + 10);
    return 255;
}

std::vector<uint8_t> compiler_t::strlit_data(ast_node_t const& n)
{
    std::vector<uint8_t> d;
    d.reserve(n.data.size());
    for(auto it = n.data.begin(); it != n.data.end(); ++it)
    {
        char c = *it;
        if(c == '\\')
        {
            ++it;
            if(it == n.data.end()) break;
            switch(*it)
            {
            case 'n' : c = '\n'; break;
            case 'r' : c = '\r'; break;
            case 't' : c = '\t'; break;
            case '"' : c = '\"'; break;
            case '\'': c = '\''; break;
            case '\\': c = '\\'; break;
            case 'x':
            {
                if(++it == n.data.end()) break;
                uint8_t nib1 = hex2val(*it);
                if(++it == n.data.end()) break;
                uint8_t nib0 = hex2val(*it);
                if(nib0 == 255 || nib1 == 255) break;
                c = nib1 * 16 + nib0;
                break;
            }
            default: break;
            }
        }
        d.push_back(c);
    }
    return d;
}

compiler_type_t compiler_t::strlit_type(size_t len)
{
    compiler_type_t type{};
    type.type = compiler_type_t::REF;
    type.prim_size = 3;
    type.children.resize(1);
    auto& t = type.children[0];
    t.type = compiler_type_t::ARRAY;
    t.prim_size = len;
    t.children.push_back(TYPE_CHAR);
    make_prog(t);
    return type;
}

void compiler_t::add_custom_label_ref(
    std::string const& name,
    compiler_type_t const& t)
{
    if(symbol_exists(name))
    {
        errs.push_back({ "Duplicate symbol \"" + name + "\"" });
        return;
    }
    auto& g = globals[name];
    g.name = name;
    g.var.type = t;
    g.var.label_ref = name;
}


std::string type_name(ards::compiler_type_t const& t, bool noprog)
{
    std::stringstream ss;
    auto tt = t.without_prog();
    if(tt == ards::TYPE_VOID)
        ss << "void";
    else if(tt == ards::TYPE_SPRITES)
        ss << "sprites";
    else if(tt == ards::TYPE_FONT)
        ss << "font";
    else if(tt == ards::TYPE_TONES)
        ss << "tones";
    else if(t.is_array())
    {
        ss << type_name(t.children[0], t.children[0].is_prog) << "[" << t.array_size() << "]";
        if(t.children[0].is_prog) ss << " prog";
    }
    else if(t.is_prim())
    {
        if(tt == ards::TYPE_BOOL) ss << "bool";
        else if(tt == ards::TYPE_CHAR) ss << "char";
        else
        {
            ss << (t.is_signed ? "i" : "u");
            ss << (t.prim_size * 8);
        }
    }
    else if(t.is_ref())
    {
        ss << type_name(t.children[0]) << "&";
    }
    else if(t.is_array_ref())
    {
        ss << type_name(t.children[0], t.children[0].is_prog) << "[]";
        if(t.children[0].is_prog) ss << " prog";
        ss << "&";
    }
    if(!noprog && t.is_prog) ss << " prog";
    return ss.str();
}

}
