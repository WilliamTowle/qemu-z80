/*
 * Minimal QEmu Z80 CPU - misc helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Ported/provisional reimplementations by William Towle 2018-2022
 *  All versions under GPL
 */

#include "qemu/osdep.h"
#include "cpu.h"

#include "exec/helper-proto.h"
#include "exec/exec-all.h"


void helper_debug(CPUZ80State *env)
{
    CPUState *cs = env_cpu(env);

    cs->exception_index = EXCP_DEBUG;
#if QEMU_VERSION_MAJOR < 2  /* v0.15.0 style */
    cpu_loop_exit(env);
#else
    /* trigger call to cpu_handle_debug_exception() */
    cpu_loop_exit(cs);
#endif
}
