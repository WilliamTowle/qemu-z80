/*
 * QEmu Z80 CPU - instruction translation
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting by William Towle 2018-2022
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */


#include "qemu/osdep.h"
#include "cpu.h"

#include "qemu/error-report.h"
#include "exec/exec-all.h"
#include "exec/cpu_ldst.h"
#include "exec/translator.h"
#include "tcg/tcg-op.h"

#include "exec/gen-icount.h"
#ifdef CONFIG_USER_ONLY
#include "qemu.h"       /* bblbrx wants TaskState struct */
#endif


#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 translate: " fmt , ## __VA_ARGS__); } while(0)


#define EMIT_INSNS 1
#define zprintf(fmt, ...) \
    do { if (EMIT_INSNS) printf(fmt , ## __VA_ARGS__); } while(0)


/* Prefixes and parsing mode defines */

#define PREFIX_CB   0x01
#define PREFIX_DD   0x02
#define PREFIX_ED   0x04
#define PREFIX_FD   0x08

#define MODE_NORMAL 0
#define MODE_DD     1
#define MODE_FD     2


/* global register indexes */
static TCGv cpu_A0;
#if 0   /* overkill? feature unused for z80 */
static TCGv_i32 cpu_cc_op;
#endif
/* local temps */
static TCGv cpu_T[3];           /* n=2, n=3 unused? */


#define MEM_INDEX 0     /* MMU_USER_IDX? */

typedef struct DisasContext {
    DisasContextBase base;
    /* [WmT] repo.or.cz does not implement:
     *  dc->cc_{op|op_dirty}
     *  dc->tf ("TF cpu flag") [but HF_INHIBIT_IRQ is present]
     */

    /* current insn context */
    int                 prefix;
    target_ulong pc;
    //int model;

    /* current block context */
#if 0   /* overkill? feature unused for z80 */
    CCOp cc_op;  /* current CC operation */
    bool cc_op_dirty;
#endif
    uint32_t flags; /* all execution flags */
#ifdef CONFIG_USER_ONLY
    target_ulong    magic_ramloc;
#endif
} DisasContext;


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
    OR_IYmem
};

static const char *regnames[]= {
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

static const char *const idxnames[]= {
    [OR_IXmem] = "ix",
    [OR_IYmem] = "iy",
};


/* signed hex byte value for printf */
#define shexb(val) (val < 0 ? '-' : '+'), (abs(val))


/* Register accessor functions */

#if defined(WORDS_BIGENDIAN)
#define UNIT_OFFSET(type, units, num) (sizeof(type) - ((num + 1) * units))
#else
#define UNIT_OFFSET(type, units, num) (num * units)
#endif

#define BYTE_OFFSET(type, num) UNIT_OFFSET(type, 1, num)
#define WORD_OFFSET(type, num) UNIT_OFFSET(type, 2, num)

#define REGPAIR AF
#define REGHIGH A
#define REGLOW  F
#include "genreg_template_af.h"
#undef REGPAIR
#undef REGHIGH
#undef REGLOW

#define REGPAIR BC
#define REGHIGH B
#define REGLOW  C
#include "genreg_template.h"
#undef REGPAIR
#undef REGHIGH
#undef REGLOW

#define REGPAIR DE
#define REGHIGH D
#define REGLOW  E
#include "genreg_template.h"
#undef REGPAIR
#undef REGHIGH
#undef REGLOW

#define REGPAIR HL
#define REGHIGH H
#define REGLOW  L
#include "genreg_template.h"
#undef REGPAIR
#undef REGHIGH
#undef REGLOW

#define REGPAIR IX
#define REGHIGH IXh
#define REGLOW  IXl
#include "genreg_template.h"
#undef REGPAIR
#undef REGHIGH
#undef REGLOW

#define REGPAIR IY
#define REGHIGH IYh
#define REGLOW  IYl
#include "genreg_template.h"
#undef REGPAIR
#undef REGHIGH
#undef REGLOW

#define REGPAIR AFX
#define REGHIGH AX
#define REGLOW  FX
#include "genreg_template_af.h"
#undef REGPAIR
#undef REGHIGH
#undef REGLOW

#define REGPAIR BCX
#define REGHIGH BX
#define REGLOW  CX
#include "genreg_template.h"
#undef REGPAIR
#undef REGHIGH
#undef REGLOW

#define REGPAIR DEX
#define REGHIGH DX
#define REGLOW  EX
#include "genreg_template.h"
#undef REGPAIR
#undef REGHIGH
#undef REGLOW

#define REGPAIR HLX
#define REGHIGH HX
#define REGLOW  LX
#include "genreg_template.h"
#undef REGPAIR
#undef REGHIGH
#undef REGLOW

#define REGPAIR SP
#include "genreg_template.h"
#undef REGPAIR


#if 0   /* overkill: feature unused for z80 */
static void gen_update_cc_op(DisasContext *s)
{
    if (s->cc_op_dirty) {
        tcg_gen_movi_i32(cpu_cc_op, s->cc_op);
        s->cc_op_dirty = false;
    }
}
#endif


typedef void (gen_mov_func)(TCGv v);
typedef void (gen_mov_func_idx)(TCGv v, uint16_t ofs);


static inline void gen_movb_v_HLmem(TCGv v)
{
    TCGv addr = tcg_temp_new();
    gen_movw_v_HL(addr);
    tcg_gen_qemu_ld8u(v, addr, MEM_INDEX);
    tcg_temp_free(addr);
}

static inline void gen_movb_HLmem_v(TCGv v)
{
    TCGv addr = tcg_temp_new();
    gen_movw_v_HL(addr);
    tcg_gen_qemu_st8(v, addr, MEM_INDEX);
    tcg_temp_free(addr);
}

static inline void gen_movb_v_IXmem(TCGv v, uint16_t ofs)
{
    TCGv addr = tcg_temp_new();
    gen_movw_v_IX(addr);
    tcg_gen_addi_tl(addr, addr, ofs);
    tcg_gen_ext16u_tl(addr, addr);
    tcg_gen_qemu_ld8u(v, addr, MEM_INDEX);
    tcg_temp_free(addr);
}

static inline void gen_movb_v_IYmem(TCGv v, uint16_t ofs)
{
    TCGv addr = tcg_temp_new();
    gen_movw_v_IY(addr);
    tcg_gen_addi_tl(addr, addr, ofs);
    tcg_gen_ext16u_tl(addr, addr);
    tcg_gen_qemu_ld8u(v, addr, MEM_INDEX);
    tcg_temp_free(addr);
}

static inline void gen_movb_IXmem_v(TCGv v, uint16_t ofs)
{
    TCGv addr = tcg_temp_new();
    gen_movw_v_IX(addr);
    tcg_gen_addi_tl(addr, addr, ofs);
    tcg_gen_ext16u_tl(addr, addr);
    tcg_gen_qemu_st8(v, addr, MEM_INDEX);
    tcg_temp_free(addr);
}

static inline void gen_movb_IYmem_v(TCGv v, uint16_t ofs)
{
    TCGv addr = tcg_temp_new();
    gen_movw_v_IY(addr);
    tcg_gen_addi_tl(addr, addr, ofs);
    tcg_gen_ext16u_tl(addr, addr);
    tcg_gen_qemu_st8(v, addr, MEM_INDEX);
    tcg_temp_free(addr);
}


static inline void gen_pushw(TCGv v)
{
    TCGv addr = tcg_temp_new();
    gen_movw_v_SP(addr);
    tcg_gen_subi_i32(addr, addr, 2);
    tcg_gen_ext16u_i32(addr, addr);
    gen_movw_SP_v(addr);
    tcg_gen_qemu_st16(v, addr, MEM_INDEX);
    tcg_temp_free(addr);
}

static inline void gen_popw(TCGv v)
{
    TCGv addr = tcg_temp_new();
    gen_movw_v_SP(addr);
    tcg_gen_qemu_ld16u(v, addr, MEM_INDEX);
    tcg_gen_addi_i32(addr, addr, 2);
    tcg_gen_ext16u_i32(addr, addr);
    gen_movw_SP_v(addr);
    tcg_temp_free(addr);
}

static gen_mov_func *const gen_movb_v_reg_tbl[] = {
    [OR_B]     = gen_movb_v_B,
    [OR_C]     = gen_movb_v_C,
    [OR_D]     = gen_movb_v_D,
    [OR_E]     = gen_movb_v_E,
    [OR_H]     = gen_movb_v_H,
    [OR_L]     = gen_movb_v_L,
    [OR_HLmem] = gen_movb_v_HLmem,
    [OR_A]     = gen_movb_v_A,

    [OR_IXh]   = gen_movb_v_IXh,
    [OR_IXl]   = gen_movb_v_IXl,

    [OR_IYh]   = gen_movb_v_IYh,
    [OR_IYl]   = gen_movb_v_IYl,
};

static inline void gen_movb_v_reg(TCGv v, int reg)
{
    gen_movb_v_reg_tbl[reg](v);
}

static gen_mov_func_idx *const gen_movb_v_idx_tbl[] = {
    [OR_IXmem] = gen_movb_v_IXmem,
    [OR_IYmem] = gen_movb_v_IYmem,
};

static inline void gen_movb_v_idx(TCGv v, int idx, int ofs)
{
    gen_movb_v_idx_tbl[idx](v, ofs);
}

static gen_mov_func *const gen_movb_reg_v_tbl[] = {
    [OR_B]     = gen_movb_B_v,
    [OR_C]     = gen_movb_C_v,
    [OR_D]     = gen_movb_D_v,
    [OR_E]     = gen_movb_E_v,
    [OR_H]     = gen_movb_H_v,
    [OR_L]     = gen_movb_L_v,
    [OR_HLmem] = gen_movb_HLmem_v,
    [OR_A]     = gen_movb_A_v,

    [OR_IXh]   = gen_movb_IXh_v,
    [OR_IXl]   = gen_movb_IXl_v,

    [OR_IYh]   = gen_movb_IYh_v,
    [OR_IYl]   = gen_movb_IYl_v,
};

static inline void gen_movb_reg_v(int reg, TCGv v)
{
    gen_movb_reg_v_tbl[reg](v);
}

static gen_mov_func_idx *const gen_movb_idx_v_tbl[] = {
    [OR_IXmem] = gen_movb_IXmem_v,
    [OR_IYmem] = gen_movb_IYmem_v,
};

static inline void gen_movb_idx_v(int idx, TCGv v, int ofs)
{
    gen_movb_idx_v_tbl[idx](v, ofs);
}

static inline int regmap(int reg, int m)
{
    switch (m) {
    case MODE_DD:
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
    case MODE_FD:
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
    case MODE_NORMAL:
    default:
        return reg;
    }
}

static inline int is_indexed(int reg)
{
    if (reg == OR_IXmem || reg == OR_IYmem) {
        return 1;
    } else {
        return 0;
    }
}

static const int reg[8] = {
    OR_B,
    OR_C,
    OR_D,
    OR_E,
    OR_H,
    OR_L,
    OR_HLmem,
    OR_A
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

static const char *const regpairnames[]= {
    [OR2_AF]    = "af",
    [OR2_BC]    = "bc",
    [OR2_DE]    = "de",
    [OR2_HL]    = "hl",

    [OR2_IX]    = "ix",
    [OR2_IY]    = "iy",
    [OR2_SP]    = "sp",

    [OR2_AFX]   = "afx",
    [OR2_BCX]   = "bcx",
    [OR2_DEX]   = "dex",
    [OR2_HLX]   = "hlx"
};


static gen_mov_func *const gen_movw_v_reg_tbl[]= {
    [OR2_AF]  = gen_movw_v_AF,
    [OR2_BC]  = gen_movw_v_BC,
    [OR2_DE]  = gen_movw_v_DE,
    [OR2_HL]  = gen_movw_v_HL,

    [OR2_IX]  = gen_movw_v_IX,
    [OR2_IY]  = gen_movw_v_IY,
    [OR2_SP]  = gen_movw_v_SP,

    [OR2_AFX] = gen_movw_v_AFX,
    [OR2_BCX] = gen_movw_v_BCX,
    [OR2_DEX] = gen_movw_v_DEX,
    [OR2_HLX] = gen_movw_v_HLX,
};

static inline void gen_movw_v_reg(TCGv v, int regpair)
{
    gen_movw_v_reg_tbl[regpair](v);
}


static gen_mov_func *const gen_movw_reg_v_tbl[]= {
    [OR2_AF]    = gen_movw_AF_v,
    [OR2_BC]    = gen_movw_BC_v,
    [OR2_DE]    = gen_movw_DE_v,
    [OR2_HL]    = gen_movw_HL_v,

    [OR2_IX]    = gen_movw_IX_v,
    [OR2_IY]    = gen_movw_IY_v,
    [OR2_SP]    = gen_movw_SP_v,

    [OR2_AFX]   = gen_movw_AFX_v,
    [OR2_BCX]   = gen_movw_BCX_v,
    [OR2_DEX]   = gen_movw_DEX_v,
    [OR2_HLX]   = gen_movw_HLX_v
};

static inline void gen_movw_reg_v(int regpair, TCGv v)
{
    gen_movw_reg_v_tbl[regpair](v);
}


static inline int regpairmap(int regpair, int m)
{
    switch (regpair) {
    case OR2_HL:
        switch (m) {
        case MODE_DD:
            return OR2_IX;
        case MODE_FD:
            return OR2_IY;
        case MODE_NORMAL:
        default:
            return OR2_HL;
        }
    default:
        return regpair;
    }
}

static const int regpair[4]= {
    OR2_BC,
    OR2_DE,
    OR2_HL,
    OR2_SP,
};

static const int regpair2[4]= {
    OR2_BC,
    OR2_DE,
    OR2_HL,
    OR2_AF,
};


static inline void gen_jmp_im(target_ulong pc)
{
    gen_helper_movl_pc_im(cpu_env, tcg_const_tl(pc));
}


/* Conditions */

static const char *const cc[8] = {
    "nz",
    "z",
    "nc",
    "c",
    "po",
    "pe",
    "p",
    "m",
};


//enum {
//    COND_NZ = 0,
//    COND_Z,
//    COND_NC,
//    COND_C,
//    COND_PO,
//    COND_PE,
//    COND_P,
//    COND_M,
//};


static const int cc_flags[4] = {
    CC_Z,
    CC_C,
    CC_P,
    CC_S,
};


/* Arithmetic/logic operations */

static const char *const alu[8]= {
    "add a,",
    "adc a,",
    "sub ",
    "sbc a,",
    "and ",
    "xor ",
    "or ",
    "cp ",
};


typedef void (alu_helper_func)(TCGv_ptr cpu_env);

static alu_helper_func *const gen_alu[8] = {
    gen_helper_add_cc,
    gen_helper_adc_cc,
    gen_helper_sub_cc,
    gen_helper_sbc_cc,
    gen_helper_and_cc,
    gen_helper_xor_cc,
    gen_helper_or_cc,
    gen_helper_cp_cc,
};


/* Rotation/shift operations */

static const char *const rot[8] = {
    "rlc",
    "rrc",
    "rl",
    "rr",
    "sla",
    "sra",
    "sll",
    "srl",
};


typedef void (rot_helper_func)(TCGv_ptr cpu_env);

static rot_helper_func *const gen_rot_T0[8] = {
    gen_helper_rlc_T0_cc,
    gen_helper_rrc_T0_cc,
    gen_helper_rl_T0_cc,
    gen_helper_rr_T0_cc,
    gen_helper_sla_T0_cc,
    gen_helper_sra_T0_cc,
    gen_helper_sll_T0_cc,
    gen_helper_srl_T0_cc,
};


/* Block instructions */

static const char *const bli[4][4] = {
    { "ldi",  "cpi",  "ini",  "outi", },
    { "ldd",  "cpd",  "ind",  "outd", },
    { "ldir", "cpir", "inir", "otir", },
    { "lddr", "cpdr", "indr", "otdr", },
};

static const int imode[8] = {
    0, 0, 1, 2, 0, 0, 1, 2,
};


static void gen_eob(DisasContext *s);

static inline void gen_goto_tb(DisasContext *s, int tb_num, target_ulong pc)
{
    gen_jmp_im(pc);
    gen_eob(s);
}

static inline void gen_cond_jump(int cc, TCGLabel *l1)
{
    gen_movb_v_F(cpu_T[0]);

    tcg_gen_andi_tl(cpu_T[0], cpu_T[0], cc_flags[cc >> 1]);

    tcg_gen_brcondi_tl((cc & 1) ? TCG_COND_NE : TCG_COND_EQ, cpu_T[0], 0, l1);
}

static inline void gen_jcc(DisasContext *s, int cc,
                                target_ulong val, target_ulong next_pc)
{
    //TranslationBlock *tb;     /* as repo.or.cz: "set but unused" */
    TCGLabel *l1;

    //tb = s->tb;
    l1 = gen_new_label();

    gen_cond_jump(cc, l1);
    gen_goto_tb(s, 0, next_pc);

    gen_set_label(l1);
    gen_goto_tb(s, 1, val);

    s->base.is_jmp = DISAS_NORETURN;
}

static inline void gen_callcc(DisasContext *s, int cc,
                                      target_ulong val, target_ulong next_pc)
{
    //TranslationBlock *tb;     /* as repo.or.cz: "set but unused" */
    TCGLabel *l1;

    //tb = s->tb;
    l1 = gen_new_label();

    gen_cond_jump(cc, l1);
    gen_goto_tb(s, 0, next_pc);

    gen_set_label(l1);
    tcg_gen_movi_tl(cpu_T[0], next_pc);
    gen_pushw(cpu_T[0]);
    gen_goto_tb(s, 1, val);
    s->base.is_jmp = DISAS_NORETURN;
}

static inline void gen_retcc(DisasContext *s, int cc,
                                     target_ulong next_pc)
{
    //TranslationBlock *tb;     /* as repo.or.cz: "set but unused" */
    TCGLabel *l1;

    //tb = s->tb;
    l1 = gen_new_label();

    gen_cond_jump(cc, l1);
    gen_goto_tb(s, 0, next_pc);
    gen_set_label(l1);
    gen_popw(cpu_T[0]);
    gen_helper_jmp_T0(cpu_env);
    gen_eob(s);
    s->base.is_jmp = DISAS_NORETURN;
}

static inline void gen_ex(int regpair1, int regpair2)
{
    TCGv tmp1 = tcg_temp_new();
    TCGv tmp2 = tcg_temp_new();

    gen_movw_v_reg(tmp1, regpair1);
    gen_movw_v_reg(tmp2, regpair2);
    gen_movw_reg_v(regpair2, tmp1);
    gen_movw_reg_v(regpair1, tmp2);
    tcg_temp_free(tmp1);
    tcg_temp_free(tmp2);
}


/* Generate an end of block. Trace exception is also generated if needed.
   If INHIBIT, set HF_INHIBIT_IRQ_MASK if it isn't already set.
   If RECHECK_TF, emit a rechecking helper for #DB, ignoring the state of
   S->TF.  This is used by the syscall/sysret insns.  */
static void
do_gen_eob_worker(DisasContext *s, bool inhibit, bool recheck_tf, bool jr)
{
#if 1   /* WmT - PARTIAL */
;DPRINTF("DEBUG: Reached PARTIAL %s() with s=%p, inhibit %s, recheck_tf %s, jr %s\n", __func__, s, inhibit?"y":"n", recheck_tf?"y":"n", jr?"y":"n");
#endif
#if 0   /* overkill? feature unused for z80 */
    gen_update_cc_op(s);
#endif

    /* TODO: how i386-specific is this block? */
    /* If several instructions disable interrupts, only the first does it.  */
    if (inhibit && !(s->flags & HF_INHIBIT_IRQ_MASK)) {
#if 1   /* WmT - TRACE */
;DPRINTF("*** DEBUG: %s() missing gen_set_hflag()? ***\n", __func__);
;exit(1);
#endif
        //gen_set_hflag(s, HF_INHIBIT_IRQ_MASK);
    } else {
#if 1   /* WmT - TRACE */
;DPRINTF("*** DEBUG: %s() missing gen_reset_hflag()? ***\n", __func__);
#endif
        //gen_reset_hflag(s, HF_INHIBIT_IRQ_MASK);
    }
//    if (s->base.tb->flags & HF_RF_MASK) {
//#if 1   /* WmT - TRACE */
//;DPRINTF("*** DEBUG: %s() missing gen_helper_reset_rf()? ***\n", __func__);
//;exit(1);   /* only i386? */
//#endif
//        //gen_helper_reset_rf(cpu_env);
//    }
    if (s->base.singlestep_enabled) {
        gen_helper_debug(cpu_env);
    } else if (recheck_tf) {
#if 1   /* WmT - TRACE */
;DPRINTF("*** DEBUG: %s() recheck_tf true - missing gen_helper_rechecking_single_step()? ***\n", __func__);
;exit(1);   /* no 'tf' at repo.or.cz */
#endif
        //gen_helper_rechecking_single_step(cpu_env);
        tcg_gen_exit_tb(0);
//    } else if (s->tf) {
//#if 1   /* WmT - TRACE */
//;DPRINTF("*** DEBUG: %s() missing gen_helper_rechecking_single_step()? ***\n", __func__);
//;exit(1);
//#endif
//        gen_helper_single_step(cpu_env);
    } else if (jr) {
#if 1   /* WmT - TRACE */
;DPRINTF("*** DEBUG: %s() jr=true unexpected? ***\n", __func__);
;exit(1);
#endif
        tcg_gen_lookup_and_goto_ptr();
    } else {
        //tcg_gen_exit_tb(NULL, 0);
        tcg_gen_exit_tb(0);
    }
    s->base.is_jmp = DISAS_NORETURN;
}

static inline void
gen_eob_worker(DisasContext *s, bool inhibit, bool recheck_tf)
{
    do_gen_eob_worker(s, inhibit, recheck_tf, false);
}

/* End of block, resetting the inhibit irq flag.  */
static void gen_eob(DisasContext *s)
{
    gen_eob_worker(s, false, false);
}


static void gen_exception(DisasContext *s, int trapno, target_ulong cur_pc)
{
#if 1   /* WmT - PARTIAL */
;DPRINTF("DEBUG: Reached %s() ** PARTIAL **\n", __func__);
#endif
#if 0   /* overkill: feature unused for z80 */
    gen_update_cc_op(s);
#endif
    gen_jmp_im(cur_pc);
    gen_helper_raise_exception(cpu_env, tcg_const_i32(trapno));
    s->base.is_jmp = DISAS_NORETURN;
}

/* Called by gen_unknown_opcode() only. For Z80, we don't have the
 * concept of modes in which instructions are disallowed
 */
static void gen_illegal_opcode(DisasContext *s)
{
#if 1   /* WmT - TRACE */
//;DPRINTF("DEBUG: %s() deferring to gen_exception(); passing pc_start 0x%04x less cs_base 0x%04x\n", __func__, s->pc_start, s->cs_base);
;DPRINTF("DEBUG: %s() deferring to gen_exception(); passing s->base.pc_next 0x%04x\n", __func__, s->base.pc_next);
#endif
    //gen_exception(s, EXCP_ILLOP, s->pc_start - s->cs_base);
    gen_exception(s, EXCP_ILLOP, s->base.pc_next);
}

/* Signal a missing opcode or an unimplemented feature. For I386
 * there is also a concept of "bogus instruction stream", whereas
 * Z80 simulates a 'nop'.
 */
static void gen_unknown_opcode(CPUZ80State *env, DisasContext *s)
{
    gen_illegal_opcode(s);

    if (qemu_loglevel_mask(LOG_UNIMP)) {
        target_ulong pc = s->base.pc_next, end = s->pc;
        qemu_log_lock();
        qemu_log("ILLOPC: " TARGET_FMT_lx ":", pc);
        for (; pc < end; ++pc) {
            qemu_log(" %02x", cpu_ldub_code(env, pc));
        }
        qemu_log("\n");
        qemu_log_unlock();
    }
}


static target_ulong advance_pc(CPUZ80State *env, DisasContext *s, int num_bytes)
{
    uint32_t pc = s->pc;

    /* adjust program counter, wrapping around at RAMTOP=64KiB */
    s->pc += num_bytes;
    s->pc&= 0xffff;

    return pc;
}

/* Helpers for physical memory read */

static inline int8_t z80_ldsb_code(CPUZ80State *env, DisasContext *s)
{
    return cpu_ldsb_code(env, advance_pc(env, s, 1));
}

static inline uint8_t z80_ldub_code(CPUZ80State *env, DisasContext *s)
{
    return cpu_ldub_code(env, advance_pc(env, s, 1));
}

static inline uint16_t z80_lduw_code(CPUZ80State *env, DisasContext *s)
{
    return cpu_lduw_code(env, advance_pc(env, s, 2));
}


/* Convert one instruction and return the next PC value */
static target_ulong disas_insn(DisasContext *s, CPUState *cpu)
{
    /* PARTIAL. For testing purposes, we implement:
     * 1. Reading of a byte (at least one ensures TB has non-zero size)
     * 2. Triggering of ILLOP exception for missing translation cases
     * 3. Unprefixed and prefixed instruction parse/CPU differentiation
     */
    CPUZ80State *env = cpu->env_ptr;
    target_ulong pc_start = s->base.pc_next;
    unsigned int b;     /* instruction byte */
    int         prefixes, m;

    //s->pc_start = s->pc = pc_start;
    s->pc = pc_start;
    prefixes= 0;

    /* prefix handling jumps here to keep track
     * of prefixes seen in state
     */
    zprintf("PC = %04x: ", s->pc);
next_byte:
    //s->prefix= prefixes;

/* START */

    if (prefixes & PREFIX_DD) {
        m = MODE_DD;
    } else if (prefixes & PREFIX_FD) {
        m = MODE_FD;
    } else {
        m = MODE_NORMAL;
    }

    if ((prefixes & (PREFIX_CB | PREFIX_ED)) == 0) {
        /* DD/FD/plain insns without CB/ED */
        unsigned int x, y, z, p, q;
        int n, d;           /* immediate 'n', displacement 'd' */
        int r1, r2;         /* register number */

        b= z80_ldub_code(env, s);
        //s->pc++;

        x = (b >> 6) & 0x03;    /* isolate bits 7, 6 */
        y = (b >> 3) & 0x07;    /* isolate bits 5, 4, 3 */
        z = b & 0x07;           /* isolate bits 2, 1, 0 */
        p = y >> 1;
        q = y & 0x01;

#if 1   /* WmT - TRACE */
;DPRINTF("INFO: Byte read at pc_start 0x%04x got value 0x%02x - has x %o, y %o [p=%o/q=%o], z %o\n", pc_start, b, x, y,p,q, z);
#endif

        switch (x)
        {
        case 0:      /* insn pattern 00yyyzzz */
            switch (z)
            {
            case 0:
                switch (y)
                {
                case 0:
                    zprintf("nop\n");
                    break;
                case 1:
                    gen_ex(OR2_AF, OR2_AFX);
                    zprintf("ex af,af'\n");
                    break;
                case 2:
                    n = z80_ldsb_code(env, s);
                    //s->pc++;
                    gen_helper_djnz(cpu_env, tcg_const_tl(s->pc + n), tcg_const_tl(s->pc));
                    gen_eob(s);
                    s->base.is_jmp = DISAS_NORETURN;
                    zprintf("djnz $%02x\n", n);
                    break;

                case 3:
                    n = z80_ldsb_code(env, s);
                    //s->pc++;
                    gen_jmp_im(s->pc + n);
                    gen_eob(s);
                    s->base.is_jmp = DISAS_NORETURN;
                    zprintf("jr $%02x\n", n);
                    break;

                case 4:
                case 5:
                case 6:
                case 7:
                    n = z80_ldsb_code(env, s);
                    //s->pc++;
                    zprintf("jr %s,$%04x\n", cc[y-4], (s->pc + n) & 0xffff);
                    gen_jcc(s, y-4, s->pc + n, s->pc);
                    break;
                }   /* end z=0 switch(y) */
                break;
            case 1:
                switch (q) {
                case 0:	/* 0x21 */
                    n = z80_lduw_code(env, s);
                    //s->pc+= 2;
                    tcg_gen_movi_tl(cpu_T[0], n);
                    r1 = regpairmap(regpair[p], m);
                    gen_movw_reg_v(r1, cpu_T[0]);
                    zprintf("ld %s,$%04x\n", regpairnames[r1], n);
                    break;

                case 1:
                    r1 = regpairmap(regpair[p], m);
                    r2 = regpairmap(OR2_HL, m);
                    gen_movw_v_reg(cpu_T[0], r1);
                    gen_movw_v_reg(cpu_T[1], r2);
                    gen_helper_addw_T0_T1_cc(cpu_env);
                    gen_movw_reg_v(r2, cpu_T[0]);
                    zprintf("add %s,%s\n", regpairnames[r2], regpairnames[r1]);
                    break;
                }
                break;

            case 2:
                switch (q) {
                case 0:
                    switch (p) {
                    case 0:
                        gen_movb_v_A(cpu_T[0]);
                        gen_movw_v_BC(cpu_A0);
                        tcg_gen_qemu_st8(cpu_T[0], cpu_A0, MEM_INDEX);
                        zprintf("ld (bc),a\n");
                        break;
                    case 1:
                        gen_movb_v_A(cpu_T[0]);
                        gen_movw_v_DE(cpu_A0);
                        tcg_gen_qemu_st8(cpu_T[0], cpu_A0, MEM_INDEX);
                        zprintf("ld (de),a\n");
                        break;
                    case 2:
                        n = z80_lduw_code(env, s);
                        //s->pc += 2;
                        r1 = regpairmap(OR2_HL, m);
                        gen_movw_v_reg(cpu_T[0], r1);
                        tcg_gen_movi_i32(cpu_A0, n);
                        tcg_gen_qemu_st16(cpu_T[0], cpu_A0, MEM_INDEX);
                        zprintf("ld ($%04x),%s\n", n, regpairnames[r1]);
                        break;
                    case 3:
                        n = z80_lduw_code(env, s);
                        //s->pc += 2;
                        gen_movb_v_A(cpu_T[0]);
                        tcg_gen_movi_i32(cpu_A0, n);
                        tcg_gen_qemu_st8(cpu_T[0], cpu_A0, MEM_INDEX);
                        zprintf("ld ($%04x),a\n", n);
                        break;
                    }
                    break;
                case 1:
                    switch (p) {
                    case 0:
                        gen_movw_v_BC(cpu_A0);
                        tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
                        gen_movb_A_v(cpu_T[0]);
                        zprintf("ld a,(bc)\n");
                        break;
                    case 1:
                        gen_movw_v_DE(cpu_A0);
                        tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
                        gen_movb_A_v(cpu_T[0]);
                        zprintf("ld a,(de)\n");
                        break;
                    case 2:
                        n = z80_lduw_code(env, s);
                        //s->pc += 2;
                        r1 = regpairmap(OR2_HL, m);
                        tcg_gen_movi_i32(cpu_A0, n);
                        tcg_gen_qemu_ld16u(cpu_T[0], cpu_A0, MEM_INDEX);
                        gen_movw_reg_v(r1, cpu_T[0]);
                        zprintf("ld %s,($%04x)\n", regpairnames[r1], n);
                        break;
                    case 3:
                        n = z80_lduw_code(env, s);
                        //s->pc += 2;
                        tcg_gen_movi_i32(cpu_A0, n);
                        tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
                        gen_movb_A_v(cpu_T[0]);
                        zprintf("ld a,($%04x)\n", n);
                        break;
                    }
                    break;
                }
                break;
            case 3:
                switch (q)
                {
                case 0:
                    r1 = regpairmap(regpair[p], m);
                    gen_movw_v_reg(cpu_T[0], r1);
                    tcg_gen_addi_tl(cpu_T[0], cpu_T[0], 1);
                    gen_movw_reg_v(r1, cpu_T[0]);
                    zprintf("inc %s\n", regpairnames[r1]);
                    break;
                case 1:
                    r1 = regpairmap(regpair[p], m);
                    gen_movw_v_reg(cpu_T[0], r1);
                    tcg_gen_subi_tl(cpu_T[0], cpu_T[0], 1);
                    gen_movw_reg_v(r1, cpu_T[0]);
                    zprintf("dec %s\n", regpairnames[r1]);
                    break;
                }
                break;

            case 4:
                r1 = regmap(reg[y], m);
                if (is_indexed(r1)) {
                    d = z80_ldsb_code(env, s);
                    //s->pc++;
                    gen_movb_v_idx(cpu_T[0], r1, d);
                } else {
                    gen_movb_v_reg(cpu_T[0], r1);
                }
                gen_helper_incb_T0_cc(cpu_env);
                if (is_indexed(r1)) {
                    gen_movb_idx_v(r1, cpu_T[0], d);
                } else {
                    gen_movb_reg_v(r1, cpu_T[0]);
                }
                if (is_indexed(r1)) {
                    zprintf("inc (%s%c$%02x)\n", idxnames[r1], shexb(d));
                } else {
                    zprintf("inc %s\n", regnames[r1]);
                }
                break;

            case 5:
                r1 = regmap(reg[y], m);
                if (is_indexed(r1)) {
                    d = z80_ldsb_code(env, s);
                    //s->pc++;
                    gen_movb_v_idx(cpu_T[0], r1, d);
                } else {
                    gen_movb_v_reg(cpu_T[0], r1);
                }
                gen_helper_decb_T0_cc(cpu_env);
                if (is_indexed(r1)) {
                    gen_movb_idx_v(r1, cpu_T[0], d);
                } else {
                    gen_movb_reg_v(r1, cpu_T[0]);
                }
                if (is_indexed(r1)) {
                    zprintf("dec (%s%c$%02x)\n", idxnames[r1], shexb(d));
                } else {
                    zprintf("dec %s\n", regnames[r1]);
                }
                break;

            case 6: /* 0x3e */
                r1 = regmap(reg[y], m);
                if (is_indexed(r1)) {
                    d = z80_ldsb_code(env, s);
                    //s->pc++;
                }
                n = z80_ldub_code(env, s);
                //s->pc++;
                tcg_gen_movi_tl(cpu_T[0], n);
                if (is_indexed(r1)) {
                    gen_movb_idx_v(r1, cpu_T[0], d);
                } else {
                    gen_movb_reg_v(r1, cpu_T[0]);
                }
                if (is_indexed(r1)) {
                    zprintf("ld (%s%c$%02x),$%02x\n", idxnames[r1], shexb(d), n);
                } else {
                    zprintf("ld %s,$%02x\n", regnames[r1], n);
                }
                break;

            case 7:
                switch (y)
                {
                case 0:
                    gen_helper_rlca_cc(cpu_env);
                    zprintf("rlca\n");
                    break;
                case 1:
                    gen_helper_rrca_cc(cpu_env);
                    zprintf("rrca\n");
                    break;
                case 2:
                    gen_helper_rla_cc(cpu_env);
                    zprintf("rla\n");
                    break;
                case 3:
                    gen_helper_rra_cc(cpu_env);
                    zprintf("rra\n");
                    break;
                case 4:
                    gen_helper_daa_cc(cpu_env);
                    zprintf("daa\n");
                    break;
                case 5:
                    gen_helper_cpl_cc(cpu_env);
                    zprintf("cpl\n");
                    break;
                case 6:
                    gen_helper_scf_cc(cpu_env);
                    zprintf("scf\n");
                    break;
                case 7:
                    gen_helper_ccf_cc(cpu_env);
                    zprintf("ccf\n");
                    break;
                }
                break;
            }
            break;

        case 1: /* insn pattern 01yyyzzz */
            if (z == 6 && y == 6) {
                gen_jmp_im(s->pc);
                gen_helper_halt(cpu_env);
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
                    d = z80_ldsb_code(env, s);
                    //s->pc++;
                }
                if (is_indexed(r1)) {
                    gen_movb_v_idx(cpu_T[0], r1, d);
                } else {
                    gen_movb_v_reg(cpu_T[0], r1);
                }
                if (is_indexed(r2)
#if __GNUC__    /* gcc 6.3.0: avoid "'d' may be used uninitialised" */
                    && !is_indexed(r1))
#endif
                    {
                    //d= z80_ldsb_code(env, s);
                    gen_movb_idx_v(r2, cpu_T[0], d);
                } else {
                    gen_movb_reg_v(r2, cpu_T[0]);
                }
                if (is_indexed(r1)) {
                    zprintf("ld %s,(%s%c$%02x)\n", regnames[r2], idxnames[r1], shexb(d));
                } else if (is_indexed(r2)) {
                    zprintf("ld (%s%c$%02x),%s\n", idxnames[r2], shexb(d), regnames[r1]);
                } else {
                    zprintf("ld %s,%s\n", regnames[r2], regnames[r1]);
                }
            }
            break;

        case 2: /* insn pattern 10yyyzzz - arithmetic/logic */
            r1 = regmap(reg[z], m);
            if (is_indexed(r1)) {
                d = z80_ldsb_code(env, s);
                //s->pc++;
                gen_movb_v_idx(cpu_T[0], r1, d);
            } else {
                gen_movb_v_reg(cpu_T[0], r1);
            }
            gen_alu[y](cpu_env); /* places output in A */
            if (is_indexed(r1)) {
                zprintf("%s(%s%c$%02x)\n", alu[y], idxnames[r1], shexb(d));
            } else {
                zprintf("%s%s\n", alu[y], regnames[r1]);
            }
            break;

        case 3: /* insn pattern 11yyyzzz */
            switch (z) {
            case 0:
                gen_retcc(s, y, s->pc);
                zprintf("ret %s\n", cc[y]);
                break;

            case 1:
                switch (q)
                {
                case 0:
                    r1 = regpairmap(regpair2[p], m);
                    gen_popw(cpu_T[0]);
                    gen_movw_reg_v(r1, cpu_T[0]);
                    zprintf("pop %s\n", regpairnames[r1]);
                    break;
                case 1:
                    switch (p)
                    {
                    case 0: /* 0xc9 */
                        gen_popw(cpu_T[0]);
                        gen_helper_jmp_T0(cpu_env);
                        zprintf("ret\n");
                        gen_eob(s);
                        s->base.is_jmp = DISAS_NORETURN;
//                      s->is_ei = 1;
                        break;
                    case 1:
                        gen_ex(OR2_BC, OR2_BCX);
                        gen_ex(OR2_DE, OR2_DEX);
                        gen_ex(OR2_HL, OR2_HLX);
                        zprintf("exx\n");
                        break;
                    case 2:
                        r1 = regpairmap(OR2_HL, m);
                        gen_movw_v_reg(cpu_T[0], r1);
                        gen_helper_jmp_T0(cpu_env);
                        zprintf("jp %s\n", regpairnames[r1]);
                        gen_eob(s);
                        s->base.is_jmp = DISAS_NORETURN;
                        break;
                    case 3:
                        r1 = regpairmap(OR2_HL, m);
                        gen_movw_v_reg(cpu_T[0], r1);
                        gen_movw_SP_v(cpu_T[0]);
                        zprintf("ld sp,%s\n", regpairnames[r1]);
                        break;
                    }
                    break;
                }
                break;

            case 2:
                n = z80_lduw_code(env, s);
                //s->pc += 2;
                gen_jcc(s, y, n, s->pc);
                zprintf("jp %s,$%04x\n", cc[y], n);
                /* TODO: gen_eob() w/ DISAS_NORETURN missing? */
                break;

            case 3:
                switch (y)
                {
                case 0:
                    n = z80_lduw_code(env, s);
                    //s->pc += 2;
                    gen_jmp_im(n);
                    zprintf("jp $%04x\n", n);
                    gen_eob(s);
                    s->base.is_jmp = DISAS_NORETURN;
                    break;
                case 1:
                    //zprintf("cb prefix\n");
                    prefixes |= PREFIX_CB;
                    goto next_byte;
                    break;

                case 2:
                    n = z80_ldub_code(env, s);
                    //s->pc++;
                    gen_movb_v_A(cpu_T[0]);
                    if (use_icount) {
                        gen_io_start();
                    }
                    gen_helper_out_T0_im(cpu_env, tcg_const_tl(n));
                    if (use_icount) {
                        gen_io_end();
                        gen_jmp_im(s->pc);
                    }
                    zprintf("out ($%02x),a\n", n);
                    break;

                case 3:
                    n = z80_ldub_code(env, s);
                    //s->pc++;
                    if (use_icount) {
                        gen_io_start();
                    }
                    gen_helper_in_T0_im(cpu_env, tcg_const_tl(n));
                    gen_movb_A_v(cpu_T[0]);
                    if (use_icount) {
                        gen_io_end();
                        gen_jmp_im(s->pc);
                    }
                    zprintf("in a,($%02x)\n", n);
                    break;

                case 4:
                    r1 = regpairmap(OR2_HL, m);
                    gen_popw(cpu_T[1]);
                    gen_movw_v_reg(cpu_T[0], r1);
                    gen_pushw(cpu_T[0]);
                    gen_movw_reg_v(r1, cpu_T[1]);
                    zprintf("ex (sp),%s\n", regpairnames[r1]);
                    break;
                case 5:
                    gen_ex(OR2_DE, OR2_HL);
                    zprintf("ex de,hl\n");
                    break;
                case 6:
                    gen_helper_di(cpu_env);
                    zprintf("di\n");
                    break;
                case 7:
                    gen_helper_ei(cpu_env);
                    zprintf("ei\n");
//                  gen_eob(s);
//                  s->is_ei = 1;
                    break;
                }
                break;

            case 4:
                n = z80_lduw_code(env, s);
                //s->pc += 2;
                gen_callcc(s, y, n, s->pc);
                zprintf("call %s,$%04x\n", cc[y], n);
                break;

            case 5:
                switch (q)
                {
                case 0:
                    r1 = regpairmap(regpair2[p], m);
                    gen_movw_v_reg(cpu_T[0], r1);
                    gen_pushw(cpu_T[0]);
                    zprintf("push %s\n", regpairnames[r1]);
                    break;
                case 1:
                    switch (p)
                    {
                    case 0:
                        n = z80_lduw_code(env, s);
                        //s->pc += 2;
                        tcg_gen_movi_tl(cpu_T[0], s->pc);
                        gen_pushw(cpu_T[0]);
                        gen_jmp_im(n);
                        zprintf("call $%04x\n", n);
                        gen_eob(s);
                        s->base.is_jmp = DISAS_NORETURN;
                        break;
                    case 1:
                        //zprintf("dd prefix\n");
                        prefixes |= PREFIX_DD;
                        goto next_byte;
                        break;
                    case 2:
                        //zprintf("ed prefix\n");
                        prefixes |= PREFIX_ED;
                        goto next_byte;
                        break;
                    case 3:
                        //zprintf("fd prefix\n");
                        prefixes |= PREFIX_FD;
                        goto next_byte;
                        break;
                    }   /* switch(p) ends */
                    break;
                }
                break;

            case 6:
                n = z80_ldub_code(env, s);
                //s->pc++;
                tcg_gen_movi_tl(cpu_T[0], n);
                gen_alu[y](cpu_env); /* places output in A */
                zprintf("%s$%02x\n", alu[y], n);
                break;
            case 7:
                tcg_gen_movi_tl(cpu_T[0], s->pc);
                gen_pushw(cpu_T[0]);
                gen_jmp_im(y*8);
                zprintf("rst $%02x\n", y*8);
                gen_eob(s);
                s->base.is_jmp = DISAS_NORETURN;
                break;
            }
            break;
        }   /* switch(x) ends */
    }
    else if (prefixes & PREFIX_CB)
    {	/* cb mode: */
        unsigned int x, y, z;
        //unsigned int p, q;
        int d;              /* displacement 'd' */
        int r1, r2;         /* register number */

        if (m != MODE_NORMAL) {
            /* 0xDD 0xCB DISP OP or 0xFD 0xCB DISP OP cases */
            d = z80_ldsb_code(env, s);
            //s->pc++;
        }

        b = z80_ldub_code(env, s);
        //s->pc++;

        x= (b >> 6) & 0x03;     /* isolate bits 7, 6 */
        y= (b >> 3) & 0x07;     /* isolate bits 5, 4, 3 */
        z= b & 0x07;            /* isolate bits 2, 1, 0 */
        //p = y >> 1;
        //q = y & 0x01;

        if (m != MODE_NORMAL) {
            r1 = regmap(OR_HLmem, m);
            gen_movb_v_idx(cpu_T[0], r1, d);
            if (z != 6) {
                r2 = regmap(reg[z], 0);
            }
        } else {
            r1 = regmap(reg[z], m);
            gen_movb_v_reg(cpu_T[0], r1);
        }

        switch (x)
        {
        case 0:
            /* TODO: TST instead of SLL for R800 */
            gen_rot_T0[y](cpu_env);
            if (m != MODE_NORMAL) {
                gen_movb_idx_v(r1, cpu_T[0], d);
                if (z != 6) {
                    gen_movb_reg_v(r2, cpu_T[0]);
                }
            } else {
                gen_movb_reg_v(r1, cpu_T[0]);
            }
            zprintf("%s %s\n", rot[y], regnames[r1]);
            break;
        case 1:
            gen_helper_bit_T0(cpu_env, tcg_const_tl(1 << y));
            zprintf("bit %i,%s\n", y, regnames[r1]);
            break;
        case 2:
            tcg_gen_andi_tl(cpu_T[0], cpu_T[0], ~(1 << y));
            if (m != MODE_NORMAL) {
                gen_movb_idx_v(r1, cpu_T[0], d);
                if (z != 6) {
#ifdef __GNUC__ /* ffs gcc, you didn't complain above!! [bug in v8.3.0] */
                    r2 = regmap(reg[z], 0);
#endif
                    gen_movb_reg_v(r2, cpu_T[0]);
                }
            } else {
                gen_movb_reg_v(r1, cpu_T[0]);
            }
            zprintf("res %i,%s\n", y, regnames[r1]);
            break;
        case 3:
            tcg_gen_ori_tl(cpu_T[0], cpu_T[0], 1 << y);
            if (m != MODE_NORMAL) {
                gen_movb_idx_v(r1, cpu_T[0], d);
                if (z != 6) {
#ifdef __GNUC__ /* ffs gcc, you didn't complain above!! [bug in v8.3.0] */
                    r2 = regmap(reg[z], 0);
#endif
                    gen_movb_reg_v(r2, cpu_T[0]);
                }
            } else {
                gen_movb_reg_v(r1, cpu_T[0]);
            }
            zprintf("set %i,%s\n", y, regnames[r1]);
            break;
        }
    }
    else if (prefixes & PREFIX_ED)
    {	/* ed mode: */
        unsigned int x, y, z, p, q;
        int n;              /* immediate 'n' */
        int r1, r2;         /* register number */

        b = z80_ldub_code(env, s);
        //s->pc++;

        x= (b >> 6) & 0x03;     /* isolate bits 7, 6 */
        y= (b >> 3) & 0x07;     /* isolate bits 5, 4, 3 */
        z= b & 0x07;            /* isolate bits 2, 1, 0 */
        p = y >> 1;
        q = y & 0x01;

        switch (x)
        {
        case 0:
            zprintf("nop\n");
            break;
        case 3: /* NB. follows x=0 due to fallthrough between x=1,x=2 */
            /* PARTIAL: incomplete for R800 here
             * z=1: mulub
             * z=3: muluw
             */
            zprintf("nop\n");
            break;
        case 1:
            switch (z)
            {
            case 0:
                if (use_icount) {
                    gen_io_start();
                }
                gen_helper_in_T0_bc_cc(cpu_env);
                if (y != 6) {
                    r1 = regmap(reg[y], m);
                    gen_movb_reg_v(r1, cpu_T[0]);
                    zprintf("in %s,(c)\n", regnames[r1]);
                } else {
                    zprintf("in (c)\n");
                }
                if (use_icount) {
                    gen_io_end();
                    gen_jmp_im(s->pc);
                }
                break;

            case 1:
                if (y != 6) {
                    r1 = regmap(reg[y], m);
                    gen_movb_v_reg(cpu_T[0], r1);
                    zprintf("out (c),%s\n", regnames[r1]);
                } else {
                    tcg_gen_movi_tl(cpu_T[0], 0);
                    zprintf("out (c),0\n");
                }
                if (use_icount) {
                    gen_io_start();
                }
                gen_helper_out_T0_bc(cpu_env);
                if (use_icount) {
                    gen_io_end();
                    gen_jmp_im(s->pc);
                }
                break;

            case 2:
                r1 = regpairmap(OR2_HL, m);
                r2 = regpairmap(regpair[p], m);
                gen_movw_v_reg(cpu_T[0], r1);
                gen_movw_v_reg(cpu_T[1], r2);
                if (q == 0) {
                    zprintf("sbc %s,%s\n", regpairnames[r1], regpairnames[r2]);
                    gen_helper_sbcw_T0_T1_cc(cpu_env);
                } else {
                    zprintf("adc %s,%s\n", regpairnames[r1], regpairnames[r2]);
                    gen_helper_adcw_T0_T1_cc(cpu_env);
                }
                gen_movw_reg_v(r1, cpu_T[0]);
                break;
            case 3:
                n = z80_lduw_code(env, s);
                //s->pc += 2;
                r1 = regpairmap(regpair[p], m);
                if (q == 0) {
                    gen_movw_v_reg(cpu_T[0], r1);
                    tcg_gen_movi_i32(cpu_A0, n);
                    tcg_gen_qemu_st16(cpu_T[0], cpu_A0, MEM_INDEX);
                    zprintf("ld ($%02x),%s\n", n, regpairnames[r1]);
                } else {
                    tcg_gen_movi_i32(cpu_A0, n);
                    tcg_gen_qemu_ld16u(cpu_T[0], cpu_A0, MEM_INDEX);
                    gen_movw_reg_v(r1, cpu_T[0]);
                    zprintf("ld %s,($%02x)\n", regpairnames[r1], n);
                }
                break;
            case 4:
                zprintf("neg\n");
                gen_helper_neg_cc(cpu_env);
                break;
            case 5:
                /* FIXME [WmT: why?] */
                gen_popw(cpu_T[0]);
                gen_helper_jmp_T0(cpu_env);
                gen_helper_ri(cpu_env);
                if (q == 0) {
                    zprintf("retn\n");
                } else {
                    zprintf("reti\n");
                }
                gen_eob(s);
                s->base.is_jmp = DISAS_NORETURN;
//              s->is_ei = 1;
                break;
            case 6:
                gen_helper_imode(cpu_env, tcg_const_tl(imode[y]));
                zprintf("im im[%i]\n", imode[y]);
//              gen_eob(s);
//              s->is_ei = 1;
                break;
            case 7:
                switch (y)
                {
                case 0:
                    gen_helper_ld_I_A(cpu_env);
                    zprintf("ld i,a\n");
                    break;
                case 1:
                    gen_helper_ld_R_A(cpu_env);
                    zprintf("ld r,a\n");
                    break;
                case 2:
                    gen_helper_ld_A_I(cpu_env);
                    zprintf("ld a,i\n");
                    break;
                case 3:
                    gen_helper_ld_A_R(cpu_env);
                    zprintf("ld a,r\n");
                    break;
                case 4:
                    gen_movb_v_HLmem(cpu_T[0]);
                    gen_helper_rrd_cc(cpu_env);
                    gen_movb_HLmem_v(cpu_T[0]);
                    zprintf("rrd\n");
                    break;
                case 5:
                    gen_movb_v_HLmem(cpu_T[0]);
                    gen_helper_rld_cc(cpu_env);
                    gen_movb_HLmem_v(cpu_T[0]);
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
            /* FIXME [WmT: upstream comment ...unclear why] */
            if (y >= 4) {
                switch (z)
                {
                case 0: /* ldi/ldd/ldir/lddr */
                    gen_movw_v_HL(cpu_A0);
                    tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
                    gen_movw_v_DE(cpu_A0);
                    tcg_gen_qemu_st8(cpu_T[0], cpu_A0, MEM_INDEX);

                    if (!(y & 1)) {
                        gen_helper_bli_ld_inc_cc(cpu_env);
                    } else {
                        gen_helper_bli_ld_dec_cc(cpu_env);
                    }
                    if ((y & 2)) {
                        gen_helper_bli_ld_rep(cpu_env, tcg_const_tl(s->pc));
                        gen_eob(s);
                        s->base.is_jmp = DISAS_NORETURN;
                    }
                    break;

                case 1: /* cpi/cpd/cpir/cpdr */
                    gen_movw_v_HL(cpu_A0);
                    tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
                    gen_helper_bli_cp_cc(cpu_env);

                    if (!(y & 1)) {
                        gen_helper_bli_cp_inc_cc(cpu_env);
                    } else {
                        gen_helper_bli_cp_dec_cc(cpu_env);
                    }
                    if ((y & 2)) {
                        gen_helper_bli_cp_rep(cpu_env, tcg_const_tl(s->pc));
                        gen_eob(s);
                        s->base.is_jmp = DISAS_NORETURN;
                    }
                    break;

                case 2: /* ini/ind/inir/indr */
                    if (use_icount) {
                        gen_io_start();
                    }
                    gen_helper_in_T0_bc_cc(cpu_env);
                    if (use_icount) {
                        gen_io_end();
                    }
                    gen_movw_v_HL(cpu_A0);
                    tcg_gen_qemu_st8(cpu_T[0], cpu_A0, MEM_INDEX);
                    if (!(y & 1)) {
                        gen_helper_bli_io_T0_inc(cpu_env, tcg_const_tl(0));
                    } else {
                        gen_helper_bli_io_T0_dec(cpu_env, tcg_const_tl(0));
                    }
                    if ((y & 2)) {
                        gen_helper_bli_io_rep(cpu_env, tcg_const_tl(s->pc));
                        gen_eob(s);
                        s->base.is_jmp = DISAS_NORETURN;
                    } else if (use_icount) {
                        gen_jmp_im(s->pc);
                    }
                    break;

                case 3: /* outi/outd/otir/otdr */
                    gen_movw_v_HL(cpu_A0);
                    tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
                    if (use_icount) {
                        gen_io_start();
                    }
                    gen_helper_out_T0_bc(cpu_env);
                    if (use_icount) {
                        gen_io_end();
                    }
                    if (!(y & 1)) {
                        gen_helper_bli_io_T0_inc(cpu_env, tcg_const_tl(1));
                    } else {
                        gen_helper_bli_io_T0_dec(cpu_env, tcg_const_tl(1));
                    }
                    if ((y & 2)) {
                        gen_helper_bli_io_rep(cpu_env, tcg_const_tl(s->pc));
                        gen_eob(s);
                        s->base.is_jmp = DISAS_NORETURN;
                    } else if (use_icount) {
                        gen_jmp_im(s->pc);
                    }
                    break;     /* case z=3 ends */
                }   /* switch(z) ends */
                zprintf("%s\n", bli[y-4][z]);
                break;
            }  /* case 2 y>=4 end - falls through for y=0..3 */
        }	/* switch(x) ends */
    }

    /* For Z80, there are no "illegal" instructions to signal here.
     * After parsing either we have acted on a fetch; we saw a 'nop'
     * and should do nothing; or we should otherwise simulate a 'nop'
     * [by not acting further]. In the last case, the fetch is allowed
     * to have side effects on internal state/interrupt configuration
     */
    prefixes= 0;

#if 1   /* WmT - TRACE */
;DPRINTF("** EXIT %s() - OK - retval from s->pc 0x%04x **\n", __func__, s->pc);
#endif
    return s->pc;

 unknown_op:    /* "bad insn" case (Z80: normally unreachable) */
    gen_unknown_opcode(env, s);
#if 1   /* WmT - TRACE */
;DPRINTF("** EXIT %s() - unknown opcode 0x%02x seen - next s->pc 0x%04x **\n", __func__, b, s->pc);
#endif
    return s->pc;
}


void tcg_z80_init(void)
{
    /* CPU initialisation for i386 has:
     * - cpu_regs[] as a static TCGv[] above, initialised here
     * - "local temps" cpu_{T0|T1|A0} are set up at disas context init
     */

#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() tcg_global_mem_new_i32() for cpu_T[0]...\n", __func__);
#endif
    cpu_T[0] = tcg_global_mem_new_i32(cpu_env, offsetof(CPUZ80State, t0), "T0");
    cpu_T[1] = tcg_global_mem_new_i32(cpu_env, offsetof(CPUZ80State, t1), "T1");
    cpu_A0= tcg_global_mem_new_i32(cpu_env, offsetof(CPUZ80State, a0), "A0");
}


static int z80_tr_init_disas_context(DisasContextBase *dcbase, CPUState *cpu,
                                      int max_insns)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
#ifdef CONFIG_USER_ONLY
    TaskState       *ts = cpu->opaque;
    target_ulong    magic= ts->bprm->magic_ramloc;
#endif
/* TODO: v2 sets:
    ...dc->mem_index to 0, or cpu_mmu_index() result [CONFIG_SOFTMMU]
    ...dc->flags
    ...dc->jmp_opt, based on singlestepping configuration
    ...initialisation of relevant 'static TCGv's
 */
//    CPUX86State *env = cpu->env_ptr;
    uint32_t flags = dc->base.tb->flags;
//    target_ulong cs_base = dc->base.tb->cs_base;
//
//    dc->pe = (flags >> HF_PE_SHIFT) & 1;
//    dc->code32 = (flags >> HF_CS32_SHIFT) & 1;
//    dc->ss32 = (flags >> HF_SS32_SHIFT) & 1;
//    dc->addseg = (flags >> HF_ADDSEG_SHIFT) & 1;
//    dc->f_st = 0;
//    dc->vm86 = (flags >> VM_SHIFT) & 1;
//    dc->cpl = (flags >> HF_CPL_SHIFT) & 3;
//    dc->iopl = (flags >> IOPL_SHIFT) & 3;
//    dc->tf = (flags >> TF_SHIFT) & 1;
//    dc->cc_op = CC_OP_DYNAMIC;
//    dc->cc_op_dirty = false;
//    dc->cs_base = cs_base;
//    dc->popl_esp_hack = 0;
//    /* select memory access functions */
//    dc->mem_index = 0;
//#ifdef CONFIG_SOFTMMU
//    dc->mem_index = cpu_mmu_index(env, false);
//#endif
//    dc->cpuid_features = env->features[FEAT_1_EDX];
//    dc->cpuid_ext_features = env->features[FEAT_1_ECX];
//    dc->cpuid_ext2_features = env->features[FEAT_8000_0001_EDX];
//    dc->cpuid_ext3_features = env->features[FEAT_8000_0001_ECX];
//    dc->cpuid_7_0_ebx_features = env->features[FEAT_7_0_EBX];
//    dc->cpuid_xsave_features = env->features[FEAT_XSAVE];
//#ifdef TARGET_X86_64
//    dc->lma = (flags >> HF_LMA_SHIFT) & 1;
//    dc->code64 = (flags >> HF_CS64_SHIFT) & 1;
//#endif

    /* [QEmu v2] z80-softmmu initialises with non-zero flags (due
     * to obsolete HF_SOFTMMU_MASK use?)
     */
    dc->flags = flags;

//    dc->jmp_opt = !(dc->tf || dc->base.singlestep_enabled ||
//                    (flags & HF_INHIBIT_IRQ_MASK));
//    /* Do not optimize repz jumps at all in icount mode, because
//       rep movsS instructions are execured with different paths
//       in !repz_opt and repz_opt modes. The first one was used
//       always except single step mode. And this setting
//       disables jumps optimization and control paths become
//       equivalent in run and single step modes.
//       Now there will be no jump optimization for repz in
//       record/replay modes and there will always be an
//       additional step for ecx=0 when icount is enabled.
//     */
//    dc->repz_opt = !dc->jmp_opt && !(tb_cflags(dc->base.tb) & CF_USE_ICOUNT);
//#if 0
//    /* check addseg logic */
//    if (!dc->addseg && (dc->vm86 || !dc->pe || !dc->code32))
//        printf("ERROR addseg\n");
//#endif
//
//    cpu_T0 = tcg_temp_new();
//    cpu_T1 = tcg_temp_new();
//    cpu_A0 = tcg_temp_new();
//
//    cpu_tmp0 = tcg_temp_new();
//    cpu_tmp1_i64 = tcg_temp_new_i64();
//    cpu_tmp2_i32 = tcg_temp_new_i32();
//    cpu_tmp3_i32 = tcg_temp_new_i32();
//    cpu_tmp4 = tcg_temp_new();
//    cpu_ptr0 = tcg_temp_new_ptr();
//    cpu_ptr1 = tcg_temp_new_ptr();
//    cpu_cc_srcT = tcg_temp_local_new();

#ifdef CONFIG_USER_ONLY
    dc->magic_ramloc= magic;
#endif

#ifdef CONFIG_USER_ONLY
    dc->magic_ramloc= magic;
#endif

    return max_insns;
}

static void z80_tr_tb_start(DisasContextBase *db, CPUState *cpu)
{
#if 0   /* WmT - PARTIAL */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else
    /* nothing to do */
#endif
}

static void z80_tr_insn_start(DisasContextBase *dcbase, CPUState *cpu)
{
#if 0   /* WmT - PARTIAL */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    //tcg_gen_insn_start(dc->base.pc_next, dc->cc_op);
    tcg_gen_insn_start(dc->base.pc_next);
#endif
}

static bool z80_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cpu,
                                     const CPUBreakpoint *bp)
{
#if 1   /* WmT - PARTIAL */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    /* If RF is set, suppress an internally generated breakpoint.  */
    int flags = dc->base.tb->flags & HF_RF_MASK ? BP_GDB : BP_ANY;
    if (bp->flags & flags) {
        gen_debug(dc, dc->base.pc_next - dc->cs_base);
        dc->base.is_jmp = DISAS_NORETURN;
        /* The address covered by the breakpoint must be included in
           [tb->pc, tb->pc + tb->size) in order to for it to be
           properly cleared -- thus we increment the PC here so that
           the generic logic setting tb->size later does the right thing.  */
        dc->base.pc_next += 1;
        return true;
    } else {
        return false;
    }
#endif
}

static bool z80_pre_translate_insn(DisasContext *dc)
{
#ifdef CONFIG_USER_ONLY
    if (dc->base.pc_next == dc->magic_ramloc)
    {
        //gen_exception(dc, EXCP_KERNEL_TRAP, dc->pc_start - dc->cs_base);
        gen_exception(dc, EXCP_KERNEL_TRAP, dc->base.pc_next);
        dc->base.is_jmp = DISAS_NORETURN;
        return true; /* "handled" */
    }
#endif

    return false;
}

static void z80_tr_translate_insn(DisasContextBase *dcbase, CPUState *cpu)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    target_ulong pc_next;

    if (z80_pre_translate_insn(dc)) {
        return;
    }

    pc_next = disas_insn(dc, cpu);

#if 1   /* WmT - PARTIAL */
;DPRINTF("DEBUG: %s() PARTIAL - proceeding with pc_next 0x%04x from disas_insn()\n", __func__, pc_next);
#endif
    if (/* dc->tf || */ (dc->base.tb->flags & HF_INHIBIT_IRQ_MASK)) {
        /* if single step mode, we generate only one instruction and
           generate an exception */
        /* if irq were inhibited with HF_INHIBIT_IRQ_MASK, we clear
           the flag and abort the translation to give the irqs a
           chance to happen */
        dc->base.is_jmp = DISAS_TOO_MANY;
    } else if ((tb_cflags(dc->base.tb) & CF_USE_ICOUNT)
               && ((dc->base.pc_next & TARGET_PAGE_MASK)
                   != ((dc->base.pc_next + TARGET_MAX_INSN_SIZE - 1)
                       & TARGET_PAGE_MASK)
                   || (dc->base.pc_next & ~TARGET_PAGE_MASK) == 0)) {
        /* Do not cross the boundary of the pages in icount mode,
           it can cause an exception. Do it only when boundary is
           crossed by the first instruction in the block.
           If current instruction already crossed the bound - it's ok,
           because an exception hasn't stopped this code.
         */
        dc->base.is_jmp = DISAS_TOO_MANY;
    } else if ((pc_next - dc->base.pc_first) >= (TARGET_PAGE_SIZE - 32)) {
        dc->base.is_jmp = DISAS_TOO_MANY;
    }

    dc->base.pc_next = pc_next;
}

static void z80_tr_tb_stop(DisasContextBase *dcbase, CPUState *cpu)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    if (dc->base.is_jmp == DISAS_TOO_MANY) {
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() handling DISAS_TOO_MANY - using pc_next 0x%04x for gen_jmp_im()\n", __func__, dc->base.pc_next);
#endif
        //gen_jmp_im(dc->base.pc_next - dc->cs_base);
        gen_jmp_im(dc->base.pc_next);
        gen_eob(dc);
    }
}

