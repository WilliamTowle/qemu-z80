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
//DEF_HELPER_2(djnz, void, i32, i32)

#include "def-helper.h"
