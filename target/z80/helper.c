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

#include "qemu/error-report.h"
#include "exec/cpu_ldst.h"


#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 helper: " fmt , ## __VA_ARGS__); } while(0)


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


/* Plain load/store to physical RAM */
static inline uint16_t z80_lduw_kernel(CPUZ80State *env, uint16_t addr)
{
    return cpu_lduw_data(env, addr);
}

static inline void z80_stuw_kernel(CPUZ80State *env, uint16_t addr, uint16_t v)
{
    cpu_stw_data(env, addr, v);
}

#if !defined(CONFIG_USER_ONLY)
/*
 * Begin execution of an interruption. is_int is TRUE if coming from
 * the int instruction. next_eip is the env->eip value AFTER the interrupt
 * instruction. It is only relevant if is_int is TRUE.
 */
//static void do_interrupt_all(Z80CPU *cpu, int intno, int is_int,
//                             int error_code /*, target_ulong next_eip, int is_hw */)
static void do_interrupt_all(Z80CPU *cpu, int intno)
{
    CPUZ80State *env= &cpu->env;
    //CPUState *cs = env_cpu(env);
//;DPRINTF("z80: do_interrupt()\n");
;DPRINTF("DEBUG: Reached %s() OK; IFF %s...\n", __func__, env->iff1?"value non-zero":"will bail");

    /* Verbatim from repo.or.cz do_interrupt():
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
#else	/* in v1.7.x we called cpu_stw_data() with 'env' etc */
        //cpu_stw_data(env, sp, env->pc);
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
#else	/* in v1.7.x we called cpu_stw_data() with 'env' etc */
        //env->pc = cpu_lduw_data(env, (env->regs[R_I] << 8) | d);
        env->pc = z80_lduw_kernel(env, (env->regs[R_I] << 8) | d);
#endif
        break;
    }
;DPRINTF("DEBUG: %s() set pc to 0x%04x\n", __func__, env->pc);
}
#endif

void z80_cpu_do_interrupt(CPUState *cs)
{
    Z80CPU *cpu = Z80_CPU(cs);
    CPUZ80State *env = &cpu->env;

    /* Handle the ILLOP exception as thrown by disas_insn() by
     * bailing cleanly. For z80-bblbrx-user there are no I/O
     * interrupts
     */
    //if (intno == EXCP_ILLOP)
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
#if 0	/* do_interrupt_all() from i386 */
        do_interrupt_all(cpu, cs->exception_index,
                         env->exception_is_int,
                         env->error_code /*,
                         env->exception_next_eip, 0 */);
        /* successfully delivered */
        env->old_exception = -1;
#else
        do_interrupt_all(cpu, cs->exception_index);
#endif
    //}
#endif
}


#if 0   /* preferred location */
//Xvoid do_interrupt_z80_hardirq(CPUZ80State *cs, int intno, int is_hw)
static void do_interrupt_z80_hardirq(CPUState *cs, int intno)
{
    Z80CPU *cpu = Z80_CPU(cs);

#if 0	/* do_interrupt_all() from i386 */
    do_interrupt_all(env_archcpu(env), intno, 0, 0, 0, is_hw);
#else
    do_interrupt_all(cpu, intno);
#endif
}
#endif

bool z80_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    /* NB: i386 checks interrupt request for
     * - CPU_INTERRUPT_POLL (not applicable)
     * - CPU_INTERRUPT_SIPI (not applicable)
     * - CPU_INTERRUPT_{SMI|NMI|MCE} (not applicable/not implemented)
     * - CPU_INTERRUPT_HARD
     * - CPU_INTERRUPT_VIRQ (not applicable)
     */

    //Z80CPU *cpu = Z80_CPU(cs);
    //CPUZ80State *env = &cpu->env;
    bool ret;

    //interrupt_request = z80_cpu_pending_interrupt(cs, interrupt_request);
    if (!interrupt_request) {
        return false;
    }

    ret = false;
    /* Don't process multiple interrupt requests in a single call.
     * This is required to make icount-driven execution deterministic.
     */
    switch (interrupt_request) {
#if 0   /* how i386 does NMI  */
    case CPU_INTERRUPT_NMI:
        cpu_svm_check_intercept_param(env, SVM_EXIT_NMI, 0, 0);
        cs->interrupt_request &= ~CPU_INTERRUPT_NMI;
        env->hflags2 |= HF2_NMI_MASK;
        do_interrupt_x86_hardirq(env, EXCP02_NMI, 1);
        break;
#endif
    case CPU_INTERRUPT_HARD:
#if 0   /* i386 verbatim */
        cpu_svm_check_intercept_param(env, SVM_EXIT_INTR, 0, 0);
        cs->interrupt_request &= ~(CPU_INTERRUPT_HARD |
                                   CPU_INTERRUPT_VIRQ);
        intno = cpu_get_pic_interrupt(env);
        qemu_log_mask(CPU_LOG_TB_IN_ASM,
                      "Servicing hardware INT=0x%02x\n", intno);
        do_interrupt_x86_hardirq(env, intno, 1);
#else
        cs->interrupt_request &= ~CPU_INTERRUPT_HARD;
#if 0	/* overkill? */
	/* TODO: i386 cpu_get_pic_interrupt() is either:
	 * - defined in linux-user/main.c, always returns -1
	 * - defined in hw/i386/pc.c, can return >= 0
	 */
;DPRINTF("*** DEBUG: %s() about to pass CPU_INTERRUPT_HARD (=%d) to do_interrupt_z80_hardirq()... ***\n", __func__, CPU_INTERRUPT_HARD);
        do_interrupt_z80_hardirq(cs, CPU_INTERRUPT_HARD /* ?intno */);
#else
        {
            CPUClass *cc = CPU_GET_CLASS(cs);
            cc->do_interrupt(cs);
        }
        ret = true;
#endif
#endif
        break;

        default:
;DPRINTF("*** DEBUG: %s() switch on interrupt_request unmatched ***\n", __func__);
    }

    /* Indicate not to modify the TB jump, due to interrupt handling
     * having changed the program flow
     */
    return ret;
}
