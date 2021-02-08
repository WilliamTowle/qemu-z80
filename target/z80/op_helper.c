/*
 * QEmu Z80 CPU - opcode helpers
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
#include "exec/helper-proto.h"
#include "exec.h"

#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 op_helper: " fmt , ## __VA_ARGS__); } while(0)


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


void helper_movl_pc_im(CPUZ80State *env, uint32_t new_pc)
{
    PC= (uint16_t)new_pc;
}


/* Misc */

void helper_bit_T0(CPUZ80State *env, uint32_t val)
{
    int sf, zf, pf;

    sf = (T0 & val & 0x80) ? CC_S : 0;
    zf = (T0 & val) ? 0 : CC_Z;
    pf = (T0 & val) ? 0 : CC_P;
    F = (F & CC_C) | sf | zf | CC_H | pf;
}

void helper_jmp_T0(CPUZ80State *env)
{
    PC = T0;
}

void helper_djnz(CPUZ80State *env, uint32_t pc1, uint32_t pc2)
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


/* 8-bit arithmetic */

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


/* Rotation/shift operations */

void helper_rlc_T0_cc(CPUZ80State *env)
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

void helper_rrc_T0_cc(CPUZ80State *env)
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

void helper_rl_T0_cc(CPUZ80State *env)
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

void helper_rr_T0_cc(CPUZ80State *env)
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

void helper_sla_T0_cc(CPUZ80State *env)
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

void helper_sra_T0_cc(CPUZ80State *env)
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
void helper_sll_T0_cc(CPUZ80State *env)
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

void helper_srl_T0_cc(CPUZ80State *env)
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

void helper_rld_cc(CPUZ80State *env)
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

void helper_rrd_cc(CPUZ80State *env)
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

void helper_bli_ld_inc_cc(CPUZ80State *env)
{
    int pf;

    BC = (uint16_t)(BC - 1);
    DE = (uint16_t)(DE + 1);
    HL = (uint16_t)(HL + 1);

    pf = BC ? CC_P : 0;
    F = (F & (CC_S | CC_Z | CC_C)) | pf;
}

void helper_bli_ld_dec_cc(CPUZ80State *env)
{
    int pf;

    BC = (uint16_t)(BC - 1);
    DE = (uint16_t)(DE - 1);
    HL = (uint16_t)(HL - 1);

    pf = BC ? CC_P : 0;
    F = (F & (CC_S | CC_Z | CC_C)) | pf;
}

void helper_bli_ld_rep(CPUZ80State *env, uint32_t next_pc)
{
    if (BC) {
        PC = (uint16_t)(next_pc - 2);
    } else {
        PC = next_pc;
    }
}

void helper_bli_cp_cc(CPUZ80State *env)
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

void helper_bli_cp_inc_cc(CPUZ80State *env)
{
    int pf;

    BC = (uint16_t)(BC - 1);
    HL = (uint16_t)(HL + 1);

    pf = BC ? CC_P : 0;
    F = (F & ~CC_P) | pf;
}

void helper_bli_cp_dec_cc(CPUZ80State *env)
{
    int pf;

    BC = (uint16_t)(BC - 1);
    HL = (uint16_t)(HL - 1);

    pf = BC ? CC_P : 0;
    F = (F & ~CC_P) | pf;
}

void helper_bli_cp_rep(CPUZ80State *env, uint32_t next_pc)
{
    if (BC && T0 != A) {
        PC = (uint16_t)(next_pc - 2);
    } else {
        PC = next_pc;
    }
}

void helper_bli_io_T0_inc(CPUZ80State *env, uint32_t out)
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

void helper_bli_io_T0_dec(CPUZ80State *env, uint32_t out)
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

void helper_bli_io_rep(CPUZ80State *env, uint32_t next_pc)
{
    if (F & CC_Z) {
        PC = (uint16_t)(next_pc - 2);
    } else {
        PC = next_pc;
    }
}


/* Misc */

