/*
 * Minimal QEmu Z80 CPU - helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Ported/provisional reimplementations by William Towle 2018-2022
 *  All versions under GPL
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
