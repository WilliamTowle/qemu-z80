/*
 *  Z80 translation
 * 
 *  Copyright (c) 2007 Stuart Brady
 *  Copyright (c) 2003 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <assert.h>

#include "cpu.h"
#include "exec-all.h"
#include "disas.h"
#include "tcg-op.h"

#define PREFIX_CB  0x01
#define PREFIX_DD  0x02
#define PREFIX_ED  0x04
#define PREFIX_FD  0x08

#define zprintf(...)
//#define zprintf printf

typedef struct DisasContext {
    /* current insn context */
    int override; /* -1 if no override */
    int prefix;
    uint16_t pc; /* pc = pc + cs_base */
    int is_jmp; /* 1 = means jump (stop translation), 2 means CPU
                   static state change (stop translation) */
    /* current block context */
    target_ulong cs_base; /* base of CS segment */
    int cc_op;  /* current CC operation */
    int singlestep_enabled; /* "hardware" single step enabled */
    int jmp_opt; /* use direct block chaining for direct jumps */
    int flags; /* all execution flags */
    struct TranslationBlock *tb;
} DisasContext;

static void gen_eob(DisasContext *s);
static void gen_jmp(DisasContext *s, target_ulong pc);
static void gen_jmp_tb(DisasContext *s, target_ulong pc, int tb_num);

enum {
    /* 8-bit registers */
    OR_B,
    OR_C,
    OR_D,
    OR_E,
    OR_H,
    OR_L,
    OR_HLmem,
    OR_A,

    OR_IXh,
    OR_IXl,

    OR_IYh,
    OR_IYl,

    OR_IXmem,
    OR_IYmem,
};

char *regnames[] = {
    [OR_B]     = "b",
    [OR_C]     = "c",
    [OR_D]     = "d",
    [OR_E]     = "e",
    [OR_H]     = "h",
    [OR_L]     = "l",
    [OR_HLmem] = "(hl)",
    [OR_A]     = "a",

    [OR_IXh]   = "ixh",
    [OR_IXl]   = "ixl",

    [OR_IYh]   = "iyh",
    [OR_IYl]   = "iyl",

    [OR_IXmem] = "(ix+d)",
    [OR_IYmem] = "(iy+d)",
};

char *idxnames[] = {
    [OR_IXmem] = "ix",
    [OR_IYmem] = "iy",
};

/* signed hex byte value for printf */
#define shexb(val) (val < 0 ? '-' : '+'), (abs(val))

static GenOpFunc *gen_op_movb_T0_reg[] = {
    [OR_B]     = gen_op_movb_T0_B,
    [OR_C]     = gen_op_movb_T0_C,
    [OR_D]     = gen_op_movb_T0_D,
    [OR_E]     = gen_op_movb_T0_E,
    [OR_H]     = gen_op_movb_T0_H,
    [OR_L]     = gen_op_movb_T0_L,
    [OR_HLmem] = gen_op_movb_T0_HLmem,
    [OR_A]     = gen_op_movb_T0_A,
                 
    [OR_IXh]   = gen_op_movb_T0_IXh,
    [OR_IXl]   = gen_op_movb_T0_IXl,
                 
    [OR_IYh]   = gen_op_movb_T0_IYh,
    [OR_IYl]   = gen_op_movb_T0_IYl,
};               
    
static GenOpFunc1 *gen_op_movb_T0_idx[] = {
    [OR_IXmem] = gen_op_movb_T0_IXmem,
    [OR_IYmem] = gen_op_movb_T0_IYmem,
};

static GenOpFunc *gen_op_movb_reg_T0[] = {
    [OR_B]     = gen_op_movb_B_T0,
    [OR_C]     = gen_op_movb_C_T0,
    [OR_D]     = gen_op_movb_D_T0,
    [OR_E]     = gen_op_movb_E_T0,
    [OR_H]     = gen_op_movb_H_T0,
    [OR_L]     = gen_op_movb_L_T0,
    [OR_HLmem] = gen_op_movb_HLmem_T0,
    [OR_A]     = gen_op_movb_A_T0,
              
    [OR_IXh]   = gen_op_movb_IXh_T0,
    [OR_IXl]   = gen_op_movb_IXl_T0,
                                
    [OR_IYh]   = gen_op_movb_IYh_T0,
    [OR_IYl]   = gen_op_movb_IYl_T0,
};            
    
static GenOpFunc1 *gen_op_movb_idx_T0[] = {
    [OR_IXmem] = gen_op_movb_IXmem_T0,
    [OR_IYmem] = gen_op_movb_IYmem_T0,
};

static inline int regmap(int reg, int m) {
    switch (m) {
    case 1:
        switch (reg) {
        case OR_H:
            return OR_IXh;
        case OR_L:
            return OR_IXl;
        case OR_HLmem:
            return OR_IXmem;
        default:
            return reg;
        }
    case 2:
        switch (reg) {
        case OR_H:
            return OR_IYh;
        case OR_L:
            return OR_IYl;
        case OR_HLmem:
            return OR_IYmem;
        default:
            return reg;
        }
    default:
        return reg;
    }
}

static inline int is_indexed(int reg) {
    if (reg == OR_IXmem || reg == OR_IYmem)
        return 1;
    else
        return 0;
}

int reg[8] = {
    OR_B,
    OR_C,
    OR_D,
    OR_E,
    OR_H,
    OR_L,
    OR_HLmem,
    OR_A,
};

enum {
    /* 16-bit registers and register pairs */
    OR2_AF,
    OR2_BC,
    OR2_DE,
    OR2_HL,

    OR2_IX,
    OR2_IY,
    OR2_SP,

    OR2_AFX,
    OR2_BCX,
    OR2_DEX,
    OR2_HLX,
};

