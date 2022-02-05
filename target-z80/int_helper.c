/*
 * QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * Original under GPLv2 by Stuart Brady 2007-2009
 * Porting by William Towle 2018-2022
 */

#include "cpu.h"

void z80_cpu_do_interrupt(CPUState *cs)
{
    Z80CPU *cpu = Z80_CPU(cs);
    CPUZ80State *env = &cpu->env;
;fprintf(stderr, "INFO: using cpu %p, env %p\n", cpu, env);

    /* Handle the ILLOP exception as thrown by disas_insn() by
     * bailing cleanly. For z80-bblbrx-user there are no I/O
     * interrupts
     */
    if (env->exception_index == EXCP06_ILLOP)
    {
        cpu_abort(env, "EXCP_ILLOP at pc=0x%04x", env->pc);
    }

#if defined(CONFIG_USER_ONLY)
    /* target-i386 has do_interrupt_user() to simulate a fake
     * exception for handling outside the CPU execution loop. Our
     * magic_ramloc test handles this elsewhere
     */
#else
;fprintf(stderr, "** about to do_interrupt() ** passing env=%p\n", env);
    /* Interrupt the CPU */
    do_interrupt(env);  /* target-i386: do_interrupt_all() */
#endif
#if 0	/* never implemented for target-z80 */
    /* successfully delivered */
    env->old_exception = -1;
#endif
}
