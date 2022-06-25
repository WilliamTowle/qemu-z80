/*
 * QEmu Zaphod machine family
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef HW_Z80_ZAPHOD_H
#define HW_Z80_ZAPHOD_H

#define ZAPHOD_DEBUG	1

#include "config.h"
/* Feature configuration - see also z80-softmmu.mak */
#define ZAPHOD_HAS_SERCON

#include "exec/address-spaces.h"
#include "exec/ioport.h"

#ifdef ZAPHOD_HAS_SERCON
#include "zaphod_sercon.h"
#endif


/* Z80_MAX_RAM_SIZE:
 * Address space for a Z80 ends at 64K (some emulations might use less)
 */
#define	Z80_MAX_RAM_SIZE	(64 * 1024) /* 64KiB */


typedef struct ZaphodState {
    CPUZ80State     *cpu;
    MemoryRegion    *ram;
    PortioList      *ports;

    //CharDriverState *sercon;            /* QEmu serial0 console */
    uint8_t         inkey;

#ifdef ZAPHOD_HAS_SERCON
    ZaphodSerConState   *sercon;
#endif
} ZaphodState;

#endif	/*  HW_Z80_ZAPHOD_H  */
