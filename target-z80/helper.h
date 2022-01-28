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
//DEF_HELPER_2(djnz, void, i32, i32)

#include "exec/def-helper.h"
