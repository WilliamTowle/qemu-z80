/*
 * QEmu Z80 CPU - op helpers
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

#include "cpu.h"
#include "helper.h"


#include "exec/ioport.h"

#if 1  /* was: TARGET_LONG_BITS > HOST_LONG_BITS
        * but TCG_AREG{1|2} no longer available for else case
        */

/* no registers can be used */
#define T0 (env->t0)
#define T1 (env->t1)

#else
/* XXX: use unsigned long instead of target_ulong - better code will
   be generated for 64 bit CPUs */
target_ulong T0;
//register target_ulong T0 asm(AREG1);
target_ulong T1;
//register target_ulong T1 asm(AREG2);
#endif /* ! (TARGET_LONG_BITS > HOST_LONG_BITS) */

#define A0 (env->a0)

#define A   (env->regs[R_A])
#define F   (env->regs[R_F])
#define BC  (env->regs[R_BC])
#define DE  (env->regs[R_DE])
#define HL  (env->regs[R_HL])
#define IX  (env->regs[R_IX])
#define IY  (env->regs[R_IY])
#define SP  (env->regs[R_SP])
#define I   (env->regs[R_I])
#define R   (env->regs[R_R])
#define AX  (env->regs[R_AX])
#define FX  (env->regs[R_FX])
#define BCX (env->regs[R_BCX])
#define DEX (env->regs[R_DEX])
#define HLX (env->regs[R_HLX])

#define PC  (env->pc)


const uint8_t parity_table[256] = {
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
};



//void HELPER(set_inhibit_irq)(CPUZ80State *env)
//{
//    env->hflags |= HF_INHIBIT_IRQ_MASK;
//}

void HELPER(reset_inhibit_irq)(CPUZ80State *env)
{
    env->hflags &= ~HF_INHIBIT_IRQ_MASK;
}


void HELPER(movl_pc_im)(CPUZ80State *env, uint32_t new_pc)
{
    PC = (uint16_t)new_pc;
}


/* Z80 instruction-specific helpers */

/* In / Out */

void HELPER(in_T0_im)(CPUZ80State *env, uint32_t val)
{
#if 0   /* github.com/legumbre: "[v0.10.x] segfaults when A non-zero" */
    //    T0 = cpu_inb(env, (A << 8) | val);
    T0 = cpu_inb(env, val);
#else   /* QEmu v1.x ioport-user.c API */
    T0 = cpu_inb(val);
#endif
}

void HELPER(in_T0_bc_cc)(CPUZ80State *env)
{
    int sf, zf, pf;

    //T0 = cpu_inb(env, BC);
    T0 = cpu_inb(BC);

    sf = (T0 & 0x80) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;
    pf = parity_table[(uint8_t)T0];
    F = (F & CC_C) | sf | zf | pf;
}

void HELPER(out_T0_im)(CPUZ80State *env, uint32_t val)
{
#if 0   /* github.com/legumbre: "[v0.10.x] segfaults when A non-zero" */
    // cpu_outb(env, (A << 8) | val, T0);
    cpu_outb(env, val, T0);
#else   /* QEmu v1.x ioport-user.c API */
    cpu_outb(val, T0);
#endif
}

void HELPER(out_T0_bc)(CPUZ80State *env)
{
    //cpu_outb(env, BC, T0);
    cpu_outb(BC, T0);
}

/* Misc */

void HELPER(bit_T0)(CPUZ80State *env, uint32_t val)
{
    int sf, zf, pf;

    sf = (T0 & val & 0x80) ? CC_S : 0;
    zf = (T0 & val) ? 0 : CC_Z;
    pf = (T0 & val) ? 0 : CC_P;
    F = (F & CC_C) | sf | zf | CC_H | pf;
}

void HELPER(jmp_T0)(CPUZ80State *env)
{
    PC = T0;
}

