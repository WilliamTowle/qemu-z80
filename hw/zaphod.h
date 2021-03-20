/*
 * QEmu Zaphod machine family
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2021, under GPL...]
 */

#ifndef _ZAPHOD_H_
#define _ZAPHOD_H_

#define ZAPHOD_DEBUG	1

#include "config.h"

#include "exec-memory.h"

/* Z80_MAX_RAM_SIZE:
 * Address space for a Z80 ends at 64K (some emulations might use less)
 */
#define	Z80_MAX_RAM_SIZE	(64 * 1024) /* 64KiB */

typedef struct {
    CPUState        *cpu;
    MemoryRegion    *ram;
    PortioList      *ports;

    CharDriverState *sercon;            /* QEmu serial0 console */
    uint8_t         inkey;
} ZaphodState;

#endif	/*  _ZAPHOD_H_  */
