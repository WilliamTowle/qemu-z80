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
#define ZAPHOD_HAS_SCREEN
#define ZAPHOD_HAS_SERCON


#include "exec/address-spaces.h"
#include "exec/ioport.h"

#ifdef ZAPHOD_HAS_SCREEN
#include "zaphod_screen.h"
#endif
#define ZAPHOD_HAS_MC6850
#ifdef ZAPHOD_HAS_SERCON
#include "zaphod_sercon.h"
#endif

/* Z80_MAX_RAM_SIZE:
 * Address space for a Z80 ends at 64K (some emulations might use less)
 */
#define	Z80_MAX_RAM_SIZE	(64 * 1024) /* 64KiB */


#ifdef ZAPHOD_HAS_MC6850
typedef struct {
    PortioList *ports;
} ZaphodMC6850State;
#endif


typedef enum {                  /* "features" bit map bit offsets */
    ZAPHOD_SIMPLE_SCREEN = 0,
    ZAPHOD_FEATURE_MC6850 = 1
} zaphod_feature_t;


struct ZaphodState {
    int             features;
    CPUZ80State     *cpu;
    MemoryRegion    *ram;
    uint8_t         inkey;

#ifdef ZAPHOD_HAS_SCREEN
    ZaphodScreenState    *screen;
#endif
#ifdef ZAPHOD_HAS_MC6850
    ZaphodMC6850State   *mc6850;
#endif
#ifdef ZAPHOD_HAS_SERCON
    ZaphodSerConState   *sercon;
#endif
};


int zaphod_has_feature(ZaphodState *zs, zaphod_feature_t n);

#endif	/*  HW_Z80_ZAPHOD_H  */
