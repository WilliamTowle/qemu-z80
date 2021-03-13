/*
 * QEmu Z80 CPU
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


#ifndef Z80_CPU_PARAM_H
#define Z80_CPU_PARAM_H

/* Optimal host size of target-ulong. '32' leads to TARGET_FMT_lx
 * of "%08x" from cpu-defs.h
 */
#define TARGET_LONG_BITS 32

/* TODO: MMU inclusion is optional; size of pages may vary between
 * devices (eg. 8 * 16k banks for 128K spectrum)
 */
#if 0   /* as repo.or.cz/matches i386 */
#define TARGET_PHYS_ADDR_SPACE_BITS 24
#define TARGET_VIRT_ADDR_SPACE_BITS 24
#define TARGET_PAGE_BITS 12     /* 4KiB pages */
#else
#define TARGET_PHYS_ADDR_SPACE_BITS 28  /* ? */
#define TARGET_VIRT_ADDR_SPACE_BITS 28  /* ? */
#define TARGET_PAGE_BITS 14     /* 16KiB pages */
#endif
#define NB_MMU_MODES 1

#endif /* !defined Z80_CPU_PARAM_H */