char *regpairnames[] = {
    [OR2_AF]  = "af",
    [OR2_BC]  = "bc",
    [OR2_DE]  = "de",
    [OR2_HL]  = "hl",
           
    [OR2_IX]  = "ix",
    [OR2_IY]  = "iy",
    [OR2_SP]  = "sp",
            
    [OR2_AFX] = "afx",
    [OR2_BCX] = "bcx",
    [OR2_DEX] = "dex",
    [OR2_HLX] = "hlx",
};

static GenOpFunc *gen_op_movw_T0_reg[] = {
    [OR2_AF]  = gen_op_movw_T0_AF,
    [OR2_BC]  = gen_op_movw_T0_BC,
    [OR2_DE]  = gen_op_movw_T0_DE,
    [OR2_HL]  = gen_op_movw_T0_HL,
                
    [OR2_IX]  = gen_op_movw_T0_IX,
    [OR2_IY]  = gen_op_movw_T0_IY,
    [OR2_SP]  = gen_op_movw_T0_SP,
                
    [OR2_AFX] = gen_op_movw_T0_AFX,
    [OR2_BCX] = gen_op_movw_T0_BCX,
    [OR2_DEX] = gen_op_movw_T0_DEX,
    [OR2_HLX] = gen_op_movw_T0_HLX,
};

static GenOpFunc *gen_op_movw_T1_reg[] = {
    [OR2_AF]  = gen_op_movw_T1_AF,
    [OR2_BC]  = gen_op_movw_T1_BC,
    [OR2_DE]  = gen_op_movw_T1_DE,
    [OR2_HL]  = gen_op_movw_T1_HL,
                
    [OR2_IX]  = gen_op_movw_T1_IX,
    [OR2_IY]  = gen_op_movw_T1_IY,
    [OR2_SP]  = gen_op_movw_T1_SP,
                
    [OR2_AFX] = gen_op_movw_T1_AFX,
    [OR2_BCX] = gen_op_movw_T1_BCX,
    [OR2_DEX] = gen_op_movw_T1_DEX,
    [OR2_HLX] = gen_op_movw_T1_HLX,
};

static GenOpFunc *gen_op_movw_reg_T0[] = {
    [OR2_AF]  = gen_op_movw_AF_T0,
    [OR2_BC]  = gen_op_movw_BC_T0,
    [OR2_DE]  = gen_op_movw_DE_T0,
    [OR2_HL]  = gen_op_movw_HL_T0,
                
    [OR2_IX]  = gen_op_movw_IX_T0,
    [OR2_IY]  = gen_op_movw_IY_T0,
    [OR2_SP]  = gen_op_movw_SP_T0,
                
    [OR2_AFX] = gen_op_movw_AFX_T0,
    [OR2_BCX] = gen_op_movw_BCX_T0,
    [OR2_DEX] = gen_op_movw_DEX_T0,
    [OR2_HLX] = gen_op_movw_HLX_T0,
};

static GenOpFunc *gen_op_movw_reg_T1[] = {
    [OR2_AF]  = gen_op_movw_AF_T1,
    [OR2_BC]  = gen_op_movw_BC_T1,
    [OR2_DE]  = gen_op_movw_DE_T1,
    [OR2_HL]  = gen_op_movw_HL_T1,
                
    [OR2_IX]  = gen_op_movw_IX_T1,
    [OR2_IY]  = gen_op_movw_IY_T1,
    [OR2_SP]  = gen_op_movw_SP_T1,
                
    [OR2_AFX] = gen_op_movw_AFX_T1,
    [OR2_BCX] = gen_op_movw_BCX_T1,
    [OR2_DEX] = gen_op_movw_DEX_T1,
    [OR2_HLX] = gen_op_movw_HLX_T1,
};

static inline int regpairmap(int regpair, int m) {
    switch (regpair) {
    case OR2_HL:
        switch (m) {
        case 1:
            return OR2_IX;
        case 2:
            return OR2_IY;
        default:
            return OR2_HL;
        }
    default:
        return regpair;
    }
}

int regpair[4] = {
    OR2_BC,
    OR2_DE,
    OR2_HL,
    OR2_SP,
};

int regpair2[4] = {
    OR2_BC,
    OR2_DE,
    OR2_HL,
    OR2_AF,
};

static inline void gen_jmp_im(target_ulong pc)
{
    gen_op_movl_pc_im(pc);
}

static void gen_debug(DisasContext *s, target_ulong cur_pc)
{
    if (s->cc_op != CC_OP_DYNAMIC)
        gen_op_set_cc_op(s->cc_op);
    gen_jmp_im(cur_pc);
    gen_op_debug();
    s->is_jmp = 3;
}

static void gen_eob(DisasContext *s)
{
    if (s->cc_op != CC_OP_DYNAMIC)
        gen_op_set_cc_op(s->cc_op);
    if (s->tb->flags & HF_INHIBIT_IRQ_MASK) {
        gen_op_reset_inhibit_irq();
    }
    if (s->singlestep_enabled) {
        gen_op_debug();
    } else {
        tcg_gen_exit_tb(0);
    }
    s->is_jmp = 3;
}

static void gen_exception(DisasContext *s, int trapno, target_ulong cur_pc)
{
    if (s->cc_op != CC_OP_DYNAMIC)
        gen_op_set_cc_op(s->cc_op);
    gen_jmp_im(cur_pc);
    gen_op_raise_exception(trapno);
    s->is_jmp = 3;
}

/* Conditions */

char *cc[8] = {
    "nz",
    "z",
    "nc",
    "c",
    "po",
    "pe",
    "p",
    "m",
};

static GenOpFunc1 *gen_op_jp_cc[8] = {
    gen_op_jp_nz,
    gen_op_jp_z,
    gen_op_jp_nc,
    gen_op_jp_c,
    gen_op_jp_po,
    gen_op_jp_pe,
    gen_op_jp_p,
    gen_op_jp_m,
};

enum {
    COND_NZ,
    COND_Z,
    COND_NC,
    COND_C,
    COND_PO,
    COND_PE,
    COND_P,
    COND_M,
};

