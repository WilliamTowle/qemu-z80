#include "exec/def-helper.h"

DEF_HELPER_1(debug, void, env)
//DEF_HELPER_1(raise_exception, void, i32)
DEF_HELPER_3(raise_interrupt, void, env, int, int)
DEF_HELPER_2(raise_exception, void, env, int)

//DEF_HELPER_1(set_inhibit_irq, void, env)
DEF_HELPER_1(reset_inhibit_irq, void, env)

//DEF_HELPER_1(movl_pc_im, void, i32)
DEF_HELPER_2(movl_pc_im, void, env, i32)

///* In / Out */

/* Misc */

//DEF_HELPER_1(bit_T0, void, i32)
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

///* Rotation/shifts */

///* Block instructions */

/* Misc */

DEF_HELPER_0(rlca_cc, void)
DEF_HELPER_0(rrca_cc, void)
DEF_HELPER_0(rla_cc, void)
DEF_HELPER_0(rra_cc, void)
DEF_HELPER_0(daa_cc, void)
DEF_HELPER_0(cpl_cc, void)
DEF_HELPER_0(scf_cc, void)
DEF_HELPER_0(ccf_cc, void)
//DEF_HELPER_0(neg_cc, void)

/* 16-bit arithmetic */
DEF_HELPER_1(sbcw_T0_T1_cc, void, env)
DEF_HELPER_1(addw_T0_T1_cc, void, env)
DEF_HELPER_1(adcw_T0_T1_cc, void, env)
DEF_HELPER_1(incb_T0_cc, void, env)
DEF_HELPER_1(decb_T0_cc, void, env)

#include "exec/def-helper.h"
