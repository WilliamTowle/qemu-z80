/*
 * Z80 translation
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting to QEmu 0.12+ by William Towle <william_towle@yahoo.co.uk>
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

#include <stdio.h>

#include "cpu.h"
#include "tcg-op.h"

#include "helper.h"
#define GEN_HELPER 1
#include "helper.h"


#if 1	/* debug */
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif

#if 0
#define PREFIX_CB  0x01
#define PREFIX_DD  0x02
#define PREFIX_ED  0x04
#define PREFIX_FD  0x08

#define MODE_NORMAL 0
#define MODE_DD     1
#define MODE_FD     2
#endif

#if 1	/* debug instruction decode? */
#define zprintf printf
#else
#define zprintf(...)
#endif


/* global register indexes and instruction counting routines */
static TCGv_ptr cpu_env;
#include "exec/gen-icount.h"


typedef struct DisasContext {
    /* current insn context */
    int override; /* -1 if no override */
    int prefix;
    uint16_t pc; /* pc = pc + cs_base */
    int is_jmp; /* 1 = means jump (stop translation), 2 means CPU
                   static state change (stop translation) */
//    int model;
    /* current block context */
    target_ulong cs_base; /* base of CS segment */
    int singlestep_enabled; /* "hardware" single step enabled */
    int jmp_opt; /* use direct block chaining for direct jumps */
    int flags; /* all execution flags */
    struct TranslationBlock *tb;
} DisasContext;

static inline void gen_jmp_im(target_ulong pc)
{
    gen_helper_movl_pc_im(cpu_env, tcg_const_tl(pc));
}


static void gen_exception(DisasContext *s, int trapno, target_ulong cur_pc)
{
    gen_jmp_im(cur_pc);
#if 0	/* obsolete by 0.15.0 */
    gen_helper_raise_exception(trapno);
#else	/* v0.15.0+ */
    gen_helper_raise_exception(cpu_env, tcg_const_i32(trapno));
#endif
    s->is_jmp = 3;
}

/* convert one instruction. s->is_jmp is set if the translation must
   be stopped. Return the next pc value */
