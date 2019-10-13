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


/* TODO: target-x86 uses 'excp_helper.c' */
void helper_raise_interrupt(CPUZ80State *env, int intno, int next_eip_addend)
{
    raise_interrupt(env, intno, 1, 0, next_eip_addend);
}

void helper_raise_exception(CPUZ80State *env, int exception_index)
{
    raise_exception(env, exception_index);
}

/*
 * Signal an interruption. It is executed in the main CPU loop.
 * is_int is TRUE if coming from the int instruction. next_eip is the
 * EIP value AFTER the interrupt instruction. It is only relevant if
 * is_int is TRUE.
 */
static void QEMU_NORETURN raise_interrupt2(CPUZ80State *env, int intno,
                                           int is_int, int error_code,
                                           int next_eip_addend)
{
#if 0   /* x86-specific */
    if (!is_int) {
        cpu_svm_check_intercept_param(env, SVM_EXIT_EXCP_BASE + intno,
                                      error_code);
        intno = check_exception(env, intno, &error_code);
    } else {
        cpu_svm_check_intercept_param(env, SVM_EXIT_SWINT, 0);
    }
#endif

    env->exception_index = intno;
#if 0   /* never ported from target-i386 */
    env->error_code = error_code;
    env->exception_is_int = is_int;
    env->exception_next_eip = env->eip + next_eip_addend;
#endif
    cpu_loop_exit(env);
}

/* shortcuts to generate exceptions */

void QEMU_NORETURN raise_interrupt(CPUZ80State *env, int intno, int is_int,
                                   int error_code, int next_eip_addend)
{
    raise_interrupt2(env, intno, is_int, error_code, next_eip_addend);
}

void raise_exception_err(CPUZ80State *env, int exception_index,
                         int error_code)
{
    raise_interrupt2(env, exception_index, 0, error_code, 0);
}

void raise_exception(CPUZ80State *env, int exception_index)
{
    raise_interrupt2(env, exception_index, 0, 0, 0);
}

void HELPER(debug)(CPUZ80State *env)
{
    env->exception_index = EXCP_DEBUG;
#if 0	/* obsolete */
    cpu_loop_exit();
#else	/* v0.15.0+ */
    cpu_loop_exit(env);
#endif
}

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

void HELPER(jmp_T0)(CPUZ80State *env)
{
    PC = T0;
}

void HELPER(djnz)(uint32_t pc1, uint32_t pc2)
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

///* Rotation/shift operations */

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
