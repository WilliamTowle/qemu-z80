#include "def-helper.h"

DEF_HELPER_0(debug, void)
DEF_HELPER_1(raise_exception, void, i32)

//DEF_HELPER_0(set_inhibit_irq, void)
DEF_HELPER_0(reset_inhibit_irq, void)

DEF_HELPER_1(movl_pc_im, void, i32)

///* In / Out */

/* Misc */

//DEF_HELPER_1(bit_T0, void, i32)
DEF_HELPER_0(jmp_T0, void)
DEF_HELPER_2(djnz, void, i32, i32)

/* 8-bit arithmetic */
DEF_HELPER_0(add_cc, void)
DEF_HELPER_0(adc_cc, void)
DEF_HELPER_0(sub_cc, void)
DEF_HELPER_0(sbc_cc, void)
DEF_HELPER_0(and_cc, void)
DEF_HELPER_0(xor_cc, void)
DEF_HELPER_0(or_cc, void)
DEF_HELPER_0(cp_cc, void)

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
DEF_HELPER_0(sbcw_T0_T1_cc, void)
DEF_HELPER_0(addw_T0_T1_cc, void)
DEF_HELPER_0(adcw_T0_T1_cc, void)
DEF_HELPER_0(incb_T0_cc, void)
DEF_HELPER_0(decb_T0_cc, void)

#include "def-helper.h"
