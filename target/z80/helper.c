/*
 * QEmu Z80 CPU - helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting by William Towle 2018-2023
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

#include "qemu/qemu-print.h"
#include "exec/cpu_ldst.h"


#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 helper: " fmt , ## __VA_ARGS__); } while(0)


void z80_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    Z80CPU *cpu= Z80_CPU(cs);
    CPUZ80State *env= &cpu->env;
    int fl = env->regs[R_F];

    qemu_fprintf(f, "AF =%04x BC =%04x DE =%04x HL =%04x IX=%04x\n"
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


/* Plain load/store to physical RAM */
static inline uint16_t z80_lduw_kernel(CPUZ80State *env, uint16_t addr)
{
    return cpu_lduw_data(env, addr);
}

static inline void z80_stuw_kernel(CPUZ80State *env, uint16_t addr, uint16_t v)
{
    cpu_stw_data(env, addr, v);
}


/*
 * Begin execution of an interruption. Matches target-i386 [code now
 * in seg_helper.c] but without parameters for CPU register control
 */
static void do_interrupt_all(Z80CPU *cpu, int intno)
{
    CPUZ80State *env= &cpu->env;
//;DPRINTF("z80: do_interrupt()\n");
;DPRINTF("DEBUG: Reached %s() OK -- intno %d, IFF %s...\n", __func__, intno, env->iff1?"value non-zero":"will bail");

    /* NB: target-i386 do_interrupt_all() starts with logging */

    /* Verbatim from repo.or.cz do_interrupt() [op_helper.c]
     * Interrupt the CPU. For the usermode case QEmu exits the
     * execution loop (returning the exception number) later
     */
    if (!env->iff1) {
        return;
    }

    env->iff1 = 0;
    env->iff2 = 0; /* XXX: Unchanged for NMI */

    {
        target_ulong sp;
        sp = (uint16_t)(env->regs[R_SP] - 2);
        env->regs[R_SP] = sp;
#if 0	/* stw_kernel() missing without MMU modes */
        stw_kernel(sp, env->pc);
#else
        z80_stuw_kernel(env, sp, env->pc);
#endif
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
#if 0	/* stw_kernel() missing without MMU modes */
        env->pc = lduw_kernel((env->regs[R_I] << 8) | d);
#else
        env->pc = z80_lduw_kernel(env, (env->regs[R_I] << 8) | d);
#endif
        break;
    }
}

void z80_cpu_do_interrupt(CPUState *cs)
{
    Z80CPU *cpu = Z80_CPU(cs);
    CPUZ80State *env = &cpu->env;

    /* Handle the ILLOP exception as thrown by disas_insn() by
     * bailing cleanly. For z80-bblbrx-user there are no I/O
     * interrupts
     */
    if (cs->exception_index == EXCP_ILLOP)
    {
        cpu_abort(cs, "EXCP_ILLOP at pc=0x%04x", env->pc);
    }


#if defined(CONFIG_USER_ONLY)
    /* target-i386 has do_interrupt_user() to simulate a fake
     * exception for handling outside the CPU execution loop. Our
     * magic_ramloc test handles this elsewhere
     */
//    do_interrupt_user(env, cs->exception_index,
//                      env->exception_is_int,
//                      env->error_code /*,
//                      env->exception_next_eip */);
//#if 0	/* i386-specific */
//    /* successfully delivered */
//    env->old_exception = -1;
//#endif
#else
#if 0	/* codes above EXCP_VMEXIT are produced by i386 svm_helper.c */
    if (cs->exception_index >= EXCP_VMEXIT) {
        assert(env->old_exception == -1);
        do_vmexit(env, cs->exception_index - EXCP_VMEXIT, env->error_code);
    } else {
#endif
#if 0	/* target-i386 style do_interrupt_all() */
        do_interrupt_all(cpu, cs->exception_index,
                         env->exception_is_int,
                         env->error_code /*,
                         env->exception_next_eip, 0 */);
        /* successfully delivered */
        env->old_exception = -1;
#else
        do_interrupt_all(cpu, cs->exception_index   /* ? intno */);
#endif
    //}
#endif
}


#if 0   /* overkill? */
static void do_interrupt_z80_hardirq(CPUState *cs, int intno)
{
    Z80CPU *cpu = Z80_CPU(cs);

#if 0	/* target-i386 style do_interrupt_all() */
    do_interrupt_all(env_archcpu(env), intno, 0, 0, 0, is_hw);
#else
    do_interrupt_all(cpu, intno);
#endif
}
#endif

bool z80_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    /* NB: i386 checks interrupt request for
     * - CPU_INTERRUPT_POLL if !CONFIG_USER_ONLY (not applicable)
     * - CPU_INTERRUPT_SIPI (not applicable)
     * - CPU_INTERRUPT_{SMI|NMI|MCE} (not applicable/not implemented)
     * - CPU_INTERRUPT_HARD
     * - CPU_INTERRUPT_VIRQ (not applicable)
     */

    //Z80CPU *cpu = Z80_CPU(cs);
    //CPUZ80State *env = &cpu->env;

    //interrupt_request = z80_cpu_pending_interrupt(cs, interrupt_request);
    if (!interrupt_request) {
        return false;
    }

    /* Don't process multiple interrupt requests in a single call.
     * This is required to make icount-driven execution deterministic.
     */
    switch (interrupt_request) {
#if 0   /* how target-i386 does NMI  */
    case CPU_INTERRUPT_NMI:
        cpu_svm_check_intercept_param(env, SVM_EXIT_NMI, 0, 0);
        cs->interrupt_request &= ~CPU_INTERRUPT_NMI;
        env->hflags2 |= HF2_NMI_MASK;
        do_interrupt_x86_hardirq(env, EXCP02_NMI, 1);
        break;
#endif
    case CPU_INTERRUPT_HARD:
        cs->interrupt_request &= ~CPU_INTERRUPT_HARD;
        {   /* NB. for INTERRUPT_HARD (and INTERRUPT_VIRQ),
             * target-i386 determines an 'intno' value to pass to
             * do_interrupt_x86_hardirq() (which wraps
             * do_interrupt_all()). It will be -1 (usermode) or >= 0
             */
            Z80CPU *cpu = Z80_CPU(cs);
            do_interrupt_all(cpu, 0 /* intno */);
        }
        break;

        default:
;DPRINTF("*** DEBUG: %s() unmatched interrupt_request %d in switch ***\n", __func__, interrupt_request);
    }

    /* Ensure that no TB jump will be modified as the program flow was changed.  */
    return true;
}
