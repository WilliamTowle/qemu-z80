/*
 * Minimal QEmu Z80 CPU - helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPL...]
 */


DEF_HELPER_1(halt, void, env)

DEF_HELPER_2(raise_exception, void, env, int)

DEF_HELPER_2(movl_pc_im, void, env, i32)


/* Misc */

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

/* 16-bit arithmetic */

DEF_HELPER_1(incb_T0_cc, void, env)