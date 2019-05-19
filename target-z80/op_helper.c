/*
 * Z80 helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007 Stuart Brady <stuart.brady@gmail.com>
 *  Porting to QEmu 0.12+ by William Towle <william_towle@yahoo.co.uk>
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

/* TODO: target-x86 uses 'excp_helper.c' */
void helper_raise_interrupt(CPUZ80State *env, int intno, int next_eip_addend)
{
    raise_interrupt(env, intno, 1, 0, next_eip_addend);
}

void helper_raise_exception(CPUZ80State *env, int exception_index)
{
    raise_exception(env, exception_index);
}

/*
 * Signal an interruption. It is executed in the main CPU loop.
 * is_int is TRUE if coming from the int instruction. next_eip is the
 * EIP value AFTER the interrupt instruction. It is only relevant if
 * is_int is TRUE.
 */
static void QEMU_NORETURN raise_interrupt2(CPUZ80State *env, int intno,
                                           int is_int, int error_code,
                                           int next_eip_addend)
{
#if 0   /* x86-specific */
    if (!is_int) {
        cpu_svm_check_intercept_param(env, SVM_EXIT_EXCP_BASE + intno,
                                      error_code);
        intno = check_exception(env, intno, &error_code);
    } else {
        cpu_svm_check_intercept_param(env, SVM_EXIT_SWINT, 0);
    }
#endif

    env->exception_index = intno;
#if 0   /* never ported from target-i386 */
    env->error_code = error_code;
    env->exception_is_int = is_int;
    env->exception_next_eip = env->eip + next_eip_addend;
#endif
    cpu_loop_exit(env);
}

/* shortcuts to generate exceptions */

void QEMU_NORETURN raise_interrupt(CPUZ80State *env, int intno, int is_int,
                                   int error_code, int next_eip_addend)
{
    raise_interrupt2(env, intno, is_int, error_code, next_eip_addend);
}

void raise_exception_err(CPUZ80State *env, int exception_index,
                         int error_code)
{
    raise_interrupt2(env, exception_index, 0, error_code, 0);
}

void raise_exception(CPUZ80State *env, int exception_index)
{
    raise_interrupt2(env, exception_index, 0, 0, 0);
}

void HELPER(movl_pc_im)(CPUZ80State *env, uint32_t new_pc)
{
    PC = (uint16_t)new_pc;
}
