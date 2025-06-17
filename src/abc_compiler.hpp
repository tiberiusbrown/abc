#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <cassert>

#include "abc_assembler.hpp"
#include "abc_error.hpp"

namespace abc
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
    DIRECTIVE, // children are keyword and string literal
    IMPORT_STMT,  // child is path string literal
    BLOCK,        // children are child statements
    EMPTY_STMT,
    EXPR_STMT,    // child is expr
    STRUCT_STMT,  // children are ident and decl_stmt*
    ENUM_STMT,    // children are ident and list, if any
    ENUM_ITEM_LIST, // children are enum items
    ENUM_ITEM,    // children are ident [and expr]
    DECL_STMT,    // after parse, children are type, DECL_ITEM*
                  // before codegen, children are type, ident [, expr]
    DECL_ITEM,    // children are ident [, expr]
    FUNC_STMT,    // children are type, ident, block, args
    IF_STMT,      // children are expr, stmt, stmt (for else)
    FOR_STMT,     // children are expr, body stmt, iter stmt, init stmt(s)
    WHILE_STMT,   // children are expr and stmt
    DO_WHILE_STMT,// children are expr and stmt
    RETURN_STMT,  // child is expr if it exists
    BREAK_STMT,   // no children
    CONTINUE_STMT,// no children
    SWITCH_STMT,  // children are expr and switch_case's
    SWITCH_CASE,  // children are stmt and values ('default' case has just stmt)
    SWITCH_CASE_ITEM, // children are start and end exprs for range, or just expr if one child

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

    // ternary conditional operator
    OP_TERNARY, // children are A, B, C from A ? B : C

    // right-associative assignment operators
    OP_ASSIGN,
    OP_ASSIGN_COMPOUND,

    OP_INC_POST,
    OP_DEC_POST,

    // special op to deref a duped ref val for a compound assignment
    OP_COMPOUND_ASSIGNMENT_DEREF,

    OP_UNARY, // children are op and expr

    OP_CAST, // children are type and expr
    OP_AREF, // create UAR from child

    FUNC_CALL, // children are func-expr and FUNC_ARGS

    ARRAY_LEN,     // same structure as FUNC_CALL
    ARRAY_INDEX,   // children are array and index
    ARRAY_SLICE,   // children are array, start-index, and stop-index
    ARRAY_SLICE_LEN, // children are array, start-index, and length
    //SPRITES_LEN,   // same structure and FUNC_CALL
    STRUCT_MEMBER, // children are struct and member

    COMPOUND_LITERAL, // children are elements

    //
    // atoms
    //

    STRING_LITERAL,
    INT_CONST,
    FLOAT_CONST,
    IDENT,
    SPRITES,    // children are w, h, TOKEN (children are rows/TOKEN)
                //           or w, h, path string
    FONT,       // children are pixel height and path string
    MUSIC,      // children are path string
    TILEMAP,    // children are path string [and layer name string]
    TONES,      // children are path string
                //           or note, dur, note, dur, ...
    TONES_RTTTL,// children are d, o, b, notes...

    TYPE,
    TYPE_REF,   // reference (child is type)
    TYPE_AREF,  // unsized array reference (child is type)
    TYPE_AREF_PROG, // unsized array reference to prog (child is type)
    TYPE_ARRAY, // sized array (children are size and type*)
    TYPE_PROG,  // sized array (child is type)
    TYPE_FUNC_REF, // children are return type and arg types
};

struct compiler_type_t
{
    // size of type in bytes (0 means void)
    size_t prim_size;

    enum type_t : uint8_t
    {
        PRIM,
        STRUCT,
        ARRAY,
        REF,
        ARRAY_REF,
        FUNC_REF, // children are return type and arg types
        SPRITES,
        FONT,
        TONES,
        MUSIC,
        TILEMAP,
    } type;

    bool is_signed;

    bool is_bool;
    bool is_char;
    bool is_float;
    bool is_byte;

    bool is_constexpr;
    bool is_saved;
    bool is_prog;

