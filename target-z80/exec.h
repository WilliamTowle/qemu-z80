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

/* op_helper.c */
void raise_interrupt(int intno, int is_int, int error_code,
                     int next_eip_addend);
void raise_exception(int exception_index);
