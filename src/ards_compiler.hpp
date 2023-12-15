#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <cassert>

#include "ards_assembler.hpp"
#include "ards_error.hpp"

namespace ards
{

enum class AST
{
    NONE,

    //
    // pseudo-nodes
    //

    TOKEN,      // used to pass through token    
    LIST,       // used to pass through list of nodes
    FUNC_DECLS, // for function decl arg list
    LABEL_REF,  // used for constexpr label ref types like sprites

    //
    // program/statement nodes
    //

    PROGRAM,      // children are global declarations and functions
    IMPORT_STMT,  // child is path string literal
    BLOCK,        // children are child statements
    EMPTY_STMT,
    EXPR_STMT,    // child is expr
    STRUCT_STMT,  // children are ident and decl_stmt*
    DECL_STMT,    // after parse, children are type, DECL_ITEM*
                  // before codegen, children are type, ident [, expr]
    DECL_ITEM,    // children are ident [, expr]
    FUNC_STMT,    // children are type, ident, block, args
    IF_STMT,      // children are expr, stmt, stmt (for else)
    WHILE_STMT,   // children are expr and stmt [and stmt if for loop]
    RETURN_STMT,  // child is expr if it exists
    BREAK_STMT,   // no children
    CONTINUE_STMT,// no children

    //
    // expression nodes
    //
    EXPR_BEGIN,

    FUNC_ARGS,  // for function call arg list

    // left-associative chained infix operators
    OP_EQUALITY,   // chain of ops and infix == / != tokens
    OP_LOGICAL_AND,
    OP_LOGICAL_OR,
    OP_BITWISE_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,
    OP_RELATIONAL, // chain of ops and infix <= / >= / < / > tokens
    OP_SHIFT,      // chain of ops and infix << / >> tokens
    OP_ADDITIVE,   // chain of ops and infix + / - tokens
    OP_MULTIPLICATIVE,

    // right-associative assignment operators
    OP_ASSIGN,

    OP_UNARY, // children are op and expr

    OP_CAST, // children are type and expr
    OP_AREF, // create UAR from child

    FUNC_CALL, // children are func-expr and FUNC_ARGS

    ARRAY_LEN,     // same structure as FUNC_CALL
    ARRAY_INDEX,   // children are array and index
    STRUCT_MEMBER, // children are struct and member

    COMPOUND_LITERAL, // children are elements

    //
    // atoms
    //

    STRING_LITERAL,
    INT_CONST,
    IDENT,
    SPRITES,    // children are w, h, TOKEN (children are rows/TOKEN)
                //           or w, h, path string
    FONT,       // children are pixel height and path string

    TYPE,
    TYPE_REF,   // reference (child is type)
    TYPE_AREF,  // unsized array reference (child is type)
    TYPE_AREF_PROG, // unsized array reference to prog (child is type)
    TYPE_ARRAY, // sized array (children are size and type*)
    TYPE_PROG,  // sized array (child is type)
};

struct compiler_type_t
{
    // size of type in bytes (0 means void)
    size_t prim_size;

    bool is_signed;
    bool is_bool;
    bool is_constexpr;
    bool is_prog;
    bool is_char;

    enum type_t
    {
        PRIM,
        STRUCT,
        ARRAY,
        REF,
        ARRAY_REF,
        SPRITES,
        FONT,
    } type;

    bool is_ref() const { return type == REF; }
    bool is_array_ref() const { return type == ARRAY_REF; }
    bool is_any_ref() const { return is_ref() || is_array_ref(); }
    bool is_prim() const { return type == PRIM; }
    bool is_array() const { return type == ARRAY; }
    bool is_struct() const { return type == STRUCT; }
    bool is_sprites() const { return type == SPRITES; }
    bool is_font() const { return type == FONT; }

    bool is_label_ref() const { return is_sprites() || is_font(); }

    // empty for primitives
    // element type for arrays
    // members for structs
    std::vector<compiler_type_t> children;

    // only nonempty for structs
    // data is name, offset
    std::vector<std::pair<std::string, size_t>> members;