/* Arithmetic/logic operations */

char *alu[8] = {
    "add a,",
    "adc a,",
    "sub ",
    "sbc a,",
    "and ",
    "xor ",
    "or ",
    "cp ",
};

static GenOpFunc *gen_op_alu[8] = {
    gen_op_add_cc,
    gen_op_adc_cc,
    gen_op_sub_cc,
    gen_op_sbc_cc,
    gen_op_and_cc,
    gen_op_xor_cc,
    gen_op_or_cc,
    gen_op_cp_cc,
};

/* Rotation/shift operations */

char *rot[8] = {
    "rlc",
    "rrc",
    "rl",
    "rr",
    "sla",
    "sra",
    "sll",
    "srl",
};

static GenOpFunc *gen_op_rot_T0[8] = {
    gen_op_rlc_T0_cc,
    gen_op_rrc_T0_cc,
    gen_op_rl_T0_cc,
    gen_op_rr_T0_cc,
    gen_op_sla_T0_cc,
    gen_op_sra_T0_cc,
    gen_op_sll_T0_cc,
    gen_op_srl_T0_cc,
};

/* Block instructions */

char *bli[4][4] = {
    { "ldi",  "cpi",  "ini",  "outi", },
    { "ldd",  "cpd",  "ind",  "outd", },
    { "ldir", "cpir", "inir", "otir", },
    { "lddr", "cpdr", "indr", "otdr", },
};

int imode[8] = {
    0, 0, 1, 2, 0, 0, 1, 2,
};

static inline void gen_goto_tb(DisasContext *s, int tb_num, target_ulong pc)
{
    gen_jmp_im(pc);
    gen_eob(s);
}

static inline void gen_jcc(DisasContext *s, int cc,
                           target_ulong val, target_ulong next_pc) {
    TranslationBlock *tb;
    GenOpFunc1 *func;
    int l1;

    func = gen_op_jp_cc[cc];

    tb = s->tb;

    l1 = gen_new_label();
    func(l1);

    gen_goto_tb(s, 0, next_pc);

    gen_set_label(l1);
    gen_goto_tb(s, 1, val);

    s->is_jmp = 3;
}

static inline void gen_callcc(DisasContext *s, int cc,
                              target_ulong val, target_ulong next_pc) {
    TranslationBlock *tb;
    GenOpFunc1 *func;
    int l1;

    func = gen_op_jp_cc[cc];

    tb = s->tb;

    l1 = gen_new_label();
    func(l1);

    gen_goto_tb(s, 0, next_pc);

    gen_set_label(l1);
    gen_op_mov_T0_im(next_pc);
    gen_op_pushw_T0();
    gen_goto_tb(s, 1, val);

    s->is_jmp = 3;
}

static inline void gen_retcc(DisasContext *s, int cc,
                             target_ulong next_pc) {
    TranslationBlock *tb;
    GenOpFunc1 *func;
    int l1;

    func = gen_op_jp_cc[cc];

    tb = s->tb;

    l1 = gen_new_label();
    func(l1);

    gen_goto_tb(s, 0, next_pc);

    gen_set_label(l1);
    gen_op_popw_T0();
    gen_op_jmp_T0();
    gen_eob(s);

    s->is_jmp = 3;
}

/* TODO: do flags properly, using cc_table. */
/* (search for compute_all in i386 code) */
/* see also opc_read_flags[], opc_write_flags[] and opc_simpler[] */

/* micro-ops that modify condition codes should end in _cc */

/* convert one instruction. s->is_jmp is set if the translation must
   be stopped. Return the next pc value */
