/*
 * QEmu Z80 CPU - helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007 Stuart Brady <stuart.brady@gmail.com>
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

DEF_HELPER_1(debug, void, env)


DEF_HELPER_1(halt, void, env)

DEF_HELPER_2(raise_exception, void, env, int)

DEF_HELPER_2(movl_pc_im, void, env, i32)


/* In/Out */

DEF_HELPER_2(in_T0_im, void, env, i32)
DEF_HELPER_1(in_T0_bc_cc, void, env)
DEF_HELPER_2(out_T0_im, void, env, i32)
DEF_HELPER_1(out_T0_bc, void, env)


/* Misc */

DEF_HELPER_2(bit_T0, void, env, i32)
DEF_HELPER_1(jmp_T0, void, env)
DEF_HELPER_3(djnz, void, env, i32, i32)


/* 8-bit arithmetic */

DEF_HELPER_1(add_cc, void, env)
DEF_HELPER_1(adc_cc, void, env)
DEF_HELPER_1(sub_cc, void, env)
DEF_HELPER_1(sbc_cc, void, env)
DEF_HELPER_1(and_cc, void, env)
DEF_HELPER_1(xor_cc, void, env)
DEF_HELPER_1(or_cc, void, env)
DEF_HELPER_1(cp_cc, void, env)


/* Rotation/shifts */

DEF_HELPER_1(rlc_T0_cc, void, env)
DEF_HELPER_1(rrc_T0_cc, void, env)
DEF_HELPER_1(rl_T0_cc, void, env)
DEF_HELPER_1(rr_T0_cc, void, env)
DEF_HELPER_1(sla_T0_cc, void, env)
DEF_HELPER_1(sra_T0_cc, void, env)
DEF_HELPER_1(sll_T0_cc, void, env)
DEF_HELPER_1(srl_T0_cc, void, env)
DEF_HELPER_1(rld_cc, void, env)
DEF_HELPER_1(rrd_cc, void, env)


/* Block instructions */

DEF_HELPER_1(bli_ld_inc_cc, void, env)
DEF_HELPER_1(bli_ld_dec_cc, void, env)
DEF_HELPER_2(bli_ld_rep, void, env, i32)
DEF_HELPER_1(bli_cp_cc, void, env)
DEF_HELPER_1(bli_cp_inc_cc, void, env)
DEF_HELPER_1(bli_cp_dec_cc, void, env)
DEF_HELPER_2(bli_cp_rep, void, env, i32)
DEF_HELPER_2(bli_io_T0_inc, void, env, i32)
DEF_HELPER_2(bli_io_T0_dec, void, env, i32)
DEF_HELPER_2(bli_io_rep, void, env, i32)


/* Misc */

DEF_HELPER_1(rlca_cc, void, env)
DEF_HELPER_1(rrca_cc, void, env)
DEF_HELPER_1(rla_cc, void, env)
DEF_HELPER_1(rra_cc, void, env)
DEF_HELPER_1(daa_cc, void, env)
DEF_HELPER_1(cpl_cc, void, env)
DEF_HELPER_1(scf_cc, void, env)
DEF_HELPER_1(ccf_cc, void, env)
DEF_HELPER_1(neg_cc, void, env)


/* 16-bit arithmetic */

DEF_HELPER_1(sbcw_T0_T1_cc, void, env)
DEF_HELPER_1(addw_T0_T1_cc, void, env)
DEF_HELPER_1(adcw_T0_T1_cc, void, env)
DEF_HELPER_1(incb_T0_cc, void, env)
DEF_HELPER_1(decb_T0_cc, void, env)


/* Interrupt handling / IR registers */

DEF_HELPER_2(imode, void, env, i32)
DEF_HELPER_1(ei, void, env)
DEF_HELPER_1(di, void, env)
DEF_HELPER_1(ri, void, env)
DEF_HELPER_1(ld_R_A, void, env)
DEF_HELPER_1(ld_I_A, void, env)
DEF_HELPER_1(ld_A_R, void, env)
DEF_HELPER_1(ld_A_I, void, env)