    type_t ref_type() const
    {
        return is_ref() ? children[0].type : type;
    }
    const compiler_type_t& without_ref() const
    {
        return is_ref() ? children[0].without_ref() : *this;
    }
    compiler_type_t sized_to(size_t size) const
    {
        assert(type == PRIM);
        compiler_type_t t = *this;
        t.prim_size = size;
        return t;
    }
    compiler_type_t without_prog() const
    {
        compiler_type_t t = *this;
        t.is_prog = false;
        return t;
    }

    size_t array_size() const
    {
        auto const& wr = without_ref();
        if(!wr.is_array()) return 0;
        assert(wr.children.size() == 1);
        return wr.prim_size / wr.children[0].prim_size;
    }

    bool has_child_ref() const
    {
        if(is_ref())
            return children[0].has_child_ref();
        if(is_array() || is_struct())
        {
            for(auto const& child : children)
                if(child.is_any_ref() || child.has_child_ref())
                    return true;
        }
        return false;
    }

    auto tie() const { return std::tie(prim_size, is_signed, is_bool, is_prog, children); }
    bool operator==(compiler_type_t const& t) const { return tie() == t.tie(); }
    bool operator!=(compiler_type_t const& t) const { return !operator==(t); }

    compiler_type_t with_prog() const
    {
        compiler_type_t r = *this;
        r.is_prog = true;
        return r;
    }

    compiler_type_t with_array_ref() const
    {
        compiler_type_t r{};
        r.type = ARRAY_REF;
        r.children.push_back(*this);
        r.prim_size = is_prog ? 6 : 4;
        return r;
    }
};

const compiler_type_t TYPE_NONE = { 0, true };

const compiler_type_t TYPE_VOID = { 0, false };
const compiler_type_t TYPE_BOOL = { 1, false, true };
const compiler_type_t TYPE_U8 = { 1, false };
const compiler_type_t TYPE_U16 = { 2, false };
const compiler_type_t TYPE_U24 = { 3, false };
const compiler_type_t TYPE_U32 = { 4, false };
const compiler_type_t TYPE_I8 = { 1, true };
const compiler_type_t TYPE_I16 = { 2, true };
const compiler_type_t TYPE_I24 = { 3, true };
const compiler_type_t TYPE_I32 = { 4, true };

const compiler_type_t TYPE_SPRITES = {
    3, false, false, false, false, false, compiler_type_t::SPRITES };

const compiler_type_t TYPE_FONT = {
    3, false, false, false, false, false, compiler_type_t::FONT };

const compiler_type_t TYPE_CHAR = {
    1, false, false, false, false, true };

inline compiler_type_t type_char_array(size_t n)
{
    return {
        n, false, false, false, false, false, compiler_type_t::ARRAY,
        { TYPE_CHAR }
    };
};

const compiler_type_t TYPE_STR = TYPE_CHAR.with_array_ref();
const compiler_type_t TYPE_STR_PROG = TYPE_CHAR.with_prog().with_array_ref();

compiler_type_t prim_type_for_dec(uint32_t x, bool is_signed);
compiler_type_t prim_type_for_hex(uint32_t x, bool is_signed);

struct compiler_instr_t
{
    instr_t instr;
    uint16_t line;
    uint32_t imm;
    uint32_t imm2;
    std::string label; // can also be label arg of instr
    bool is_label;
};

struct compiler_func_decl_t
{
    compiler_type_t return_type;
    std::vector<compiler_type_t> arg_types;
};

struct compiler_var_t
{
    compiler_type_t type;
    int64_t value;         // for constexprs
    std::string label_ref; // for constexprs
    bool is_constexpr;
};

struct compiler_global_t
{
    std::string name;
    compiler_var_t var;
    std::string constexpr_ref;
    bool is_constexpr_ref() const { return !constexpr_ref.empty(); }
};

struct compiler_local_t
{
    size_t frame_offset;
    compiler_var_t var;
};

struct compiler_scope_t
{
    size_t size;
    std::unordered_map<std::string, compiler_local_t> locals;
};

struct compiler_frame_t
{
    size_t size;
    std::vector<compiler_scope_t> scopes; // in-order
    void push()
    {
        scopes.resize(scopes.size() + 1);
    }
    void pop()
    {
        size -= scopes.back().size;
        scopes.pop_back();
    }
};

struct ast_node_t
{
    std::pair<size_t, size_t> line_info;
    AST type;
    std::string_view data;
    std::vector<ast_node_t> children;
    int64_t value;
    compiler_type_t comp_type;