static target_ulong disas_insn(DisasContext *s, target_ulong pc_start)
{
    int b, prefixes;
    int rex_w, rex_r;
    int m;

    s->pc = pc_start;
    prefixes = 0;
    s->override = -1;
    rex_w = -1;
    rex_r = 0;

    //printf("PC = %04x: ", s->pc);
//    gen_op_dump_registers(s->pc);
next_byte:
    s->prefix = prefixes;

/* START */

    if (prefixes & PREFIX_DD)
        m = 1;
    else if (prefixes & PREFIX_FD)
        m = 2;
    else
        m = 0;

    /* unprefixed opcodes */

    if ((prefixes & (PREFIX_CB | PREFIX_ED)) == 0) {
        b = ldub_code(s->pc);
        s->pc++;

        int x, y, z, p, q;
        int n, d;
        int r1, r2;

        x = (b >> 6) & 0x03;
        y = (b >> 3) & 0x07;
        z = b & 0x07;
        p = y >> 1;
        q = y & 0x01;

        switch (x) {
        case 0:
            switch (z) {

            case 0:
                switch (y) {
                case 0:
                    zprintf("nop\n");
                    break;
                case 1:
                    gen_op_ex_af_afx();
                    zprintf("ex af,af'\n");
                    break;
                case 2:
                    n = ldsb_code(s->pc);
                    s->pc++;
                    gen_op_djnz(s->pc + n, s->pc);
                    gen_eob(s);
                    s->is_jmp = 3;
                    zprintf("djnz $%02x\n", n);
                    break;
                case 3:
                    n = ldsb_code(s->pc);
                    s->pc++;
                    gen_jmp_im(s->pc + n);
                    gen_eob(s);
                    s->is_jmp = 3;
                    zprintf("jr $%02x\n", n);
                    break;
                case 4:
                case 5:
                case 6:
                case 7:
                    n = ldsb_code(s->pc);
                    s->pc++;
                    zprintf("jr %s,$%04x\n", cc[y-4], (s->pc + n) & 0xffff);
                    gen_jcc(s, y-4, s->pc + n, s->pc);
                    break;
                }
                break;

            case 1:
                switch (q) {
                case 0:
                    n = lduw_code(s->pc);
                    s->pc += 2;
                    gen_op_mov_T0_im(n);
                    r1 = regpairmap(regpair[p], m);
                    gen_op_movw_reg_T0[r1]();
                    zprintf("ld %s,$%04x\n", regpairnames[r1], n);
                    break;
                case 1:
                    r1 = regpairmap(regpair[p], m);
                    r2 = regpairmap(OR2_HL, m);
                    gen_op_movw_T0_reg[r1]();
                    gen_op_movw_T1_reg[r2]();
                    gen_op_addw_T0_T1_cc();
                    gen_op_movw_reg_T0[r2]();
                    zprintf("add %s,%s\n", regpairnames[r2], regpairnames[r1]);
                    break;
                }
                break;

            case 2:
                switch (q) {
                case 0:
                    switch (p) {
                    case 0:
                        gen_op_movb_T0_reg[OR_A]();
                        gen_op_movw_A0_BC();
                        gen_op_stb_T0_A0();
                        zprintf("ld (bc),a\n");
                        break;
                    case 1:
                        gen_op_movb_T0_reg[OR_A]();
                        gen_op_movw_A0_DE();
                        gen_op_stb_T0_A0();
                        zprintf("ld (de),a\n");
                        break;
                    case 2:
                        n = lduw_code(s->pc);
                        s->pc += 2;
                        r1 = regpairmap(OR2_HL, m);
                        gen_op_movw_T0_reg[r1]();
                        gen_op_mov_A0_im(n);
                        gen_op_stw_T0_A0();
                        zprintf("ld ($%04x),%s\n", n, regpairnames[r1]);
                        break;
                    case 3:
                        n = lduw_code(s->pc);
                        s->pc += 2;
                        gen_op_movb_T0_reg[OR_A]();
                        gen_op_mov_A0_im(n);
                        gen_op_stb_T0_A0();
                        zprintf("ld ($%04x),a\n", n);
                        break;
                    }
                    break;
                case 1:
                    switch (p) {
                    case 0:
                        gen_op_movw_A0_BC();
                        gen_op_ldb_T0_A0();
                        gen_op_movb_reg_T0[OR_A]();
                        zprintf("ld a,(bc)\n");
                        break;
                    case 1:
                        gen_op_movw_A0_DE();
                        gen_op_ldb_T0_A0();
                        gen_op_movb_reg_T0[OR_A]();
                        zprintf("ld a,(de)\n");
                        break;
                    case 2:
                        n = lduw_code(s->pc);
                        s->pc += 2;
                        r1 = regpairmap(OR2_HL, m);
                        gen_op_mov_A0_im(n);
                        gen_op_ldw_T0_A0();
                        gen_op_movw_reg_T0[r1]();
                        zprintf("ld %s,($%04x)\n", regpairnames[r1], n);
                        break;
                    case 3:
                        n = lduw_code(s->pc);
                        s->pc += 2;
                        gen_op_mov_A0_im(n);
                        gen_op_ldb_T0_A0();
                        gen_op_movb_reg_T0[OR_A]();
                        zprintf("ld a,($%04x)\n", n);
                        break;
                    }
                    break;
                }
                break;

            case 3:
                switch (q) {
                case 0:
                    r1 = regpairmap(regpair[p], m);
                    gen_op_movw_T0_reg[r1]();
                    gen_op_incw_T0();
                    gen_op_movw_reg_T0[r1]();
                    zprintf("inc %s\n", regpairnames[r1]);
                    break;
                case 1:
                    r1 = regpairmap(regpair[p], m);
                    gen_op_movw_T0_reg[r1]();
                    gen_op_decw_T0();
                    gen_op_movw_reg_T0[r1]();
                    zprintf("dec %s\n", regpairnames[r1]);
                    break;
                }
                break;

            case 4:
                r1 = regmap(reg[y], m);
                if (is_indexed(r1)) {
                    d = ldsb_code(s->pc);
                    s->pc++;
                    gen_op_movb_T0_idx[r1](d);
                } else
                    gen_op_movb_T0_reg[r1]();
                gen_op_incb_T0_cc();
                if (is_indexed(r1))
                    gen_op_movb_idx_T0[r1](d);
                else
                    gen_op_movb_reg_T0[r1]();
                if (is_indexed(r1))
                    zprintf("inc (%s%c$%02x)\n", idxnames[r1], shexb(d));
                else
                    zprintf("inc %s\n", regnames[r1]);
                break;

            case 5:
                r1 = regmap(reg[y], m);
                if (is_indexed(r1)) {
                    d = ldsb_code(s->pc);
                    s->pc++;
                    gen_op_movb_T0_idx[r1](d);
                } else
                    gen_op_movb_T0_reg[r1]();
                gen_op_decb_T0_cc();
                if (is_indexed(r1))
                    gen_op_movb_idx_T0[r1](d);
                else
                    gen_op_movb_reg_T0[r1]();
                if (is_indexed(r1))
                    zprintf("dec (%s%c$%02x)\n", idxnames[r1], shexb(d));
                else
                    zprintf("dec %s\n", regnames[r1]);
                break;

            case 6:
                r1 = regmap(reg[y], m);
                if (is_indexed(r1)) {
                    d = ldsb_code(s->pc);
                    s->pc++;
                }
                n = ldub_code(s->pc);
                s->pc++;
                gen_op_mov_T0_im(n);
                if (is_indexed(r1))
                    gen_op_movb_idx_T0[r1](d);
                else
                    gen_op_movb_reg_T0[r1]();
                if (is_indexed(r1))
                    zprintf("ld (%s%c$%02x),$%02x\n", idxnames[r1], shexb(d), n);
                else
                    zprintf("ld %s,$%02x\n", regnames[r1], n);
                break;

            case 7:
                switch (y) {
                case 0:
                    gen_op_rlca_cc();
                    zprintf("rlca\n");
                    break;
                case 1:
                    gen_op_rrca_cc();
                    zprintf("rrca\n");
                    break;
                case 2:
                    gen_op_rla_cc();
                    zprintf("rla\n");
                    break;
                case 3:
                    gen_op_rra_cc();
                    zprintf("rra\n");
                    break;
                case 4:
                    gen_op_daa_cc();
                    zprintf("daa\n");
                    break;
                case 5:
                    gen_op_cpl_cc();
                    zprintf("cpl\n");
                    break;
                case 6:
                    gen_op_scf_cc();
                    zprintf("scf\n");
                    break;
                case 7:
                    gen_op_ccf_cc();
                    zprintf("ccf\n");
                    break;
                }
                break;
            }
            break;

        case 1:
            if (z == 6 && y == 6) {
                gen_jmp_im(s->pc);
                gen_op_halt();
                zprintf("halt\n");
            } else {
                if (z == 6) {
                    r1 = regmap(reg[z], m);
                    r2 = regmap(reg[y], 0);
                } else if (y == 6) {
                    r1 = regmap(reg[z], 0);
                    r2 = regmap(reg[y], m);
                } else {
                    r1 = regmap(reg[z], m);
                    r2 = regmap(reg[y], m);
                }
                if (is_indexed(r1) || is_indexed(r2)) {
                    d = ldsb_code(s->pc);
                    s->pc++;
                }
                if (is_indexed(r1))
                    gen_op_movb_T0_idx[r1](d);
                else
                    gen_op_movb_T0_reg[r1]();
                if (is_indexed(r2))
                    gen_op_movb_idx_T0[r2](d);
                else
                    gen_op_movb_reg_T0[r2]();
                if (is_indexed(r1))
                    zprintf("ld %s,(%s%c$%02x)\n", regnames[r2], idxnames[r1], shexb(d));
                else if (is_indexed(r2))
                    zprintf("ld (%s%c$%02x),%s\n", idxnames[r2], shexb(d), regnames[r1]);
                else
                    zprintf("ld %s,%s\n", regnames[r2], regnames[r1]);
            }
            break;

        case 2:
            r1 = regmap(reg[z], m);
            if (is_indexed(r1)) {
                d = ldsb_code(s->pc);
                s->pc++;
                gen_op_movb_T0_idx[r1](d);
            } else
                gen_op_movb_T0_reg[r1]();
            gen_op_alu[y](); /* places output in A */
            if (is_indexed(r1))
                zprintf("%s(%s%c$%02x)\n", alu[y], idxnames[r1], shexb(d));
            else
                zprintf("%s%s\n", alu[y], regnames[r1]);
            break;

        case 3:
            switch (z) {
            case 0:
                gen_retcc(s, y, s->pc);
                zprintf("ret %s\n", cc[y]);
                break;

            case 1:
                switch (q) {
                case 0:
                    r1 = regpairmap(regpair2[p], m);
                    gen_op_popw_T0();
                    gen_op_movw_reg_T0[r1]();
                    zprintf("pop %s\n", regpairnames[r1]);
                    break;
                case 1:
                    switch (p) {
                    case 0:
                        gen_op_popw_T0();
                        gen_op_jmp_T0();
                        zprintf("ret\n");
                        gen_eob(s);
                        s->is_jmp = 3;
//                      s->is_ei = 1;
                        break;
                    case 1:
                        gen_op_exx();
                        zprintf("exx\n");
                        break;
                    case 2:
                        r1 = regpairmap(OR2_HL, m);
                        gen_op_movw_T0_reg[r1]();
                        gen_op_jmp_T0();
                        zprintf("jp %s\n", regpairnames[r1]);
                        gen_eob(s);
                        s->is_jmp = 3;
                        break;
                    case 3:
                        r1 = regpairmap(OR2_HL, m);
                        gen_op_movw_T0_reg[r1]();
                        gen_op_movw_reg_T0[OR2_SP]();
                        zprintf("ld sp,%s\n", regpairnames[r1]);
                        gen_op_dump_registers(s->pc);
                        break;
                    }
                    break;
                }
                break;

            case 2:
                n = lduw_code(s->pc);
                s->pc += 2;
                gen_jcc(s, y, n, s->pc);
                zprintf("jp %s,$%04x\n", cc[y], n);
                break;

            case 3:
                switch (y) {
                case 0:
                    n = lduw_code(s->pc);
                    s->pc += 2;
                    gen_jmp_im(n);
                    zprintf("jp $%04x\n", n);
                    gen_eob(s);
                    s->is_jmp = 3;
                    break;
                case 1:
                    zprintf("cb prefix\n");
                    prefixes |= PREFIX_CB;
                    goto next_byte;
                    break;
                case 2:
                    n = ldub_code(s->pc);
                    s->pc++;
                    gen_op_movb_T0_reg[OR_A]();
                    gen_op_out_T0_im(n);
                    zprintf("out ($%02x),a\n", n);
                    break;
                case 3:
                    n = ldub_code(s->pc);
                    s->pc++;
                    gen_op_in_T0_im(n);
                    gen_op_movb_reg_T0[OR_A]();
                    zprintf("in a,($%02x)\n", n);
                    break;
                case 4:
                    r1 = regpairmap(OR2_HL, m);
                    gen_op_popw_T1();
                    gen_op_movw_T0_reg[r1]();
                    gen_op_pushw_T0();
                    gen_op_movw_reg_T1[r1]();
                    zprintf("ex (sp),%s\n", regpairnames[r1]);
                    break;
                case 5:
                    gen_op_ex_de_hl();
                    zprintf("ex de,hl\n");
                    break;
                case 6:
                    gen_op_di();
                    zprintf("di\n");
                    break;
                case 7:
                    gen_op_ei();
//                  gen_op_dump_registers(s->pc);
                    zprintf("ei\n");
//                  gen_eob(s);
//                  s->is_ei = 1;
                    break;
                }
                break;

            case 4:
                n = lduw_code(s->pc);
                s->pc += 2;
                gen_callcc(s, y, n, s->pc);
                zprintf("call %s,$%04x\n", cc[y], n);
                break;

            case 5:
                switch (q) {
                case 0:
                    r1 = regpairmap(regpair2[p], m);
                    gen_op_movw_T0_reg[r1]();
                    gen_op_pushw_T0();
                    zprintf("push %s\n", regpairnames[r1]);
                    break;
                case 1:
                    switch (p) {
                    case 0:
                        n = lduw_code(s->pc);
                        s->pc += 2;
                        gen_op_mov_T0_im(s->pc);
                        gen_op_pushw_T0();
                        gen_jmp_im(n);
                        zprintf("call $%04x\n", n);
                        gen_eob(s);
                        s->is_jmp = 3;
                        break;
                    case 1:
                        zprintf("dd prefix\n");
                        prefixes |= PREFIX_DD;
                        goto next_byte;
                        break;
                    case 2:
                        zprintf("ed prefix\n");
                        prefixes |= PREFIX_ED;
                        goto next_byte;
                        break;
                    case 3:
                        zprintf("fd prefix\n");
                        prefixes |= PREFIX_FD;
                        goto next_byte;
                        break;
                    }
                    break;
                }
                break;

            case 6:
                n = ldub_code(s->pc);
                s->pc++;
                gen_op_mov_T0_im(n);
                gen_op_alu[y](); /* places output in A */
                zprintf("%s$%02x\n", alu[y], n);
                break;

            case 7:
                gen_op_mov_T0_im(s->pc);
                gen_op_pushw_T0();
                gen_jmp_im(y*8);
                zprintf("rst $%02x\n", y*8);
                gen_eob(s);
                s->is_jmp = 3;
                break;
            }
            break;
        }
    } else if (prefixes & PREFIX_CB) {
        /* cb mode: */

        int x, y, z, p, q;
        int d;
        int r1, r2;

        if (m) {
            d = ldsb_code(s->pc);
            s->pc++;
        }

        b = ldub_code(s->pc);
        s->pc++;

        x = (b >> 6) & 0x03;
        y = (b >> 3) & 0x07;
        z = b & 0x07;
        p = y >> 1;
        q = y & 0x01;

        if (m) {
            r1 = regmap(OR_HLmem, m);
            gen_op_movb_T0_idx[r1](d);
            if (z != 6)
                r2 = regmap(reg[z], 0);
        } else {
            r1 = regmap(reg[z], m);
            gen_op_movb_T0_reg[r1]();
        }

        switch (x) {
        case 0:
            gen_op_rot_T0[y]();
            if (m) {
                gen_op_movb_idx_T0[r1](d);
                if (z != 6)
                    gen_op_movb_reg_T0[r2]();
            } else {
                gen_op_movb_reg_T0[r1]();
            }
            zprintf("%s %s\n", rot[y], regnames[r1]);
            break;
        case 1:
            gen_op_bit_T0(1 << y);
            zprintf("bit %i,%s\n", y, regnames[r1]);
            break;
        case 2:
            gen_op_res_T0(~(1 << y));
            if (m) {
                gen_op_movb_idx_T0[r1](d);
                if (z != 6)
                    gen_op_movb_reg_T0[r2]();
            } else {
                gen_op_movb_reg_T0[r1]();
            }
            zprintf("res %i,%s\n", y, regnames[r1]);
            break;
        case 3:
            gen_op_set_T0(1 << y);
            if (m) {
                gen_op_movb_idx_T0[r1](d);
                if (z != 6)
                    gen_op_movb_reg_T0[r2]();
            } else {
                gen_op_movb_reg_T0[r1]();
            }
            zprintf("set %i,%s\n", y, regnames[r1]);
            break;
        }

    } else if (prefixes & PREFIX_ED) {
        /* ed mode: */

        b = ldub_code(s->pc);
        s->pc++;

        int x, y, z, p, q;
        int n;
        int r1, r2;

        x = (b >> 6) & 0x03;
        y = (b >> 3) & 0x07;
        z = b & 0x07;
        p = y >> 1;
        q = y & 0x01;

        switch (x) {
        case 0:
        case 3:
            zprintf("nop\n");
            break;
      
        case 1:
            switch (z) {
            case 0:
                gen_op_in_T0_bc_cc();
                if (y != 6) {
                    r1 = regmap(reg[y], m);
                    gen_op_movb_reg_T0[r1]();
                    zprintf("in %s,(c)\n", regnames[r1]);
                } else {
                    zprintf("in (c)\n");
                }
                break;
            case 1:
                if (y != 6) {
                    r1 = regmap(reg[y], m);
                    gen_op_movb_T0_reg[r1]();
                    zprintf("out (c),%s\n", regnames[r1]);
                } else {
                    gen_op_mov_T0_im(0);
                    zprintf("out (c),0\n");
                }
                gen_op_out_T0_bc();
                break;
            case 2:
                r1 = regpairmap(OR2_HL, m);
                r2 = regpairmap(regpair[p], m);
                gen_op_movw_T0_reg[r1]();
                gen_op_movw_T1_reg[r2]();
                if (q == 0) {
                    zprintf("sbc %s,%s\n", regpairnames[r1], regpairnames[r2]);
                    gen_op_sbcw_T0_T1_cc();
                } else {
                    zprintf("adc %s,%s\n", regpairnames[r1], regpairnames[r2]);
                    gen_op_adcw_T0_T1_cc();
                }
                gen_op_movw_reg_T0[r1]();
                break;
            case 3:
                n = lduw_code(s->pc);
                s->pc += 2;
                r1 = regpairmap(regpair[p], m);
                if (q == 0) {
                    gen_op_movw_T0_reg[r1]();
                    gen_op_mov_A0_im(n);
                    gen_op_stw_T0_A0();
                    zprintf("ld ($%02x),%s\n", n, regpairnames[r1]);
                } else {
                    gen_op_mov_A0_im(n);
                    gen_op_ldw_T0_A0();
                    gen_op_movw_reg_T0[r1]();
                    zprintf("ld %s,($%02x)\n", regpairnames[r1], n);
                }
                break;
            case 4:
                zprintf("neg\n");
                gen_op_neg_cc();
                break;
            case 5:
                /* FIXME */
                gen_op_popw_T0();
                gen_op_jmp_T0();
                gen_op_ri();
                if (q == 0) {
                    zprintf("retn\n");
                } else {
                    zprintf("reti\n");
                }
                gen_eob(s);
                s->is_jmp = 3;
//              s->is_ei = 1;
                break;
            case 6:
                gen_op_imode(imode[y]);
                zprintf("im im[%i]\n", imode[y]);
//              gen_eob(s);
//              s->is_ei = 1;
                break;
            case 7:
                switch (y) {
                case 0:
                    gen_op_ld_I_A();
                    zprintf("ld i,a\n");
                    break;
                case 1:
                    gen_op_ld_R_A();
                    zprintf("ld r,a\n");
                    break;
                case 2:
                    gen_op_ld_A_I();
                    zprintf("ld a,i\n");
                    break;
                case 3:
                    gen_op_ld_A_R();
                    zprintf("ld a,r\n");
                    break;
                case 4:
                    gen_op_movb_T0_reg[OR_HLmem]();
                    gen_op_rrd_cc();
                    gen_op_movb_reg_T0[OR_HLmem]();
                    zprintf("rrd\n");
                    break;
                case 5:
                    gen_op_movb_T0_reg[OR_HLmem]();
                    gen_op_rld_cc();
                    gen_op_movb_reg_T0[OR_HLmem]();
                    zprintf("rld\n");
                    break;
                case 6:
                case 7:
                    zprintf("nop2\n");
                    /* nop */
                    break;
                }
                break;
            }
            break;

        case 2:
            /* FIXME */
            if (y >= 4) {
                switch (z) {
                case 0: /* ldi/ldd/ldir/lddr */
                    gen_op_movw_A0_HL();
                    gen_op_ldb_T0_A0();
                    gen_op_movw_A0_DE();
                    gen_op_stb_T0_A0();

                    if (!(y & 1))
                        gen_op_bli_ld_inc_cc();
                    else
                        gen_op_bli_ld_dec_cc();
                    if ((y & 2)) {
                        gen_op_bli_ld_rep(s->pc);
                        gen_eob(s);
                        s->is_jmp = 3;
                    }
                    break;

                case 1: /* cpi/cpd/cpir/cpdr */
                    gen_op_movw_A0_HL();
                    gen_op_ldb_T0_A0();
                    gen_op_bli_cp_cc();

                    if (!(y & 1))
                        gen_op_bli_cp_inc_cc();
                    else
                        gen_op_bli_cp_dec_cc();
                    if ((y & 2)) {
                        gen_op_bli_cp_rep(s->pc);
                        gen_eob(s);
                        s->is_jmp = 3;
                    }
                    break;

                case 2: /* ini/ind/inir/indr */
                    gen_op_in_T0_bc_cc();
                    gen_op_movw_A0_HL();
                    gen_op_stb_T0_A0();
                    if (!(y & 1))
                        gen_op_bli_io_inc();
                    else
                        gen_op_bli_io_dec();
                    if ((y & 2)) {
                        gen_op_bli_io_rep(s->pc);
                        gen_eob(s);
                        s->is_jmp = 3;
                    }
                    break;

                case 3: /* outi/outd/otir/otdr */
                    gen_op_movw_A0_HL();
                    gen_op_ldb_T0_A0();
                    gen_op_out_T0_bc();
                    if (!(y & 1))
                        gen_op_bli_io_inc();
                    else
                        gen_op_bli_io_dec();
                    if ((y & 2)) {
                        gen_op_bli_io_rep(s->pc);
                        gen_eob(s);
                        s->is_jmp = 3;
                    }
                    break;
                }

                zprintf("%s\n", bli[y-4][z]);
                break;
            }
        }
    }

    prefixes = 0;

    /* now check op code */
//    switch (b) {
//    default:
//        goto illegal_op;
//    }
    /* lock generation */
    return s->pc;
 illegal_op:
    /* XXX: ensure that no lock was generated */
    gen_exception(s, EXCP06_ILLOP, pc_start - s->cs_base);
    return s->pc;
}

