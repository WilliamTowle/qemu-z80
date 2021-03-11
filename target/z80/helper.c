/*
 * QEmu Z80 CPU - helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting by William Towle 2018-2022
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


void z80_cpu_dump_state(CPUState *cs, FILE *f, fprintf_function cpu_fprintf,
                        int flags)
{
    Z80CPU *cpu= Z80_CPU(cs);
    CPUZ80State *env= &cpu->env;

    int fl = env->regs[R_F];

    cpu_fprintf(f, "AF =%04x BC =%04x DE =%04x HL =%04x IX=%04x\n"
                    "AF'=%04x BC'=%04x DE'=%04x HL'=%04x IY=%04x\n"
                    "PC =%04x SP =%04x F=[%c%c%c%c%c%c%c%c]\n"
                    "IM=%i IFF1=%i IFF2=%i I=%02x R=%02x\n",

                    (env->regs[R_A] << 8) | env->regs[R_F],
                    env->regs[R_BC], env->regs[R_DE],
                    env->regs[R_HL], env->regs[R_IX],

                    (env->regs[R_AX] << 8) | env->regs[R_FX],
                    env->regs[R_BCX], env->regs[R_DEX],
                    env->regs[R_HLX], env->regs[R_IY],

                    env->pc, env->regs[R_SP],
                    fl & 0x80 ? 'S' : '-',
                    fl & 0x40 ? 'Z' : '-',
                    fl & 0x20 ? 'Y' : '-',
                    fl & 0x10 ? 'H' : '-',
                    fl & 0x08 ? 'X' : '-',
                    fl & 0x04 ? 'P' : '-',
                    fl & 0x02 ? 'N' : '-',
                    fl & 0x01 ? 'C' : '-',
                    env->imode, env->iff1, env->iff2, env->regs[R_I], env->regs[R_R]);
}

#if !defined(CONFIG_USER_ONLY)
hwaddr z80_cpu_get_phys_page_debug(CPUState *cs, vaddr addr)
{
	return addr; /* assumes 1:1 address correspondance */
}
#endif