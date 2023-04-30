/*
 * QEmu Z80 CPU - helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007 Stuart Brady <stuart.brady@gmail.com>
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


/* QEmu internal */

DEF_HELPER_1(debug, void, env)

DEF_HELPER_1(halt, void, env)

DEF_HELPER_2(raise_exception, void, env, int)

//DEF_HELPER_1(set_inhibit_irq, void, env)
DEF_HELPER_1(reset_inhibit_irq, void, env)

DEF_HELPER_2(movl_pc_im, void, env, int)


/* Misc */

DEF_HELPER_1(jmp_T0, void, env)
DEF_HELPER_3(djnz, void, env, int, int)


/* 8-bit arithmetic ops */

DEF_HELPER_1(add_cc, void, env)
DEF_HELPER_1(adc_cc, void, env)
DEF_HELPER_1(sub_cc, void, env)
DEF_HELPER_1(sbc_cc, void, env)
DEF_HELPER_1(and_cc, void, env)
DEF_HELPER_1(xor_cc, void, env)
DEF_HELPER_1(or_cc, void, env)
DEF_HELPER_1(cp_cc, void, env)


/* 16-bit arithmetic */

DEF_HELPER_1(incb_T0_cc, void, env)
//DEF_HELPER_0(decb_T0_cc, void)