    bool is_void() const { return prim_size == 0 && !is_signed; }
    bool is_ref() const { return type == REF; }
    bool is_array_ref() const { return type == ARRAY_REF; }
    bool is_any_ref() const { return is_ref() || is_array_ref(); }
    bool is_func_ref() const { return type == FUNC_REF; }
    bool is_prim() const { return type == PRIM; }
    bool is_array() const { return type == ARRAY; }
    bool is_struct() const { return type == STRUCT; }
    bool is_sprites() const { return type == SPRITES; }
    bool is_font() const { return type == FONT; }
    bool is_tones() const { return type == TONES; }
    bool is_music() const { return type == MUSIC; }
    bool is_tilemap() const { return type == TILEMAP; }

    bool is_any_nonprog_ref() const {
        return is_any_ref() && (!children[0].is_prog || children[0].is_any_nonprog_ref());
    }

    bool is_label_ref() const
    {
        return is_sprites() || is_font() || is_tones() || is_music() || is_tilemap();
    }

    // empty for primitives
    // element type for arrays
    // members for structs
    std::vector<compiler_type_t> children;

    // only nonempty for structs
    // data is name, offset
    std::vector<std::pair<std::string, size_t>> members;
    std::string struct_name;

    type_t ref_type() const
    {
        return is_ref() ? children[0].type : type;
    }
    const compiler_type_t& without_ref_single() const
    {
        return is_ref() ? children[0] : *this;
    }
    const compiler_type_t& without_ref() const
    {
        return is_ref() ? children[0].without_ref() : *this;
    }
    bool is_prog_array() const
    {
        if(is_ref())
            return children[0].is_prog_array();
        if(is_array() || is_array_ref())
            return children[0].is_prog;
        assert(false);
        return false;
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
        if(is_array() || is_struct())
            for(auto& child : t.children)
                child = child.without_prog();
        return t;
    }

    size_t array_size() const
    {
        auto const& wr = without_ref();
        if(!wr.is_array()) return 0;
        assert(wr.children.size() == 1);
        return wr.prim_size / wr.children[0].prim_size;
    }

