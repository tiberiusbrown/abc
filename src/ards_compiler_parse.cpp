#include "ards_compiler.hpp"

#include <iostream>
#include <streambuf>
#include <variant>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4456)
#endif
#include <peglib.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace ards
{

static peg::parser parser;
static bool parser_initialized = false;

template<AST T> ast_node_t infix(peg::SemanticValues const& v)
{
    size_t num_ops = v.size() / 2;
    if(num_ops == 0)
        return std::any_cast<ast_node_t>(v[0]);
    ast_node_t a{ v.line_info(), T, v.token() };
    for(auto& child : v)
        a.children.push_back(std::any_cast<ast_node_t>(child));
    return a;
};

void compiler_t::parse(std::vector<char> const& input, ast_node_t& ast)
{
    if(!parser_initialized)
        init_parser();
    if(!parser.parse({ input.data(), input.size() }, ast) && errs.empty())
        errs.push_back({ "An unknown parse error occurred." });
}

void compiler_t::init_parser()
{
    peg::parser& p = parser;

    p.set_logger([&](size_t line, size_t column, std::string const& msg) {
        errs.push_back({ msg, { line, column } });
    });
    p.load_grammar(
#if 0
// fuller grammar (TODO)
R"(
        
program             <- global_stmt*

global_stmt         <- import_stmt / decl_stmt / func_stmt
import_stmt         <- 'import' import_path ';'
import_path         <- ident ('.' ident)*
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

global_stmt         <- import_stmt / struct_stmt / decl_stmt / func_stmt
import_stmt         <- 'import' import_path ';'
import_path         <- ident ('.' ident)*
decl_stmt           <- 'constexpr' type_name decl_stmt_item_list ';' /
                       'saved' type_name decl_stmt_item_list ';' /
                       type_name decl_stmt_item_list ';'
decl_stmt_item_list <- decl_stmt_item (',' decl_stmt_item)*
decl_stmt_item      <- ident ('=' expr)?
struct_stmt         <- 'struct' ident '{' struct_decl_stmt* '}' ';'
struct_decl_stmt    <- type_name ident (',' ident)* ';'
func_stmt           <- type_name ident '(' arg_decl_list? ')' compound_stmt
compound_stmt       <- '{' stmt* '}'
stmt                <- compound_stmt /
                       return_stmt   /
                       if_stmt       /
                       while_stmt    /
                       for_stmt      /
                       break_stmt    /
                       continue_stmt /
                       decl_stmt     /
                       expr_stmt
if_stmt             <- 'if' '(' expr ')' stmt ('else' stmt)?
while_stmt          <- 'while' '(' expr ')' stmt
for_stmt            <- 'for' '(' for_init_stmt expr ';' expr ')' stmt
for_init_stmt       <- decl_stmt / expr_stmt
expr_stmt           <- ';' / expr ';'
return_stmt         <- 'return' expr? ';'
break_stmt          <- 'break' ';'
continue_stmt       <- 'continue' ';'

# right-associative binary assignment operator
expr                <- '{' '}' /
                       '{' expr (',' expr)* ','? '}' /
                       postfix_expr assignment_op expr /
                       logical_or_expr

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
multiplicative_expr <- unary_expr          (multiplicative_op unary_expr         )*

unary_expr          <- '++' unary_expr /
                       '--' unary_expr /
                       unary_op unary_expr /
                       postfix_expr
postfix_expr        <- primary_expr postfix*
postfix             <- '(' arg_expr_list? ')' /
                       '[' expr ']' / # '[' expr (':' expr)? ']' /
                       '.' ident /
                       '++' /
                       '--'

primary_expr        <- hex_literal /
                       float_literal /
                       decimal_literal /
                       bool_literal /
                       char_literal /
                       sprites_literal /
                       font_literal /
                       tones_literal /
                       ident /
                       '(' expr ')' /
                       string_literal

type_name           <- ident type_name_postfix*
type_name_postfix   <- '[' expr ']' / '&' / 'prog' / '[' ']' 'prog' '&' / '[' ']' '&'
arg_decl_list       <- type_name ident (',' type_name ident)*
arg_expr_list       <- expr (',' expr)*

equality_op         <- < '==' / '!=' >
shift_op            <- < '<<' / '>>' >
additive_op         <- < [+-] >
multiplicative_op   <- < [*/%] >
relational_op       <- < '<=' / '>=' / '<' / '>' >
assignment_op       <- < '=' / [*/%&|^+-]'=' / '<<=' / '>>=' >
unary_op            <- < '!' / '-' / '~' >

sprites_literal     <- 'sprites' '{' string_literal '}' /
                       'sprites' '{' decimal_literal 'x' decimal_literal sprite_data '}'
sprite_data         <- string_literal / sprite_row+
sprite_row          <- < [^\n}]+ >

# TODO: change to float literal for font size
font_literal        <- 'font' '{' decimal_literal string_literal '}'

tones_literal       <- 'tones' '{' string_literal '}' /
                       'tones' '{' (tones_note decimal_literal)+ '}'
tones_note          <- < [A-G0-9b#-]+ >

decimal_literal     <- < [0-9]+'u'? >
float_literal       <- < [0-9]*'.'[0-9]+('e'[+-]?[0-9]+)? > /
                       < [0-9]+'.'[0-9]*('e'[+-]?[0-9]+)? > /
                       < [0-9]+'e'[+-]?[0-9]+ >
hex_literal         <- < '0x'[0-9a-fA-F]+'u'? >
char_literal        <- < '\'' < string_char > '\'' >
bool_literal        <- < 'true' / 'false' >
ident               <- < '$'?[a-zA-Z_][a-zA-Z_0-9]* >

string_literal      <- < '"' < string_char* > '"' >
string_char         <- char_escape /
                       [^\\"\n]
char_escape         <- '\\x'[0-9a-fA-F][0-9a-fA-F] /
                       '\\'[nr\\t"']

%whitespace         <- ([ \t\r\n] / comment / multiline_comment)*
comment             <- '//' (!linebreak .)* linebreak
linebreak           <- [\n\r]
multiline_comment   <- '/*' (! '*/' .)* '*/'

)"
#endif
    );

    if(!errs.empty()) return;

    /*
    * 
    At the AST level, for statements are transformed:

        for(A; B; C)
            D;

    is transformed into a while loop with two statements (body and iterator adv)

        {
            A;
            while(B)
            {
                D;
                C;
            }
        }

        the "value" of the while AST node is set to 1 to indicate a for loop
        for label stack purposes

    */
    p["for_stmt"] = [](peg::SemanticValues const& v) {
        ast_node_t a{ v.line_info(), AST::BLOCK, v.token() };
        auto A = std::any_cast<ast_node_t>(v[0]);
        auto B = std::any_cast<ast_node_t>(v[1]);
        auto C = std::any_cast<ast_node_t>(v[2]);
        auto D = std::any_cast<ast_node_t>(v[3]);
        a.children.emplace_back(std::move(A));
        a.children.push_back({ v.line_info(), AST::WHILE_STMT, v.token() });
        auto& w = a.children.back();
        w.children.emplace_back(std::move(B));
        w.children.push_back({ D.line_info, AST::BLOCK, D.data });
        w.children.back().children.emplace_back(std::move(D));
        w.children.push_back({ C.line_info, AST::EXPR_STMT, C.data, { C } });
        return a;
    };
    p["for_init_stmt"] = [](peg::SemanticValues const& v) {
        return std::any_cast<ast_node_t>(v[0]);
    };

    p["decimal_literal"] = [](peg::SemanticValues const& v) {
        int64_t x = 0;
        std::from_chars(v.token().data(), v.token().data() + v.token().size(), x, 10);
        ast_node_t a{ v.line_info(), AST::INT_CONST, v.token(), {} };
        a.value = x;
        a.comp_type = prim_type_for_dec((uint32_t)x, a.data.back() != 'u');
        return a;
    };
    p["float_literal"] = [](peg::SemanticValues const& v) {
        double x = 0;
        std::string t(v.token());
        sscanf(t.c_str(), "%lf", &x);
        //std::from_chars(
        //    v.token().data(), v.token().data() + v.token().size(),
        //    x, std::chars_format::general);
        ast_node_t a{ v.line_info(), AST::FLOAT_CONST, v.token(), {} };
        a.fvalue = x;
        a.comp_type = TYPE_FLOAT;
        return a;
    };
    p["hex_literal"] = [](peg::SemanticValues const& v) {
        int64_t x = 0;
        auto t = v.token().substr(2);
        std::from_chars(t.data(), t.data() + t.size(), x, 16);
        ast_node_t a{ v.line_info(), AST::INT_CONST, v.token(), {} };
        a.value = x;
        a.comp_type = prim_type_for_hex((uint32_t)x, a.data.back() != 'u');
        return a;
    };

    p["bool_literal"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::INT_CONST, v.token() };
        a.comp_type = TYPE_BOOL;
        if(v.token() == "true")
            a.value = 1;
        else
        {
            assert(v.token() == "false");
            a.value = 0;
        }
        return a;
    };

    p["char_literal"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::INT_CONST, v.token() };
        a.comp_type = TYPE_CHAR;
        auto d = strlit_data(a);
        if(!d.empty())
            a.value = (int8_t)d[0];
        return a;
        };

    p["ident"] = [](peg::SemanticValues const& v) -> ast_node_t {
        return { v.line_info(), AST::IDENT, v.token() };
    };
    p["type_name"] = [](peg::SemanticValues const& v) -> ast_node_t {
        auto ident = std::any_cast<ast_node_t>(v[0]);
        ast_node_t a{ v.line_info(), AST::TYPE, ident.data };
        for(size_t i = 1; i < v.size(); ++i)
        {
            ast_node_t b = std::any_cast<ast_node_t>(v[i]);
            b.children.emplace_back(std::move(a));
            a = std::move(b);
        }
        return a;
    };
    p["type_name_postfix"] = [](peg::SemanticValues const& v) -> ast_node_t {
        switch(v.choice())
        {
        case 0:
            // sized array
            return {
                v.line_info(), AST::TYPE_ARRAY, v.token(),
                { std::any_cast<ast_node_t>(v[0]) }
            };
        case 1:
            // reference
            return { v.line_info(), AST::TYPE_REF, v.token() };
        case 2:
        {
            // prog
            ast_node_t a{ v.line_info(), AST::TYPE_PROG, v.token() };
            a.comp_type.is_prog = true;
            return a;
        }
        case 3:
            // unsized array reference to prog array
            return { v.line_info(), AST::TYPE_AREF_PROG, v.token() };
        case 4:
            // unsized array reference
            return { v.line_info(), AST::TYPE_AREF, v.token() };
        default:
            break;
        }
        assert(false);
        return {};
    };

    p["unary_expr"] = [](peg::SemanticValues const& v) -> ast_node_t {
        switch(v.choice())
        {
        case 0:
        case 1:
        {
            // pre-increment, pre-decrement
            // transform into child0 +=/-= 1
            auto child0 = std::any_cast<ast_node_t>(v[0]);
            bool simple = (child0.type == AST::IDENT);
            ast_node_t a{
                v.line_info(),
                simple ? AST::OP_ASSIGN : AST::OP_ASSIGN_COMPOUND,
                v.token(), { child0 } };
            auto opdata = v.token().substr(0, 1);
            ast_node_t op{ v.line_info(), AST::OP_ADDITIVE, v.token() };
            if(simple)
                op.children.push_back(child0);
            else
                op.children.push_back({ v.line_info(), AST::OP_COMPOUND_ASSIGNMENT_DEREF, child0.data });
            op.children.push_back({ v.line_info(), AST::TOKEN, opdata });
            ast_node_t one{ v.line_info(), AST::INT_CONST };
            one.value = 1;
            op.children.emplace_back(std::move(one));
            a.children.emplace_back(std::move(op));
            return a;
        }
        case 2:
            return {
                v.line_info(), AST::OP_UNARY, v.token(),
                { std::any_cast<ast_node_t>(v[0]), std::any_cast<ast_node_t>(v[1]) }
            };
        case 3:
            return std::any_cast<ast_node_t>(v[0]);
        default:
            assert(false);
            return {};
        }
    };

    // form a left-associative binary tree
    p["postfix_expr"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a = std::any_cast<ast_node_t>(v[0]);
        if(v.size() == 1) return a;
        for(size_t i = 1; i < v.size(); ++i)
        {
            ast_node_t b = std::any_cast<ast_node_t>(v[i]);
            ast_node_t pair{ v.line_info(), AST::NONE, v.token() };
            auto type = b.type;
            if(type == AST::FUNC_ARGS)
            {
                pair.type = AST::FUNC_CALL;
                pair.children.emplace_back(std::move(a));
                pair.children.emplace_back(std::move(b));
            }
            else if(type == AST::ARRAY_INDEX || b.type == AST::STRUCT_MEMBER)
            {
                pair.type = type;
                pair.children.emplace_back(std::move(a));
                pair.children.emplace_back(std::move(b.children[0]));
            }
            else if(b.type == AST::ARRAY_SLICE)
            {
                pair.type = type;
                pair.children.emplace_back(std::move(a));
                pair.children.emplace_back(std::move(b.children[0]));
                pair.children.emplace_back(std::move(b.children[1]));
            }
            else if(type == AST::OP_INC_POST || type == AST::OP_DEC_POST)
            {
                pair.type = type;
                pair.children.emplace_back(std::move(a));
            }
            a = std::move(pair);
        }
        return a;
    };

    p["postfix"] = [](peg::SemanticValues const& v) -> ast_node_t {
        if(v.choice() == 0)
        {
            // function call args
            ast_node_t a = { v.line_info(), AST::FUNC_ARGS, v.token() };
            if(v.size() == 1)
            {
                auto child = std::any_cast<ast_node_t>(v[0]);
                a.children = std::move(child.children);
            }
            return a;
        }
        else if(v.choice() == 1 && v.size() == 1)
        {
            // array indexing
            ast_node_t a = { v.line_info(), AST::ARRAY_INDEX, v.token() };
            a.children.push_back(std::any_cast<ast_node_t>(v[0]));
            return a;
        }
        else if(v.choice() == 1 && v.size() == 2)
        {
            // array slice
            ast_node_t a = { v.line_info(), AST::ARRAY_SLICE, v.token() };
            a.children.push_back(std::any_cast<ast_node_t>(v[0]));
            a.children.push_back(std::any_cast<ast_node_t>(v[1]));
            return a;
        }
        else if(v.choice() == 2)
        {
            // struct member
            ast_node_t a = { v.line_info(), AST::STRUCT_MEMBER, v.token() };
            a.children.push_back(std::any_cast<ast_node_t>(v[0]));
            return a;
        }
        else if(v.choice() == 3)
        {
            // post-increment
            ast_node_t a = { v.line_info(), AST::OP_INC_POST, v.token() };
            return a;
        }
        else if(v.choice() == 4)
        {
            // post-decrement
            ast_node_t a = { v.line_info(), AST::OP_DEC_POST, v.token() };
            return a;
        }
        return {};
    };

    p["arg_decl_list"] = [](peg::SemanticValues const& v) {
        assert(v.size() % 2 == 0);
        ast_node_t a = { v.line_info(), AST::LIST, v.token() };
        for(auto& child : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(child)));
        return a;
    };
    p["arg_expr_list"] = [](peg::SemanticValues const& v) {
        ast_node_t a = { v.line_info(), AST::LIST, v.token() };
        for(auto& child : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(child)));
        return a;
    };

    p["primary_expr"] = [](peg::SemanticValues const& v) {
        return std::any_cast<ast_node_t>(v[0]);
    };

    const auto token = [](peg::SemanticValues const& v) {
        return ast_node_t{ v.line_info(), AST::TOKEN, v.token() };
    };
    p["assignment_op"    ] = token;
    p["equality_op"      ] = token;
    p["relational_op"    ] = token;
    p["shift_op"         ] = token;
    p["additive_op"      ] = token;
    p["multiplicative_op"] = token;
    p["unary_op"         ] = token;
    p["sprite_row"       ] = token;
    p["tones_note"       ] = token;

    p["bitwise_and_expr"   ] = infix<AST::OP_BITWISE_AND>;
    p["bitwise_or_expr"    ] = infix<AST::OP_BITWISE_OR>;
    p["logical_and_expr"   ] = infix<AST::OP_LOGICAL_AND>;
    p["logical_or_expr"    ] = infix<AST::OP_LOGICAL_OR>;
    p["bitwise_xor_expr"   ] = infix<AST::OP_BITWISE_XOR>;
    p["equality_expr"      ] = infix<AST::OP_EQUALITY>;
    p["relational_expr"    ] = infix<AST::OP_RELATIONAL>;
    p["shift_expr"         ] = infix<AST::OP_SHIFT>;
    p["additive_expr"      ] = infix<AST::OP_ADDITIVE>;
    p["multiplicative_expr"] = infix<AST::OP_MULTIPLICATIVE>;

    p["string_literal"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::STRING_LITERAL, v.token() };
        return a;
    };

    p["font_literal"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::FONT, v.token() };
        for(auto& child : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(child)));
        return a;
    };

    p["tones_literal"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::TONES, v.token() };
        for(auto& child : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(child)));
        return a;
    };

    p["sprites_literal"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::SPRITES, v.token() };
        if(v.choice() == 0)
        {
            a.children.push_back({ v.line_info(), AST::INT_CONST, "0", {}, 0 });
            a.children.push_back({ v.line_info(), AST::INT_CONST, "0", {}, 0 });
        }
        for(auto& child : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(child)));
        return a;
    };

    p["sprite_data"] = [](peg::SemanticValues const& v) -> ast_node_t {
        if(v.choice() == 0)
        {
            return std::any_cast<ast_node_t>(v[0]);
        }
        else if(v.choice() == 1)
        {
            ast_node_t a{ v.line_info(), AST::TOKEN, v.token() };
            for(auto& child : v)
                a.children.emplace_back(std::move(std::any_cast<ast_node_t>(child)));
            return a;
        }
        return {};
    };

    p["expr"] = [](peg::SemanticValues const& v) -> ast_node_t {
        if(v.choice() == 0 || v.choice() == 1)
        {
            ast_node_t a{ v.line_info(), AST::COMPOUND_LITERAL, v.token() };
            for(auto& child : v)
                a.children.push_back(std::move(std::any_cast<ast_node_t>(child)));
            return a;
        }
        auto child0 = std::any_cast<ast_node_t>(v[0]);
        if(v.choice() == 3) return child0;

        // normal assignment
        auto child1 = std::any_cast<ast_node_t>(v[1]);
        auto child2 = std::any_cast<ast_node_t>(v[2]);
        assert(child1.type == AST::TOKEN);
        if(child1.data == "=")
            return { v.line_info(), AST::OP_ASSIGN, v.token(), { std::move(child0), std::move(child2) } };
        
        // compound assignment
        bool simple = (child0.type == AST::IDENT);
        ast_node_t a{
            v.line_info(),
            simple ? AST::OP_ASSIGN : AST::OP_ASSIGN_COMPOUND,
            v.token(), { child0 } };
        auto type = AST::OP_ADDITIVE;
        auto opdata = child1.data.substr(0, child1.data.size() - 1);
        if(opdata == "*" || opdata == "/" || opdata == "%")
            type = AST::OP_MULTIPLICATIVE;
        else if(opdata == "<<" || opdata == ">>")
            type = AST::OP_SHIFT;
        else if(opdata == "|")
            type = AST::OP_BITWISE_OR;
        else if(opdata == "&")
            type = AST::OP_BITWISE_AND;
        else if(opdata == "^")
            type = AST::OP_BITWISE_XOR;
        ast_node_t op{ v.line_info(), type, v.token() };
        if(simple)
            op.children.push_back(child0);
        else
            op.children.push_back({ v.line_info(), AST::OP_COMPOUND_ASSIGNMENT_DEREF, child0.data });
        if(type == AST::OP_ADDITIVE || type == AST::OP_MULTIPLICATIVE || type == AST::OP_SHIFT)
            op.children.push_back({ v.line_info(), AST::TOKEN, opdata });
        op.children.emplace_back(std::move(child2));
        a.children.emplace_back(std::move(op));
        return a;
    };
    p["if_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        auto B = std::any_cast<ast_node_t>(v[1]);
        ast_node_t BB{ B.line_info, AST::BLOCK, B.data };
        BB.children.emplace_back(std::move(B));
        ast_node_t a{
            v.line_info(), AST::IF_STMT, v.token(),
            //{ std::any_cast<ast_node_t>(v[0]), std::any_cast<ast_node_t>(v[1]) }
        };
        a.children.push_back(std::any_cast<ast_node_t>(v[0]));
        a.children.emplace_back(std::move(BB));
        //a.children.emplace_back(std::move(B));
        // always include else clause, even if only an empty statement
        ast_node_t else_stmt{};
        if(v.size() >= 3)
        {
            auto E = std::any_cast<ast_node_t>(v[2]);
            ast_node_t EB{ E.line_info, AST::BLOCK, E.data };
            EB.children.emplace_back(std::move(E));
            a.children.emplace_back(std::move(EB));
        }
        else
            a.children.push_back({ {}, AST::EMPTY_STMT, "" });
        return a;
    };
    p["expr_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        if(v.empty()) return { v.line_info(), AST::EMPTY_STMT, v.token() };
        ast_node_t a{ v.line_info(), AST::EXPR_STMT, v.token() };
        a.children.emplace_back(std::move(std::any_cast<ast_node_t>(v[0])));
        return a;
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
        auto& block = a.children.back();
        if(block.children.empty() || block.children.back().type != AST::RETURN_STMT)
            block.children.push_back({ {}, AST::RETURN_STMT, "" });
        if(v.size() == 4)
        {
            // arg decls
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(v[2])));
        }
        else
        {
            a.children.push_back({ {}, AST::LIST, "" });
        }
        return a;
    };
    p["struct_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::STRUCT_STMT, v.token() };
        for(auto& child : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(child)));
        return a;
    };
    p["struct_decl_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::DECL_STMT, v.token() };
        for(auto& t : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(t)));
        return a;
    };
    p["decl_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::DECL_STMT, v.token() };
        a.children.emplace_back(std::move(std::any_cast<ast_node_t>(v[0])));
        if(v.choice() == 0) // constexpr
            a.children[0].comp_type.is_constexpr = true;
        if(v.choice() == 1) // saved
            a.children[0].comp_type.is_saved = true;
        auto list = std::move(std::any_cast<ast_node_t>(v[1]));
        for(auto& t : list.children)
            a.children.emplace_back(std::move(t));
        return a;
    };
    p["decl_stmt_item"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::DECL_ITEM, v.token() };
        for(auto& t : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(t)));
        return a;
    };
    p["decl_stmt_item_list"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::LIST, v.token() };
        for(auto& t : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(t)));
        return a;
    };
    p["decl_expr"] = [](peg::SemanticValues const& v) -> ast_node_t {
        return std::any_cast<ast_node_t>(v[0]);
    };
    p["global_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        return std::any_cast<ast_node_t>(v[0]);
    };
    p["import_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        return std::any_cast<ast_node_t>(v[0]);
    };
    p["import_path"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::IMPORT_STMT, v.token() };
        for(auto& child : v)
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(child)));
        return a;
    };
    p["while_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::WHILE_STMT, v.token() };
        a.children.emplace_back(std::move(std::any_cast<ast_node_t>(v[0])));
        auto B = std::any_cast<ast_node_t>(v[1]);
        ast_node_t block{ B.line_info, AST::BLOCK, B.data };
        block.children.emplace_back(std::move(B));
        a.children.emplace_back(std::move(block));
        return a;
    };
    p["return_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::RETURN_STMT, v.token() };
        if(!v.empty())
            a.children.emplace_back(std::move(std::any_cast<ast_node_t>(v[0])));
        return a;
    };
    p["break_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        return { v.line_info(), AST::BREAK_STMT, v.token() };
    };
    p["continue_stmt"] = [](peg::SemanticValues const& v) -> ast_node_t {
        return { v.line_info(), AST::CONTINUE_STMT, v.token() };
    };
    p["program"] = [](peg::SemanticValues const& v) -> ast_node_t {
        ast_node_t a{ v.line_info(), AST::PROGRAM, v.token() };
        for(auto& child : v)
            a.children.push_back(std::move(std::any_cast<ast_node_t>(child)));
        return a;
    };

    parser_initialized = true;
}

}
