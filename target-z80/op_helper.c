/*
 * Z80 helpers
 *
 *  Copyright (c) 2007 Stuart Brady <stuart.brady@gmail.com>
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

#include "cpu.h"
#include "exec.h"
#include "helper.h"
#include "dyngen-exec.h"

#if 1  /* was: TARGET_LONG_BITS > HOST_LONG_BITS
        * but TCG_AREG{1|2} no longer available for else case
        */

/* no registers can be used */
#define T0 (env->t0)
#define T1 (env->t1)

#else
/* XXX: use unsigned long instead of target_ulong - better code will
   be generated for 64 bit CPUs */
target_ulong T0;
//register target_ulong T0 asm(AREG1);
target_ulong T1;
//register target_ulong T1 asm(AREG2);
#endif /* ! (TARGET_LONG_BITS > HOST_LONG_BITS) */

//#define A0 (env->a0)
//
//#define A   (env->regs[R_A])
//#define F   (env->regs[R_F])
//#define BC  (env->regs[R_BC])
//#define DE  (env->regs[R_DE])
//#define HL  (env->regs[R_HL])
//#define IX  (env->regs[R_IX])
//#define IY  (env->regs[R_IY])
//#define SP  (env->regs[R_SP])
//#define I   (env->regs[R_I])
//#define R   (env->regs[R_R])
//#define AX  (env->regs[R_AX])
//#define FX  (env->regs[R_FX])
//#define BCX (env->regs[R_BCX])
//#define DEX (env->regs[R_DEX])
//#define HLX (env->regs[R_HLX])

#define PC  (env->pc)

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
#if 0	/* unimplemented */
    env->error_code = error_code;
    env->exception_is_int = is_int;
    env->exception_next_pc = env->pc + next_eip_addend;
#endif
#if 0	/* obsolete */
    cpu_loop_exit();
#else	/* v0.15.0+ */
    cpu_loop_exit(env);
#endif
}

void raise_exception(int exception_index)
{
    raise_interrupt(exception_index, 0, 0, 0);
}

void HELPER(raise_exception)(uint32_t exception_index)
{
    raise_exception(exception_index);
}

void HELPER(debug)(void)
{
    env->exception_index = EXCP_DEBUG;
#if 0	/* obsolete */
    cpu_loop_exit();
#else	/* v0.15.0+ */
    cpu_loop_exit(env);
#endif
}

//void HELPER(set_inhibit_irq)(void)
//{
//    env->hflags |= HF_INHIBIT_IRQ_MASK;
//}

void HELPER(reset_inhibit_irq)(void)
{
    env->hflags &= ~HF_INHIBIT_IRQ_MASK;
}

void HELPER(movl_pc_im)(uint32_t new_pc)
{
    PC = (uint16_t)new_pc;
}

/* Z80 instruction-specific helpers */

void HELPER(jmp_T0)(void)
{
    PC = T0;
}