static void z80_tr_disas_log(const DisasContextBase *dcbase,
                              CPUState *cpu)
{
#if 1   /* WmT - PARTIAL */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    qemu_log("IN: %s\n", lookup_symbol(dc->base.pc_first));
    log_target_disas(cpu, dc->base.pc_first, dc->base.tb->size);
#endif
}

static const TranslatorOps z80_tr_ops = {
    .init_disas_context = z80_tr_init_disas_context,
    .tb_start           = z80_tr_tb_start,
    .insn_start         = z80_tr_insn_start,
    .breakpoint_check   = z80_tr_breakpoint_check,
    .translate_insn     = z80_tr_translate_insn,
    .tb_stop            = z80_tr_tb_stop,
    .disas_log          = z80_tr_disas_log,
};

/* generate intermediate code for basic block 'tb'.  */
void gen_intermediate_code(CPUState *cpu, TranslationBlock *tb)
{
    DisasContext dc;
#if 1   /* WmT - TRACE */
;DPRINTF("*** ENTER %s() ****\n", __func__);
#endif

    translator_loop(&z80_tr_ops, &dc.base, cpu, tb);
#if 1   /* WmT - TRACE */
;DPRINTF("*** EXIT %s(), OK ***\n", __func__);
#endif
}


void restore_state_to_opc(CPUZ80State *env, TranslationBlock *tb,
                            target_ulong *data)
{
    //int cc_op = data[1];  /* unused for Z80 */
    env->pc = data[0];
}