void helper_rlca_cc(CPUZ80State *env)
{
    int cf;
    int tmp;

    tmp = A;
    A = (uint8_t)((A << 1) | !!(tmp & 0x80));
    cf = (tmp & 0x80) ? CC_C : 0;
    F = (F & (CC_S | CC_Z | CC_P)) | cf;
}

void helper_rrca_cc(CPUZ80State *env)
{
    int cf;
    int tmp;

    tmp = A;
    A = (A >> 1) | ((tmp & 0x01) ? 0x80 : 0);
    cf = (tmp & 0x01) ? CC_C : 0;
    F = (F & (CC_S | CC_Z | CC_P)) | cf;
}

void helper_rla_cc(CPUZ80State *env)
{
    int cf;
    int tmp;

    tmp = A;
    A = (uint8_t)((A << 1) | !!(F & CC_C));
    cf = (tmp & 0x80) ? CC_C : 0;
    F = (F & (CC_S | CC_Z | CC_P)) | cf;
}

void helper_rra_cc(CPUZ80State *env)
{
    int cf;
    int tmp;

    tmp = A;
    A = (A >> 1) | ((F & CC_C) ? 0x80 : 0);
    cf = (tmp & 0x01) ? CC_C : 0;
    F = (F & (CC_S | CC_Z | CC_P)) | cf;
}

void helper_daa_cc(CPUZ80State *env)
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

void helper_cpl_cc(CPUZ80State *env)
{
    A = (uint8_t)~A;
    F |= CC_H | CC_N;
}

void helper_scf_cc(CPUZ80State *env)
{
    F = (F & (CC_S | CC_Z | CC_P)) | CC_C;
}

void helper_ccf_cc(CPUZ80State *env)
{
    int hf, cf;

    hf = (F & CC_C) ? CC_H : 0;
    cf = (F & CC_C) ^ CC_C;
    F = (F & (CC_S | CC_Z | CC_P)) | hf | cf;
}


/* misc */

void helper_neg_cc(CPUZ80State *env)
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

void helper_sbcw_T0_T1_cc(CPUZ80State *env)
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

void helper_addw_T0_T1_cc(CPUZ80State *env)
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


void helper_adcw_T0_T1_cc(CPUZ80State *env)
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


/* Interrupt handling / IR registers */

/* value on data bus is 0xff for speccy */
/* IM0 = execute data on bus (rst $38 on speccy) */
/* IM1 = execute rst $38 (ROM uses this)*/
/* IM2 = indirect jump -- address is held at (I << 8) | DATA */

/* when an interrupt occurs, iff1 and iff2 are reset, disabling interrupts */
/* when an NMI occurs, iff1 is reset. iff2 is left unchanged */

void helper_imode(CPUZ80State *env, uint32_t imode)
{
    env->imode = imode;
}

/* enable interrupts */
void helper_ei(CPUZ80State *env)
{
    env->iff1 = 1;
    env->iff2 = 1;
}

/* disable interrupts */
void helper_di(CPUZ80State *env)
{
    env->iff1 = 0;
    env->iff2 = 0;
}

/* reenable interrupts if enabled */
void helper_ri(CPUZ80State *env)
{
    env->iff1 = env->iff2;
}

void helper_ld_R_A(CPUZ80State *env)
{
    R = A;
}

void helper_ld_I_A(CPUZ80State *env)
{
    I = A;
}

void helper_ld_A_R(CPUZ80State *env)
{
    int sf, zf, pf;

    A = R;
    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = env->iff2 ? CC_P : 0;

    F = (F & CC_C) | sf | zf | pf;
}

void helper_ld_A_I(CPUZ80State *env)
{
    int sf, zf, pf;

    A = I;
    sf = (A & 0x80) ? CC_S : 0;
    zf = A ? 0 : CC_Z;
    pf = env->iff2 ? CC_P : 0;

    F = (F & CC_C) | sf | zf | pf;
}
