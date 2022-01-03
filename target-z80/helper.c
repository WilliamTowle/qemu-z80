/*
 *  Z80 helpers
 * 
 *  Copyright (c) 2007 Stuart Brady
 *  Copyright (c) 2003 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "exec.h"

//#define DEBUG_PCALL

const uint8_t parity_table[256] = {
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
};

/* modulo 17 table */
const uint8_t rclw_table[32] = {
    0, 1, 2, 3, 4, 5, 6, 7, 
    8, 9,10,11,12,13,14,15,
   16, 0, 1, 2, 3, 4, 5, 6,
    7, 8, 9,10,11,12,13,14,
};

/* modulo 9 table */
const uint8_t rclb_table[32] = {
    0, 1, 2, 3, 4, 5, 6, 7, 
    8, 0, 1, 2, 3, 4, 5, 6,
    7, 8, 0, 1, 2, 3, 4, 5, 
    6, 7, 8, 0, 1, 2, 3, 4,
};

    
/* thread support */

spinlock_t global_cpu_lock = SPIN_LOCK_UNLOCKED;

void cpu_lock(void)
{
    spin_lock(&global_cpu_lock);
}

void cpu_unlock(void)
{
    spin_unlock(&global_cpu_lock);
}

void do_interrupt(CPUZ80State *env)
{
// printf("z80: do_interrupt()\n");

    if (!env->iff1)
        return;

    env->iff1 = 0;
    env->iff2 = 0; /* XXX: Unchanged for NMI */

    {
        target_ulong sp;
        sp = (uint16_t)(env->regs[R_SP] - 2);
        env->regs[R_SP] = sp;
        stw_kernel(sp, env->pc);
    }

    /* IM0 = execute data on bus (0xff == rst $38) */
    /* IM1 = execute rst $38 (ROM uses this)*/
    /* IM2 = indirect jump -- address is held at (I << 8) | DATA */

    /* value on data bus is 0xff for the zx spectrum */

    /* when an interrupt occurs, iff1 and iff2 are reset, disabling interrupts */
    /* when an NMI occurs, iff1 is reset. iff2 is left unchanged */

    uint8_t d;
    switch (env->imode) {
        case 0:
            /* XXX: assuming 0xff on data bus */
        case 1:
            env->pc = 0x0038;
            break;
        case 2:
            /* XXX: assuming 0xff on data bus */
            d = 0xff; 
            env->pc = lduw_kernel((env->regs[R_I] << 8) | d);
            break;
    }
}

/*
 * Signal an interruption. It is executed in the main CPU loop.
 * is_int is TRUE if coming from the int instruction. next_eip is the
 * EIP value AFTER the interrupt instruction. It is only relevant if
 * is_int is TRUE.  
 */
void raise_interrupt(int intno, int is_int, int error_code, 
                     int next_eip_addend)
{
    env->exception_index = intno;
    env->error_code = error_code;
    env->exception_is_int = is_int;
    env->exception_next_pc = env->pc + next_eip_addend;
    cpu_loop_exit();
}

/* same as raise_exception_err, but do not restore global registers */
static void raise_exception_err_norestore(int exception_index, int error_code)
{
    env->exception_index = exception_index;
    env->error_code = error_code;
    env->exception_is_int = 0;
    env->exception_next_pc = 0;
    longjmp(env->jmp_env, 1);
}

/* shortcuts to generate exceptions */

void (raise_exception_err)(int exception_index, int error_code)
{
    raise_interrupt(exception_index, 0, error_code, 0);
}

void raise_exception(int exception_index)
{
    raise_interrupt(exception_index, 0, 0, 0);
}

void helper_hlt(void)
{
    //printf("halting at PC 0x%x\n",env->pc);
    env->halted = 1;
    env->hflags &= ~HF_INHIBIT_IRQ_MASK; /* needed if sti is just before */
    env->hflags |= HF_HALTED_MASK;
    env->exception_index = EXCP_HLT;
    cpu_loop_exit();
}

void helper_monitor(void)
{
}

void helper_mwait(void)
{
}

#if !defined(CONFIG_USER_ONLY) 

#define MMUSUFFIX _mmu
#define GETPC() (__builtin_return_address(0))

#define SHIFT 0
#include "softmmu_template.h"

#define SHIFT 1
#include "softmmu_template.h"

#define SHIFT 2
#include "softmmu_template.h"

#define SHIFT 3
#include "softmmu_template.h"

#endif

/* try to fill the TLB and return an exception if error. If retaddr is
   NULL, it means that the function was called in C code (i.e. not
   from generated code or from helper.c) */
/* XXX: fix it to restore all registers */
void tlb_fill(target_ulong addr, int is_write, int is_user, void *retaddr)
{
    TranslationBlock *tb;
    int ret;
    unsigned long pc;
    CPUZ80State *saved_env;

    /* XXX: hack to restore env in all cases, even if not called from
       generated code */
    saved_env = env;
    env = cpu_single_env;

    ret = cpu_z80_handle_mmu_fault(env, addr, is_write, is_user, 1);
    if (ret) {
        if (retaddr) {
            /* now we have a real cpu fault */
            pc = (unsigned long)retaddr;
            tb = tb_find_pc(pc);
            if (tb) {
                /* the PC is inside the translated code. It means that we have
                   a virtual CPU fault */
                cpu_restore_state(tb, env, pc, NULL);
            }
        }
        if (retaddr)
            raise_exception_err(env->exception_index, env->error_code);
        else
            raise_exception_err_norestore(env->exception_index, env->error_code);
    }
    env = saved_env;
}

void helper_in_debug(int port)
{
//    printf("IN with port %02x\n", port);
}

void helper_dump_registers(int pc)
{
    int fl = env->regs[R_F];
#if 0
    printf("--------------\n"
           "AF =%04x BC =%04x DE =%04x HL =%04x IX=%04x\n"
           "AF'=%04x BC'=%04x DE'=%04x HL'=%04x IY=%04x\n"
           "PC =%04x SP =%04x F=[%c%c%c%c%c%c%c%c]\n"
           "IM=%i IFF1=%i IFF2=%i I=%02x R=%02x\n",
           (env->regs[R_A] << 8) | env->regs[R_F],
           env->regs[R_BC],
           env->regs[R_DE],
           env->regs[R_HL],
           env->regs[R_IX],
           env->regs[R_AFX],
           env->regs[R_BCX],
           env->regs[R_DEX],
           env->regs[R_HLX],
           env->regs[R_IY],
           pc == -1 ? env->pc : pc,
           env->regs[R_SP],
           fl & 0x80 ? 'S' : '-',
           fl & 0x40 ? 'Z' : '-',
           fl & 0x20 ? '5' : '-',
           fl & 0x10 ? 'H' : '-',
           fl & 0x08 ? '3' : '-',
           fl & 0x04 ? 'P' : '-',
           fl & 0x02 ? 'N' : '-',
           fl & 0x01 ? 'C' : '-',
           env->imode, env->iff1, env->iff2, env->regs[R_I], env->regs[R_R]);
#endif
}