    ast_node_t* parent;

    uint16_t line() const { return (uint16_t)line_info.first; }

    template<class F>
    void recurse(F&& f)
    {
        for(auto& child : children)
            child.recurse(std::forward<F>(f));
        f(*this);
    }
};

struct compiler_lvalue_t
{
    compiler_type_t type;
    bool is_global;
    uint8_t stack_index;
    uint16_t line;
    std::string global_name;
    ast_node_t const* ref_ast;
};

struct compiler_func_t
{
    ast_node_t block;
    compiler_func_decl_t decl;
    std::string name;
    std::string filename;
    std::vector<std::string> arg_names;
    std::vector<compiler_instr_t> instrs;
    size_t label_count;
    sysfunc_t sys;
    bool is_sys;
};

struct compiler_progdata_t
{
    std::vector<uint8_t> data;
    std::vector<std::pair<size_t, std::string>> relocs_prog;
    std::vector<std::pair<size_t, std::string>> relocs_glob;
};

struct compiler_constexpr_ref_t
{
    std::string label;
    size_t offset;
};

extern std::unordered_set<std::string> const keywords;
extern std::unordered_map<sysfunc_t, compiler_func_decl_t> const sysfunc_decls;
extern std::unordered_map<std::string, compiler_type_t> const primitive_types;

struct builtin_constexpr_t
{
    std::string name;
    compiler_type_t type;
    int64_t value;
};

extern std::vector<builtin_constexpr_t> const builtin_constexprs;

struct compiler_t
{
    compiler_t();

    void compile(
        std::string const& path,
        std::string const& name,
        std::function<bool(std::string const&, std::vector<char>&)> const& loader,
        std::ostream& fo);

    void compile(
        std::string const& path,
        std::string const& name,
        std::ostream& fo)
    {
        compile(path, name, [](std::string const& fname, std::vector<char>& t) -> bool {
            std::ifstream f(fname, std::ios::in | std::ios::binary);
            if(f.fail()) return false;
            t = std::vector<char>(
                (std::istreambuf_iterator<char>(f)),
                (std::istreambuf_iterator<char>()));
            return true;
        }, fo);
    }

    void add_custom_label_ref(std::string const& name, compiler_type_t const& t);

    std::vector<error_t> const& errors() const { return errs; }
    std::vector<error_t> const& warnings() const { return warns; }

    std::string error_file() const { return current_path; }

private:

    void parse(std::vector<char> const& fi, ast_node_t& ast);

    void compile_recurse(std::string const& path, std::string const& name);

    bool check_identifier(ast_node_t const& n);

    std::string resolve_label_ref(
        compiler_frame_t const& frame, ast_node_t const& n, compiler_type_t const& t);

    compiler_local_t const* resolve_local(compiler_frame_t const& frame, ast_node_t const& n);
    compiler_global_t const* resolve_global(ast_node_t const& n);
    compiler_type_t const* resolve_member(
        ast_node_t const& a, std::string const& name, size_t* offset = nullptr);

    compiler_type_t resolve_type(ast_node_t const& n);
    compiler_func_t resolve_func(ast_node_t const& n);
    compiler_lvalue_t resolve_lvalue(
        compiler_func_t const& f, compiler_frame_t const& frame,
        ast_node_t const& n);
    compiler_lvalue_t return_lvalue(compiler_func_t const& f, compiler_frame_t const& frame);
    
