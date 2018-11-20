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

/* Feature configuration - see also z80-softmmu.mak */

#define ZAPHOD_HAS_SERCON

/* Z80_MAX_RAM_SIZE:
 * Address space for a Z80 ends at 64K (some emulations might use less)
 */
#define	Z80_MAX_RAM_SIZE	(64 * 1024) /* 64KiB */

typedef struct ZaphodState ZaphodState;

#ifdef ZAPHOD_HAS_SERCON
typedef struct {
    ZaphodState     *super;

    PortioList      *ports;
    CharDriverState *sercon;            /* QEmu serial0 console */
} ZaphodSerConState;
#endif

struct ZaphodState {
    CPUState        *cpu;
    MemoryRegion    *ram;
    uint8_t         inkey;

#ifdef ZAPHOD_HAS_SERCON
    ZaphodSerConState   *sercon;
#endif
};

#ifdef ZAPHOD_HAS_SERCON
void zaphod_sercon_putchar(ZaphodSerConState *zss, const unsigned char ch);

ZaphodSerConState *zaphod_new_sercon(ZaphodState *zs, CharDriverState* sercon);
#endif

#endif	/*  _ZAPHOD_H_  */
