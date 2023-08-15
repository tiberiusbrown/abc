#pragma once

#include <iostream>
#include <variant>
#include <vector>

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

    TOKEN,     // pseudo-node used to pass through token    
    LIST,      // pseudo-node used to pass through list of nodes
    FUNC_ARGS, // pseudo-node for function call argument list

    //
    // program/statement nodes
    //

    PROGRAM, // children are global declarations and functions
    BLOCK, // children are child statements
    EMPTY_STMT,
    EXPR_STMT, // child is expr
    DECL_STMT, // children are type, ident
    FUNC_STMT, // children are type, ident, block
    WHILE_STMT, // children are expr and stmt

    //
    // expression nodes
    //

    // left-associative chained infix operators
    OP_ADDITIVE, // chain of ops and infix plus / minus tokens

    // right-associative assignment operators
    OP_ASSIGN,

    OP_CAST, // children are type and expr

    OP_UNARY, // children are op and expr,

    FUNC_CALL, // children are func-expr and FUNC_ARGS

    //
    // atoms
    //

    INT_CONST,
    IDENT,
    TYPE,
};

struct compiler_instr_t
{
    instr_t instr;
    uint32_t imm;
    std::string label; // can also be label arg of instr
    bool is_label;
};

struct compiler_type_t
{
    size_t prim_size; // 0 means void
    bool prim_signed;
};

struct compiler_func_decl_t
{
    compiler_type_t return_type;
    std::vector<compiler_type_t> arg_types;
};

struct compiler_global_t
{
    compiler_type_t type;
    std::string name;
};

struct compiler_local_t
{
    size_t frame_offset;
    compiler_type_t type;
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

struct compiler_lvalue_t
{
    size_t size;
    bool is_global;
    uint8_t stack_index;
    std::string global_name;
};

struct ast_node_t
{
    std::pair<size_t, size_t> line_info;
    AST type;
    std::string_view data;
    std::vector<ast_node_t> children;
    int64_t value;
    compiler_type_t comp_type;

    template<class F>
    void recurse(F&& f)
    {
        for(auto& child : children)
            child.recurse(std::forward<F>(f));
        f(*this);
    }
};

struct compiler_func_t
{
    ast_node_t block;
    compiler_func_decl_t decl;
    std::string name;
    std::vector<std::string> arg_names;
    std::vector<compiler_instr_t> instrs;
    sysfunc_t sys;
    bool is_sys;
};

extern std::unordered_map<sysfunc_t, compiler_func_decl_t> const sysfunc_decls;

struct compiler_t
{
    compiler_t() {};

    void compile(std::istream& fi, std::ostream& fo);
    std::vector<error_t> const& errors() const { return errs; }
    std::vector<error_t> const& warnings() const { return warns; }

private:

    void parse(std::istream& fi);

    compiler_type_t resolve_type(ast_node_t const& n);
    compiler_func_t resolve_func(ast_node_t const& n);
    void type_annotate(ast_node_t& n, compiler_frame_t const& frame);
    void transform_left_assoc_infix(ast_node_t& n);
    compiler_lvalue_t resolve_lvalue(ast_node_t const& n, compiler_frame_t const& frame);

    void codegen_function(compiler_func_t& f);
    void codegen(compiler_func_t& f, compiler_frame_t& frame, ast_node_t& a);
    void codegen_expr(compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a);
    void codegen_store_lvalue(compiler_func_t& f, compiler_lvalue_t const& lvalue);
    void codegen_convert(
        compiler_func_t& f, compiler_frame_t& frame,
        compiler_type_t const& to, compiler_type_t const& from);

    // parse data
    std::string input_data;
    ast_node_t ast;

    // codegen data
    std::unordered_map<std::string, compiler_func_t> funcs;
    std::unordered_map<std::string, compiler_global_t> globals;

    std::vector<error_t> errs;
    std::vector<error_t> warns;
};

}
