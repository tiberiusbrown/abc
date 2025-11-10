#include "abc_compiler.hpp"

#include <algorithm>
#include <sstream>
#include <assert.h>

namespace abc
{

compiler_local_t const* compiler_t::resolve_local(compiler_frame_t const& frame, ast_node_t const& n)
{
    std::string name(n.data);
    for(auto it = frame.scopes.rbegin(); it != frame.scopes.rend(); ++it)
    {
        auto jt = it->locals.find(name);
        if(jt != it->locals.end())
        {
            size_t offset = frame.size - jt->second.frame_offset;
            if(offset >= 256)
                errs.push_back({ "Stack frame exceeded 256 bytes", n.line_info });
            return &jt->second;
        }
    }
    return nullptr;
}

compiler_global_t const* compiler_t::resolve_global(ast_node_t const& n)
{
    std::string name(n.data);
    auto it = globals.find(name);
    if(it == globals.end())
        return nullptr;
    auto& g = it->second;
    if(g.var.is_constexpr && g.var.type.is_font() && g.builtin)
        create_builtin_font(g);
    return &g;
}

compiler_type_t const* compiler_t::resolve_member(
    ast_node_t const& a, std::string const& name, size_t* offset)
{
    auto const& t = a.comp_type.without_ref();
    assert(t.is_struct());
    assert(t.children.size() == t.members.size());
    for(size_t i = 0; i < t.children.size(); ++i)
    {
        if(t.members[i].first != name) continue;
        if(offset) *offset = t.members[i].second;
        return &t.children[i];
    }
    errs.push_back({ "Unknown member \"" + name + "\"", a.line_info });
    return nullptr;
}

compiler_lvalue_t compiler_t::resolve_lvalue(
        compiler_func_t const& f, compiler_frame_t const& frame,
        ast_node_t const& n)
{
    if(n.type != AST::IDENT && n.type != AST::ARRAY_INDEX && n.type != AST::STRUCT_MEMBER)
    {
        errs.push_back({
            "\"" + std::string(n.data) + "\" cannot be assigned to",
            n.line_info });
        return {};
    }
    (void)f;
    uint16_t line = n.line();
    if(n.type == AST::ARRAY_INDEX || n.type == AST::STRUCT_MEMBER)
    {
        compiler_lvalue_t t{};
        t.type = n.comp_type;
        t.ref_ast = &n;
        t.line = line;
        return t;
    }
    if(auto* local = resolve_local(frame, n))
        return { local->var.type, false, uint8_t(frame.size - local->frame_offset), line };
    std::string name(n.data);
    if(auto* global = resolve_global(n))
        return { global->var.type, true, 0, line, name };
    errs.push_back({ "Undefined variable \"" + name + "\"", n.line_info });
    return {};
}

void compiler_t::codegen_store_return(
        compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a)
{
    compiler_lvalue_t t{};
    auto const& type = f.decl.return_type;
    f.instrs.push_back({ I_SETLN, a.line(), (uint8_t)type.prim_size, (uint8_t)frame.size });
    frame.size -= type.prim_size;
}

void compiler_t::codegen_return(compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& n)
{
    // store return value
    if(!n.children.empty())
    {
        if(n.children[0].type == AST::COMPOUND_LITERAL)
        {
            codegen_expr_compound(f, frame, n.children[0], f.decl.return_type);
        }
        else
        {
            codegen_expr(f, frame, n.children[0], false);
            codegen_convert(
                f, frame, n,
                f.decl.return_type,
                n.children[0].comp_type);
        }
        codegen_store_return(f, frame, n);
    }
    else if(f.decl.return_type.prim_size != 0)
    {
        errs.push_back({ "Missing return value", n.line_info });
        return;
    }
    if(!errs.empty()) return;

    // pop remaining func args
    assert(frame.size < 256);
    if(frame.size != 0)
        f.instrs.push_back({ I_POPN, n.line(), uint8_t(frame.size) });
    f.instrs.push_back({ I_RET, n.line() });
}

void compiler_t::codegen_function(compiler_func_t& f)
{
    if(!f.decl.return_type.is_copyable())
    {
        errs.push_back({
            "Return type of function \"" + f.name + "\" is not copyable",
            f.line_info });
        return;
    }
    if(f.decl.return_type.is_prog)
    {
        errs.push_back({
            "Return type of function \"" + f.name + "\" is prog",
            f.line_info });
        return;
    }

    compiler_frame_t frame{};

    frame.push();

    // add func args to scope in reverse order
    assert(f.arg_names.size() == f.decl.arg_types.size());
    auto& scope = frame.scopes.back();
    for(size_t i = f.arg_names.size(); i != 0; --i)
    {
        auto const& name = f.arg_names[i - 1];
        auto const& type = f.decl.arg_types[i - 1];
        auto& local = scope.locals[name];
        size_t size = type.prim_size;
        local.frame_offset = frame.size;
        local.var.type = type;
        scope.size += size;
        frame.size += size;
    }

    frame.push();

    codegen(f, frame, f.block);

    // no need to call codegen_return here
    // all function blocks are guaranteed to end with a return statement
    assert(f.block.children.back().type == AST::RETURN_STMT);
}

std::string compiler_t::new_label(compiler_func_t& f)
{
    std::ostringstream ss;
    ss << f.label_count;
    f.label_count += 1;
    std::string label = "$L_" + f.name + "_" + ss.str();
    return label;
}

std::string compiler_t::codegen_label(compiler_func_t& f)
{
    std::string label = new_label(f);
    codegen_label(f, label);
    return label;
}

void compiler_t::codegen_label(compiler_func_t& f, std::string const& label)
{
    f.instrs.push_back({ I_NOP, 0, 0, 0, label, true });
}

void compiler_t::codegen(compiler_func_t& f, compiler_frame_t& frame, ast_node_t& a)
{
    if(!errs.empty()) return;
    auto prev_size = frame.size;
    switch(a.type)
    {
    case AST::EMPTY_STMT:
        break;
    case AST::IF_STMT:
    {
        assert(a.children.size() == 3);
        type_annotate(a.children[0], frame);

        if(a.children[0].type == AST::INT_CONST && a.children[0].value != 0)
        {
            // only codegen main statement body
            codegen(f, frame, a.children[1]);
            break;
        }

        if(a.children[0].type == AST::INT_CONST && a.children[0].value == 0)
        {
            // only codegen else statement
            codegen(f, frame, a.children[2]);
            break;
        }

        codegen_expr(f, frame, a.children[0], false);
        // TODO: unnecessary for a.children[0].comp_type.prim_size == 1
        codegen_convert(f, frame, a, TYPE_BOOL, a.children[0].comp_type);
        size_t cond_index = f.instrs.size();
        f.instrs.push_back({ I_BZ, a.line() });
        frame.size -= 1;
        codegen(f, frame, a.children[1]);
        size_t jmp_index = f.instrs.size();
        if(a.children[2].type != AST::EMPTY_STMT)
            f.instrs.push_back({ I_JMP, a.line() });
        auto else_label = codegen_label(f);
        f.instrs[cond_index].label = else_label;
        if(a.children[2].type != AST::EMPTY_STMT)
        {
            codegen(f, frame, a.children[2]);
            auto end_label = codegen_label(f);
            f.instrs[jmp_index].label = end_label;
        }
        break;
    }
    case AST::WHILE_STMT:
    case AST::FOR_STMT:
    case AST::DO_WHILE_STMT:
    {
        size_t prev_instrs = f.instrs.size();
        bool is_for = (a.type == AST::FOR_STMT);
        bool is_do_while = (a.type == AST::DO_WHILE_STMT);
        if(is_for)
        {
            for(size_t i = 3; i < a.children.size(); ++i)
                codegen(f, frame, a.children[i]);
        }
        type_annotate(a.children[0], frame);
        if(!is_do_while && a.children[0].type == AST::INT_CONST && a.children[0].value == 0)
            break;
        bool nocond = (a.children[0].type == AST::INT_CONST && a.children[0].value != 0);
        std::string end = new_label(f);

        if(!is_do_while && !nocond)
        {
            // duplicate codegen for condition
            codegen_expr(f, frame, a.children[0], false);
            codegen_convert(f, frame, a, TYPE_BOOL, a.children[0].comp_type);
            f.instrs.push_back({ I_BZ, a.children[0].line(), 0, 0, end });
            frame.size -= 1;
        }
        std::string start = codegen_label(f);
        std::string cont = is_for ? new_label(f) : start;

        break_stack.push_back({ end, frame.size });
        continue_stack.push_back({ cont, frame.size });
        codegen(f, frame, a.children[1]);
        if(is_for)
        {
            codegen_label(f, cont);
            codegen(f, frame, a.children[2]);
        }
        break_stack.pop_back();
        continue_stack.pop_back();

        size_t body_instrs = f.instrs.size() - prev_instrs;
        unroll_info_t u;
        if(enable_sized_unrolling && is_for && can_unroll_for_loop_sized(a, u) &&
            u.num * body_instrs <= unroll_sized_max_instrs &&
            u.num <= unroll_sized_max_iters)
        {
            f.instrs.resize(prev_instrs);
            if(is_for) { frame.pop(); frame.push(); }
            unroll_loop_sized(a, u, f, frame);
            break;
        }

        // TODO: unrolling can defeat/prevent inlining if body contains function call

        // unsized loop unroll
        if(enable_unsized_unrolling && unroll_unsized_max_iters > 1 &&
            body_instrs <= unroll_unsized_max_add_instrs)
        {
            size_t extra_instrs = body_instrs;
            for(size_t i = 1; i < unroll_unsized_max_iters; ++i)
            {
                if(!nocond)
                {
                    codegen_expr(f, frame, a.children[0], false);
                    codegen_convert(f, frame, a, TYPE_BOOL, a.children[0].comp_type);
                    f.instrs.push_back({ I_BZ, a.children[0].line(), 0, 0, end });
                    frame.size -= 1;
                }

                // TODO: continue defeats unrolling

                break_stack.push_back({ end, frame.size });
                continue_stack.push_back({ cont, frame.size });
                codegen(f, frame, a.children[1]);
                if(is_for)
                    codegen(f, frame, a.children[2]);
                break_stack.pop_back();
                continue_stack.pop_back();

                extra_instrs += body_instrs;
                if(extra_instrs > unroll_unsized_max_add_instrs)
                    break;
            }
            if(!nocond)
            {
                codegen_expr(f, frame, a.children[0], false);
                codegen_convert(f, frame, a, TYPE_BOOL, a.children[0].comp_type);
                f.instrs.push_back({ I_BNZ, a.children[0].line(), 0, 0, start });
                frame.size -= 1;
            }
            else
                f.instrs.push_back({ I_JMP, a.children[0].line(), 0, 0, start });
            codegen_label(f, end);
            break;
        }

        if(!nocond)
        {
            codegen_expr(f, frame, a.children[0], false);
            codegen_convert(f, frame, a, TYPE_BOOL, a.children[0].comp_type);
            f.instrs.push_back({ I_BNZ, a.children[0].line(), 0, 0, start });
            frame.size -= 1;
        }
        else
        {
            f.instrs.push_back({ I_JMP, a.children[0].line(), 0, 0, start });
        }
        codegen_label(f, end);
        break;
    }
    case AST::BREAK_STMT:
    {
        if(break_stack.empty())
        {
            errs.push_back({ "No control block for 'break'", a.line_info });
            return;
        }
        assert(!break_stack.empty());
        size_t n = frame.size - break_stack.back().second;
        if(n != 0)
            f.instrs.push_back({ I_POPN, a.line(), (uint8_t)n });
        f.instrs.push_back({ I_JMP, a.line(), 0, 0, break_stack.back().first });
        break;
    }
    case AST::CONTINUE_STMT:
    {
        if(continue_stack.empty())
        {
            errs.push_back({ "No control block for 'continue'", a.line_info });
            return;
        }
        assert(!continue_stack.empty());
        size_t n = frame.size - continue_stack.back().second;
        if(n != 0)
            f.instrs.push_back({ I_POPN, a.line(), uint8_t(n) });
        f.instrs.push_back({ I_JMP, a.line(), 0, 0, continue_stack.back().first });
        break;
    }
    case AST::BLOCK:
    {
        size_t block_prev_size = frame.size;
        frame.push();
        for(auto& child : a.children)
            codegen(f, frame, child);
        // don't need to pop locals after final return statement
        if(&a != &f.block)
        {
            uint16_t line = 0;
            for(auto it = f.instrs.rbegin(); it != f.instrs.rend(); ++it)
                if((line = it->line) != 0)
                    break;
            assert(line != 0);
            if(frame.scopes.back().size != 0)
                f.instrs.push_back({ I_POPN, line, uint8_t(frame.scopes.back().size) });
        }
        frame.pop();
        (void)block_prev_size;
        assert(!errs.empty() || frame.size == block_prev_size);
        break;
    }
    case AST::RETURN_STMT:
    {
        if(!a.children.empty())
            type_annotate(a.children[0], frame, f.decl.return_type.prim_size);
        codegen_return(f, frame, a);
        break;
    }
    case AST::EXPR_STMT:
    {
        size_t expr_prev_size = frame.size;
        assert(a.children.size() == 1);
        type_annotate(a.children[0], frame);
        codegen_expr(f, frame, a.children[0], false);
        assert(frame.size < 256);
        int n = int(frame.size - expr_prev_size);
        assert(n >= 0);
        if(n >= 0)
            f.instrs.push_back({ I_POPN, a.line(), uint8_t(n) });
        frame.size = expr_prev_size;
        break;
    }
    case AST::DECL_STMT:
    {
        decl(f, frame, a);
        break;
    }
    case AST::SWITCH_STMT:
    {
        codegen_switch(f, frame, a);
        break;
    }
    default:
        assert(false);
        errs.push_back({ "(codegen) Unimplemented AST node", a.line_info });
        return;
    }
    (void)prev_size;
    if(a.type != AST::DECL_STMT && a.type != AST::FOR_STMT && errs.empty())
        assert(frame.size == prev_size);
}

void compiler_t::codegen_switch(
    compiler_func_t& f, compiler_frame_t& frame, ast_node_t& a)
{
    type_annotate(a.children[0], frame);
    for(size_t i = 1; i < a.children.size(); ++i)
    {
        auto& a_case = a.children[i];
        assert(a_case.type == AST::SWITCH_CASE);
        for(size_t j = 1; j < a_case.children.size(); ++j)
        {
            auto& a_case_item = a_case.children[j];
            assert(a_case_item.type == AST::SWITCH_CASE_ITEM);
            for(auto& child : a_case_item.children)
            {
                child.insert_cast(a.children[0].comp_type.without_ref());
                type_annotate(child, frame);
                if(child.type != AST::INT_CONST)
                {
                    errs.push_back({
                        "Case value must be integral constant expression",
                        child.line_info });
                    return;
                }
            }
        }
    }

    auto expr_type = a.children[0].comp_type.without_ref();
    if(!expr_type.is_prim() || expr_type.is_float)
    {
        errs.push_back({
            "Switch expression must be of integral type",
            a.children[0].line_info });
        return;
    }

    // verify no more than one default label
    {
        bool default_found = false;
        for(size_t i = 1; i < a.children.size(); ++i)
        {
            if(a.children[i].children.size() <= 1)
            {
                if(default_found)
                {
                    errs.push_back({
                        "Switch statement cannot have more than one default case",
                        a.children[i].line_info });
                    return;
                }
                default_found = true;
            }
        }
    }

    struct range_t
    {
        int64_t a, b;
        std::string_view data;
        std::pair<size_t, size_t> line_info;
        size_t index;
    };
    std::vector<range_t> ranges;

    for(size_t i = 1; i < a.children.size(); ++i)
    {
        auto const& a_case = a.children[i];
        for(size_t j = 1; j < a_case.children.size(); ++j)
        {
            auto const& a_case_item = a_case.children[j];
            auto const& c = a_case_item.children;
            std::string_view data = a_case_item.data;
            auto line_info = a_case_item.line_info;
            if(c.size() == 1)
                ranges.push_back({ c[0].value, c[0].value, data, line_info, i - 1 });
            else if(c.size() == 2)
                ranges.push_back({ c[0].value, c[1].value, data, line_info, i - 1 });
            else
                assert(false);
        }
    }
    std::sort(
        ranges.begin(), ranges.end(),
        [](range_t const& x, range_t const& y) { return x.a < y.a; });

    // verify no overlapping cases
    for(size_t i = 1; i < ranges.size(); ++i)
    {
        auto const& ra = ranges[i - 1];
        auto const& rb = ranges[i];
        if(ra.b >= rb.a)
        {
            errs.push_back({
                "Case \"" + std::string(ra.data) + "\" overlaps with case \"" + std::string(rb.data) + "\"",
                ra.line_info });
            return;
        }
    }

    size_t expr_offset = frame.size;
    codegen_expr(f, frame, a.children[0], false);
    codegen_convert(f, frame, a.children[0], expr_type, a.children[0].comp_type);

    std::string end_label = new_label(f);
    std::string default_label;
    std::vector<std::string> case_labels;
    int64_t rmin, rmax;
    bool is_jump_table = false;

    if(ranges.empty())
        goto pop_expr;

    rmin = ranges.front().a;
    rmax = ranges.back().b;

    case_labels.reserve(a.children.size());
    for(size_t i = 1; i < a.children.size(); ++i)
    {
        case_labels.push_back(new_label(f));
        if(a.children[i].children.size() <= 1)
            default_label = case_labels.back();
    }
    case_labels.push_back(end_label);
    if(default_label.empty())
        default_label = end_label;

    is_jump_table = expr_type.prim_size == 1 &&
        ranges.size() >= switch_min_ranges_for_jump_table && (
        !expr_type.is_signed && rmin >= 0 && rmax <= 255 ||
        expr_type.is_signed && rmin >= -128 && rmax <= 127);

    // 8-bit jump table logic
    if(is_jump_table)
    {
        std::vector<std::string> table;
        table.resize(256);
        for(auto& t : table)
            t = default_label;
        for(size_t i = 0; i < ranges.size(); ++i)
        {
            auto const& r = ranges[i];
            for(auto j = r.a; j <= r.b; ++j)
                table[(uint8_t)j] = case_labels[r.index];
        }
        std::string jump_table = progdata_label();
        auto& pdata = progdata[jump_table];
        pdata.data.resize(256 * 3);
        pdata.relocs_prog.resize(256);
        for(size_t i = 0; i < 256; ++i)
            pdata.relocs_prog[i] = { i * 3, table[i] };

        // TODO: dedicated jump table instruction that does all this?
        f.instrs.push_back({ I_PUSH, a.children[0].line(), 0 });
        f.instrs.push_back({ I_PUSH, a.children[0].line(), 3 });
        f.instrs.push_back({ I_PUSH, a.children[0].line(), 0 });
        f.instrs.push_back({ I_MUL2, a.children[0].line() });
        f.instrs.push_back({ I_PUSH, a.children[0].line(), 0 });
        f.instrs.push_back({ I_PUSHL, a.children[0].line(), 0, 0, jump_table });
        f.instrs.push_back({ I_ADD3, a.children[0].line() });
        f.instrs.push_back({ I_GETPN, a.children[0].line(), 3 });
        f.instrs.push_back({ I_IJMP, a.children[0].line() });
        frame.size -= 1;
    }

    // linear search logic
    if(!is_jump_table)
    {
        for(size_t i = 1; i < a.children.size(); ++i)
        {
            auto const& a_case = a.children[i];
            auto const& label = case_labels[i - 1];
            for(size_t j = 1; j < a_case.children.size(); ++j)
            {
                auto const& a_case_item = a_case.children[j];
                auto line = a_case_item.line();
                instr_t isub = instr_t(I_SUB + expr_type.prim_size - 1);
                instr_t ibool = instr_t(I_BOOL + expr_type.prim_size - 1);
                if(a_case_item.children.size() == 1)
                {
                    // single case value
                    f.instrs.push_back({
                        I_GETLN, line,
                        (uint32_t)expr_type.prim_size,
                        (uint32_t)(frame.size - expr_offset) });
                    frame.size += expr_type.prim_size;
                    codegen_expr(f, frame, a_case_item.children[0], false);
                    codegen_convert(
                        f, frame, a_case_item.children[0],
                        expr_type, a_case_item.children[0].comp_type);
                    f.instrs.push_back({ isub, line });
                    f.instrs.push_back({ ibool, line });
                    f.instrs.push_back({ I_BZ, line, 0, 0, label });
                    frame.size -= expr_type.prim_size * 2;
                }
                else if(a_case_item.children.size() == 2)
                {
                    // case range: start ... end
                    std::string temp_label = new_label(f);
                    instr_t ilt = instr_t(
                        (expr_type.is_signed ? I_CSLT : I_CULT) +
                        expr_type.prim_size - 1);

                    // if(val < start) goto next case item
                    f.instrs.push_back({
                        I_GETLN, line,
                        (uint32_t)expr_type.prim_size,
                        (uint32_t)(frame.size - expr_offset) });
                    frame.size += expr_type.prim_size;
                    codegen_expr(f, frame, a_case_item.children[0], false);
                    codegen_convert(
                        f, frame, a_case_item.children[0],
                        expr_type, a_case_item.children[0].comp_type);
                    f.instrs.push_back({ ilt, line });
                    f.instrs.push_back({ I_BNZ, line, 0, 0, temp_label });
                    frame.size -= expr_type.prim_size * 2;

                    // if(!(end < val)) goto case label
                    codegen_expr(f, frame, a_case_item.children[1], false);
                    codegen_convert(
                        f, frame, a_case_item.children[1],
                        expr_type, a_case_item.children[1].comp_type);
                    f.instrs.push_back({
                        I_GETLN, line,
                        (uint32_t)expr_type.prim_size,
                        (uint32_t)(frame.size - expr_offset) });
                    frame.size += expr_type.prim_size;
                    f.instrs.push_back({ ilt, line });
                    f.instrs.push_back({ I_BZ, line, 0, 0, label });
                    frame.size -= expr_type.prim_size * 2;

                    codegen_label(f, temp_label);
                }
                else assert(false);
            }
        }
        f.instrs.push_back({ I_JMP, a.line(), 0, 0, default_label });
    }

    // case statements
    for(size_t i = 1; i < a.children.size(); ++i)
    {
        break_stack.push_back({ end_label, frame.size });
        continue_stack.push_back({ case_labels[i], frame.size });
        codegen_label(f, case_labels[i - 1]);
        codegen(f, frame, a.children[i].children[0]);
        f.instrs.push_back({ I_JMP, a.line(), 0, 0, end_label });
        break_stack.pop_back();
        continue_stack.pop_back();
    }

    codegen_label(f, end_label);

pop_expr:
    if(!is_jump_table)
    {
        if(expr_type.prim_size != 0)
            f.instrs.push_back({ I_POPN, a.line(), uint8_t(expr_type.prim_size) });
        frame.size -= expr_type.prim_size;
    }
}

void compiler_t::codegen_store(
    compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& a)
{
    if(!errs.empty()) return;
    assert(a.comp_type.without_ref().prim_size < 256);
    auto size = a.comp_type.without_ref().prim_size;
    auto const* t = &a.comp_type.without_ref_single();
    while(t->is_ref())
    {
        codegen_dereference(f, frame, a, *t);
        t = &t->children[0];
    }
    f.instrs.push_back({ I_SETRN, a.line(), (uint8_t)size});
    frame.size -= 2;
    frame.size -= size;
}

void compiler_t::codegen_convert(
    compiler_func_t& f, compiler_frame_t& frame,
    ast_node_t const& n,
    compiler_type_t const& orig_to, compiler_type_t const& orig_from)
{
    if(!errs.empty()) return;

    if(orig_to.is_ref() && orig_from.is_ref() &&
        orig_to.children[0].is_prog != orig_from.children[0].is_prog)
    {
        errs.push_back({
            "Cannot convert between prog reference and non-prog reference",
            n.line_info });
        return;
    }

    auto const& rfrom = orig_from.without_ref();
    auto const& rto = orig_to.without_ref();

    if(rfrom.is_func_ref() != rto.is_func_ref())
    {
        errs.push_back({
            "Cannot convert between function reference and non-function reference",
            n.line_info });
        return;
    }

    if(rfrom.is_func_ref() && rfrom != rto)
    {
        errs.push_back({
            "Cannot convert between function references of mismatched signatures",
            n.line_info });
        return;
    }

    if(rfrom.is_struct() && rto.is_struct() && rfrom.struct_name != rto.struct_name)
    {
        errs.push_back({
            "Cannot convert between different structs",
            n.line_info });
        return;
    }

    if(rto.is_label_ref() && (
        !rfrom.is_label_ref() || rto.without_prog() != rfrom.without_prog()))
    {
        errs.push_back({
            "Invalid conversion to asset handle",
            n.line_info });
        return;
    }

    auto* pfrom = &orig_from;

    while(pfrom->is_ref() && pfrom->children[0].is_ref())
    {
        pfrom = &pfrom->children[0];
        codegen_dereference(f, frame, n, *pfrom);
    }

    if(rto.is_array() && rfrom.is_array_ref())
    {
        errs.push_back({
            "Cannot create array from unsized array reference",
            n.line_info });
        return;
    }
    if(rto.is_array_ref())
    {
        if(orig_from.is_ref() && rfrom.is_array() &&
            rfrom.array_md_base_type() == rto.children[0])
        {
            // multidimensional array to unsized array reference
            size_t size = rfrom.array_md_size();
            compiler_type_t new_type;
            new_type.prim_size = rfrom.prim_size;
            new_type.type = compiler_type_t::ARRAY;
            new_type.children.push_back(rfrom.array_md_base_type());
            new_type.is_prog = rfrom.is_prog;
            codegen_convert(f, frame, n, orig_to, new_type);
            return;
        }
        else if((rfrom.is_array() || rfrom.is_array_ref()) &&
            rfrom.children[0] != rto.children[0] &&
            !rto.children[0].is_byte)
        {
            errs.push_back({
                "Cannot create unsized array reference: mismatched element types",
                n.line_info });
            return;
        }
        if(rfrom.is_array_ref())
        {
            while(pfrom->is_ref())
            {
                pfrom = &pfrom->children[0];
                codegen_dereference(f, frame, n, *pfrom);
            }
            assert(rto.children[0] == rfrom.children[0] || rto.children[0].is_byte);
            return;
        }
        if(rto.children[0].is_byte && !rfrom.is_copyable())
        {
            errs.push_back({
                "Cannot create byte UAR from non-copyable type",
                n.line_info });
            return;
        }
        if(rfrom.is_array() || rto.children[0].is_byte)
        {
            auto size = rto.children[0].is_byte ? rfrom.prim_size : rfrom.array_size();
            bool prog = rto.children[0].is_prog;
            frame.size += (prog ? 3 : 2);
            f.instrs.push_back({ I_PUSH, n.line(), (uint8_t)(size >> 0) });
            f.instrs.push_back({ I_PUSH, n.line(), (uint8_t)(size >> 8) });
            if(prog)
                f.instrs.push_back({ I_PUSH, n.line(), (uint8_t)(size >> 16) });
            return;
        }
        errs.push_back({ "Cannot create unsized array reference from non-array", n.line_info });
        return;
    }
    if(rfrom.is_array())
    {
        if(!rto.is_array())
        {
            errs.push_back({ "Cannot convert array to non-array", n.line_info });
            return;
        }
        if(rfrom.children[0].without_prog() != rto.children[0])
        {
            errs.push_back({ "Cannot implicitly convert array elements", n.line_info });
            return;
        }
        if(rto.children[0] == TYPE_CHAR)
        {
            // allow below resizing logic for char arrays
        }
        else if(rto.array_size() != rfrom.array_size())
        {
            errs.push_back({ "Incompatible array lengths", n.line_info });
            return;
        }
    }
    while(pfrom->is_ref() && *pfrom != orig_to)
    {
        pfrom = &pfrom->children[0];
        codegen_dereference(f, frame, n, *pfrom);
    }
    auto const& from = *pfrom;
    assert(from.prim_size != 0);

    if(*pfrom == orig_to)
        return;

    auto const& to = orig_to.without_ref_single();

    assert(!(to.is_float && to.is_signed));
    assert(!(from.is_float && from.is_signed));
    if(to == from) return;

    if(to.is_float && from.is_signed)
    {
        codegen_convert(f, frame, n, TYPE_I32, from);
        f.instrs.push_back({ I_I2F, n.line() });
        return;
    }
    if(to.is_float && !from.is_signed)
    {
        codegen_convert(f, frame, n, TYPE_U32, from);
        f.instrs.push_back({ I_U2F, n.line() });
        return;
    }
    if(from.is_float && to.is_signed)
    {
        f.instrs.push_back({ I_F2I, n.line() });
        codegen_convert(f, frame, n, to, TYPE_I32);
        return;
    }
    if(from.is_float && !to.is_signed)
    {
        f.instrs.push_back({ I_F2U, n.line() });
        codegen_convert(f, frame, n, to, TYPE_U32);
        return;
    }

    if(to.is_bool)
    {
        if(from.is_bool) return;
        int num = int(from.prim_size - 1);
        frame.size -= num;
        static_assert(I_BOOL2 == I_BOOL + 1);
        static_assert(I_BOOL3 == I_BOOL + 2);
        static_assert(I_BOOL4 == I_BOOL + 3);
        f.instrs.push_back({ instr_t(I_BOOL + num), n.line() });
        return;
    }
    if(to.prim_size == from.prim_size) return;
    if(to.prim_size < from.prim_size)
    {
        size_t num = from.prim_size - to.prim_size;
        frame.size -= num;
        if(num != 0)
            f.instrs.push_back({ I_POPN, n.line(), uint8_t(num) });
    }
    if(to.prim_size > from.prim_size)
    {
        size_t num = to.prim_size - from.prim_size;
        frame.size += num;
        compiler_instr_t instr;
        if(from.is_signed)
            instr = { I_SEXT, n.line() };
        else
            instr = { I_PUSH, n.line(), 0 };
        for(size_t i = 0; i < num; ++i)
            f.instrs.push_back(instr);
    }
}

void compiler_t::codegen_dereference(
    compiler_func_t& f, compiler_frame_t& frame,
    ast_node_t const& n, compiler_type_t const& t)
{
    size_t size = t.prim_size;
    if(t.prim_size >= 256)
    {
        errs.push_back({ "Expression too large (256 bytes or more)", n.line_info });
        return;
    }
    f.instrs.push_back({ t.is_prog ? I_GETPN : I_GETRN, n.line(), (uint8_t)size });
    frame.size -= (t.is_prog ? 3 : 2);
    frame.size += size;
}

}