    bool contains_void() const
    {
        if(is_void()) return true;
        if(is_array() || is_any_ref())
            return children[0].contains_void();
        if(is_struct())
            for(auto const& child : children)
                if(child.contains_void())
                    return true;
        return false;
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

    bool has_nonprog_child_ref() const
    {
        if(is_ref())
            return children[0].has_nonprog_child_ref();
        if(is_array() || is_struct())
        {
            for(auto const& child : children)
                if(child.is_any_nonprog_ref() || child.has_nonprog_child_ref())
                    return true;
        }
        return false;
    }

    bool is_copyable() const
    {
        return !(is_any_nonprog_ref() || has_nonprog_child_ref());
    }

    auto tie() const
    {
        return std::tie(
            prim_size, type,
            is_signed, is_bool, is_prog, is_char, is_float, is_byte,
            children, struct_name);
    }
    bool operator==(compiler_type_t const& t) const { return tie() == t.tie(); }
    bool operator!=(compiler_type_t const& t) const { return !operator==(t); }

    void make_prog()
    {
        is_prog = true;
        if(is_any_ref())
            return;
        for(auto& child : children)
            child.make_prog();
    }

    compiler_type_t with_prog() const
    {
        compiler_type_t r = *this;
        r.make_prog();
        return r;
    }

    compiler_type_t with_ref() const
    {
        compiler_type_t r{};
        r.type = REF;
        r.children.push_back(*this);
        r.prim_size = is_prog ? 3 : 2;
        return r;
    }

    compiler_type_t with_array_ref() const
    {
        assert(!is_array_ref());
        compiler_type_t r{};
        r.type = ARRAY_REF;
        r.children.push_back(*this);
        r.prim_size = is_prog ? 6 : 4;
        return r;
    }

    compiler_type_t with_array(size_t n) const
    {
        compiler_type_t r{};
        r.type = ARRAY;
        r.prim_size = n * prim_size;
        r.is_prog = is_prog;
        r.children.push_back(*this);
        return r;
    }

    bool is_nonprog_string() const
    {
        if(type == ARRAY || type == ARRAY_REF)
            return children[0].is_char && !children[0].is_prog;
        if(type == REF)
            return children[0].is_nonprog_string();
        return false;
    }

    bool is_prog_string() const
    {
        if(type == ARRAY || type == ARRAY_REF)
            return children[0].is_char && children[0].is_prog;
        if(type == REF)
            return children[0].is_prog_string();
        return false;
    }

    bool is_string() const
    {
        if(type == ARRAY || type == ARRAY_REF)
            return children[0].is_char;
        if(type == REF)
            return children[0].is_string();
        return false;
    }
};

const compiler_type_t TYPE_NONE = { 0, compiler_type_t::PRIM, true };

const compiler_type_t TYPE_VOID = { 0, compiler_type_t::PRIM, false };
const compiler_type_t TYPE_BOOL = { 1, compiler_type_t::PRIM, false, true };
const compiler_type_t TYPE_U8 = { 1, compiler_type_t::PRIM, false };
const compiler_type_t TYPE_U16 = { 2, compiler_type_t::PRIM, false };
const compiler_type_t TYPE_U24 = { 3, compiler_type_t::PRIM, false };
const compiler_type_t TYPE_U32 = { 4, compiler_type_t::PRIM, false };
const compiler_type_t TYPE_I8 = { 1, compiler_type_t::PRIM, true };
const compiler_type_t TYPE_I16 = { 2, compiler_type_t::PRIM, true };
const compiler_type_t TYPE_I24 = { 3, compiler_type_t::PRIM, true };
const compiler_type_t TYPE_I32 = { 4, compiler_type_t::PRIM, true };

const compiler_type_t TYPE_SPRITES = {
    3, compiler_type_t::SPRITES };

const compiler_type_t TYPE_FONT = {
    3, compiler_type_t::FONT };

const compiler_type_t TYPE_TONES = {
    3, compiler_type_t::TONES };

const compiler_type_t TYPE_MUSIC = {
    3, compiler_type_t::MUSIC };

const compiler_type_t TYPE_TILEMAP = {
    3, compiler_type_t::TILEMAP };

const compiler_type_t TYPE_CHAR = {
    1, compiler_type_t::PRIM, false, false, true };

const compiler_type_t TYPE_FLOAT = {
    4, compiler_type_t::PRIM, false, false, false, true };

const compiler_type_t TYPE_BYTE = {
    1, compiler_type_t::PRIM, false, false, false, false, true };

const compiler_type_t TYPE_STR = TYPE_CHAR.with_array_ref();
const compiler_type_t TYPE_STR_PROG = TYPE_CHAR.with_prog().with_array_ref();

const compiler_type_t TYPE_BYTE_AREF = TYPE_BYTE.with_array_ref();
const compiler_type_t TYPE_BYTE_PROG_AREF = TYPE_BYTE.with_prog().with_array_ref();

compiler_type_t prim_type_for_dec(uint32_t x, bool is_signed);
compiler_type_t prim_type_for_hex(uint32_t x, bool is_signed);

std::string type_name(abc::compiler_type_t const& t, bool noprog = false);

struct compiler_instr_t
{
    instr_t instr;
    uint16_t line;
    uint32_t imm;
    uint32_t imm2;
    std::string label; // can also be label arg of instr
    bool is_label;
    uint16_t file;
};

struct compiler_func_decl_t
{
    compiler_type_t return_type;
    std::vector<compiler_type_t> arg_types;
    std::vector<std::string> arg_names;
};

struct compiler_var_t
{
    compiler_type_t type;
    union
    {
        int64_t value;         // for constexprs
        double fvalue;         // for constexprs
    };
    std::string label_ref; // for constexprs
    bool is_constexpr;
};

struct compiler_global_t
{
    std::string name;
    compiler_var_t var;
    std::string constexpr_ref;
    bool saved;
    bool builtin; // builtin-resource but not initialized yet
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
    union
    {
        int64_t value;
        double fvalue;
    };
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

    void insert_cast(compiler_type_t const& t);

    std::string string_literal() const
    {
        if(type != AST::STRING_LITERAL)
            return std::string(data);
        std::string t;
        for(auto const& child : children)
        {
            assert(child.type == AST::TOKEN);
            t += std::string(child.data);
        }
        return t;
    }