void HELPER(djnz)(CPUZ80State *env, uint32_t pc1, uint32_t pc2)
{
    BC = (uint16_t)(BC - 0x0100);
    if (BC & 0xff00) {
        PC = (uint16_t)pc1;
    } else {
        PC = (uint16_t)pc2;
    }
}

/* Arithmetic/logic operations */

#define signed_overflow_add(op1, op2, res, size) \
    (!!((~(op1 ^ op2) & (op1 ^ res)) >> (size - 1)))

#define signed_overflow_sub(op1, op2, res, size) \
    (!!(((op1 ^ op2) & (op1 ^ res)) >> (size - 1)))

void HELPER(add_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf, cf;
    int tmp = A;
    int carry;

    A = (uint8_t)(A + T0);
    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    carry = (tmp & T0) | ((tmp | T0) & ~A);
    hf = (carry & 0x08) ? CC_H : 0;
    pf = signed_overflow_add(tmp, T0, A, 8) ? CC_P : 0;
    cf = (carry & 0x80) ? CC_C : 0;

    F = sf | zf | hf | pf | cf;
}

void HELPER(adc_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf, cf;
    int tmp = A;
    int carry;

    A = (uint8_t)(A + T0 + !!(F & CC_C));
    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    carry = (tmp & T0) | ((tmp | T0) & ~A);
    hf = (carry & 0x08) ? CC_H : 0;
    pf = signed_overflow_add(tmp, T0, A, 8) ? CC_P : 0;
    cf = (carry & 0x80) ? CC_C : 0;

    F = sf | zf | hf | pf | cf;
}

void HELPER(sub_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf, cf;
    int tmp = A;
    int carry;

    A = (uint8_t)(A - T0);
    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    carry = (~tmp & T0) | (~(tmp ^ T0) & A);
    hf = (carry & 0x08) ? CC_H : 0;
    pf = signed_overflow_sub(tmp, T0, A, 8) ? CC_P : 0;
    cf = (carry & 0x80) ? CC_C : 0;

    F = sf | zf | hf | pf | CC_N | cf;
}

void HELPER(sbc_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf, cf;
    int tmp = A;
    int carry;

    A = (uint8_t)(A - T0 - !!(F & CC_C));
    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    carry = (~tmp & T0) | (~(tmp ^ T0) & A);
    hf = (carry & 0x08) ? CC_H : 0;
    pf = signed_overflow_sub(tmp, T0, A, 8) ? CC_P : 0;
    cf = (carry & 0x80) ? CC_C : 0;

    F = sf | zf | hf | pf | CC_N | cf;
}

void HELPER(and_cc)(CPUZ80State *env)
{
    int sf, zf, pf;
    A = (uint8_t)(A & T0);

    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = parity_table[(uint8_t)A];
    F = sf | zf | CC_H | pf;
}

void HELPER(xor_cc)(CPUZ80State *env)
{
    int sf, zf, pf;
    A = (uint8_t)(A ^ T0);

    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = parity_table[(uint8_t)A];
    F = sf | zf | pf;
}

void HELPER(or_cc)(CPUZ80State *env)
{
    int sf, zf, pf;
    A = (uint8_t)(A | T0);

    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = parity_table[(uint8_t)A];
    F = sf | zf | pf;
}

void HELPER(cp_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf, cf;
    int res, carry;

    res = (uint8_t)(A - T0);
    sf = (res & 0x80) ? CC_S : 0;
    zf = res ? 0 : CC_Z;
    carry = (~A & T0) | (~(A ^ T0) & res);
    hf = (carry & 0x08) ? CC_H : 0;
    pf = signed_overflow_sub(A, T0, res, 8) ? CC_P : 0;
    cf = (carry & 0x80) ? CC_C : 0;

    F = sf | zf | hf | pf | CC_N | cf;
//  CC_DST = (uint8_t)(A - T0);
}

/* Rotation/shift operations */

