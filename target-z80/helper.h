#include "exec/def-helper.h"

DEF_HELPER_1(debug, void, env)
//DEF_HELPER_1(raise_exception, void, i32)
DEF_HELPER_3(raise_interrupt, void, env, int, int)
DEF_HELPER_2(raise_exception, void, env, int)

//DEF_HELPER_1(set_inhibit_irq, void, env)
DEF_HELPER_1(reset_inhibit_irq, void, env)

//DEF_HELPER_1(movl_pc_im, void, i32)
DEF_HELPER_2(movl_pc_im, void, env, i32)

DEF_HELPER_1(halt, void, env)

///* In / Out */

/* Misc */

DEF_HELPER_1(bit_T0, void, i32)
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
DEF_HELPER_0(rlc_T0_cc, void)
DEF_HELPER_0(rrc_T0_cc, void)
DEF_HELPER_0(rl_T0_cc, void)
DEF_HELPER_0(rr_T0_cc, void)
DEF_HELPER_0(sla_T0_cc, void)
DEF_HELPER_0(sra_T0_cc, void)
DEF_HELPER_0(sll_T0_cc, void)
DEF_HELPER_0(srl_T0_cc, void)
DEF_HELPER_0(rld_cc, void)
DEF_HELPER_0(rrd_cc, void)

/* Block instructions */
DEF_HELPER_0(bli_ld_inc_cc, void)
DEF_HELPER_0(bli_ld_dec_cc, void)
DEF_HELPER_1(bli_ld_rep, void, i32)
DEF_HELPER_0(bli_cp_cc, void)
DEF_HELPER_0(bli_cp_inc_cc, void)
DEF_HELPER_0(bli_cp_dec_cc, void)
DEF_HELPER_1(bli_cp_rep, void, i32)
DEF_HELPER_1(bli_io_T0_inc, void, i32)
DEF_HELPER_1(bli_io_T0_dec, void, i32)
DEF_HELPER_1(bli_io_rep, void, i32)

/* Misc */

DEF_HELPER_1(rlca_cc, void, env)
DEF_HELPER_1(rrca_cc, void, env)
DEF_HELPER_1(rla_cc, void, env)
DEF_HELPER_1(rra_cc, void, env)
DEF_HELPER_1(daa_cc, void, env)
DEF_HELPER_1(cpl_cc, void, env)
DEF_HELPER_1(scf_cc, void, env)
DEF_HELPER_1(ccf_cc, void, env)
DEF_HELPER_0(neg_cc, void)

/* 16-bit arithmetic */
DEF_HELPER_1(sbcw_T0_T1_cc, void, env)
DEF_HELPER_1(addw_T0_T1_cc, void, env)
DEF_HELPER_1(adcw_T0_T1_cc, void, env)
DEF_HELPER_1(incb_T0_cc, void, env)
DEF_HELPER_1(decb_T0_cc, void, env)

/* Interrupt handling / IR registers */
DEF_HELPER_1(imode, void, i32)
DEF_HELPER_1(ei, void, env)
DEF_HELPER_1(di, void, env)
DEF_HELPER_0(ri, void)
DEF_HELPER_0(ld_R_A, void)
DEF_HELPER_0(ld_I_A, void)
DEF_HELPER_0(ld_A_R, void)
DEF_HELPER_0(ld_A_I, void)

#include "exec/def-helper.h"