#define CC_SZHPNC (CC_S | CC_Z | CC_H | CC_P | CC_N | CC_C)
#define CC_SZHPN (CC_S | CC_Z | CC_H | CC_P | CC_N)

static uint8_t opc_read_flags[NB_OPS] = {
    [INDEX_op_jp_nz] = CC_SZHPNC,
    [INDEX_op_jp_z] = CC_SZHPNC,
    [INDEX_op_jp_nc] = CC_SZHPNC,
    [INDEX_op_jp_c] = CC_SZHPNC,
    [INDEX_op_jp_po] = CC_SZHPNC,
    [INDEX_op_jp_pe] = CC_SZHPNC,
    [INDEX_op_jp_p] = CC_SZHPNC,
    [INDEX_op_jp_m] = CC_SZHPNC,
//...
};

static uint8_t opc_write_flags[NB_OPS] = {
    [INDEX_op_add_cc] = CC_SZHPNC,
    [INDEX_op_adc_cc] = CC_SZHPNC,
    [INDEX_op_sub_cc] = CC_SZHPNC,
    [INDEX_op_sbc_cc] = CC_SZHPNC,
    [INDEX_op_and_cc] = CC_SZHPNC,
    [INDEX_op_xor_cc] = CC_SZHPNC,
    [INDEX_op_or_cc] = CC_SZHPNC,
    [INDEX_op_cp_cc] = CC_SZHPNC,
//...
};

