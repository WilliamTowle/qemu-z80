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


void helper_halt(CPUZ80State *env)
{
    CPUState *cs = CPU(z80_env_get_cpu(env));
    //printf("halting at PC 0x%x\n",env->pc);

    cs->halted = 1;
    env->hflags &= ~HF_INHIBIT_IRQ_MASK; /* needed if sti is just before */
    cs->exception_index = EXCP_HLT;
#if 0  /* obsolete */
    cpu_loop_exit();
#else  /* v0.15.0+ */
    //cpu_loop_exit(env);
    cpu_loop_exit(cs);
#endif
}


void helper_debug(CPUZ80State *env)
{
    CPUState *cs = CPU(z80_env_get_cpu(env));

    cs->exception_index = EXCP_DEBUG;
#if QEMU_VERSION_MAJOR < 2  /* v0.15.0 style */
    cpu_loop_exit(env);
#else
    /* trigger call to cpu_handle_debug_exception() */
    cpu_loop_exit(cs);
#endif
}
