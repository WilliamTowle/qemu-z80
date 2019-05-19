#include "exec/def-helper.h"

//DEF_HELPER_1(raise_exception, void, i32)
DEF_HELPER_3(raise_interrupt, void, env, int, int)
DEF_HELPER_2(raise_exception, void, env, int)

//DEF_HELPER_0(set_inhibit_irq, void)
//DEF_HELPER_0(reset_inhibit_irq, void)

//DEF_HELPER_1(movl_pc_im, void, i32)
DEF_HELPER_2(movl_pc_im, void, env, i32)

#include "exec/def-helper.h"
