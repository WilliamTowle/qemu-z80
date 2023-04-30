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
