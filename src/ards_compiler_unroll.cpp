#include "ards_compiler.hpp"

namespace ards
{

static ast_node_t const& without_cast(ast_node_t const& n)
{
    if(n.type == AST::OP_CAST)
        return n.children[1];
    return n;
}

static bool is_ident(ast_node_t const& n, std::string const& ident)
{
    auto const& t = without_cast(n);
    if(t.type != AST::IDENT) return false;
    if(std::string(t.data) != ident) return false;
    return true;
}

static bool test_cond(ast_node_t const& cond, int64_t condval, int64_t x)
{
    if(cond.data == "<" ) return x <  condval;
    if(cond.data == "!=") return x != condval;
    if(cond.data == "<=") return x <= condval;
    if(cond.data == ">" ) return x >  condval;
    if(cond.data == ">=") return x >= condval;
    if(cond.data == "==") return x == condval;
    return false;
}

void compiler_t::unroll_for_loop(
    ast_node_t const& n, unroll_info_t const& u,
    compiler_func_t& f, compiler_frame_t& frame)
{
    auto const& stmt_init = n.children[0];
    auto const& stmt_body = n.children[1].children[1];
    auto type = resolve_type(stmt_init.children[0]);
    int64_t x = u.init;
    std::string label_break = new_label(f);
    std::string label_continue;
    break_stack.push_back({ label_break, frame.size });
    for(uint32_t iter = 0; iter < u.num; ++iter)
    {
        assert(iter < for_unroll_max_iters);

        label_continue = new_label(f);
        continue_stack.push_back({ label_continue, frame.size });

        frame.push();

        ast_node_t body_copy = stmt_body;

        auto& v = frame.scopes.back().locals[u.var_name];
        v.var.is_constexpr = true;
        v.var.value = x;
        v.var.type = type;

        codegen(f, frame, body_copy);

        x = truncate_value(type, x + u.incr);

        {
            uint16_t line = 0;
            for(auto it = f.instrs.rbegin(); it != f.instrs.rend(); ++it)
                if((line = it->line) != 0)
                    break;
            if(frame.scopes.back().size != 0)
                f.instrs.push_back({ I_POPN, line, uint8_t(frame.scopes.back().size) });
        }
        frame.pop();

        continue_stack.pop_back();
        codegen_label(f, label_continue);
    }

    break_stack.pop_back();
    codegen_label(f, label_break);
}

bool compiler_t::can_unroll_for_loop(ast_node_t const& n, unroll_info_t& u)
{
    // analyze init statement, increment, and condition
    assert(n.type == AST::BLOCK_FOR_STMT);
    assert(n.children.size() == 2);
    assert(n.children[1].type == AST::WHILE_STMT);
    assert(n.children[1].children.size() == 3);
    assert(n.children[1].children[1].type == AST::BLOCK);

    auto const& stmt_init = n.children[0];
    auto const& cond = without_cast(n.children[1].children[0]);
    //auto const& stmt_body = n.children[1].children[1];
    auto const& stmt_iter = n.children[1].children[2];

    auto type = resolve_type(stmt_init.children[0]);

    if(stmt_iter.type != AST::EXPR_STMT)
        return false;
    auto const& iter = stmt_iter.children[0];

    u.var_name.clear();
    u.init = 0;
    u.incr = 0;
    u.num = 0;

    // figure out variable name and initial value
    if(stmt_init.type == AST::DECL_STMT)
    {
        auto const& nv = stmt_init.children[1];
        if(nv.type != AST::IDENT) return false;
        u.var_name = std::string(nv.data);
        auto const& expr = without_cast(stmt_init.children[2]);
        if(expr.type != AST::INT_CONST) return false;
        u.init = truncate_value(type, expr.value);
    }
    else if(stmt_init.type == AST::EXPR_STMT)
    {
        auto const& child = stmt_init.children[0];
        if(child.type != AST::OP_ASSIGN) return false;
        auto const& nv = child.children[0];
        if(nv.type != AST::IDENT) return false;
        u.var_name = std::string(nv.data);
        auto const& expr = without_cast(child.children[1]);
        if(expr.type != AST::INT_CONST) return false;
        u.init = expr.value;
    }
    else
        return false;

    // figure out increment value
    if(iter.type == AST::OP_ASSIGN)
    {
        auto const& nv = iter.children[0];
        if(!is_ident(nv, u.var_name)) return false;
        auto const& expr = without_cast(iter.children[1]);
        if(expr.type != AST::OP_ADDITIVE) return false;
        if(expr.children.size() != 2) return false;
        if(!is_ident(expr.children[0], u.var_name)) return false;
        auto const& val = without_cast(expr.children[1]);
        if(val.type != AST::INT_CONST) return false;
        u.incr = val.value;
        if(expr.data == "-")
            u.incr = -u.incr;
    }
    else return 0;

    // figure out number of iterations
    if(u.incr == 0)
        return false;
    if(cond.type != AST::OP_RELATIONAL && cond.type != AST::OP_EQUALITY)
        return false;
    if(!is_ident(cond.children[0], u.var_name))
        return false;
    if(without_cast(cond.children[1]).type != AST::INT_CONST)
        return false;
    int64_t x = u.init;
    int64_t condval = without_cast(cond.children[1]).value;
    if(!test_cond(cond, condval, x)) // no iterations
        return true;
    for(size_t i = 1; i <= for_unroll_max_iters; ++i)
    {
        x += u.incr;
        x = truncate_value(type, x);
        if(!test_cond(cond, condval, x))
        {
            u.num = (uint32_t)i;
            return true;
        }
    }

    return false;
}

}
