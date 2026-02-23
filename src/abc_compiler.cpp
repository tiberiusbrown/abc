#include "abc_compiler.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>

#include <assert.h>

#include <fmt/chrono.h>

#include "fonts/all_fonts.hpp"

namespace abc
{

static std::string const GLOBINIT_FUNC = "$globinit";
static std::string const FILE_INTERNAL = "<internal>";

static std::vector<std::string> const directive_keywords =
{
    "title", "author", "version", "description",
    "date", "genre", "publisher", "idea", "code", "art", "sound",
    "url", "sourceUrl", "email", "companion",
    "shades",
};

std::unordered_set<std::string> const keywords =
{
    "u8", "i8", "u16", "i16", "u24", "i24", "u32", "i32",
    "void", "bool", "uchar", "char", "uint", "int", "ulong", "long",
    "sprites", "font", "tones", "music", "tilemap",
    "constexpr", "saved", "prog",
    "if", "else", "while", "for", "return", "break", "continue",
    "struct", "import", "len", "float", "byte", "enum", "do",
    "switch", "case", "default",
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
    { "float",   TYPE_FLOAT   },
    { "byte",    TYPE_BYTE    },
    { "long",    TYPE_I32     },
    { "sprites", TYPE_SPRITES },
    { "font",    TYPE_FONT    },
    { "tones",   TYPE_TONES   },
    { "music",   TYPE_MUSIC   },
    { "tilemap", TYPE_TILEMAP },
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
    { "WHITE",        TYPE_U8,    1 },
    { "LIGHT_GRAY",   TYPE_U8,    1 },
    { "LIGHT_GREY",   TYPE_U8,    1 },
    { "GRAY",         TYPE_U8,    1 },
    { "GREY",         TYPE_U8,    1 },
    { "DARK_GRAY",    TYPE_U8,    0 },
    { "DARK_GREY",    TYPE_U8,    0 },
    { "BLACK",        TYPE_U8,    0 },
    { "SHADES",       TYPE_U8,    2 },
    { "A_BUTTON",     TYPE_U8,    1 << 3 },
    { "B_BUTTON",     TYPE_U8,    1 << 2 },
    { "UP_BUTTON",    TYPE_U8,    1 << 7 },
    { "DOWN_BUTTON",  TYPE_U8,    1 << 4 },
    { "LEFT_BUTTON",  TYPE_U8,    1 << 5 },
    { "RIGHT_BUTTON", TYPE_U8,    1 << 6 },
    { "PI",           TYPE_FLOAT, 0, 3.14159265358979323846 },
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

bool compiler_t::convertible(compiler_type_t const& dst, compiler_type_t const& src)
{
    if(dst.is_any_ref() && src.is_constexpr)
        return false;
    if(dst.is_prim() && src.without_ref().is_prim())
        return true;
    if(dst.is_array_ref() && dst.children[0].is_byte)
    {
        if(!src.is_any_ref())
            return false;
        return dst.children[0].is_prog == src.children[0].is_prog;
    }
    auto const& rdst = dst.without_ref();
    auto const& rsrc = src.without_ref();
    if(rsrc.without_prog() == dst)
        return true;
    if(rdst.is_struct_or_union() && rsrc.is_struct_or_union())
        return rdst.struct_name == rsrc.struct_name;
    if(dst.is_array_ref() && src.is_ref() && rsrc.is_array())
        return dst.children[0] == rsrc.children[0];
    if(dst.is_ref())
        return dst == src;
    return dst == rsrc;
}

compiler_type_t compiler_t::resolve_type(ast_node_t const& n)
{

    if(n.type == AST::TYPE_PROG)
    {
        assert(n.children.size() == 1);
        compiler_type_t t = resolve_type(n.children[0]);
        t.make_prog();
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
        else if(auto it3 = enums.find(name); it3 != enums.end())
            pt = &it3->second;
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
            t.children[0].make_prog();
        t.prim_size = t.children[0].is_prog ? 6 : 4;
        return t;
    }
    if(n.type == AST::TYPE_ARRAY)
    {
        assert(n.children.size() == 2);
        if(n.children[0].type != AST::INT_CONST && n.children[0].type != AST::FLOAT_CONST)
        {
            errs.push_back({
                "Array dimension \"" + std::string(n.children[0].data) +
                "\" is not a constant expression",
                n.children[0].line_info });
            return TYPE_NONE;
        }
        bool is_float = (n.children[0].type == AST::FLOAT_CONST);
        int64_t value = (is_float ? (int64_t)n.children[0].fvalue : n.children[0].value);
        if(value <= 0)
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
        t.prim_size = size_t(value) * t.children[0].prim_size;
        t.is_prog = t.children[0].is_prog;
        return t;
    }
    if(n.type == AST::STRUCT_STMT)
    {
        compiler_type_t t{};
        t.type = compiler_type_t::STRUCT;
        if(n.children[0].data == "union")
            t.type = compiler_type_t::UNION;
        t.struct_name = n.children[1].data;
        size_t size = 0;
        for(size_t i = 2; i < n.children.size(); ++i)
        {
            auto const& decl = n.children[i];
            for(size_t j = 1; j < decl.children.size(); ++j)
            {
                auto type = resolve_type(decl.children[0]);
                std::string name = std::string(decl.children[j].data);
                if(t.is_union() && !type.is_copyable())
                {
                    errs.push_back({
                        "Union member \"" + name + "\" is not copyable "
                        "(all members of a union must be copyable)",
                        decl.children[0].line_info });
                    return {};
                }
                if(type.is_prog)
                {
                    errs.push_back({
                        "Struct and union members cannot be declared 'prog'",
                        decl.children[0].line_info });
                    return {};
                }
                t.children.emplace_back(std::move(type));
                size_t offset = t.is_struct() ? size : 0;
                t.members.push_back({ name, offset });
                if(t.is_struct())
                    size += t.children.back().prim_size;
                else
                    size = std::max(size, t.children.back().prim_size);
            }
        }
        t.prim_size = size;
        return t;
    }

    if(n.type == AST::ENUM_STMT)
    {
        assert(0);
        return TYPE_NONE;
    }

    if(n.type == AST::TYPE_FUNC_REF)
    {
        compiler_type_t t{};
        t.type = compiler_type_t::FUNC_REF;
        t.prim_size = 3;
        for(auto const& child : n.children)
            t.children.emplace_back(std::move(resolve_type(child)));
        return t;
    }

    return TYPE_NONE;
}

bool compiler_t::check_sysfunc_overload(compiler_func_decl_t const& decl, ast_node_t const& n)
{
    assert(n.children.size() >= 2);
    auto const& args = n.children[1];
    assert(args.type == AST::FUNC_ARGS);
    if(args.children.size() != decl.arg_types.size())
        return false;
    for(size_t i = 0; i < args.children.size(); ++i)
    {
        if(!convertible(decl.arg_types[i], args.children[i].comp_type))
            return false;
    }
    return true;
}

compiler_func_t compiler_t::resolve_func(ast_node_t const& n)
{
    assert(n.type == AST::FUNC_CALL);
    assert(n.children[0].type == AST::IDENT);
    std::string name(n.children[0].data);
    assert(!name.empty());
    assert(name != "len");

    if(name[0] == '$')
    {
        auto subname = name.substr(1);
        if(auto it = sys_overloads.find(subname); it != sys_overloads.end())
        {
            for(auto const& oname : it->second)
            {
                auto jt = sys_names.find(oname);
                assert(jt != sys_names.end());
                auto s = jt->second;
                auto kt = sysfunc_decls.find(s);
                assert(kt != sysfunc_decls.end());
                if(!check_sysfunc_overload(kt->second.decl, n))
                    continue;
                compiler_func_t f{};
                f.sys = s;
                f.is_sys = true;
                f.name = oname;
                f.decl = kt->second.decl;
                return f;
            }
        }
        if(auto it = sys_names.find(subname); it != sys_names.end())
        {
            compiler_func_t f{};
            f.sys = it->second;
            f.is_sys = true;
            f.name = name;
            auto jt = sysfunc_decls.find(it->second);
            assert(jt != sysfunc_decls.end());
            f.decl = jt->second.decl;
            return f;
        }
        errs.push_back({ "Undefined system function: \"" + name + "\"", n.line_info });
        return {};
    }

    {
        auto it = funcs.find(name);
        if(it != funcs.end())
        {
            return it->second;
        }
    }

    //errs.push_back({ "Undefined function: \"" + name + "\"", n.line_info });
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
    // TODO: add caches for each resource type
    if(n.type == AST::LABEL_REF)
    {
        return std::string(n.data);
    }
    else if(n.type == AST::SPRITES)
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
        key.second = n.children[1].string_literal();
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
    else if(n.type == AST::MUSIC)
    {
        std::string label = progdata_label();
        add_progdata(label, TYPE_MUSIC, n);
        return label;
    }
    else if(n.type == AST::TONES || n.type == AST::TONES_RTTTL)
    {
        std::string label = progdata_label();
        add_progdata(label, TYPE_TONES, n);
        return label;
    }
    else if(n.type == AST::TILEMAP)
    {
        std::string label = progdata_label();
        add_progdata(label, TYPE_TILEMAP, n);
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

void compiler_t::create_builtin_font(compiler_global_t& g)
{
    std::string label = "$PDF_" + g.name;
    if(progdata.count(label))
    {
        g.constexpr_ref = label;
        g.var.label_ref = g.constexpr_ref;
        return;
    }
    for(auto const& f : ALL_FONTS)
    {
        if(g.name != f.name) continue;
        if(!g.constexpr_ref.empty()) return;
        g.constexpr_ref = label;
        g.var.label_ref = g.constexpr_ref;
        std::vector<uint8_t> data;
        data.resize(f.size);
        memcpy(data.data(), f.data, data.size());
        add_custom_progdata(label, data);
        break;
    }
}

static auto tie_var(compiler_global_t const* v)
{
    return std::tie(v->var.type.prim_size, v->name);
}

void compiler_t::compile(
    std::string const& fpath,
    std::string const& fname,
    std::function<bool(std::string const&, std::vector<char>&)> const& loader)
{
    assert(sysfunc_decls.size() == SYS_NUM);
    for(auto const& [k, v] : sysfunc_decls)
        assert(v.decl.arg_types.size() == v.decl.arg_names.size());

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

    arduboy_file_directives.clear();
    arduboy_file_directives["title"] = "Untitled Arduboy Game";
    arduboy_file_directives["author"] = "Unknown Author";
    arduboy_file_directives["version"] = "1.0";
    arduboy_file_directives["date"] =
        fmt::format("{:%Y-%m-%d}", fmt::localtime(std::time(nullptr)));
    shades = 2;
    non_directive_found = false;

    for(auto const& d : builtin_constexprs)
    {
        assert(globals.count(d.name) == 0);
        auto& g = globals[d.name];
        g.name = d.name;
        g.var.type = d.type;
        g.var.is_constexpr = true;
        if(d.type.is_float)
            g.var.fvalue = d.fvalue;
        else
            g.var.value = d.value;
    }

    // builtin fonts
    for(auto const& f : ALL_FONTS)
    {
        auto& g = globals[f.name];
        g.name = f.name;
        g.var.type = TYPE_FONT;
        g.var.is_constexpr = true;
        g.var.value = 0;
        g.builtin = true;
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
    {
        uint16_t ret_line = 1;
        auto& f = funcs[GLOBINIT_FUNC];
        if(f.instrs.size() > 0)
            ret_line = f.instrs.back().line;
        f.instrs.push_back({ I_RET, ret_line });
    }

    // generate code for all functions
    for(auto& [n, f] : funcs)
    {
        if(!errs.empty()) return;
        current_file = f.short_filename;
        if(f.block.type == AST::NONE) continue;
        codegen_function(f);
    }

    // merge labels of the same address
    for(auto& [n, f] : funcs)
    {
        if(!errs.empty()) return;
        for(size_t i = 0; i + 1 < f.instrs.size(); ++i)
        {
            auto& i0 = f.instrs[i];
            auto const& i1 = f.instrs[i + 1];
            if(!(i0.is_label && i1.is_label))
                continue;
            i0.instr = I_REMOVE;
            for(auto& [tn, tf] : funcs)
                for(auto& ti : tf.instrs)
                    if(!ti.is_label && ti.label == i0.label)
                        ti.label = i1.label;
        }
        clear_removed_instrs(f.instrs);
    }

    // annotate instructions with file info to preserve over inlining
    {
        std::unordered_map<std::string, uint16_t> filename_map;
        debug_filenames.clear();
        for(auto& [n, f] : funcs)
        {
            uint16_t file = 0;
            if(!filename_map.count(f.filename))
            {
                debug_filenames.push_back(f.filename);
                file = (uint16_t)debug_filenames.size();
                filename_map[f.filename] = file;
            }
            else
                file = filename_map[f.filename];
            for(auto& i : f.instrs)
                i.file = file;
        }
    }

    // in case the program has calls to text functions but does not have
    // any font data, insert a call to set_text_font at the end of $globinit
    if(font_label_cache.empty())
    {
        bool found_text_function = false;

        for(auto const& [n, f] : funcs)
        {
            for(auto const& i : f.instrs)
            {
                if(i.instr != I_SYS) continue;
                switch(i.imm)
                {
                case SYS_DRAW_TEXT:
                case SYS_DRAW_TEXT_P:
                case SYS_DRAW_TEXTF:
                case SYS_TEXT_WIDTH:
                case SYS_TEXT_WIDTH_P:
                case SYS_WRAP_TEXT:
                    found_text_function = true;
                    break;
                default:
                    break;
                }
            }
        }
        if(found_text_function)
        {
            ast_node_t n{ {}, AST::IDENT };
            n.data = "FONT_ADAFRUIT";
            std::string label = resolve_label_ref({}, n, TYPE_FONT);
            auto& g = funcs[GLOBINIT_FUNC];
            if(!g.instrs.empty())
            {
                compiler_instr_t i0{}, i1{};
                i0.instr = I_PUSHL;
                i0.label = label;
                i1.instr = I_SYS;
                i1.imm = SYS_SET_TEXT_FONT;
                i0.line = i1.line = g.instrs.back().line;
                i0.file = i1.file = g.instrs.back().file;
                g.instrs.insert(g.instrs.begin() + g.instrs.size() - 1, { i0, i1 });
            }
        }
    }

    optimize();

    // attempt to find githash
    {
        std::filesystem::path p(base_path);
        std::string h;
        for(int i = 0; h.empty() && i < 64; ++i)
        {
            auto head = p / ".git" / "HEAD";
            if(std::filesystem::is_regular_file(head))
            {
                std::ifstream fhead(head);
                if(fhead.good())
                {
                    std::string t;
                    std::getline(fhead, t);
                    t = t.substr(5);
                    std::filesystem::path ref = p / ".git" / t;
                    if(std::filesystem::is_regular_file(ref))
                    {
                        std::ifstream fref(ref);
                        if(fref.good())
                            fref >> h;
                    }
                }
            }
            if(p.has_parent_path())
                p = p.parent_path();
        }
        if(!h.empty())
            githash = h;
    }

    // sort globals by ascending size for optimizing access

    sorted_globals.clear();
    for(auto const& [name, global] : globals)
        sorted_globals.push_back(&global);

    std::sort(sorted_globals.begin(), sorted_globals.end(),
        [](compiler_global_t const* a, compiler_global_t const* b) {
            return tie_var(a) < tie_var(b);
    });
}

void compiler_t::compile(
        std::string const& path,
        std::string const& name,
        std::function<bool(std::string const&, std::vector<char>&)> const& loader,
        std::ostream& fo)
{
    compile(path, name, loader);
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
    compile_data.short_filename = fname;
    ast_node_t& ast = compile_data.ast;
    if(!file_loader || !file_loader(filename, compile_data.filedata))
    {
        errs.push_back({ "Unable to open module \"" + fname + "\"" });
        return;
    }

    // extract line indices
    {
        auto& lines = compile_data.lines;
        auto const& data = compile_data.filedata;
        size_t start = 0;
        for(size_t i = 0; i < data.size(); ++i)
        {
            if(data[i] == '\n')
            {
                lines.push_back({ start, i });
                start = i + 1;
            }
            else if(i + 1 < data.size() && data[i] == '\r' && data[i+1] == '\n')
            {
                lines.push_back({ start, i });
                start = i + 2;
                ++i;
            }
        }
        if(start < data.size())
            lines.push_back({ start, data.size() });
    }

    parse(compile_data.filedata, ast);
    if(!errs.empty()) return;

    // trim all token whitespace
    ast.recurse([](ast_node_t& n) {
        if(n.type == AST::STRING_LITERAL) return;
        if(n.type == AST::TOKEN) return;
        size_t size;
        size_t i;
        size = n.data.size();
        if(size == 0) return;
        for(i = 0; i < size && isspace(n.data[i]); ++i);
        n.data.remove_prefix(i);
        size = n.data.size();
        if(size == 0) return;
        for(i = 0; i < size && isspace(n.data[size - i - 1]); ++i);
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

    // set parents
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
            n.type == AST::ENUM_STMT ||
            n.type == AST::IMPORT_STMT ||
            n.type == AST::DIRECTIVE);
        if(n.type != AST::DIRECTIVE)
            non_directive_found = true;
        if(n.type == AST::DECL_STMT)
        {
            compiler_frame_t empty_frame{};
            decl(funcs[GLOBINIT_FUNC], empty_frame, n);
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
            f.filename = filename;
            f.short_filename = compile_data.short_filename;
            f.block = std::move(n.children[2]);
            f.line_info = n.line_info;

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

            // reference type
            f.ref_type = {};
            f.ref_type.type = compiler_type_t::FUNC_REF;
            f.ref_type.prim_size = 3;
            f.ref_type.children.push_back(f.decl.return_type);
            for(auto const& t : f.decl.arg_types)
                f.ref_type.children.push_back(t);

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
            std::string name(n.children[1].data);
            if(symbol_exists(name))
            {
                errs.push_back({
                    "Duplicate symbol \"" + name + "\"",
                    n.line_info });
                return;
            }
            structs[name] = resolve_type(n);
        }
        else if(n.type == AST::ENUM_STMT)
        {
            std::string name(n.children[0].data);
            if(!name.empty() && symbol_exists(name))
            {
                errs.push_back({
                    "Duplicate symbol \"" + name + "\"",
                    n.line_info });
                return;
            }
            compiler_type_t t{};
            t.type = compiler_type_t::PRIM;
            t.prim_size = 1;
            t.is_signed = false;
            // temporarily set all children to i32
            if(n.children.size() >= 2)
            {
                int64_t v = 0;
                for(auto& c : n.children[1].children)
                {
                    std::string ident = std::string(c.children[0].data);
                    if(globals.count(ident) != 0)
                    {
                        errs.push_back({
                            "Duplicate symbol \"" + ident + "\"",
                            c.children[0].line_info });
                        return;
                    }
                    auto& g = globals[ident];
                    g.name = ident;
                    g.var.is_constexpr = true;
                    g.var.value = v;
                    if(c.children.size() >= 2)
                    {
                        type_annotate(c.children[1], {});
                        if(c.children[1].type != AST::INT_CONST)
                        {
                            errs.push_back({
                                "Value for \"" + ident + "\" is not a constant integral expression",
                                c.children[1].line_info });
                            return;
                        }
                        g.var.value = c.children[1].value;
                    }
                    v = g.var.value;
                    auto& gt = g.var.type;
                    gt.type = compiler_type_t::PRIM;
                    if(v < 0) gt.is_signed = true;
                    gt.prim_size = 1;
                    if(gt.is_signed)
                    {
                        if(v >= 0x100) gt.prim_size = std::max<size_t>(gt.prim_size, 2);
                        if(v >= 0x10000) gt.prim_size = std::max<size_t>(gt.prim_size, 3);
                        if(v >= 0x1000000) gt.prim_size = 4;
                    }
                    else
                    {
                        if(v < -0x80 || v >= 0x80) gt.prim_size = std::max<size_t>(gt.prim_size, 2);
                        if(v < -0x8000 || v >= 0x8000) gt.prim_size = std::max<size_t>(gt.prim_size, 3);
                        if(v < -0x800000 || v >= 0x800000) gt.prim_size = 4;
                    }
                    if(gt.is_signed) t.is_signed = true;
                    t.prim_size = std::max(t.prim_size, gt.prim_size);
                    v += 1;
                }
            }
            if(!name.empty())
                enums[name] = t;
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
            {
                auto tp = std::filesystem::path(new_path)
                    .lexically_relative(base_path)
                    .generic_string();
                if(tp != ".")
                    current_file = tp + "/" + new_file;
                else
                    current_file = new_file;
            }
            compile_recurse(new_path, new_file);
            if(errs.empty())
            {
                current_path = std::move(old_path);
                current_file = std::move(old_file);
            }
            import_set.erase(filename);
        }
        else if(n.type == AST::DIRECTIVE)
        {
            assert(n.children.size() == 2);
            assert(n.children[0].type == AST::TOKEN);
            assert(n.children[1].type == AST::STRING_LITERAL);
            if(non_directive_found)
            {
                errs.push_back({
                    "Compiler directives must occur before all other statements",
                    n.line_info
                });
                return;
            }
            std::string k = std::string(n.children[0].data);
            assert(!k.empty());
            auto d = strlit_data(n.children[1]);
            std::string v = std::string(d.begin(), d.end());
            bool keyword_valid = false;
            for(auto const& t : directive_keywords)
                if(t == k)
                    keyword_valid = true;
            if(keyword_valid)
            {
                arduboy_file_directives[k] = v;
                if(k == "shades")
                {
                    if(v == "2")
                    {
                        shades = 2;
                        globals["WHITE"].var.value = 1;
                        globals["LIGHT_GRAY"].var.value = 1;
                        globals["GRAY"].var.value = 1;
                        globals["DARK_GRAY"].var.value = 0;
                    }
                    else if(v == "3")
                    {
                        shades = 3;
                        globals["WHITE"].var.value = 2;
                        globals["LIGHT_GRAY"].var.value = 1;
                        globals["GRAY"].var.value = 1;
                        globals["DARK_GRAY"].var.value = 1;
                    }
                    else if(v == "4")
                    {
                        shades = 4;
                        globals["WHITE"].var.value = 3;
                        globals["LIGHT_GRAY"].var.value = 2;
                        globals["GRAY"].var.value = 1;
                        globals["DARK_GRAY"].var.value = 1;
                    }
                    else
                    {
                        errs.push_back({
                            "The '#shades' directive must have a value of '2', '3', or '4'",
                            n.line_info
                        });
                        return;
                    }
                    globals["LIGHT_GREY"].var.value = globals["LIGHT_GRAY"].var.value;
                    globals["GREY"].var.value = globals["GRAY"].var.value;
                    globals["DARK_GREY"].var.value = globals["DARK_GRAY"].var.value;
                    globals["SHADES"].var.value = shades;
                }
            }
            else
            {
                std::string msg = "Invalid directive: '#";
                msg += k;
                msg += "'. Valid directives: '#";
                msg += directive_keywords[0];
                msg += "'";
                for(size_t i = 1; i < directive_keywords.size(); ++i)
                    msg += ", '#" + directive_keywords[i] + "'";
                errs.push_back({ msg, n.line_info });
                return;
            }
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
    std::string data = n.string_literal();
    d.reserve(data.size());
    for(auto it = data.begin(); it != data.end(); ++it)
    {
        char c = *it;
        if(c == '\\')
        {
            ++it;
            if(it == data.end()) break;
            switch(*it)
            {
            case '0' : c = '\0'; break;
            case 'n' : c = '\n'; break;
            case 'r' : c = '\r'; break;
            case 't' : c = '\t'; break;
            case '"' : c = '\"'; break;
            case '\'': c = '\''; break;
            case '\\': c = '\\'; break;
            case 'x':
            {
                if(++it == data.end()) break;
                uint8_t nib1 = hex2val(*it);
                if(++it == data.end()) break;
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
    t.make_prog();
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


std::string type_name(abc::compiler_type_t const& t, bool noprog)
{
    std::stringstream ss;
    auto tt = t.without_prog();
    if(tt == abc::TYPE_VOID)
        ss << "void";
    else if(tt == abc::TYPE_SPRITES)
        ss << "sprites";
    else if(tt == abc::TYPE_FONT)
        ss << "font";
    else if(tt == abc::TYPE_TONES)
        ss << "tones";
    else if(tt == abc::TYPE_MUSIC)
        ss << "music";
    else if(tt == abc::TYPE_TILEMAP)
        ss << "tilemap";
    else if(tt.is_float)
        ss << "float";
    else if(tt.is_byte)
        ss << "byte";
    else if(t.is_array())
    {
        ss << type_name(t.children[0], t.children[0].is_prog) << "[" << t.array_size() << "]";
        if(t.children[0].is_prog) ss << " prog";
    }
    else if(t.is_prim())
    {
        if(tt == abc::TYPE_BOOL) ss << "bool";
        else if(tt == abc::TYPE_CHAR) ss << "char";
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