    size_t count(size_t max = SIZE_MAX) const
    {
        size_t n = 0;
        for(auto const& child : children)
        {
            n += child.count(max);
            if(n >= max)
                return max;
        }
        return n;
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
    compiler_type_t ref_type;
    compiler_func_decl_t decl;
    std::string name;
    std::string filename;
    std::vector<std::string> arg_names;
    std::vector<compiler_instr_t> instrs;
    std::pair<size_t, size_t> line_info;
    size_t label_count;
    sysfunc_t sys;
    bool is_sys;
};

struct compiler_progdata_t
{
    uint32_t offset; // computed after compilation
    std::vector<uint8_t> data;
    std::vector<std::pair<size_t, std::string>> relocs_prog;
    std::vector<std::pair<size_t, std::string>> relocs_glob;
    std::vector<std::pair<size_t, std::string>> inter_labels;
    bool merged;
};

struct compiler_constexpr_ref_t
{
    std::string label;
    size_t offset;
};

struct sysfunc_info_t
{
    compiler_func_decl_t decl;
    std::string category;
    std::string desc;
    std::string return_desc;
    std::vector<std::string> arg_descs;
};
extern std::unordered_map<sysfunc_t, sysfunc_info_t> const sysfunc_decls;

extern std::unordered_set<std::string> const keywords;
extern std::unordered_map<std::string, compiler_type_t> const primitive_types;

bool sysfunc_is_format(sysfunc_t f);
bool sysfunc_is_format(std::string const& f);

struct builtin_constexpr_t
{
    std::string name;
    compiler_type_t type;
    int64_t value;
    double fvalue;
};

extern std::vector<builtin_constexpr_t> const builtin_constexprs;

struct compiler_t
{
    compiler_t()
        : progdata_label_index(0)
        , shades(2)
    {}

    void suppress_githash() { do_suppress_githash = true; }

    void compile(
        std::string const& path,
        std::string const& name,
        std::function<bool(std::string const&, std::vector<char>&)> const& loader);

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

    void compile(
        std::string const& path,
        std::string const& name)
    {
        compile(path, name, [](std::string const& fname, std::vector<char>& t) -> bool {
            std::ifstream f(fname, std::ios::in | std::ios::binary);
            if(f.fail()) return false;
            t = std::vector<char>(
                (std::istreambuf_iterator<char>(f)),
                (std::istreambuf_iterator<char>()));
            return true;
        });
    }

    // write assembly
    void write(std::ostream& f);

    std::vector<error_t> const& errors() const { return errs; }
    std::vector<error_t> const& warnings() const { return warns; }

    std::string error_file() const { return current_path + "/" + current_file + ".abc"; }

    std::unordered_map<std::string, std::string> arduboy_directives() const
    {
        return arduboy_file_directives;
    }

    //
    // "Private but technically public" API
    //

    bool enable_merging_progdata = true;
    bool enable_sized_unrolling = false;
    bool enable_unsized_unrolling = true;
    bool enable_inlining = true;
    bool enable_jmp_to_ret = true;
    bool enable_bake_pushl = true;
    size_t memcpy_min_bytes = 16;
    size_t max_jump_to_ret_instrs = 8;
    size_t inlining_max_add_instrs = 256;
    size_t switch_min_ranges_for_jump_table = 16;
    size_t unroll_sized_max_instrs = 1024; // max total instrs in unrolled loop
    size_t unroll_sized_max_iters = 64;
    size_t unroll_unsized_max_add_instrs = 256;
    size_t unroll_unsized_max_iters = 8;
    size_t max_getpn_bake = 16; // max bytes to bake a GETPN into PUSHs

    void add_custom_label_ref(std::string const& name, compiler_type_t const& t);

    void encode_font_ttf(
        std::vector<uint8_t>& data, ast_node_t const& n,
        uint8_t const* ttf_data, size_t ttf_size);

private:

    friend struct assembler_t;

    void init_parser();
    void parse(std::vector<char> const& fi, ast_node_t& ast);

    void create_builtin_font(compiler_global_t& g);
    
    void compile_recurse(std::string const& path, std::string const& name);

    bool check_identifier(ast_node_t const& n);

    bool is_simple_lvalue(ast_node_t const& n, compiler_frame_t const& frame);