static target_ulong disas_insn(CPUZ80State *env, DisasContext *s, target_ulong pc_start)
{
#if 1
    int b;

    /* reading at least one byte is critical to ensuring the
     * translation block has non-zero size
     */
    s->pc = pc_start;
    zprintf("PC = %04x: ", s->pc);

    b = cpu_ldub_code(env, s->pc);
    s->pc++;
    switch (b)
    {
    case 0xc9:  /* 'ret' */
        zprintf("ret (** HACK - EXCP_KERNEL_TRAP trigger **)\n");
        //gen_exception(s, EXCP06_ILLOP, pc_start - s->cs_base);
        gen_exception(s, EXCP_KERNEL_TRAP, pc_start /* - s->cs_base */);
        return s->pc;
    default:
        zprintf("byte 0x%02x (** HACK - illegal_op trigger **)\n", b);
        goto illegal_op;
    }
#else
    int b, prefixes;
    //int rex_w, rex_r;	/* unused [i386-specific?] */
    int m;
#if 1	/* WmT - TRACE */
;fprintf(stderr, "ENTER %s() - context %p, pc_start 0x%04x\n", __func__, s, pc_start);
#endif

    s->pc = pc_start;
    prefixes = 0;
    s->override = -1;
    //rex_w = -1;
    //rex_r = 0;

#if 1	/* WmT - TRACE */
;fprintf(stderr, "%s(): INFO - 'next_byte' label follows PC value dump...\n", __func__);
#endif
    zprintf("PC = %04x: ", s->pc);
//next_byte:
    s->prefix = prefixes;

/* START */

    if (prefixes & PREFIX_DD) {
        m = MODE_DD;
    } else if (prefixes & PREFIX_FD) {
        m = MODE_FD;
    } else {
        m = MODE_NORMAL;
    }

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

#if 1	/* WmT - HACK */
;fprintf(stderr, "[%s:%d] HACK - unprefixed opcode, byte 0x%02x (x %d, y %d, z %d, p %d, q %d) unhandled\n", __FILE__, __LINE__, b, x, y, z, p, q);
;goto illegal_op;
#else
;fprintf(stderr, "[%s:%d] PARTIAL - unprefixed opcode, byte 0x%02x (x %d, y %d, z %d, p %d, q %d) retrieved\n", __FILE__, __LINE__, b, x, y, z, p, q);
//...
#endif
//        switch (x) {
//        case 0:	/* instr pattern 00yyyzzz */
//            switch (z) {
//
//            case 0:
//                switch (y) {
//                case 0:
//                    zprintf("nop\n");
//                    break;
//                case 1:
//                    gen_ex(OR2_AF, OR2_AFX);
//                    zprintf("ex af,af'\n");
//                    break;
//                case 2:
//                    n = ldsb_code(s->pc);
//                    s->pc++;
//                    gen_helper_djnz(tcg_const_tl(s->pc + n), tcg_const_tl(s->pc));
//                    gen_eob(s);
//                    s->is_jmp = 3;
//                    zprintf("djnz $%02x\n", n);
//                    break;
//                case 3:
//                    n = ldsb_code(s->pc);
//                    s->pc++;
//                    gen_jmp_im(s->pc + n);
//                    gen_eob(s);
//                    s->is_jmp = 3;
//                    zprintf("jr $%02x\n", n);
//                    break;
//                case 4:
//                case 5:
//                case 6:
//                case 7:
//                    n = ldsb_code(s->pc);
//                    s->pc++;
//                    zprintf("jr %s,$%04x\n", cc[y-4], (s->pc + n) & 0xffff);
//                    gen_jcc(s, y-4, s->pc + n, s->pc);
//                    break;
//                }
//                break;
//
//            case 1:
//                switch (q) {
//                case 0:
//                    n = lduw_code(s->pc);
//                    s->pc += 2;
//                    tcg_gen_movi_tl(cpu_T[0], n);
//                    r1 = regpairmap(regpair[p], m);
//                    gen_movw_reg_v(r1, cpu_T[0]);
//                    zprintf("ld %s,$%04x\n", regpairnames[r1], n);
//                    break;
//                case 1:
//                    r1 = regpairmap(regpair[p], m);
//                    r2 = regpairmap(OR2_HL, m);
//                    gen_movw_v_reg(cpu_T[0], r1);
//                    gen_movw_v_reg(cpu_T[1], r2);
//                    gen_helper_addw_T0_T1_cc();
//                    gen_movw_reg_v(r2, cpu_T[0]);
//                    zprintf("add %s,%s\n", regpairnames[r2], regpairnames[r1]);
//                    break;
//                }
//                break;
//
//            case 2:
//                switch (q) {
//                case 0:
//                    switch (p) {
//                    case 0:
//                        gen_movb_v_A(cpu_T[0]);
//                        gen_movw_v_BC(cpu_A0);
//                        tcg_gen_qemu_st8(cpu_T[0], cpu_A0, MEM_INDEX);
//                        zprintf("ld (bc),a\n");
//                        break;
//                    case 1:
//                        gen_movb_v_A(cpu_T[0]);
//                        gen_movw_v_DE(cpu_A0);
//                        tcg_gen_qemu_st8(cpu_T[0], cpu_A0, MEM_INDEX);
//                        zprintf("ld (de),a\n");
//                        break;
//                    case 2:
//                        n = lduw_code(s->pc);
//                        s->pc += 2;
//                        r1 = regpairmap(OR2_HL, m);
//                        gen_movw_v_reg(cpu_T[0], r1);
//                        tcg_gen_movi_i32(cpu_A0, n);
//                        tcg_gen_qemu_st16(cpu_T[0], cpu_A0, MEM_INDEX);
//                        zprintf("ld ($%04x),%s\n", n, regpairnames[r1]);
//                        break;
//                    case 3:
//                        n = lduw_code(s->pc);
//                        s->pc += 2;
//                        gen_movb_v_A(cpu_T[0]);
//                        tcg_gen_movi_i32(cpu_A0, n);
//                        tcg_gen_qemu_st8(cpu_T[0], cpu_A0, MEM_INDEX);
//                        zprintf("ld ($%04x),a\n", n);
//                        break;
//                    }
//                    break;
//                case 1:
//                    switch (p) {
//                    case 0:
//                        gen_movw_v_BC(cpu_A0);
//                        tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
//                        gen_movb_A_v(cpu_T[0]);
//                        zprintf("ld a,(bc)\n");
//                        break;
//                    case 1:
//                        gen_movw_v_DE(cpu_A0);
//                        tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
//                        gen_movb_A_v(cpu_T[0]);
//                        zprintf("ld a,(de)\n");
//                        break;
//                    case 2:
//                        n = lduw_code(s->pc);
//                        s->pc += 2;
//                        r1 = regpairmap(OR2_HL, m);
//                        tcg_gen_movi_i32(cpu_A0, n);
//                        tcg_gen_qemu_ld16u(cpu_T[0], cpu_A0, MEM_INDEX);
//                        gen_movw_reg_v(r1, cpu_T[0]);
//                        zprintf("ld %s,($%04x)\n", regpairnames[r1], n);
//                        break;
//                    case 3:
//                        n = lduw_code(s->pc);
//                        s->pc += 2;
//                        tcg_gen_movi_i32(cpu_A0, n);
//                        tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
//                        gen_movb_A_v(cpu_T[0]);
//                        zprintf("ld a,($%04x)\n", n);
//                        break;
//                    }
//                    break;
//                }
//                break;
//
//            case 3:
//                switch (q) {
//                case 0:
//                    r1 = regpairmap(regpair[p], m);
//                    gen_movw_v_reg(cpu_T[0], r1);
//                    tcg_gen_addi_tl(cpu_T[0], cpu_T[0], 1);
//                    gen_movw_reg_v(r1, cpu_T[0]);
//                    zprintf("inc %s\n", regpairnames[r1]);
//                    break;
//                case 1:
//                    r1 = regpairmap(regpair[p], m);
//                    gen_movw_v_reg(cpu_T[0], r1);
//                    tcg_gen_subi_tl(cpu_T[0], cpu_T[0], 1);
//                    gen_movw_reg_v(r1, cpu_T[0]);
//                    zprintf("dec %s\n", regpairnames[r1]);
//                    break;
//                }
//                break;
//
//            case 4:
//                r1 = regmap(reg[y], m);
//                if (is_indexed(r1)) {
//                    d = ldsb_code(s->pc);
//                    s->pc++;
//                    gen_movb_v_idx(cpu_T[0], r1, d);
//                } else {
//                    gen_movb_v_reg(cpu_T[0], r1);
//                }
//                gen_helper_incb_T0_cc();
//                if (is_indexed(r1)) {
//                    gen_movb_idx_v(r1, cpu_T[0], d);
//                } else {
//                    gen_movb_reg_v(r1, cpu_T[0]);
//                }
//                if (is_indexed(r1)) {
//                    zprintf("inc (%s%c$%02x)\n", idxnames[r1], shexb(d));
//                } else {
//                    zprintf("inc %s\n", regnames[r1]);
//                }
//                break;
//
//            case 5:
//                r1 = regmap(reg[y], m);
//                if (is_indexed(r1)) {
//                    d = ldsb_code(s->pc);
//                    s->pc++;
//                    gen_movb_v_idx(cpu_T[0], r1, d);
//                } else {
//                    gen_movb_v_reg(cpu_T[0], r1);
//                }
//                gen_helper_decb_T0_cc();
//                if (is_indexed(r1)) {
//                    gen_movb_idx_v(r1, cpu_T[0], d);
//                } else {
//                    gen_movb_reg_v(r1, cpu_T[0]);
//                }
//                if (is_indexed(r1)) {
//                    zprintf("dec (%s%c$%02x)\n", idxnames[r1], shexb(d));
//                } else {
//                    zprintf("dec %s\n", regnames[r1]);
//                }
//                break;
//
//            case 6:
//                r1 = regmap(reg[y], m);
//                if (is_indexed(r1)) {
//                    d = ldsb_code(s->pc);
//                    s->pc++;
//                }
//                n = ldub_code(s->pc);
//                s->pc++;
//                tcg_gen_movi_tl(cpu_T[0], n);
//                if (is_indexed(r1)) {
//                    gen_movb_idx_v(r1, cpu_T[0], d);
//                } else {
//                    gen_movb_reg_v(r1, cpu_T[0]);
//                }
//                if (is_indexed(r1)) {
//                    zprintf("ld (%s%c$%02x),$%02x\n", idxnames[r1], shexb(d), n);
//                } else {
//                    zprintf("ld %s,$%02x\n", regnames[r1], n);
//                }
//                break;
//
//            case 7:
//                switch (y) {
//                case 0:
//                    gen_helper_rlca_cc();
//                    zprintf("rlca\n");
//                    break;
//                case 1:
//                    gen_helper_rrca_cc();
//                    zprintf("rrca\n");
//                    break;
//                case 2:
//                    gen_helper_rla_cc();
//                    zprintf("rla\n");
//                    break;
//                case 3:
//                    gen_helper_rra_cc();
//                    zprintf("rra\n");
//                    break;
//                case 4:
//                    gen_helper_daa_cc();
//                    zprintf("daa\n");
//                    break;
//                case 5:
//                    gen_helper_cpl_cc();
//                    zprintf("cpl\n");
//                    break;
//                case 6:
//                    gen_helper_scf_cc();
//                    zprintf("scf\n");
//                    break;
//                case 7:
//                    gen_helper_ccf_cc();
//                    zprintf("ccf\n");
//                    break;
//                }
//                break;
//            }
//            break;
//
//        case 1:	/* instr pattern 01yyyzzz */
//            if (z == 6 && y == 6) {
//                gen_jmp_im(s->pc);
//                gen_helper_halt();
//                zprintf("halt\n");
//            } else {
//                if (z == 6) {
//                    r1 = regmap(reg[z], m);
//                    r2 = regmap(reg[y], 0);
//                } else if (y == 6) {
//                    r1 = regmap(reg[z], 0);
//                    r2 = regmap(reg[y], m);
//                } else {
//                    r1 = regmap(reg[z], m);
//                    r2 = regmap(reg[y], m);
//                }
//                if (is_indexed(r1) || is_indexed(r2)) {
//                    d = ldsb_code(s->pc);
//                    s->pc++;
//                }
//                if (is_indexed(r1)) {
//                    gen_movb_v_idx(cpu_T[0], r1, d);
//                } else {
//                    gen_movb_v_reg(cpu_T[0], r1);
//                }
//                if (is_indexed(r2)) {
//                    gen_movb_idx_v(r2, cpu_T[0], d);
//                } else {
//                    gen_movb_reg_v(r2, cpu_T[0]);
//                }
//                if (is_indexed(r1)) {
//                    zprintf("ld %s,(%s%c$%02x)\n", regnames[r2], idxnames[r1], shexb(d));
//                } else if (is_indexed(r2)) {
//                    zprintf("ld (%s%c$%02x),%s\n", idxnames[r2], shexb(d), regnames[r1]);
//                } else {
//                    zprintf("ld %s,%s\n", regnames[r2], regnames[r1]);
//                }
//            }
//            break;
//
//        case 2:	/* instr pattern 10yyyzzz */
//            r1 = regmap(reg[z], m);
//            if (is_indexed(r1)) {
//                d = ldsb_code(s->pc);
//                s->pc++;
//                gen_movb_v_idx(cpu_T[0], r1, d);
//            } else {
//                gen_movb_v_reg(cpu_T[0], r1);
//            }
//            gen_alu[y](); /* places output in A */
//            if (is_indexed(r1)) {
//                zprintf("%s(%s%c$%02x)\n", alu[y], idxnames[r1], shexb(d));
//            } else {
//                zprintf("%s%s\n", alu[y], regnames[r1]);
//            }
//            break;
//
//        case 3:	/* instr pattern 11yyyzzz */
//            switch (z) {
//            case 0:
//                gen_retcc(s, y, s->pc);
//                zprintf("ret %s\n", cc[y]);
//                break;
//            case 1:
//                switch (q) {
//                case 0:
//                    r1 = regpairmap(regpair2[p], m);
//                    gen_popw(cpu_T[0]);
//                    gen_movw_reg_v(r1, cpu_T[0]);
//                    zprintf("pop %s\n", regpairnames[r1]);
//                    break;
//                case 1:
//                    switch (p) {
//                    case 0:
//                        gen_popw(cpu_T[0]);
//                        gen_helper_jmp_T0();
//                        zprintf("ret\n");
//                        gen_eob(s);
//                        s->is_jmp = 3;
////                      s->is_ei = 1;
//                        break;
//
//                    case 1:
//                        gen_ex(OR2_BC, OR2_BCX);
//                        gen_ex(OR2_DE, OR2_DEX);
//                        gen_ex(OR2_HL, OR2_HLX);
//                        zprintf("exx\n");
//                        break;
//                    case 2:
//                        r1 = regpairmap(OR2_HL, m);
//                        gen_movw_v_reg(cpu_T[0], r1);
//                        gen_helper_jmp_T0();
//                        zprintf("jp %s\n", regpairnames[r1]);
//                        gen_eob(s);
//                        s->is_jmp = 3;
//                        break;
//                    case 3:
//                        r1 = regpairmap(OR2_HL, m);
//                        gen_movw_v_reg(cpu_T[0], r1);
//                        gen_movw_SP_v(cpu_T[0]);
//                        zprintf("ld sp,%s\n", regpairnames[r1]);
//                        break;
//                    }
//                    break;
//                }
//                break;
//
//            case 2:
//                n = lduw_code(s->pc);
//                s->pc += 2;
//                gen_jcc(s, y, n, s->pc);
//                zprintf("jp %s,$%04x\n", cc[y], n);
//                break;
//
//            case 3:
//                switch (y) {
//                case 0:
//                    n = lduw_code(s->pc);
//                    s->pc += 2;
//                    gen_jmp_im(n);
//                    zprintf("jp $%04x\n", n);
//                    gen_eob(s);
//                    s->is_jmp = 3;
//                    break;
//                case 1:
//                    zprintf("cb prefix\n");
//                    prefixes |= PREFIX_CB;
//                    goto next_byte;
//                    break;
//                case 2:
//                    n = ldub_code(s->pc);
//                    s->pc++;
//                    gen_movb_v_A(cpu_T[0]);
//                    if (use_icount) {
//                        gen_io_start();
//                    }
//                    gen_helper_out_T0_im(tcg_const_tl(n));
//                    if (use_icount) {
//                        gen_io_end();
//                        gen_jmp_im(s->pc);
//                    }
//                    zprintf("out ($%02x),a\n", n);
//                    break;
//                case 3:
//                    n = ldub_code(s->pc);
//                    s->pc++;
//                    if (use_icount) {
//                        gen_io_start();
//                    }
//                    gen_helper_in_T0_im(tcg_const_tl(n));
//                    gen_movb_A_v(cpu_T[0]);
//                    if (use_icount) {
//                        gen_io_end();
//                        gen_jmp_im(s->pc);
//                    }
//                    zprintf("in a,($%02x)\n", n);
//                    break;
//                case 4:
//                    r1 = regpairmap(OR2_HL, m);
//                    gen_popw(cpu_T[1]);
//                    gen_movw_v_reg(cpu_T[0], r1);
//                    gen_pushw(cpu_T[0]);
//                    gen_movw_reg_v(r1, cpu_T[1]);
//                    zprintf("ex (sp),%s\n", regpairnames[r1]);
//                    break;
//                case 5:
//                    gen_ex(OR2_DE, OR2_HL);
//                    zprintf("ex de,hl\n");
//                    break;
//                case 6:
//                    gen_helper_di();
//                    zprintf("di\n");
//                    break;
//                case 7:
//                    gen_helper_ei();
//                    zprintf("ei\n");
////                  gen_eob(s);
////                  s->is_ei = 1;
//                    break;
//                }
//                break;
//
//            case 4:
//                n = lduw_code(s->pc);
//                s->pc += 2;
//                gen_callcc(s, y, n, s->pc);
//                zprintf("call %s,$%04x\n", cc[y], n);
//                break;
//
//            case 5:
//                switch (q) {
//                case 0:
//                    r1 = regpairmap(regpair2[p], m);
//                    gen_movw_v_reg(cpu_T[0], r1);
//                    gen_pushw(cpu_T[0]);
//                    zprintf("push %s\n", regpairnames[r1]);
//                    break;
//                case 1:
//                    switch (p) {
//                    case 0:
//                        n = lduw_code(s->pc);
//                        s->pc += 2;
//                        tcg_gen_movi_tl(cpu_T[0], s->pc);
//                        gen_pushw(cpu_T[0]);
//                        gen_jmp_im(n);
//                        zprintf("call $%04x\n", n);
//                        gen_eob(s);
//                        s->is_jmp = 3;
//                        break;
//                    case 1:
//                        zprintf("dd prefix\n");
//                        prefixes |= PREFIX_DD;
//                        goto next_byte;
//                        break;
//                    case 2:
//                        zprintf("ed prefix\n");
//                        prefixes |= PREFIX_ED;
//                        goto next_byte;
//                        break;
//                    case 3:
//                        zprintf("fd prefix\n");
//                        prefixes |= PREFIX_FD;
//                        goto next_byte;
//                        break;
//                    }
//                    break;
//                }
//                break;
//
//            case 6:
//                n = ldub_code(s->pc);
//                s->pc++;
//                tcg_gen_movi_tl(cpu_T[0], n);
//                gen_alu[y](); /* places output in A */
//                zprintf("%s$%02x\n", alu[y], n);
//                break;
//
//            case 7:
//                tcg_gen_movi_tl(cpu_T[0], s->pc);
//                gen_pushw(cpu_T[0]);
//                gen_jmp_im(y*8);
//                zprintf("rst $%02x\n", y*8);
//                gen_eob(s);
//                s->is_jmp = 3;
//                break;
//            }
//            break;
//        }
    } else if (prefixes & PREFIX_CB) {
        /* cb mode: */

        int x, y, z, p, q;
        int d;
        int r1, r2;

        if (m != MODE_NORMAL) {
            /* 0xDD 0xCB OFFS OP or 0xFD 0xCB OFFS OP */
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

#if 1	/* WmT - HACK */
;fprintf(stderr, "[%s:%d] HACK - PREFIX_CB case, byte 0x%02x (x %d, y %d, z %d, p %d, q %d) -> unhandled\n", __FILE__, __LINE__, b, x, y, z, p, q);
;goto illegal_op;
#else
;fprintf(stderr, "[%s:%d] PARTIAL - PREFIX_CB case, byte 0x%02x (x %d, y %d, z %d, p %d, q %d) retrieved\n", __FILE__, __LINE__, b, x, y, z, p, q);
#endif
//        if (m != MODE_NORMAL) {
//            r1 = regmap(OR_HLmem, m);
//            gen_movb_v_idx(cpu_T[0], r1, d);
//            if (z != 6) {
//                r2 = regmap(reg[z], 0);
//            }
//        } else {
//            r1 = regmap(reg[z], m);
//            gen_movb_v_reg(cpu_T[0], r1);
//        }
//
//        switch (x) {
//        case 0:
//            /* TODO: TST instead of SLL for R800 */
//            gen_rot_T0[y]();
//            if (m != MODE_NORMAL) {
//                gen_movb_idx_v(r1, cpu_T[0], d);
//                if (z != 6) {
//                    gen_movb_reg_v(r2, cpu_T[0]);
//                }
//            } else {
//                gen_movb_reg_v(r1, cpu_T[0]);
//            }
//            zprintf("%s %s\n", rot[y], regnames[r1]);
//            break;
//        case 1:
//            gen_helper_bit_T0(tcg_const_tl(1 << y));
//            zprintf("bit %i,%s\n", y, regnames[r1]);
//            break;
//        case 2:
//            tcg_gen_andi_tl(cpu_T[0], cpu_T[0], ~(1 << y));
//            if (m != MODE_NORMAL) {
//                gen_movb_idx_v(r1, cpu_T[0], d);
//                if (z != 6) {
//                    gen_movb_reg_v(r2, cpu_T[0]);
//                }
//            } else {
//                gen_movb_reg_v(r1, cpu_T[0]);
//            }
//            zprintf("res %i,%s\n", y, regnames[r1]);
//            break;
//        case 3:
//            tcg_gen_ori_tl(cpu_T[0], cpu_T[0], 1 << y);
//            if (m != MODE_NORMAL) {
//                gen_movb_idx_v(r1, cpu_T[0], d);
//                if (z != 6) {
//                    gen_movb_reg_v(r2, cpu_T[0]);
//                }
//            } else {
//                gen_movb_reg_v(r1, cpu_T[0]);
//            }
//            zprintf("set %i,%s\n", y, regnames[r1]);
//            break;
//        }
//
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

#if 1	/* WmT - HACK */
;fprintf(stderr, "[%s:%d] HACK - PREFIX_ED case, byte 0x%02x (x %d, y %d, z %d, p %d, q %d) -> unhandled\n", __FILE__, __LINE__, b, x, y, z, p, q);
;goto illegal_op;
#else
;fprintf(stderr, "[%s:%d] PARTIAL - PREFIX_ED case, byte 0x%02x (x %d, y %d, z %d, p %d, q %d) retrieved\n", __FILE__, __LINE__, b, x, y, z, p, q);
#endif
//        switch (x) {
//        case 0:
//            zprintf("nop\n");
//            break;
//        case 3:
//            if (s->model == Z80_CPU_R800) {
//                switch (z) {
//                case 1:
//                    /* does mulub work with r1 == h, l, (hl) or a? */
//                    r1 = regmap(reg[y], m);
//                    gen_movb_v_reg(cpu_T[0], r1);
//                    gen_helper_mulub_cc();
//                    zprintf("mulub a,%s\n", regnames[r1]);
//                    break;
//                case 3:
//                    if (q == 0) {
//                        /* does muluw work with r1 == de or hl? */
//                        /* what is the effect of DD/FD prefixes here? */
//                        r1 = regpairmap(regpair[p], m);
//                        gen_movw_v_reg(cpu_T[0], r1);
//                        gen_helper_muluw_cc();
//                        zprintf("muluw hl,%s\n", regpairnames[r1]);
//                    } else {
//                        zprintf("nop\n");
//                    }
//                    break;
//                default:
//                    zprintf("nop\n");
//                    break;
//                }
//            } else {
//                zprintf("nop\n");
//            }
//            break;
//
//        case 1:
//            switch (z) {
//            case 0:
//                if (use_icount) {
//                    gen_io_start();
//                }
//                gen_helper_in_T0_bc_cc();
//                if (y != 6) {
//                    r1 = regmap(reg[y], m);
//                    gen_movb_reg_v(r1, cpu_T[0]);
//                    zprintf("in %s,(c)\n", regnames[r1]);
//                } else {
//                    zprintf("in (c)\n");
//                }
//                if (use_icount) {
//                    gen_io_end();
//                    gen_jmp_im(s->pc);
//                }
//                break;
//            case 1:
//                if (y != 6) {
//                    r1 = regmap(reg[y], m);
//                    gen_movb_v_reg(cpu_T[0], r1);
//                    zprintf("out (c),%s\n", regnames[r1]);
//                } else {
//                    tcg_gen_movi_tl(cpu_T[0], 0);
//                    zprintf("out (c),0\n");
//                }
//                if (use_icount) {
//                    gen_io_start();
//                }
//                gen_helper_out_T0_bc();
//                if (use_icount) {
//                    gen_io_end();
//                    gen_jmp_im(s->pc);
//                }
//                break;
//            case 2:
//                r1 = regpairmap(OR2_HL, m);
//                r2 = regpairmap(regpair[p], m);
//                gen_movw_v_reg(cpu_T[0], r1);
//                gen_movw_v_reg(cpu_T[1], r2);
//                if (q == 0) {
//                    zprintf("sbc %s,%s\n", regpairnames[r1], regpairnames[r2]);
//                    gen_helper_sbcw_T0_T1_cc();
//                } else {
//                    zprintf("adc %s,%s\n", regpairnames[r1], regpairnames[r2]);
//                    gen_helper_adcw_T0_T1_cc();
//                }
//                gen_movw_reg_v(r1, cpu_T[0]);
//                break;
//            case 3:
//                n = lduw_code(s->pc);
//                s->pc += 2;
//                r1 = regpairmap(regpair[p], m);
//                if (q == 0) {
//                    gen_movw_v_reg(cpu_T[0], r1);
//                    tcg_gen_movi_i32(cpu_A0, n);
//                    tcg_gen_qemu_st16(cpu_T[0], cpu_A0, MEM_INDEX);
//                    zprintf("ld ($%02x),%s\n", n, regpairnames[r1]);
//                } else {
//                    tcg_gen_movi_i32(cpu_A0, n);
//                    tcg_gen_qemu_ld16u(cpu_T[0], cpu_A0, MEM_INDEX);
//                    gen_movw_reg_v(r1, cpu_T[0]);
//                    zprintf("ld %s,($%02x)\n", regpairnames[r1], n);
//                }
//                break;
//            case 4:
//                zprintf("neg\n");
//                gen_helper_neg_cc();
//                break;
//            case 5:
//                /* FIXME */
//                gen_popw(cpu_T[0]);
//                gen_helper_jmp_T0();
//                gen_helper_ri();
//                if (q == 0) {
//                    zprintf("retn\n");
//                } else {
//                    zprintf("reti\n");
//                }
//                gen_eob(s);
//                s->is_jmp = 3;
////              s->is_ei = 1;
//                break;
//            case 6:
//                gen_helper_imode(tcg_const_tl(imode[y]));
//                zprintf("im im[%i]\n", imode[y]);
////              gen_eob(s);
////              s->is_ei = 1;
//                break;
//            case 7:
//                switch (y) {
//                case 0:
//                    gen_helper_ld_I_A();
//                    zprintf("ld i,a\n");
//                    break;
//                case 1:
//                    gen_helper_ld_R_A();
//                    zprintf("ld r,a\n");
//                    break;
//                case 2:
//                    gen_helper_ld_A_I();
//                    zprintf("ld a,i\n");
//                    break;
//                case 3:
//                    gen_helper_ld_A_R();
//                    zprintf("ld a,r\n");
//                    break;
//                case 4:
//                    gen_movb_v_HLmem(cpu_T[0]);
//                    gen_helper_rrd_cc();
//                    gen_movb_HLmem_v(cpu_T[0]);
//                    zprintf("rrd\n");
//                    break;
//                case 5:
//                    gen_movb_v_HLmem(cpu_T[0]);
//                    gen_helper_rld_cc();
//                    gen_movb_HLmem_v(cpu_T[0]);
//                    zprintf("rld\n");
//                    break;
//                case 6:
//                case 7:
//                    zprintf("nop2\n");
//                    /* nop */
//                    break;
//                }
//                break;
//            }
//            break;
//
//        case 2:
//            /* FIXME */
//            if (y >= 4) {
//                switch (z) {
//                case 0: /* ldi/ldd/ldir/lddr */
//                    gen_movw_v_HL(cpu_A0);
//                    tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
//                    gen_movw_v_DE(cpu_A0);
//                    tcg_gen_qemu_st8(cpu_T[0], cpu_A0, MEM_INDEX);
//
//                    if (!(y & 1)) {
//                        gen_helper_bli_ld_inc_cc();
//                    } else {
//                        gen_helper_bli_ld_dec_cc();
//                    }
//                    if ((y & 2)) {
//                        gen_helper_bli_ld_rep(tcg_const_tl(s->pc));
//                        gen_eob(s);
//                        s->is_jmp = 3;
//                    }
//                    break;
//
//                case 1: /* cpi/cpd/cpir/cpdr */
//                    gen_movw_v_HL(cpu_A0);
//                    tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
//                    gen_helper_bli_cp_cc();
//
//                    if (!(y & 1)) {
//                        gen_helper_bli_cp_inc_cc();
//                    } else {
//                        gen_helper_bli_cp_dec_cc();
//                    }
//                    if ((y & 2)) {
//                        gen_helper_bli_cp_rep(tcg_const_tl(s->pc));
//                        gen_eob(s);
//                        s->is_jmp = 3;
//                    }
//                    break;
//
//                case 2: /* ini/ind/inir/indr */
//                    if (use_icount) {
//                        gen_io_start();
//                    }
//                    gen_helper_in_T0_bc_cc();
//                    if (use_icount) {
//                        gen_io_end();
//                    }
//                    gen_movw_v_HL(cpu_A0);
//                    tcg_gen_qemu_st8(cpu_T[0], cpu_A0, MEM_INDEX);
//                    if (!(y & 1)) {
//                        gen_helper_bli_io_T0_inc(0);
//                    } else {
//                        gen_helper_bli_io_T0_dec(0);
//                    }
//                    if ((y & 2)) {
//                        gen_helper_bli_io_rep(tcg_const_tl(s->pc));
//                        gen_eob(s);
//                        s->is_jmp = 3;
//                    } else if (use_icount) {
//                        gen_jmp_im(s->pc);
//                    }
//                    break;
//
//                case 3: /* outi/outd/otir/otdr */
//                    gen_movw_v_HL(cpu_A0);
//                    tcg_gen_qemu_ld8u(cpu_T[0], cpu_A0, MEM_INDEX);
//                    if (use_icount) {
//                        gen_io_start();
//                    }
//                    gen_helper_out_T0_bc();
//                    if (use_icount) {
//                        gen_io_end();
//                    }
//                    if (!(y & 1)) {
//                        gen_helper_bli_io_T0_inc(1);
//                    } else {
//                        gen_helper_bli_io_T0_dec(1);
//                    }
//                    if ((y & 2)) {
//                        gen_helper_bli_io_rep(tcg_const_tl(s->pc));
//                        gen_eob(s);
//                        s->is_jmp = 3;
//                    } else if (use_icount) {
//                        gen_jmp_im(s->pc);
//                    }
//                    break;
//                }
//
//                zprintf("%s\n", bli[y-4][z]);
//                break;
//            }
//        }
    }

//    prefixes = 0;
//
//    /* now check op code */
//#if 1	/* WmT - INFO */
//;fprintf(stderr, "** %s() INFO - omitted (? intended?) further illegal op test based on op=0x%02x **\n", __func__, b);
//#endif
////    switch (b) {
////    default:
////        goto illegal_op;
////    }
//    /* lock generation */
//#if 1        /* WmT - TRACE */
//;fprintf(stderr, "EXIT %s() - opcode valid, will return s->pc=0x%04x\n", __func__, s->pc);
//#endif
//    return s->pc;
#endif

 illegal_op:
#if 1	/* WmT - TRACE */
;fprintf(stderr, "EXIT %s() - via gen_exception() for EXCP06_ILLOP (trapnr=%d) [ret s->pc=0x%04x]\n", __func__, EXCP06_ILLOP, s->pc);
#endif
    /* XXX: ensure that no lock was generated */
    //gen_exception(s, EXCP06_ILLOP, pc_start - s->cs_base);
    gen_exception(s, EXCP06_ILLOP, pc_start /* - s->cs_base */);
    return s->pc;
}

