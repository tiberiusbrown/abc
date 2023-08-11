#pragma once

#include <iostream>
#include <variant>
#include <vector>

#include "ards_error.hpp"

namespace ards
{

enum class AST
{
    NONE,

    // pseudo-node used to pass through token
    TOKEN,

    // program: children are global declarations and functions
    PROGRAM,

    // types of statements
    BLOCK, // children are block, decl, if, while, return, break, continue, expr
    EXPR_STMT,
    DECL_STMT, // children are type, ident
    FUNC_STMT, // children are type, ident, block

    // left-associative chained infix operators
    OP_ADDITIVE, // chain of ops and infix plus / minus tokens

    // right-associative assignment operators
    OP_ASSIGN,

    // atoms
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

struct compiler_t
{
    compiler_t() {};

    void compile(std::istream& fi, std::ostream& fo);
    std::vector<error_t> const& errors() const { return errs; }
    std::vector<error_t> const& warnings() const { return warns; }

private:

    std::string input_data;
    ast_node_t ast;

    void parse(std::istream& fi);

    std::vector<error_t> errs;
    std::vector<error_t> warns;
};

}