static uint16_t opc_simpler[NB_OPS] = {
#if 0
    [INDEX_op_add_cc] = INDEX_op_add,
    [INDEX_op_adc_cc] = INDEX_op_adc,
    [INDEX_op_sub_cc] = INDEX_op_sub,
    [INDEX_op_sbc_cc] = INDEX_op_sbc,
    [INDEX_op_and_cc] = INDEX_op_and,
    [INDEX_op_xor_cc] = INDEX_op_xor,
    [INDEX_op_or_cc] = INDEX_op_or,
    [INDEX_op_cp_cc] = INDEX_op_cp,
//...
#endif
};

void optimize_flags_init(void)
{
    int i;
    /* put default values in arrays */
    for(i = 0; i < NB_OPS; i++) {
        if (opc_simpler[i] == 0)
            opc_simpler[i] = i;
    }
}

/* CPU flags computation optimization: we move backward thru the
   generated code to see which flags are needed. The operation is
   modified if suitable */
static void optimize_flags(uint16_t *opc_buf, int opc_buf_len)
{
}

/* generate intermediate code in gen_opc_buf and gen_opparam_buf for
   basic block 'tb'. If search_pc is TRUE, also generate PC
   information for each intermediate instruction. */
static inline int gen_intermediate_code_internal(CPUState *env,
                                                 TranslationBlock *tb, 
                                                 int search_pc)
{
    DisasContext dc1, *dc = &dc1;
    target_ulong pc_ptr;
    uint16_t *gen_opc_end;
    int flags, j, lj, cflags;
    target_ulong pc_start;
    target_ulong cs_base;
    
    /* generate intermediate code */
    pc_start = tb->pc;
    cs_base = tb->cs_base;
    flags = tb->flags;
    cflags = tb->cflags;

    dc->singlestep_enabled = env->singlestep_enabled;
    dc->cc_op = CC_OP_DYNAMIC;
    dc->cs_base = cs_base;
    dc->tb = tb;
    dc->flags = flags;
    dc->jmp_opt = !(env->singlestep_enabled ||
                    (flags & HF_INHIBIT_IRQ_MASK)
#ifndef CONFIG_SOFTMMU
                    || (flags & HF_SOFTMMU_MASK)
#endif
                    );

    gen_opc_ptr = gen_opc_buf;
    gen_opc_end = gen_opc_buf + OPC_MAX_SIZE;
    gen_opparam_ptr = gen_opparam_buf;

    dc->is_jmp = DISAS_NEXT;
    pc_ptr = pc_start;
    lj = -1;

    for(;;) {
        if (env->nb_breakpoints > 0) {
            for(j = 0; j < env->nb_breakpoints; j++) {
                if (env->breakpoints[j] == pc_ptr) {
                    gen_debug(dc, pc_ptr - dc->cs_base);
                    break;
                }
            }
        }
        if (search_pc) {
            j = gen_opc_ptr - gen_opc_buf;
            if (lj < j) {
                lj++;
                while (lj < j)
                    gen_opc_instr_start[lj++] = 0;
            }
            gen_opc_pc[lj] = pc_ptr;
//            gen_opc_cc_op[lj] = dc->cc_op;
            gen_opc_instr_start[lj] = 1;
        }
        pc_ptr = disas_insn(dc, pc_ptr);
        /* stop translation if indicated */
        if (dc->is_jmp)
            break;
        /* if single step mode, we generate only one instruction and
           generate an exception */
        /* if irq were inhibited with HF_INHIBIT_IRQ_MASK, we clear
           the flag and abort the translation to give the irqs a
           change to be happen */
        if (dc->singlestep_enabled || 
            (flags & HF_INHIBIT_IRQ_MASK) ||
            (cflags & CF_SINGLE_INSN)) {
            gen_jmp_im(pc_ptr - dc->cs_base);
            gen_eob(dc);
            break;
        }
        /* if too long translation, stop generation too */
        if (gen_opc_ptr >= gen_opc_end ||
            (pc_ptr - pc_start) >= (TARGET_PAGE_SIZE - 32)) {
            gen_jmp_im(pc_ptr - dc->cs_base);
            gen_eob(dc);
            break;
        }
    }
    *gen_opc_ptr = INDEX_op_end;
    /* we don't forget to fill the last values */
    if (search_pc) {
        j = gen_opc_ptr - gen_opc_buf;
        lj++;
        while (lj <= j)
            gen_opc_instr_start[lj++] = 0;
    }
        
#ifdef DEBUG_DISAS
    if (loglevel & CPU_LOG_TB_CPU) {
        cpu_dump_state(env, logfile, fprintf, 0);
    }
    if (loglevel & CPU_LOG_TB_IN_ASM) {
        int disas_flags;
        fprintf(logfile, "----------------\n");
        fprintf(logfile, "IN: %s\n", lookup_symbol(pc_start));
        target_disas(logfile, pc_start, pc_ptr - pc_start, disas_flags);
        fprintf(logfile, "\n");
        if (loglevel & CPU_LOG_TB_OP_OPT) {
            fprintf(logfile, "OP before opt:\n");
            tcg_dump_ops(&tcg_ctx, logfile);
            fprintf(logfile, "\n");
        }
    }
#endif

    /* optimize flag computations */
    optimize_flags(gen_opc_buf, gen_opc_ptr - gen_opc_buf);

    if (!search_pc)
        tb->size = pc_ptr - pc_start;
    return 0;
}

int gen_intermediate_code(CPUState *env, TranslationBlock *tb)
{
    return gen_intermediate_code_internal(env, tb, 0);
}

int gen_intermediate_code_pc(CPUState *env, TranslationBlock *tb)
{
    return gen_intermediate_code_internal(env, tb, 1);
}

void gen_pc_load(CPUState *env, TranslationBlock *tb,
		 unsigned long searched_pc, int pc_pos, void *puc)
{
    env->pc = gen_opc_pc[pc_pos];
}
