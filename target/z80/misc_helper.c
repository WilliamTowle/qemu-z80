/*
 * Minimal QEmu Z80 CPU - misc helpers
 *
 * [...William Towle, under GPL...]
 */

#include "qemu/osdep.h"
#include "cpu.h"

#include "exec/helper-proto.h"
#include "exec/exec-all.h"


void helper_debug(CPUZ80State *env)
{
    CPUState *cs = CPU(z80_env_get_cpu(env));

    cs->exception_index = EXCP_DEBUG;
#if QEMU_VERSION_MAJOR < 2  /* v0.15.0 style */
    cpu_loop_exit(env);
#else
    cpu_loop_exit(cs);
#endif
}
