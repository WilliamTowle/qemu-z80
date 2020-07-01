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

/* returns 0 to signify "not found" */
static int cpu_z80_find_by_name(const char *name)
{
    /* PARTIAL: qemu-z80 iterates around z80_cpu_names[] list,
     * containing "z80", "r800", and an end-of-list sentinel
     */
    if (strcmp(name, "z80") == 0)
    {
        return 1;	/* normally Z80_CPU_Z80, value 1, via list */
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
     */

    tlb_flush(env, 1);

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

/***********************************************************/

#if defined(CONFIG_USER_ONLY)

int cpu_z80_handle_mmu_fault(CPUZ80State *env, target_ulong addr,
                             int is_write, int mmu_idx, int is_softmmu)
{
    /* user mode only emulation */
    is_write &= 1;
    env->cr[2] = addr;
#if 0	/* not z80 */
    env->error_code = (is_write << PG_ERROR_W_BIT);
    env->error_code |= PG_ERROR_U_MASK;
#endif
    env->exception_index = EXCP0E_PAGE;
    return 1;
}

#else

/* return value:
   -1 = cannot handle fault
   0  = nothing more to do
   1  = generate PF fault
   2  = soft MMU activation required for this block
*/
int cpu_z80_handle_mmu_fault(CPUZ80State *env, target_ulong addr,
                             int is_write1, int mmu_idx, int is_softmmu)
{
    int prot, page_size, ret, is_write;
    unsigned long paddr, page_offset;
    target_ulong vaddr, virt_addr;
    int is_user = 0;

#if defined(DEBUG_MMU)
    printf("MMU fault: addr=" TARGET_FMT_lx " w=%d u=%d pc=" TARGET_FMT_lx "\n",
           addr, is_write1, mmu_idx, env->pc);
#endif
    is_write = is_write1 & 1;

    virt_addr = addr & TARGET_PAGE_MASK;
    prot = PAGE_READ | PAGE_WRITE | PAGE_EXEC;
    page_size = TARGET_PAGE_SIZE;

    if (env->mapaddr) {
        addr = env->mapaddr(addr);
    }
    page_offset = (addr & TARGET_PAGE_MASK) & (page_size - 1);
    paddr = (addr & TARGET_PAGE_MASK) + page_offset;
    vaddr = virt_addr + page_offset;

    //ret = tlb_set_page_exec(env, vaddr, paddr, prot, is_user, is_softmmu);
    //return ret;
    tlb_set_page(env, vaddr, paddr, prot, mmu_idx, page_size);
    return 0;
}

target_phys_addr_t cpu_get_phys_page_debug(CPUState *env, target_ulong addr)
{
    uint32_t pte, paddr, page_offset, page_size;

    pte = addr;
    page_size = TARGET_PAGE_SIZE;

    if (env->mapaddr) {
        addr = env->mapaddr(addr);
    }
    page_offset = (addr & TARGET_PAGE_MASK) & (page_size - 1);
    paddr = (pte & TARGET_PAGE_MASK) + page_offset;
    return paddr;
}

#endif /* !CONFIG_USER_ONLY */
