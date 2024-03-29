#include "abc_interp.h"
#include "abc_instr.h"

#include <assert.h>

static uint8_t space(abc_interp_t* interp, uint8_t n)
{
    return interp->sp < (uint8_t)(256 - n) ? 1 : 0;
}

static uint16_t imm16(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t t = ((uint16_t)(h->prog(h->user, interp->pc++)) << 0);
    t +=         ((uint16_t)(h->prog(h->user, interp->pc++)) << 8);
    return t;
}

static uint32_t imm24(abc_interp_t* interp, abc_host_t const* h)
{
    uint32_t t = ((uint32_t)(h->prog(h->user, interp->pc++)) << 0);
    t +=         ((uint32_t)(h->prog(h->user, interp->pc++)) << 8);
    t +=         ((uint32_t)(h->prog(h->user, interp->pc++)) << 16);
    return t;
}

static abc_result_t push(abc_interp_t* interp, uint8_t x)
{
    if(interp->sp == 255)
        return ABC_ERROR;
    interp->stack[interp->sp++] = x;
    return ABC_NORMAL;
}

static abc_result_t pushn(abc_interp_t* interp, abc_host_t const* h, uint8_t n)
{
    if(!space(interp, n)) return ABC_ERROR;
    for(uint8_t i = 0; i < n; ++i)
        (void)push(interp, h->prog(h->user, interp->pc++));
    return ABC_NORMAL;
}

static abc_result_t push_zn(abc_interp_t* interp, uint8_t n)
{
    for(uint8_t i = 0; i < n; ++i)
    {
        abc_result_t r = push(interp, 0);
        if(r != ABC_NORMAL)
            return r;
    }
    return ABC_NORMAL;
}

static uint8_t headn(abc_interp_t* interp, uint8_t n)
{
    return interp->stack[(uint8_t)(interp->sp - n)];
}

static uint8_t head(abc_interp_t* interp)
{
    return headn(interp, 1);
}

static abc_result_t dupw(abc_interp_t* interp, uint8_t n)
{
    if(!space(interp, 2)) return ABC_ERROR;
    (void)push(interp, interp->stack[interp->sp - n - 1]);
    (void)push(interp, interp->stack[interp->sp - n - 1]);
    return ABC_NORMAL;
}

static abc_result_t sextn(abc_interp_t* interp, uint8_t n)
{
    if(!space(interp, n)) return ABC_ERROR;
    uint8_t t = head(interp) & 0x80 ? 0xff : 0x00;
    for(uint8_t i = 0; i < n; ++i)
        (void)push(interp, t);
    return ABC_NORMAL;
}

static abc_result_t getgn(abc_interp_t* interp, abc_host_t const* h, uint8_t n)
{
    if(!space(interp, n)) return ABC_ERROR;
    uint16_t t = imm16(interp, h) + n;
    for(uint8_t i = 0; i < n; ++i)
        (void)push(interp, interp->globals[t - i - 1]);
    return ABC_NORMAL;
}

static abc_result_t setgn(abc_interp_t* interp, abc_host_t const* h, uint8_t n)
{
    uint16_t t = imm16(interp, h);
    for(uint8_t i = 0; i < n; ++i)
        interp->globals[t + i] = interp->stack[--interp->sp];
    return ABC_NORMAL;
}

