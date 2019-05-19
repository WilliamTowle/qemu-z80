/*
 * QEmu Z80 CPU - exception helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  WmT, 2018-2022 [...after Stuart Brady]
 *  [...all versions under GPL...]
 */

#include "cpu.h"
#include "helper.h"


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

