/*
 * Z80 execution defines
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
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

#include "config.h"
#include "dyngen-exec.h"

#include "cpu.h"	/* for TARGET_LONG_BITS */

#include "cpu-defs.h"

register struct CPUZ80State *env asm(AREG0);

//#define A0 (env->a0)

#define A   (env->regs[R_A])
#define F   (env->regs[R_F])
#define BC  (env->regs[R_BC])
#define DE  (env->regs[R_DE])
#define HL  (env->regs[R_HL])
#define IX  (env->regs[R_IX])
#define IY  (env->regs[R_IY])
#define SP  (env->regs[R_SP])
#define I   (env->regs[R_I])
#define R   (env->regs[R_R])
#define AX  (env->regs[R_AX])
#define FX  (env->regs[R_FX])
#define BCX (env->regs[R_BCX])
#define DEX (env->regs[R_DEX])
#define HLX (env->regs[R_HLX])

//#define PC  (env->pc)


/* op_helper.c */
void raise_interrupt(int intno, int is_int, int error_code,
                     int next_eip_addend);
void raise_exception(int exception_index);
