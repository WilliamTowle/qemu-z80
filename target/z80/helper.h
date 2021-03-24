/*
 * Minimal QEmu Z80 CPU - helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPL...]
 */


DEF_HELPER_2(raise_exception, void, env, int)

DEF_HELPER_2(movl_pc_im, void, env, i32)


/* Misc */

DEF_HELPER_1(jmp_T0, void, env)


/* 8-bit arithmetic */

//DEF_HELPER_0(add_cc, void)
//DEF_HELPER_0(adc_cc, void)
//DEF_HELPER_0(sub_cc, void)
//DEF_HELPER_0(sbc_cc, void)
//DEF_HELPER_0(and_cc, void)
//DEF_HELPER_0(xor_cc, void)
DEF_HELPER_1(xor_cc, void, env)
//DEF_HELPER_0(or_cc, void)
//DEF_HELPER_0(cp_cc, void)
