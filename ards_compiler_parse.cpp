#include "ards_compiler.hpp"

#include <iostream>
#include <streambuf>
#include <variant>

#include <peglib.h>

namespace ards
{

void compiler_t::parse(std::istream& fi)
{
    error_t e;
    peg::parser p;

    p.set_logger([&](size_t line, size_t column, std::string const& msg) {
        errs.push_back({ msg, line, column });
    });
    p.load_grammar(
#if 0
// fuller grammar (TODO)
R"(
        
program             <- global_stmt*

global_stmt         <- decl_stmt / func_stmt
decl_stmt           <- type_name decl_list ';'
func_stmt           <- type_name ident '(' arg_decl_list? ')' compound_stmt
compound_stmt       <- '{' stmt* '}'
stmt                <- compound_stmt /
                       decl_stmt     /
                       if_stmt       /
                       while_stmt    /
                       return_stmt   /
                       break_stmt    /
                       continue_stmt /
                       expr_stmt
if_stmt             <- 'if' '(' expr ')' stmt ('else' stmt)?
while_stmt          <- 'while' '(' expr ')' stmt
return_stmt         <- 'return' expr ';'
break_stmt          <- 'break' ';'
continue_stmt       <- 'continue' ';'
return_stmt         <- 'return' expr? ';'
expr_stmt           <- ';' / expr ';'

# right-associative binary assignment operator
expr                <- conditional_expr / unary_expr assignment_op expr

# ternary conditional operator
conditional_expr    <- logical_or_expr ('?' expr ':' conditional_expr)?

# left-associative binary operators
logical_or_expr     <- logical_and_expr    ('||'              logical_and_expr   )*
logical_and_expr    <- bitwise_or_expr     ('&&'              bitwise_or_expr    )*
bitwise_or_expr     <- bitwise_xor_expr    ('|'               bitwise_xor_expr   )*
bitwise_xor_expr    <- bitwise_and_expr    ('^'               bitwise_and_expr   )*
bitwise_and_expr    <- equality_expr       ('&'               equality_expr      )*
equality_expr       <- relational_expr     (equality_op       relational_expr    )*
relational_expr     <- shift_expr          (relational_op     shift_expr         )*
shift_expr          <- additive_expr       (shift_op          additive_expr      )*
additive_expr       <- multiplicative_expr (additive_op       multiplicative_expr)*
multiplicative_expr <- cast_expr           (multiplicative_op cast_expr          )*

# unary operators
cast_expr           <- unary_expr / '(' type_name ')' cast_expr
unary_expr          <- postfix_expr / prefix_op unary_expr / unary_op cast_expr
postfix_expr        <- primary_expr postfix*
primary_expr        <- ident / decimal_literal / '(' expr ')'

postfix             <- postfix_op / '.' ident / '[' expr ']' / '(' arg_expr_list? ')'

type_name           <- ident
decl_list           <- decl_item (',' decl_item)*
decl_item           <- ident ('=' expr)?
arg_decl_list       <- type_name ident (',' type_name ident)*
arg_expr_list       <- expr (',' expr)*

prefix_op           <- < '++' / '--' >
postfix_op          <- < '++' / '--' >
unary_op            <- < [~!+-] >
multiplicative_op   <- < [*/%] >
additive_op         <- < [+-] >
shift_op            <- < '<<' / '>>' >
relational_op       <- < '<=' / '>=' / '<' / '>' >
equality_op         <- < '==' / '!=' >
assignment_op       <- < '=' / [*/%&|^+-]'=' / '<<=' / '>>=' >
decimal_literal     <- < [0-9]+ >
ident               <- < [a-zA-Z_][a-zA-Z_0-9]* >

%whitespace         <- [ \t\r\n]*

    )"
#else
R"(

program             <- global_stmt*

global_stmt         <- decl_stmt / func_stmt
decl_stmt           <- type_name ident ';'
func_stmt           <- type_name ident '(' ')' compound_stmt
compound_stmt       <- '{' stmt* '}'
stmt                <- compound_stmt /
                       decl_stmt     /
                       expr_stmt
expr_stmt           <- ';' / expr ';'

# right-associative binary assignment operator
expr                <- primary_expr assignment_op expr / additive_expr

# left-associative binary operators
additive_expr       <- primary_expr (additive_op primary_expr)*

primary_expr        <- ident / decimal_literal / '(' expr ')'

type_name           <- ident

additive_op         <- < [+-] >
assignment_op       <- < '=' >
decimal_literal     <- < [0-9]+ >
ident               <- < [a-zA-Z_][a-zA-Z_0-9]* >

%whitespace         <- [ \t\r\n]*

)"
#endif
    );

    if(!errs.empty()) return;

    p["decimal_literal"] = [](peg::SemanticValues const& v) {
        return ast_node_t{ v.line_info(), AST::INT_CONST, v.token(), {}, v.token_to_number<int64_t>() };
    };
    p["ident"] = [](peg::SemanticValues const& v) {
        return ast_node_t{ v.line_info(), AST::IDENT, v.token() };
    };
    p["type_name"] = [](peg::SemanticValues const& v) {
        return ast_node_t{ v.line_info(), AST::TYPE, v.token() };
    };
    p["primary_expr"] = [](peg::SemanticValues const& v) {
        return std::any_cast<ast_node_t>(v[0]);
    };
    p["additive_op"] = [](peg::SemanticValues const& v) -> ast_node_t {
        return { v.line_info(), AST::TOKEN, v.token() };
    };
    p["additive_expr"] = [](peg::SemanticValues const& v) -> ast_node_t {
        size_t num_ops = v.size() / 2;
        if(num_ops == 0)
            return std::any_cast<ast_node_t>(v[0]);
        ast_node_t a{ v.line_info(), AST::OP_ADDITIVE };
        for(auto& child : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(child)));
        return a;
    };
    p["assignment_op"] = [](peg::SemanticValues const& v) -> ast_node_t {
        return { v.line_info(), AST::TOKEN, v.token() };
    };
    p["expr"] = [](peg::SemanticValues const& v) -> ast_node_t {
        auto& child0 = std::any_cast<ast_node_t>(v[0]);
        if(v.choice() == 1) return child0;
        auto& child1 = std::any_cast<ast_node_t>(v[1]);
        auto& child2 = std::any_cast<ast_node_t>(v[2]);
        assert(child1.type == AST::TOKEN);
        auto type = AST::OP_ASSIGN;
        return { v.line_info(), type, v.token(), { std::move(child0), std::move(child2) } };
    };
    p["expr_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        return { v.line_info(), AST::EXPR_STMT, v.token(), { std::any_cast<ast_node_t>(v[0]) } };
    };
    p["compound_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::BLOCK, v.token() };
        for(auto& child : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(child)));
        return a;
    };
    p["func_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::FUNC_STMT, v.token() };
        a.children.emplace_back(std::move(std::any_cast<ast_node_t>(v[0])));
        a.children.emplace_back(std::move(std::any_cast<ast_node_t>(v[1])));
        a.children.emplace_back(std::move(std::any_cast<ast_node_t>(v.back())));
        return a;
    };
    p["decl_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::DECL_STMT, v.token() };
        a.children.emplace_back(std::move(std::any_cast<ast_node_t>(v[0])));
        a.children.emplace_back(std::move(std::any_cast<ast_node_t>(v[1])));
        return a;
    };
    p["global_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        return std::any_cast<ast_node_t>(v[0]);
    };
    p["program"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::PROGRAM, v.token() };
        for(auto& child : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(child)));
        return a;
    };
    input_data = std::string(
        (std::istreambuf_iterator<char>(fi)),
        (std::istreambuf_iterator<char>()));
    
    if(!p.parse(input_data, ast) && errs.empty())
        errs.push_back({ "An unknown parse error occurred." });
}

}
