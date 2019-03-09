/*
 * Z80 virtual CPU header
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
#ifndef CPU_Z80_H
#define CPU_Z80_H

#include "config.h"

#define TARGET_LONG_BITS 32

#define CPUState struct CPUZ80State

#include "cpu-defs.h"

typedef struct CPUZ80State {
//	CPU_COMMON
} CPUZ80State;

CPUZ80State *cpu_z80_init(const char *cpu_model);

#define TARGET_VIRT_ADDR_SPACE_BITS 32

#define cpu_init cpu_z80_init

#include "cpu-all.h"

#endif /* CPU_Z80_H */