///* generate intermediate code in gen_opc_buf and gen_opparam_buf for
//   basic block 'tb'. If search_pc is TRUE, also generate PC
//   information for each intermediate instruction. */
static inline void gen_intermediate_code_internal(Z80CPU *cpu,
                                                 TranslationBlock *tb,
                                                 int search_pc)
{
    CPUState *cs = CPU(cpu);
    CPUZ80State *env = &cpu->env;
    DisasContext dc1, *dc = &dc1;
    target_ulong pc_ptr;
    uint16_t *gen_opc_end;
//    CPUBreakpoint *bp;
    int flags, j, lj /* , cflags - set but unused */ ;
    target_ulong pc_start;
    target_ulong cs_base;
    int num_insns;
    int max_insns;
#if 1	/* WmT - TRACE */
;DPRINTF("*** ENTER %s() ****\n", __func__);
#endif

    /* generate intermediate code */
    pc_start = tb->pc;
#if 1	/* WmT - TRACE */
;fprintf(stderr, "%s(): set pc_start to tb->pc 0x%04x\n", __func__, pc_start);
#endif
    cs_base = tb->cs_base;
    flags = tb->flags;
    //cflags = tb->cflags;

    dc->singlestep_enabled = cs->singlestep_enabled;
    dc->cs_base = cs_base;
    dc->tb = tb;
    dc->flags = flags;
    dc->jmp_opt = !(cs->singlestep_enabled ||
                    (flags & HF_INHIBIT_IRQ_MASK)
#ifndef CONFIG_SOFTMMU
                    || (flags & HF_SOFTMMU_MASK)
#endif
                    );

    gen_opc_ptr = gen_opc_buf;
#if 1	/* WmT - TRACE */
;DPRINTF("%s(): set gen_opc_ptr: to gen_opc_buf value %p\n", __func__, gen_opc_ptr);
#endif
    gen_opc_end = gen_opc_buf + OPC_MAX_SIZE;
    gen_opparam_ptr = gen_opparam_buf;

    dc->is_jmp = DISAS_NEXT;
    pc_ptr = pc_start;
#if 1	/* WmT - TRACE */
;fprintf(stderr, "%s(): set pc_ptr <- pc_start 0x%04x\n", __func__, pc_ptr);
#endif
    lj = -1;
#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - not considering model (missing in env?)\n", __func__);
#endif
//    dc->model = env->model;

    num_insns = 0;
    max_insns = tb->cflags & CF_COUNT_MASK;
    if (max_insns == 0) {
        max_insns = CF_COUNT_MASK;
    }
#if 1	/* WmT - TRACE */
;DPRINTF("[%s:%d] decided on max_insns %d\n", __FILE__, __LINE__, max_insns);
#endif

    gen_tb_start();
    for (;;) {
#if 1	/* WmT - TRACE */
;DPRINTF("[%s:%d] PARTIAL - ignoring breakpoints\n", __FILE__, __LINE__);
#else
        if (unlikely(!QTAILQ_EMPTY(&env->breakpoints))) {
            QTAILQ_FOREACH(bp, &env->breakpoints, entry) {
                if (bp->pc == pc_ptr) {
                    gen_debug(dc, pc_ptr - dc->cs_base);
                    break;
                }
            }
        }
#endif
        if (search_pc) {
            j = gen_opc_ptr - gen_opc_buf;
            if (lj < j) {
                lj++;
                while (lj < j) {
                    gen_opc_instr_start[lj++] = 0;
                }
            }
            gen_opc_pc[lj] = pc_ptr;
            gen_opc_instr_start[lj] = 1;
            gen_opc_icount[lj] = num_insns;
        }
        if (num_insns + 1 == max_insns && (tb->cflags & CF_LAST_IO)) {
            gen_io_start();
        }

        pc_ptr = disas_insn(env, dc, pc_ptr);
        num_insns++;
        /* stop translation if indicated */
        if (dc->is_jmp) {
#if 1	/* WmT - TRACE */
;DPRINTF("%s(): is_jmp %d -> stop translation\n", __func__, dc->is_jmp);
#endif
            break;
        }
        /* if single step mode, we generate only one instruction and
           generate an exception */
        /* if irq were inhibited with HF_INHIBIT_IRQ_MASK, we clear
           the flag and abort the translation to give the irqs a
           change to be happen */

        if (dc->singlestep_enabled ||
            (flags & HF_INHIBIT_IRQ_MASK)) {
            gen_jmp_im(pc_ptr - dc->cs_base);
            gen_eob(dc);
            break;
        }

        /* if too long translation, stop generation too */
        if (gen_opc_ptr >= gen_opc_end ||
            (pc_ptr - pc_start) >= (TARGET_PAGE_SIZE - 32) ||
            num_insns >= max_insns) {
            gen_jmp_im(pc_ptr - dc->cs_base);
            gen_eob(dc);
            break;
        }
        if (singlestep) {
            gen_jmp_im(pc_ptr - dc->cs_base);
            gen_eob(dc);
            break;
        }
    }
    if (tb->cflags & CF_LAST_IO) {
        gen_io_end();
    }
    gen_tb_end(tb, num_insns);
    *gen_opc_ptr = INDEX_op_end;
    /* we don't forget to fill the last values */
    if (search_pc) {
        j = gen_opc_ptr - gen_opc_buf;
        lj++;
        while (lj <= j) {
            gen_opc_instr_start[lj++] = 0;
        }
    }

#ifdef DEBUG_DISAS
;DPRINTF("** %s(): PARTIAL - handle DEBUG_DISAS via log_target_disas() **\n", __func__);
//    log_cpu_state_mask(CPU_LOG_TB_CPU, env, 0);
//    if (qemu_loglevel_mask(CPU_LOG_TB_IN_ASM)) {
//        qemu_log("----------------\n");
//        qemu_log("IN: %s\n", lookup_symbol(pc_start));
//        log_target_disas(pc_start, pc_ptr - pc_start, 0);
//        qemu_log("\n");
//    }
#endif

    if (!search_pc) {
        tb->size = pc_ptr - pc_start;
        tb->icount = num_insns;
    }
#if 1	/* WmT - TRACE */
;DPRINTF("*** EXIT %s(), OK ***\n", __func__);
#endif
}