void HELPER(rlc_T0_cc)(CPUZ80State *env)
{
    int sf, zf, pf, cf;
    int tmp;

    tmp = T0;
    T0 = (uint8_t)((T0 << 1) | !!(T0 & 0x80));
    sf = (T0 & 0x80) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;
    pf = parity_table[T0];
    cf = (tmp & 0x80) ? CC_C : 0;
    F = sf | zf | pf | cf;
}

void HELPER(rrc_T0_cc)(CPUZ80State *env)
{
    int sf, zf, pf, cf;
    int tmp;

    tmp = T0;
    T0 = (uint8_t)((T0 >> 1) | ((tmp & 0x01) ? 0x80 : 0));
    sf = (T0 & 0x80) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;
    pf = parity_table[T0];
    cf = (tmp & 0x01) ? CC_C : 0;
    F = sf | zf | pf | cf;
}

void HELPER(rl_T0_cc)(CPUZ80State *env)
{
    int sf, zf, pf, cf;
    int tmp;

    tmp = T0;
    T0 = (uint8_t)((T0 << 1) | !!(F & CC_C));
    sf = (T0 & 0x80) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;
    pf = parity_table[T0];
    cf = (tmp & 0x80) ? CC_C : 0;
    F = sf | zf | pf | cf;
}

void HELPER(rr_T0_cc)(CPUZ80State *env)
{
    int sf, zf, pf, cf;
    int tmp;

    tmp = T0;
    T0 = (uint8_t)((T0 >> 1) | ((F & CC_C) ? 0x80 : 0));
    sf = (T0 & 0x80) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;
    pf = parity_table[T0];
    cf = (tmp & 0x01) ? CC_C : 0;
    F = sf | zf | pf | cf;
}

void HELPER(sla_T0_cc)(CPUZ80State *env)
{
    int sf, zf, pf, cf;
    int tmp;

    tmp = T0;
    T0 = (uint8_t)(T0 << 1);
    sf = (T0 & 0x80) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;
    pf = parity_table[T0];
    cf = (tmp & 0x80) ? CC_C : 0;
    F = sf | zf | pf | cf;
}

void HELPER(sra_T0_cc)(CPUZ80State *env)
{
    int sf, zf, pf, cf;
    int tmp;

    tmp = T0;
    T0 = (uint8_t)((T0 >> 1) | (T0 & 0x80));
    sf = (T0 & 0x80) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;
    pf = parity_table[T0];
    cf = (tmp & 0x01) ? CC_C : 0;
    F = sf | zf | pf | cf;
}

/* Z80-specific: R800 has tst instruction */
void HELPER(sll_T0_cc)(CPUZ80State *env)
{
    int sf, zf, pf, cf;
    int tmp;

    tmp = T0;
    T0 = (uint8_t)((T0 << 1) | 1); /* Yes -- bit 0 is *set* */
    sf = (T0 & 0x80) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;
    pf = parity_table[T0];
    cf = (tmp & 0x80) ? CC_C : 0;
    F = sf | zf | pf | cf;
}

void HELPER(srl_T0_cc)(CPUZ80State *env)
{
    int sf, zf, pf, cf;
    int tmp;

    tmp = T0;
    T0 = (uint8_t)(T0 >> 1);
    sf = (T0 & 0x80) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;
    pf = parity_table[T0];
    cf = (tmp & 0x01) ? CC_C : 0;
    F = sf | zf | pf | cf;
}

void HELPER(rld_cc)(CPUZ80State *env)
{
    int sf, zf, pf;
    int tmp = A & 0x0f;
    A = (A & 0xf0) | ((T0 >> 4) & 0x0f);
    T0 = ((T0 << 4) & 0xf0) | tmp;

    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = parity_table[A];

    F = (F & CC_C) | sf | zf | pf;
}

void HELPER(rrd_cc)(CPUZ80State *env)
{
    int sf, zf, pf;
    int tmp = A & 0x0f;
    A = (A & 0xf0) | (T0 & 0x0f);
    T0 = (T0 >> 4) | (tmp << 4);

    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = parity_table[A];

    F = (F & CC_C) | sf | zf | pf;
}