    // how the stack size is affected by an instruction
    int instr_stack_mod(compiler_instr_t const& i);
    // how much of the stack is used by the instruction (relative to pre-execution)
    // returns whether offset is accessed and max accessed offset
    std::pair<bool, uint32_t> instr_accesses_stack(
        compiler_instr_t const& i, uint32_t off);

    std::string resolve_label_ref(
        compiler_frame_t const& frame, ast_node_t const& n, compiler_type_t const& t);

    compiler_local_t const* resolve_local(compiler_frame_t const& frame, ast_node_t const& n);
    compiler_global_t const* resolve_global(ast_node_t const& n);
    compiler_type_t const* resolve_member(
        ast_node_t const& a, std::string const& name, size_t* offset = nullptr);

    bool convertible(compiler_type_t const& dst, compiler_type_t const& src);
    bool check_sysfunc_overload(compiler_func_decl_t const& decl, ast_node_t const& n);
    compiler_type_t resolve_type(ast_node_t const& n);
    compiler_func_t resolve_func(ast_node_t const& n);
    compiler_lvalue_t resolve_lvalue(
        compiler_func_t const& f, compiler_frame_t const& frame,
        ast_node_t const& n);
    void codegen_store_return(
        compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a);
    
    void type_annotate_recurse(ast_node_t& n, compiler_frame_t const& frame);
    void type_reduce_recurse(ast_node_t& a, size_t size);
    void type_annotate(ast_node_t& n, compiler_frame_t const& frame, size_t size = 4);

    int64_t truncate_value(compiler_type_t const& t, int64_t x);
    void transform_left_assoc_infix(ast_node_t& n);
    void transform_array_len(ast_node_t& n);
    void transform_constexprs(ast_node_t& n, compiler_frame_t const& frame);

    void resolve_format_call(
        ast_node_t const& n, compiler_func_decl_t const& decl,
        std::vector<compiler_type_t>& arg_types, std::string& fmt);

    void decl(compiler_func_t& f, compiler_frame_t& frame, ast_node_t& n);

    void codegen_function(compiler_func_t& f);
    void codegen(compiler_func_t& f, compiler_frame_t& frame, ast_node_t& a);
    void codegen_switch(compiler_func_t& f, compiler_frame_t& frame, ast_node_t& a);
    void codegen_expr(compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a, bool ref);
    void codegen_expr_array_index(
        compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a, size_t& offset);
    void codegen_expr_ident(
        compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a, size_t offset);
    void codegen_expr_compound(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& a, compiler_type_t const& type);
    void codegen_expr_logical(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& a, std::string const& sc_label);
    void codegen_store(
        compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a);
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
    void codegen_int_const(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& a);
    void codegen_float_const(
        compiler_func_t& f, compiler_frame_t& frame,
        ast_node_t const& a);

    // returns 0 if can't unroll
    struct unroll_info_t
    {
        std::string var_name;
        int64_t init;
        int64_t incr;
        uint32_t num;
    };
    bool can_unroll_for_loop_sized(ast_node_t const& n, unroll_info_t& u);
    void unroll_loop_sized(
        ast_node_t const& n, unroll_info_t const& u,
        compiler_func_t& f, compiler_frame_t& frame);

    std::string progdata_label();
    void add_progdata(std::string const& label, compiler_type_t const& t, ast_node_t const& n);
    void add_custom_progdata(std::string const& label, std::vector<uint8_t>& data);
    void try_merge_progdata(
        std::string const& label, compiler_progdata_t& pdata);
    bool progdata_encode_prim(
        ast_node_t const& n, compiler_type_t const& t, std::vector<uint8_t>& data);
    bool progdata_expr_valid_memcpy(
        ast_node_t const& n, compiler_type_t const& t, std::vector<uint8_t>& data);
    void progdata_expr(ast_node_t const& n, compiler_type_t const& t, compiler_progdata_t& pdata);
    void progdata_zero(ast_node_t const& n, compiler_type_t const& t, compiler_progdata_t& pdata);

    static std::vector<uint8_t> strlit_data(ast_node_t const& n);
    static compiler_type_t strlit_type(size_t len);

