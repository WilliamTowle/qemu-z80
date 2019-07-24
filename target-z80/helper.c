/*
 * Z80 helpers (without register variable usage)
 *
 *  Copyright (c) 2007 Stuart Brady <stuart.brady@gmail.com>
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

#if 1	/* temp'y */
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

static int cpu_z80_find_by_name(const char *name);

CPUZ80State *cpu_z80_init(const char *model)
{
    CPUZ80State *env;
    static int inited;
    int id;

    id = cpu_z80_find_by_name(model);
    if (id == 0) {
        return NULL;
    }

    env= calloc(1, sizeof *env);
    cpu_exec_init(env);

    /* init various static tables */
    if (!inited) {
        inited = 1;
        z80_translate_init();
    }

#if defined(TARGET_Z80)
;printf("%s(): PARTIAL - model/flags init missing...\n", __func__);
#endif
    /* PARTIAL: cpu_z80_init() continues (requiring enhanced
     * CPUZ80State?) with:
     * 1. z80_translate_init() call, if not already done
     * 2. store id in env->model
     *
     * ...target-i386 has:
     *	1. initialising env->cpu_model_str
     *	2. one-shot flags optimisation
     *	3. breakpoint handler management
     *	4. registering CPU[s] (with cpu_x86_close() on failure)
     */

    cpu_reset(env);		/* target-i386: in wrapper functions */
    qemu_init_vcpu(env);

    return env;
}

/* Legacy cpu_*_find_by_name()
 * This function returns 0 to signify "not found"; it uses the
 * Z80_CPU_* constants in cpu.h for success values. In target-i386
 * for QEmu v1+ an index into an array is returned, with -1 on error
 */
static int cpu_z80_find_by_name(const char *name)
{
    /* PARTIAL: qemu-z80 iterates around z80_cpu_names[] list,
     * containing "z80", "r800", an end-of-list sentinel, and
     * the Z80_CPU_* constants used as return values
     */
    if (strcmp(name, "z80") == 0)
    {
        return 1;	/* value of Z80_CPU_Z80 */
    }

  return 0;
}

void cpu_reset(CPUZ80State *env)
{
#if defined(TARGET_Z80)
;printf("%s(): PARTIAL - missing breakpoint init, TLB reset...\n", __func__);
#endif

    /* PARTIAL: target-z80 version starts with:
     * 1. record of function call in logs
     * 2. memset() for breakpoints
     * 3. a tlb_flush() call:
     *	tlb_flush(env, 1);
     */

    /* init to reset state */

#ifdef CONFIG_SOFTMMU
    env->hflags |= HF_SOFTMMU_MASK;
#endif

    env->pc = 0x0000;
    env->iff1 = 0;
    env->iff2 = 0;
    env->imode = 0;
    env->regs[R_A] = 0xff;
    env->regs[R_F] = 0xff;
    env->regs[R_SP] = 0xffff;
}

void cpu_dump_state(CPUState *env, FILE *f,
                    int (*cpu_fprintf)(FILE *f, const char *fmt, ...),
                    int flags)
{
    int fl = env->regs[R_F];

    cpu_fprintf(f, "AF =%04x BC =%04x DE =%04x HL =%04x IX=%04x\n"
                   "AF'=%04x BC'=%04x DE'=%04x HL'=%04x IY=%04x\n"
                   "PC =%04x SP =%04x F=[%c%c%c%c%c%c%c%c]\n"
                   "IM=%i IFF1=%i IFF2=%i I=%02x R=%02x\n",
                   (env->regs[R_A] << 8) | env->regs[R_F],
                   env->regs[R_BC],
                   env->regs[R_DE],
                   env->regs[R_HL],
                   env->regs[R_IX],
                   (env->regs[R_AX] << 8) | env->regs[R_FX],
                   env->regs[R_BCX],
                   env->regs[R_DEX],
                   env->regs[R_HLX],
                   env->regs[R_IY],
                   env->pc, /* pc == -1 ? env->pc : pc, */
                   env->regs[R_SP],
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

/* return value:
   -1 = cannot handle fault
   0  = nothing more to do
   1  = generate pf fault
   2  = soft mmu activation required for this block
*/
int cpu_z80_handle_mmu_fault(CPUZ80State *env, target_ulong addr,
                             int is_write1, int mmu_idx, int is_softmmu)
{
#if 1	/* temp'y */
	printf("%s() skeleton ... INCOMPLETE\n", __func__);
	exit(1);
#else
	/* TODO: see g106f733 "Add target-z80 tree, z80-dis.c" */
#endif
}
