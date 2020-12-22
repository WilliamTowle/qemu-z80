/*
 * Minimal QEmu Z80 CPU - misc helpers
 *
 * [...William Towle, under GPL...]
 */

#include "qemu/osdep.h"
#include "cpu.h"

#include "exec/helper-proto.h"
#include "exec/exec-all.h"


void helper_halt(CPUZ80State *env)
{
    Z80CPU *cpu = z80_env_get_cpu(env);
    CPUState *cs = CPU(cpu);
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