/* Block instructions */

void HELPER(bli_ld_inc_cc)(CPUZ80State *env)
{
    int pf;

    BC = (uint16_t)(BC - 1);
    DE = (uint16_t)(DE + 1);
    HL = (uint16_t)(HL + 1);

    pf = BC ? CC_P : 0;
    F = (F & (CC_S | CC_Z | CC_C)) | pf;
}

void HELPER(bli_ld_dec_cc)(CPUZ80State *env)
{
    int pf;

    BC = (uint16_t)(BC - 1);
    DE = (uint16_t)(DE - 1);
    HL = (uint16_t)(HL - 1);

    pf = BC ? CC_P : 0;
    F = (F & (CC_S | CC_Z | CC_C)) | pf;
}

void HELPER(bli_ld_rep)(CPUZ80State *env, uint32_t next_pc)
{
    if (BC) {
        PC = (uint16_t)(next_pc - 2);
    } else {
        PC = next_pc;
    }
}

void HELPER(bli_cp_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf;
    int res, carry;

    res = (uint8_t)(A - T0);
    sf = (res & 0x80) ? CC_S : 0;
    zf = res ? 0 : CC_Z;
    carry = (~A & T0) | (~(A ^ T0) & res);
    hf = (carry & 0x08) ? CC_H : 0;
    pf = BC ? CC_P : 0;

    F = (F & CC_C) | sf | zf | hf | pf | CC_N;
}

void HELPER(bli_cp_inc_cc)(CPUZ80State *env)
{
    int pf;

    BC = (uint16_t)(BC - 1);
    HL = (uint16_t)(HL + 1);

    pf = BC ? CC_P : 0;
    F = (F & ~CC_P) | pf;
}

void HELPER(bli_cp_dec_cc)(CPUZ80State *env)
{
    int pf;

    BC = (uint16_t)(BC - 1);
    HL = (uint16_t)(HL - 1);

    pf = BC ? CC_P : 0;
    F = (F & ~CC_P) | pf;
}

void HELPER(bli_cp_rep)(CPUZ80State *env, uint32_t next_pc)
{
    if (BC && T0 != A) {
        PC = (uint16_t)(next_pc - 2);
    } else {
        PC = next_pc;
    }
}

void HELPER(bli_io_T0_inc)(CPUZ80State *env, uint32_t out)
{
    HL = (uint16_t)(HL + 1);
    BC = (uint16_t)(BC - 0x0100);
    /* TODO: update X & Y flags */
    uint32_t ff = out ? (HL & 0xff) : (((F & CC_C) + 1) & 0xff);
    F = ((BC & 0x8000) ? CC_S : 0) |
        ((BC & 0xff00) ? 0 : CC_Z) |
        ((T0 + ff) > 0xff ? (CC_C | CC_H) : 0) |
        parity_table[(((T0 + ff) & 0x07) ^ (BC >> 8)) & 0xff] |
        ((T0 & 0x80) ? CC_N : 0);
}

void HELPER(bli_io_T0_dec)(CPUZ80State *env, uint32_t out)
{
    HL = (uint16_t)(HL - 1);
    BC = (uint16_t)(BC - 0x0100);
    /* TODO: update X & Y flags */
    uint32_t ff = out ? (HL & 0xff) : (((F & CC_C) - 1) & 0xff);
    F = ((BC & 0x8000) ? CC_S : 0) |
        ((BC & 0xff00) ? 0 : CC_Z) |
        ((T0 + ff) > 0xff ? (CC_C | CC_H) : 0) |
        parity_table[(((T0 + ff) & 0x07) ^ (BC >> 8)) & 0xff] |
        ((T0 & 0x80) ? CC_N : 0);
}

void HELPER(bli_io_rep)(CPUZ80State *env, uint32_t next_pc)
{
    if (F & CC_Z) {
        PC = (uint16_t)(next_pc - 2);
    } else {
        PC = next_pc;
    }
}

/* misc */

