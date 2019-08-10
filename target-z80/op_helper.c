/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPLv2+...]
 */


#include "cpu.h"
#include "exec.h"
#include "helper.h"
#include "dyngen-exec.h"

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
