/*
 * QEmu Z80 CPU - exception helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2018-2023 William Towle <william_towle@yahoo.co.uk>
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


//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 excp_helper: " fmt , ## __VA_ARGS__); } while(0)


/*
 * Signal an interruption. It is executed in the main CPU loop.
 * is_int is TRUE if coming from the int instruction. next_eip is the
 * env->eip value AFTER the interrupt instruction. It is only relevant if
 * is_int is TRUE.
 */
static void QEMU_NORETURN raise_interrupt2(CPUZ80State *env, int intno,
                                           int is_int, int error_code)
{
    CPUState *cs = env_cpu(env);

    cs->exception_index = intno;
    env->error_code = error_code;
    env->exception_is_int = is_int;
    cpu_loop_exit(cs);
}

void raise_exception(CPUZ80State *env, int exception_index)
{
    raise_interrupt2(env, exception_index, 0, 0);
}

void helper_raise_exception(CPUZ80State *env, int exception_index)
{
    raise_exception(env, exception_index);
}


/* return value:
 * -1 = cannot handle fault
 * 0  = nothing more to do
 * 1  = generate PF fault
 */
#if QEMU_VERSION_MAJOR >= 5
static int handle_mmu_fault(CPUState *cs, vaddr addr, int size,
                            int is_write1, int mmu_idx)
#else
int z80_cpu_handle_mmu_fault(CPUState *cs, vaddr addr, int size,
                             int is_write1, int mmu_idx)
#endif
{
#if ! ( !defined(CONFIG_USER_ONLY) && defined(CONFIG_TCG) )
;DPRINTF("*** DEBUG: no tlb_set_page() ***\n");	/* not called? */
;exit(1);
#else
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

    tlb_set_page(cs, vaddr, paddr, prot, mmu_idx, page_size);
#endif
    return 0;
}

#if QEMU_VERSION_MAJOR >= 5
bool z80_cpu_tlb_fill(CPUState *cs, vaddr addr, int size,
                      MMUAccessType access_type, int mmu_idx,
                      bool probe, uintptr_t retaddr)
#else
void tlb_fill(CPUState *cs, target_ulong addr, int size,
              MMUAccessType access_type, int mmu_idx, uintptr_t retaddr)
#endif
{
    int ret;

    //ret = cpu_z80_handle_mmu_fault(env, addr, is_write, is_user, 1);
#if QEMU_VERSION_MAJOR >= 5
    ret = handle_mmu_fault(cs, addr, size, access_type, mmu_idx);
#else
    ret = z80_cpu_handle_mmu_fault(cs, addr, size, access_type, mmu_idx);
#endif
    if (unlikely(ret)) {    /* never a fault if no MMU */
        if (retaddr) {
            /* [QEmu v2] 'retaddr' of NULL indicates a call from
             * C code (and generated code/from helper.c otherwise). An
             * exception is always raised.
             */
            cpu_loop_exit_restore(cs, retaddr);
        }
    }
#if QEMU_VERSION_MAJOR >= 5
    return true;
#endif
}