abc_result_t run(abc_interp_t* interp, abc_host_t const* host)
{
    if(!interp || !host || !host->prog || !host->millis)
        return ABC_ERROR;

    abc_host_t h = *host;

    if(interp->pc == 0)
    {
        // initialize vm
        interp->pc = 20;
    }

    uint8_t instr = h.prog(h.user, interp->pc++);
    switch(instr)
    {
    case I_NOP:   return ABC_NORMAL;
    case I_PUSH:  return push(interp, h.prog(h.user, interp->pc++));
    case I_P0:    return push(interp, 0);
    case I_P1:    return push(interp, 1);
    case I_P2:    return push(interp, 2);
    case I_P3:    return push(interp, 3);
    case I_P4:    return push(interp, 4);
    case I_P5:    return push(interp, 5);
    case I_P6:    return push(interp, 6);
    case I_P7:    return push(interp, 7);
    case I_P8:    return push(interp, 8);
    case I_P16:   return push(interp, 16);
    case I_P32:   return push(interp, 32);
    case I_P64:   return push(interp, 64);
    case I_P128:  return push(interp, 128);
    case I_P00:   return push_zn(interp, 2);
    case I_P000:  return push_zn(interp, 3);
    case I_P0000: return push_zn(interp, 4);
    case I_PZN:   return push_zn(interp, h.prog(h.user, interp->pc++));
    case I_PUSHG: return pushn(interp, &h, 2);
    case I_PUSHL: return pushn(interp, &h, 3);
    case I_PUSH4: return pushn(interp, &h, 4);
    case I_SEXT:  return sextn(interp, 1);
    case I_SEXT2: return sextn(interp, 2);
    case I_SEXT3: return sextn(interp, 3);
    case I_DUP:   return push(interp, head(interp));
    case I_DUP2:  return push(interp, headn(interp, 2));
    case I_DUP3:  return push(interp, headn(interp, 3));
    case I_DUP4:  return push(interp, headn(interp, 4));
    case I_DUP5:  return push(interp, headn(interp, 5));
    case I_DUP6:  return push(interp, headn(interp, 6));
    case I_DUP7:  return push(interp, headn(interp, 7));
    case I_DUP8:  return push(interp, headn(interp, 8));
    case I_DUPW:  return dupw(interp, 1);
    case I_DUPW2: return dupw(interp, 2);
    case I_DUPW3: return dupw(interp, 3);
    case I_DUPW4: return dupw(interp, 4);
    case I_DUPW5: return dupw(interp, 5);
    case I_DUPW6: return dupw(interp, 6);
    case I_DUPW7: return dupw(interp, 7);
    case I_DUPW8: return dupw(interp, 8);
    case I_GETL:  goto unknown_instruction;
    case I_GETL2: goto unknown_instruction;
    case I_GETL4: goto unknown_instruction;
    case I_GETLN: goto unknown_instruction;
    case I_SETL:  goto unknown_instruction;
    case I_SETL2: goto unknown_instruction;
    case I_SETL4: goto unknown_instruction;
    case I_SETLN: goto unknown_instruction;
    case I_GETG:  return getgn(interp, &h, 1);
    case I_GETG2: return getgn(interp, &h, 2);
    case I_GETG4: return getgn(interp, &h, 4);
    case I_GETGN: return getgn(interp, &h, interp->stack[--interp->sp]);
    case I_SETG:  return setgn(interp, &h, 1);
    case I_SETG2: return setgn(interp, &h, 2);
    case I_SETG4: return setgn(interp, &h, 4);
    case I_SETGN: return setgn(interp, &h, interp->stack[--interp->sp]);
    case I_GETP:  goto unknown_instruction;
    case I_GETPN: goto unknown_instruction;
    case I_GETR:  goto unknown_instruction;
    case I_GETR2: goto unknown_instruction;
    case I_GETRN: goto unknown_instruction;
    case I_SETR:  goto unknown_instruction;
    case I_SETR2: goto unknown_instruction;
    case I_SETRN: goto unknown_instruction;
    case I_POP:   interp->sp -= 1; return ABC_NORMAL;
    case I_POP2:  interp->sp -= 2; return ABC_NORMAL;
    case I_POP3:  interp->sp -= 3; return ABC_NORMAL;
    case I_POP4:  interp->sp -= 4; return ABC_NORMAL;
    case I_POPN:  interp->sp -= h.prog(h.user, interp->pc++); return ABC_NORMAL;
    case I_AIXB1: goto unknown_instruction;
    case I_AIDXB: goto unknown_instruction;
    case I_AIDX:  goto unknown_instruction;
    case I_PIDXB: goto unknown_instruction;
    case I_UAIDX: goto unknown_instruction;
    case I_UPIDX: goto unknown_instruction;
    case I_ASLC:  goto unknown_instruction;
    case I_PSLC:  goto unknown_instruction;
    case I_REFL:  goto unknown_instruction;
    case I_REFGB: goto unknown_instruction;
    case I_INC:   interp->stack[interp->sp - 1] += 1; return ABC_NORMAL;
    case I_DEC:   interp->stack[interp->sp - 1] -= 1; return ABC_NORMAL;
    case I_LINC:  goto unknown_instruction;
    case I_PINC:  goto unknown_instruction;
    case I_PINC2: goto unknown_instruction;
    case I_PINC3: goto unknown_instruction;
    case I_PINC4: goto unknown_instruction;
    case I_PDEC:  goto unknown_instruction;
    case I_PDEC2: goto unknown_instruction;
    case I_PDEC3: goto unknown_instruction;
    case I_PDEC4: goto unknown_instruction;
    case I_PINCF: goto unknown_instruction;
    case I_PDECF: goto unknown_instruction;
    case I_ADD:   goto unknown_instruction;
    case I_ADD2:  goto unknown_instruction;
    case I_ADD3:  goto unknown_instruction;
    case I_ADD4:  goto unknown_instruction;
    case I_SUB:   goto unknown_instruction;
    case I_SUB2:  goto unknown_instruction;
    case I_SUB3:  goto unknown_instruction;
    case I_SUB4:  goto unknown_instruction;
    case I_ADD2B: goto unknown_instruction;
    case I_ADD3B: goto unknown_instruction;
    case I_SUB2B: goto unknown_instruction;
    case I_MUL2B: goto unknown_instruction;
    case I_MUL:   goto unknown_instruction;
    case I_MUL2:  goto unknown_instruction;
    case I_MUL3:  goto unknown_instruction;
    case I_MUL4:  goto unknown_instruction;
    case I_UDIV2: goto unknown_instruction;
    case I_UDIV4: goto unknown_instruction;
    case I_DIV2:  goto unknown_instruction;
    case I_DIV4:  goto unknown_instruction;
    case I_UMOD2: goto unknown_instruction;
    case I_UMOD4: goto unknown_instruction;
    case I_MOD2:  goto unknown_instruction;
    case I_MOD4:  goto unknown_instruction;
    case I_LSL:   goto unknown_instruction;
    case I_LSL2:  goto unknown_instruction;
    case I_LSL4:  goto unknown_instruction;
    case I_LSR:   goto unknown_instruction;
    case I_LSR2:  goto unknown_instruction;
    case I_LSR4:  goto unknown_instruction;
    case I_ASR:   goto unknown_instruction;
    case I_ASR2:  goto unknown_instruction;
    case I_ASR4:  goto unknown_instruction;
    case I_AND:   goto unknown_instruction;
    case I_AND2:  goto unknown_instruction;
    case I_AND4:  goto unknown_instruction;
    case I_OR:    goto unknown_instruction;
    case I_OR2:   goto unknown_instruction;
    case I_OR4:   goto unknown_instruction;
    case I_XOR:   goto unknown_instruction;
    case I_XOR2:  goto unknown_instruction;
    case I_XOR4:  goto unknown_instruction;
    case I_COMP:  goto unknown_instruction;
    case I_COMP2: goto unknown_instruction;
    case I_COMP4: goto unknown_instruction;
    case I_BOOL:  goto unknown_instruction;
    case I_BOOL2: goto unknown_instruction;
    case I_BOOL3: goto unknown_instruction;
    case I_BOOL4: goto unknown_instruction;
    case I_CULT:  goto unknown_instruction;
    case I_CULT2: goto unknown_instruction;
    case I_CULT3: goto unknown_instruction;
    case I_CULT4: goto unknown_instruction;
    case I_CSLT:  goto unknown_instruction;
    case I_CSLT2: goto unknown_instruction;
    case I_CSLT3: goto unknown_instruction;
    case I_CSLT4: goto unknown_instruction;
    case I_CFEQ:  goto unknown_instruction;
    case I_CFLT:  goto unknown_instruction;
    case I_NOT:   goto unknown_instruction;
    case I_FADD:  goto unknown_instruction;
    case I_FSUB:  goto unknown_instruction;
    case I_FMUL:  goto unknown_instruction;
    case I_FDIV:  goto unknown_instruction;
    case I_F2I:   goto unknown_instruction;
    case I_F2U:   goto unknown_instruction;
    case I_I2F:   goto unknown_instruction;
    case I_U2F:   goto unknown_instruction;
    case I_BZ:    goto unknown_instruction;
    case I_BZ1:   goto unknown_instruction;
    case I_BNZ:   goto unknown_instruction;
    case I_BNZ1:  goto unknown_instruction;
    case I_BZP:   goto unknown_instruction;
    case I_BZP1:  goto unknown_instruction;
    case I_BNZP:  goto unknown_instruction;
    case I_BNZP1: goto unknown_instruction;
    case I_JMP:   goto unknown_instruction;
    case I_JMP1:  goto unknown_instruction;
    case I_CALL:  goto unknown_instruction;
    case I_CALL1: goto unknown_instruction;
    case I_RET:   goto unknown_instruction;
    case I_SYS:   goto unknown_instruction;
    default:
    unknown_instruction:
        assert(0);
        return ABC_ERROR;
    }
}
