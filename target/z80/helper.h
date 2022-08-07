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
