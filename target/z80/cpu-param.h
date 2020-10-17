/*
 * Minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2018-2022 William Towle <william_towle@yahoo.co.uk>
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
#define TARGET_PHYS_ADDR_SPACE_BITS 24
#define TARGET_VIRT_ADDR_SPACE_BITS 24
#define TARGET_PAGE_BITS 8
#define NB_MMU_MODES 1

#endif /* !defined Z80_CPU_PARAM_H */