void gen_intermediate_code(CPUZ80State *env, TranslationBlock *tb)
{
    gen_intermediate_code_internal(z80_env_get_cpu(env), tb, false);
}

void gen_intermediate_code_pc(CPUZ80State *env, TranslationBlock *tb)
{
    gen_intermediate_code_internal(z80_env_get_cpu(env), tb, true);
}

void restore_state_to_opc(CPUZ80State *env, TranslationBlock *tb, int pc_pos)
{
#if 1
;DPRINTF("%s() UNIMPLEMENTED\n", __func__);
;exit(1);
#else   /* Suitable for X86 */
    int cc_op;
#ifdef DEBUG_DISAS
    if (qemu_loglevel_mask(CPU_LOG_TB_OP)) {
        int i;
        qemu_log("RESTORE:\n");
        for(i = 0;i <= pc_pos; i++) {
            if (tcg_ctx.gen_opc_instr_start[i]) {
                qemu_log("0x%04x: " TARGET_FMT_lx "\n", i,
                        tcg_ctx.gen_opc_pc[i]);
            }
        }
        qemu_log("pc_pos=0x%x eip=" TARGET_FMT_lx " cs_base=%x\n",
                pc_pos, tcg_ctx.gen_opc_pc[pc_pos] - tb->cs_base,
                (uint32_t)tb->cs_base);
    }
#endif
    env->eip = tcg_ctx.gen_opc_pc[pc_pos] - tb->cs_base;
    cc_op = gen_opc_cc_op[pc_pos];
    if (cc_op != CC_OP_DYNAMIC)
        env->cc_op = cc_op;
#endif
}