void HELPER(rlca_cc)(CPUZ80State *env)
{
    int cf;
    int tmp;

    tmp = A;
    A = (uint8_t)((A << 1) | !!(tmp & 0x80));
    cf = (tmp & 0x80) ? CC_C : 0;
    F = (F & (CC_S | CC_Z | CC_P)) | cf;
}

void HELPER(rrca_cc)(CPUZ80State *env)
{
    int cf;
    int tmp;

    tmp = A;
    A = (A >> 1) | ((tmp & 0x01) ? 0x80 : 0);
    cf = (tmp & 0x01) ? CC_C : 0;
    F = (F & (CC_S | CC_Z | CC_P)) | cf;
}

void HELPER(rla_cc)(CPUZ80State *env)
{
    int cf;
    int tmp;

    tmp = A;
    A = (uint8_t)((A << 1) | !!(F & CC_C));
    cf = (tmp & 0x80) ? CC_C : 0;
    F = (F & (CC_S | CC_Z | CC_P)) | cf;
}

void HELPER(rra_cc)(CPUZ80State *env)
{
    int cf;
    int tmp;

    tmp = A;
    A = (A >> 1) | ((F & CC_C) ? 0x80 : 0);
    cf = (tmp & 0x01) ? CC_C : 0;
    F = (F & (CC_S | CC_Z | CC_P)) | cf;
}

/* TODO */
void HELPER(daa_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf, cf;
    int cor = 0;
    int tmp = A;

    if (A > 0x99 || (F & CC_C)) {
        cor |= 0x60;
        cf = CC_C;
    } else {
        cf = 0;
    }

    if ((A & 0x0f) > 0x09 || (F & CC_H)) {
        cor |= 0x06;
    }

    if (!(F & CC_N)) {
        A = (uint8_t)(A + cor);
    } else {
        A = (uint8_t)(A - cor);
    }

    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    hf = ((tmp ^ A) & 0x10) ? CC_H : 0;
    pf = parity_table[(uint8_t)A];

    F = (F & CC_N) | sf | zf | hf | pf | cf;
}

void HELPER(cpl_cc)(CPUZ80State *env)
{
    A = (uint8_t)~A;
    F |= CC_H | CC_N;
}

void HELPER(scf_cc)(CPUZ80State *env)
{
    F = (F & (CC_S | CC_Z | CC_P)) | CC_C;
}

void HELPER(ccf_cc)(CPUZ80State *env)
{
    int hf, cf;

    hf = (F & CC_C) ? CC_H : 0;
    cf = (F & CC_C) ^ CC_C;
    F = (F & (CC_S | CC_Z | CC_P)) | hf | cf;
}

/* misc */

void HELPER(neg_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf, cf;
    int tmp = A;
    int carry;

    A = (uint8_t)-A;
    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    carry = (tmp & T0) | ((tmp | T0) & ~A);
    hf = (carry & 0x08) ? CC_H : 0;
    pf = signed_overflow_sub(tmp, T0, A, 8) ? CC_P : 0;
    cf = (carry & 0x80) ? CC_C : 0;

    F = sf | zf | hf | pf | CC_N | cf;
}

/* word operations -- HL only? */

void HELPER(sbcw_T0_T1_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf, cf;
    int tmp = T0;
    int carry;

    T0 = (uint16_t)(T0 - T1 - !!(F & CC_C));
    sf = (T0 & 0x8000) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;
    carry = (~tmp & T1) | (~(tmp ^ T1) & T0);
    hf = (carry & 0x0800) ? CC_H : 0;
    pf = signed_overflow_sub(tmp, T1, T0, 16) ? CC_P : 0;
    cf = (carry & 0x8000) ? CC_C : 0;

    F = sf | zf | hf | pf | CC_N | cf;
}

