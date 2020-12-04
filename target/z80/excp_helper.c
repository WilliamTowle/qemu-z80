/*
 * Minimal QEmu Z80 CPU - exception helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2018-2022 William Towle <william_towle@yahoo.co.uk>
 *  All versions under GPL
 */

#include "qemu/osdep.h"
#include "cpu.h"

#include "qemu/error-report.h"
#include "exec/exec-all.h"
#include "exec/helper-proto.h"


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
