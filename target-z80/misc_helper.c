/*
 * QEmu Z80 CPU - misc helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  WmT, 2018-2022 [...after Stuart Brady]
 *  [...all versions under GPL...]
 */

#include "cpu.h"
#include "helper.h"


void helper_halt(CPUZ80State *env)
{
    Z80CPU *cpu = z80_env_get_cpu(env);
    CPUState *cs = CPU(cpu);
    //printf("halting at PC 0x%x\n",env->pc);

    cs->halted = 1;
    env->hflags &= ~HF_INHIBIT_IRQ_MASK; /* needed if sti is just before */
    env->exception_index = EXCP_HLT;
#if 0	/* obsolete */
    cpu_loop_exit();
#else	/* v0.15.0+ */
    cpu_loop_exit(env);
#endif
}


void helper_debug(CPUZ80State *env)
{
    env->exception_index = EXCP_DEBUG;
#if 0	/* obsolete */
    cpu_loop_exit();
#else	/* v0.15.0+ */
    cpu_loop_exit(env);
#endif
}