    void type_annotate_recurse(ast_node_t& n, compiler_frame_t const& frame);
    void type_reduce_recurse(ast_node_t& a, size_t size);
    void type_annotate(ast_node_t& n, compiler_frame_t const& frame, size_t size = 4);

    void transform_left_assoc_infix(ast_node_t& n);
    void transform_array_len(ast_node_t& n);
    void transform_constexprs(ast_node_t& n, compiler_frame_t const& frame);

    void resolve_format_call(
        ast_node_t const& n, std::vector<compiler_type_t>& arg_types, std::string& fmt);

    void codegen_function(compiler_func_t& f);
    void codegen(compiler_func_t& f, compiler_frame_t& frame, ast_node_t& a);
    void codegen_expr(compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a, bool ref);
    void codegen_expr_compound(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& a, compiler_type_t const& type);
    void codegen_expr_logical(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& a, std::string const& sc_label);
    void codegen_store_lvalue(
        compiler_func_t& f, compiler_frame_t& frame, compiler_lvalue_t const& lvalue);
    void codegen_convert(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& n,
        compiler_type_t const& to, compiler_type_t const& from);
    void codegen_return(compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& n);
    std::string new_label(compiler_func_t& f);
    std::string codegen_label(compiler_func_t& f);
    void codegen_label(compiler_func_t& f, std::string const& label);
    void codegen_dereference(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& n, compiler_type_t const& t);

    std::string progdata_label();
    void add_progdata(std::string const& label, compiler_type_t const& t, ast_node_t const& n);
    void add_custom_progdata(std::string const& label, std::vector<uint8_t>& data);
    void progdata_expr(ast_node_t const& n, compiler_type_t const& t, compiler_progdata_t& pdata);
    void progdata_zero(ast_node_t const& n, compiler_type_t const& t, compiler_progdata_t& pdata);

    static std::vector<uint8_t> strlit_data(ast_node_t const& n);
    static compiler_type_t strlit_type(size_t len);

    void encode_font(std::vector<uint8_t>& data, ast_node_t const& n);
    void encode_sprites(std::vector<uint8_t>& data, ast_node_t const& n);

    // idata encoding:
    //    0 - transparent
    //    1 - black
    //    2 - white
    void encode_sprites_image(
        std::vector<uint8_t>& data, ast_node_t const& n,
        size_t iw, size_t ih, size_t w, size_t h, bool masked,
        std::vector<uint8_t> const& idata);

    // perform a series of peephole optimizations on a function
    void peephole(compiler_func_t& f);
    bool peephole_remove_pop(compiler_func_t& f);
    bool peephole_pre_push_compress(compiler_func_t& f);
    bool peephole_linc(compiler_func_t& f);
    bool peephole_ref(compiler_func_t& f);
    bool peephole_dup(compiler_func_t& f);
    bool peephole_compress_push_pop(compiler_func_t& f);
    bool peephole_compress_duplicate_pushes(compiler_func_t& f);

    void write(std::ostream& f);

    // parse data
    std::string input_data;

    // codegen data
    std::unordered_map<std::string, compiler_func_t> funcs;
    std::unordered_map<std::string, compiler_global_t> globals;
    std::unordered_map<std::string, compiler_type_t> structs;
    bool symbol_exists(std::string const& name)
    {
        return
            funcs  .count(name) != 0 ||
            globals.count(name) != 0 ||
            structs.count(name) != 0;
    }

    // font caching
    using font_key_t = std::pair<int, std::string>;
    std::map<font_key_t, std::string> font_label_cache;

    std::string current_path;
    std::function<bool(std::string const&, std::vector<char>&)> file_loader;

    size_t progdata_label_index;
    std::unordered_map<std::string, compiler_progdata_t> progdata;

    std::vector<error_t> errs;
    std::vector<error_t> warns;

    // loop label stack
    std::vector<std::string> break_stack;
    std::vector<std::string> continue_stack;

    // track files already parsed
    std::map<std::string, std::pair<std::vector<char>, ast_node_t>> compiled_files;
    std::unordered_set<std::string> import_set;
};

}