    void encode_font(std::vector<uint8_t>& data, ast_node_t const& n);
    void encode_tones(std::vector<uint8_t>& data, ast_node_t const& n);
    void encode_tones_rtttl(
        std::vector<uint8_t>& data, ast_node_t const& n);
    std::string encode_tones_midi(
        std::vector<uint8_t>& data, std::string const& filename, bool music);
    std::string encode_tilemap_tmx(
        std::vector<uint8_t>& data, std::string const& filename, std::string const& layer_name);
    std::string encode_tilemap_literal(
        std::vector<uint8_t>& data, ast_node_t const& n);
    void encode_sprites(std::vector<uint8_t>& data, ast_node_t const& n);

    // idata encoding:
    //    0 - transparent
    //    1 - black
    //    2 - white
    void encode_sprites_image(
        std::vector<uint8_t>& data, ast_node_t const& n,
        size_t iw, size_t ih, size_t w, size_t h, bool masked,
        std::vector<uint8_t> const& idata);

    bool remove_unreferenced_labels();
    bool merge_adjacent_labels();

    bool is_inlinable(std::string const& func, std::unordered_set<std::string>& refs);
    bool is_inlinable(std::string const& func);
    bool should_inline(std::string const& func, int ref_count);
    bool inline_function(std::string const& func);
    bool inline_or_remove_functions();

    // perform a series of peephole optimizations on a function
    void optimize();
    bool peephole(compiler_func_t& f);
    bool peephole_reduce(compiler_func_t& f);
    bool peephole_reduce_bake_pushl(compiler_func_t& f);
    bool peephole_pre_push_compress(compiler_func_t& f);
    bool peephole_linc(compiler_func_t& f);
    bool peephole_dup_sext(compiler_func_t& f);
    bool peephole_bzp(compiler_func_t& f);
    bool peephole_redundant_bzp(compiler_func_t& f);
    bool peephole_combine_pop(compiler_func_t& f);
    bool peephole_compress_pop(compiler_func_t& f);
    bool peephole_push_setl_popn(compiler_func_t& f);
    bool peephole_remove_inaccessible_code(compiler_func_t& f);
    bool peephole_jmp_to_ret(compiler_func_t& f);

    // non-peephole stack-based optimizations
    bool optimize_stack();
    bool optimize_stack_func(std::vector<compiler_instr_t>& instrs);

    bool peephole_compress_push_sequence(compiler_func_t& f);
    static void push_compression(
        std::vector< compiler_instr_t>& pi,
        compiler_instr_t const* b,
        compiler_instr_t const* d, size_t n);

    void tail_call_optimization();

    static void clear_removed_instrs(std::vector<compiler_instr_t>& instrs);

    void write_instr(
        std::ostream& f, compiler_instr_t const& instr, uint16_t& line,
        uint16_t& file, std::vector<std::string> const& filenames);

    // parse data
    std::string input_data;

    // codegen data
    std::map<std::string, compiler_func_t> funcs;
    std::map<std::string, compiler_global_t> globals;
    std::unordered_map<std::string, compiler_type_t> structs;
    std::unordered_map<std::string, compiler_type_t> enums;
    bool symbol_exists(std::string const& name)
    {
        return
            funcs  .count(name) != 0 ||
            globals.count(name) != 0 ||
            structs.count(name) != 0 ||
            enums  .count(name) != 0;
    }

    // font caching
    using font_key_t = std::pair<int, std::string>;
    std::map<font_key_t, std::string> font_label_cache;

    std::string base_path;
    std::string current_path;
    std::string current_file;
    std::function<bool(std::string const&, std::vector<char>&)> file_loader;

    size_t progdata_label_index;
    std::map<std::string, compiler_progdata_t> progdata;

    std::vector<error_t> errs;
    std::vector<error_t> warns;

    // loop label stack: [label, frame size] pair
    std::vector<std::pair<std::string, size_t>> break_stack;
    std::vector<std::pair<std::string, size_t>> continue_stack;

    // track files already parsed
    std::map<std::string, std::pair<std::vector<char>, ast_node_t>> compiled_files;
    std::vector<std::string> debug_filenames;
    std::unordered_set<std::string> import_set;

    std::unordered_map<std::string, std::string> arduboy_file_directives;

    std::string githash;
    std::vector<compiler_global_t const*> sorted_globals;

    int shades;
    bool non_directive_found;
    bool do_suppress_githash;
};

}
