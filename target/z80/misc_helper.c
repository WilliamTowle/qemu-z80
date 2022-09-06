/*
 * QEmu Z80 CPU - misc helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2018-2022 William Towle <william_towle@yahoo.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
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


void helper_debug(CPUZ80State *env)
{
    CPUState *cs = CPU(z80_env_get_cpu(env));

    cs->exception_index = EXCP_DEBUG;
#if 0	/* v0.15.0+ */
    cpu_loop_exit(env);
#else	/* v2+ */
    cpu_loop_exit(cs);
#endif
}
