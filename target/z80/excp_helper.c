/*
 * QEmu Z80 CPU - exception helpers
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

#include "qemu/error-report.h"
#include "exec/exec-all.h"
#include "exec/helper-proto.h"


#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 excp_helper: " fmt , ## __VA_ARGS__); } while(0)


/*
 * Signal an interruption. It is executed in the main CPU loop.
 * is_int is TRUE if coming from the int instruction. next_eip is the
 * env->eip value AFTER the interrupt instruction. It is only relevant if
 * is_int is TRUE.
 */
static void QEMU_NORETURN raise_interrupt2(CPUZ80State *env, int intno,
                                           int is_int, int error_code,
                                           int next_eip_addend,
                                           uintptr_t retaddr)
{
    CPUState *cs = CPU(z80_env_get_cpu(env));

#if 0   /* x86-specific */
    if (!is_int) {
        cpu_svm_check_intercept_param(env, SVM_EXIT_EXCP_BASE + intno,
                                      error_code, retaddr);
        intno = check_exception(env, intno, &error_code, retaddr);
    } else {
        cpu_svm_check_intercept_param(env, SVM_EXIT_SWINT, 0, retaddr);
    }
#endif

    cs->exception_index = intno;
    env->error_code = error_code;
    env->exception_is_int = is_int;
#if 0	/* i386 specific */
    env->exception_next_eip = env->eip + next_eip_addend;
#endif
    cpu_loop_exit_restore(cs, retaddr);
}

void raise_exception(CPUZ80State *env, int exception_index)
{
    raise_interrupt2(env, exception_index, 0, 0, 0, 0);
}


void helper_raise_exception(CPUZ80State *env, int exception_index)
{
    raise_exception(env, exception_index);
}


#if defined(CONFIG_USER_ONLY)
#if QEMU_VERSION_MAJOR < 2
/* [QEmu v1/v2] if called, handle_cpu_signal() asserts without an
 * MMU fault handler set. This function contains code to generate a
 * page error (as per target/i386) for cases with an MMU.
 */
int z80_cpu_handle_mmu_fault(CPUState *cs, vaddr addr, int size,
                             int is_write, int mmu_idx)
{
    /* user mode only emulation */
    is_write &= 1;
    env->cr[2] = addr;
#if 0  /* not z80 */
    env->error_code = (is_write << PG_ERROR_W_BIT);
    env->error_code |= PG_ERROR_U_MASK;
#endif
    env->exception_index = EXCP0E_PAGE;
    return 1;
}
#endif
#else   /* wrap tlb_set_page() for use by tlb_fill() */
/* return value:
 * -1 = cannot handle fault
 * 0  = nothing more to do
 * 1  = generate PF fault
 * 2  = soft MMU activation required for this block
 */
int z80_cpu_handle_mmu_fault(CPUState *cs, vaddr addr, int size,
                             int is_write1, int mmu_idx)
{
    int prot, page_size /* , ret, is_write */;
    unsigned long paddr, page_offset;
    target_ulong vaddr, virt_addr;
    //int is_user = 0;

    //is_write = is_write1 & 1;

    virt_addr = addr & TARGET_PAGE_MASK;
    prot = PAGE_READ | PAGE_WRITE | PAGE_EXEC;
    page_size = TARGET_PAGE_SIZE;

    /* TODO: port, handle memory mapping (env->mapaddr) support? */

    page_offset = (addr & TARGET_PAGE_MASK) & (page_size - 1);
    paddr = (addr & TARGET_PAGE_MASK) + page_offset;
    vaddr = virt_addr + page_offset;

    tlb_set_page(cs, vaddr, paddr, prot, mmu_idx, TARGET_PAGE_SIZE);
    return 0;
}
#endif

#if !defined(CONFIG_USER_ONLY)
void tlb_fill(CPUState *cs, target_ulong addr, int size,
              MMUAccessType access_type, int mmu_idx, uintptr_t retaddr)
{
    int ret;

    //ret = cpu_z80_handle_mmu_fault(env, addr, is_write, is_user, 1);
    ret = z80_cpu_handle_mmu_fault(cs, addr, size, access_type, mmu_idx);
    if (unlikely(ret)) {    /* no MMU -> always get '0' */
        if (retaddr) {
            /* [QEmu v2] i386 calls its wrapper to set exception error
             * state here; like lm32, we don't need the diversion
             */
            cpu_loop_exit_restore(cs, retaddr);
        }
    }
}
#endif  /* !defined(CONFIG_USER_ONLY) */
