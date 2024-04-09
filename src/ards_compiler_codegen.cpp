#include "ards_compiler.hpp"

#include <sstream>
#include <assert.h>

namespace ards
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
    f.instrs.push_back({ I_PUSH, a.line(), (uint8_t)type.prim_size });
    f.instrs.push_back({ I_SETLN, a.line(), (uint8_t)frame.size });
    frame.size -= type.prim_size;
}

void compiler_t::codegen_return(compiler_func_t& f, compiler_frame_t& frame, ast_node_t const& n)
{
    // store return value
    if(!n.children.empty())
    {
        codegen_expr(f, frame, n.children[0], false);
        codegen_convert(f, frame, n, f.decl.return_type, n.children[0].comp_type);
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
    for(size_t i = 0; i < frame.size; ++i)
        f.instrs.push_back({ I_POP, n.line() });

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
    {
        assert(a.children.size() == 2 || a.children.size() == 3);
        type_annotate(a.children[0], frame);
        if(a.children[0].type == AST::INT_CONST && a.children[0].value == 0)
            break;
        bool is_for = (a.children.size() == 3);
        bool nocond = (a.children[0].type == AST::INT_CONST && a.children[0].value != 0);
        std::string start = codegen_label(f);
        std::string end = new_label(f);
        std::string cont = is_for ? new_label(f) : start;
        if(!nocond)
        {
            codegen_expr(f, frame, a.children[0], false);
            // TODO: unnecessary for a.children[0].comp_type.prim_size == 1
            codegen_convert(f, frame, a, TYPE_BOOL, a.children[0].comp_type);
            f.instrs.push_back({ I_BZ, a.children[0].line(), 0, 0, end });
            frame.size -= 1;
        }
        break_stack.push_back({ end, frame.size });
        continue_stack.push_back({ cont, frame.size });
        size_t ni = f.instrs.size();
        codegen(f, frame, a.children[1]);
        if(is_for)
        {
            codegen_label(f, cont);
            codegen(f, frame, a.children[2]);
        }
        break_stack.pop_back();
        continue_stack.pop_back();
        if(!nocond && f.instrs.size() == ni)
        {
            // empty body: invert condition and remove jump
            auto& instr = f.instrs.back();
            instr.instr = I_BNZ;
            instr.label = start;
        }
        else
        {
            f.instrs.push_back({ I_JMP, a.children[0].line(), 0, 0, start });
            if(!nocond)
                codegen_label(f, end);
        }
        break;
    }
    case AST::BREAK_STMT:
    {
        assert(!break_stack.empty());
        size_t n = frame.size - break_stack.back().second;
        for(size_t i = 0; i < n; ++i)
            f.instrs.push_back({ I_POP, a.line() });
        f.instrs.push_back({ I_JMP, a.line(), 0, 0, break_stack.back().first });
        break;
    }
    case AST::CONTINUE_STMT:
    {
        assert(!continue_stack.empty());
        size_t n = frame.size - continue_stack.back().second;
        for(size_t i = 0; i < n; ++i)
            f.instrs.push_back({ I_POP, a.line()});
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
            for(size_t i = 0; i < frame.scopes.back().size; ++i)
                f.instrs.push_back({ I_POP, line });
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
        for(size_t i = expr_prev_size; i < frame.size; ++i)
            f.instrs.push_back({ I_POP, a.line() });
        frame.size = expr_prev_size;
        break;
    }
    case AST::DECL_STMT:
    {
        decl(f, frame, a);
        break;
    }
    default:
        assert(false);
        errs.push_back({ "(codegen) Unimplemented AST node", a.line_info });
        return;
    }
    (void)prev_size;
    if(a.type != AST::DECL_STMT && errs.empty())
        assert(frame.size == prev_size);
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

    auto* pfrom = &orig_from;
    if(rto.is_array() && rfrom.is_array_ref())
    {
        errs.push_back({
            "Cannot create array from unsized array reference",
            n.line_info });
        return;
    }
    if(rto.is_array_ref())
    {
        if((rfrom.is_array() || rfrom.is_array_ref()) &&
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
            assert(rto.children[0] == rfrom.children[0]);
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
        for(size_t i = 0; i < num; ++i)
            f.instrs.push_back({ I_POP, n.line() });
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
