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
    //Z80CPU *cpu = Z80_CPU(cs);

    /* TARGET_Z80: The Zaphod binary execution layer doesn't set
     * up IRQs, so there are no I/O interrupts but there are
     * still exceptions (ILLOP, KERNEL_TRAP).
     * For the CONFIG_USER_ONLY case, target-x86 calls
     * do_interrupt_user()
     */

#if 0
    /* successfully delivered */
    env->old_exception = -1;
#endif
}
