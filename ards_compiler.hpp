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
    BLOCK, // children are block, decl, if, while, return, break, continue, expr
    EMPTY_STMT,
    EXPR_STMT,
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

    FUNC_CALL, // children are func-expr and args...

    //
    // atoms
    //

    INT_CONST,
    IDENT,
    TYPE,
};

struct ast_node_t
{
    std::pair<size_t, size_t> line_info;
    AST type;
    std::string_view data;
    std::vector<ast_node_t> children;
    std::variant<int64_t> value;

    template<class F>
    void recurse(F&& f)
    {
        for(auto& child : children)
            child.recurse(std::forward<F>(f));
        f(*this);
    }
};

struct compiler_instr_t
{
    instr_t instr;
    bool is_label;
    uint32_t imm;
    std::string label;
};

struct compiler_type_t
{
    uint8_t prim_size; // 0 means void
    bool prim_signed;
};

struct compiler_func_decl_t
{
    compiler_type_t return_type;
    std::vector<compiler_type_t> arg_types;
};

struct compiler_func_t
{
    compiler_func_decl_t decl;
    std::string name;
    std::vector<std::string> arg_names;
    std::vector<compiler_instr_t> instrs;
};

struct compiler_global_t
{
    compiler_type_t type;
    std::string name;
};

struct compiler_t
{
    compiler_t() {};

    void compile(std::istream& fi, std::ostream& fo);
    std::vector<error_t> const& errors() const { return errs; }
    std::vector<error_t> const& warnings() const { return warns; }

private:

    void parse(std::istream& fi);

    compiler_type_t resolve_type(ast_node_t const& n);

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
