#include "def-helper.h"

DEF_HELPER_0(debug, void)
DEF_HELPER_1(raise_exception, void, i32)

//DEF_HELPER_0(set_inhibit_irq, void)
DEF_HELPER_0(reset_inhibit_irq, void)

DEF_HELPER_1(movl_pc_im, void, i32)

#include "def-helper.h"
