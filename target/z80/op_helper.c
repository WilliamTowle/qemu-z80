/*
 * QEmu Z80 CPU - opcode helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting by William Towle 2018-2023
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
#include "exec/helper-proto.h"
#include "exec.h"

//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 op_helper: " fmt , ## __VA_ARGS__); } while(0)


/* Access to registers/emulator temporary values */

#define T0 (env->t0)
//#define T1 (env->t1)
//#define A   (env->regs[R_A])
//#define F   (env->regs[R_F])
//#define BC  (env->regs[R_BC])
//#define DE  (env->regs[R_DE])
//#define HL  (env->regs[R_HL])
//#define IX  (env->regs[R_IX])
//#define IY  (env->regs[R_IY])
//#define SP  (env->regs[R_SP])
//#define I   (env->regs[R_I])
//#define R   (env->regs[R_R])
//#define AX  (env->regs[R_AX])
//#define FX  (env->regs[R_FX])
//#define BCX (env->regs[R_BCX])
//#define DEX (env->regs[R_DEX])
//#define HLX (env->regs[R_HLX])

#define PC	(env->pc)

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


/* NB. set_inhibit_irq() not invoked
 * repo.or.cz doesn't make it clear what should have called this (but
 * does this explain FIXMEs against 'retn'/'reti' insns?)
 * In target-i386 for QEmu v1, calls occurred:
 * - on pop of ES/SS/DS
 * - on 'mov seg, Gv'
 * - on 'sti'
 */
//void HELPER(set_inhibit_irq)(CPUZ80State *env)
//{
//    env->hflags |= HF_INHIBIT_IRQ_MASK;
//}

void HELPER(reset_inhibit_irq)(CPUZ80State *env)
{
    env->hflags &= ~HF_INHIBIT_IRQ_MASK;
}


void helper_movl_pc_im(CPUZ80State *env, int new_pc)
{
    PC= (uint16_t)new_pc;
}


void helper_jmp_T0(CPUZ80State *env)
{
    PC = T0;
}

void helper_djnz(CPUZ80State *env, int pc1, int pc2)
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


/* 8-bit arithmetic ops */

void helper_add_cc(CPUZ80State *env)
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

void helper_adc_cc(CPUZ80State *env)
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

void helper_sub_cc(CPUZ80State *env)
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

void helper_sbc_cc(CPUZ80State *env)
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

void helper_and_cc(CPUZ80State *env)
{
    int sf, zf, pf;
    A = (uint8_t)(A & T0);

    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = parity_table[(uint8_t)A];
    F = sf | zf | CC_H | pf;
}

void helper_xor_cc(CPUZ80State *env)
{
    int sf, zf, pf;
    A = (uint8_t)(A ^ T0);

    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = parity_table[(uint8_t)A];
    F = sf | zf | pf;
}

void helper_or_cc(CPUZ80State *env)
{
    int sf, zf, pf;
    A = (uint8_t)(A | T0);

    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = parity_table[(uint8_t)A];
    F = sf | zf | pf;
}

void helper_cp_cc(CPUZ80State *env)
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


/* 16-bit arithmetic */

void helper_incb_T0_cc(CPUZ80State *env)
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

void helper_decb_T0_cc(CPUZ80State *env)
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