void HELPER(addw_T0_T1_cc)(CPUZ80State *env)
{
    int hf, cf;
    int tmp = T0;
    int carry;

    T0 = (uint16_t)(T0 + T1);
    carry = (tmp & T1) | ((tmp | T1) & ~T0);
    hf = (carry & 0x0800) ? CC_H : 0;
    cf = (carry & 0x8000) ? CC_C : 0;

    F = (F & (CC_S | CC_Z | CC_P)) | hf | cf;
}

void HELPER(adcw_T0_T1_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf, cf;
    int tmp = T0;
    int carry;

    T0 = (uint16_t)(T0 + T1 + !!(F & CC_C));
    sf = (T0 & 0x8000) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;
    carry = (tmp & T1) | ((tmp | T1) & ~T0);
    hf = (carry & 0x0800) ? CC_H : 0;
    pf = signed_overflow_add(tmp, T1, T0, 8) ? CC_P : 0;
    cf = (carry & 0x8000) ? CC_C : 0;

    F = sf | zf | hf | pf | cf;
}

/* misc */

void HELPER(incb_T0_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf;
    int tmp;
    int carry;

    tmp = T0;
    T0 = (uint8_t)(T0 + 1);
    sf = (T0 & 0x80) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;

    carry = (tmp & 1) | ((tmp | 1) & ~T0);
    hf = (carry & 0x08) ? CC_H : 0;
    pf = signed_overflow_add(tmp, 1, T0, 8) ? CC_P : 0;

    F = (F & CC_C) | sf | zf | hf | pf;
}

void HELPER(decb_T0_cc)(CPUZ80State *env)
{
    int sf, zf, hf, pf;
    int tmp;
    int carry;

    tmp = T0;
    T0 = (uint8_t)(T0 - 1);
    sf = (T0 & 0x80) ? CC_S : 0;
    zf = T0 ? 0 : CC_Z;

    carry = (~tmp & 1) | (~(tmp ^ 1) & T0);
    hf = (carry & 0x08) ? CC_H : 0;
    pf = signed_overflow_sub(tmp, 1, T0, 8) ? CC_P : 0;

    F = (F & CC_C) | sf | zf | hf | CC_N | pf;
    /* TODO: check CC_N is set */
}

/* value on data bus is 0xff for speccy */
/* IM0 = execute data on bus (rst $38 on speccy) */
/* IM1 = execute rst $38 (ROM uses this)*/
/* IM2 = indirect jump -- address is held at (I << 8) | DATA */

/* when an interrupt occurs, iff1 and iff2 are reset, disabling interrupts */
/* when an NMI occurs, iff1 is reset. iff2 is left unchanged */

void HELPER(imode)(CPUZ80State *env, uint32_t imode)
{
    env->imode = imode;
}

/* enable interrupts */
void HELPER(ei)(CPUZ80State *env)
{
    env->iff1 = 1;
    env->iff2 = 1;
}

/* disable interrupts */
void HELPER(di)(CPUZ80State *env)
{
    env->iff1 = 0;
    env->iff2 = 0;
}

/* reenable interrupts if enabled */
void HELPER(ri)(CPUZ80State *env)
{
    env->iff1 = env->iff2;
}

void HELPER(ld_R_A)(CPUZ80State *env)
{
    R = A;
}

void HELPER(ld_I_A)(CPUZ80State *env)
{
    I = A;
}

void HELPER(ld_A_R)(CPUZ80State *env)
{
    int sf, zf, pf;

    A = R;
    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = env->iff2 ? CC_P : 0;

    F = (F & CC_C) | sf | zf | pf;
}

void HELPER(ld_A_I)(CPUZ80State *env)
{
    int sf, zf, pf;

    A = I;
    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = env->iff2 ? CC_P : 0;

    F = (F & CC_C) | sf | zf | pf;
}

void HELPER(mulub_cc)(void)
{
    /* TODO: flags */

    HL = A * T0;
}

void HELPER(muluw_cc)(void)
{
    /* TODO: flags */
    uint32_t tmp;

    tmp = HL * T0;
    DE = tmp >> 16;
    HL = tmp & 0xff;
}
