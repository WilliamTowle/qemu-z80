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
#define ZAPHOD_HAS_MC6850


#include "exec/address-spaces.h"
#include "exec/ioport.h"

#ifdef ZAPHOD_HAS_SCREEN
#include "zaphod_screen.h"
#endif
#ifdef ZAPHOD_HAS_SERCON
#include "zaphod_sercon.h"
#endif

#ifdef ZAPHOD_HAS_MC6850
#include "zaphod_mc6850.h"
#endif


/* Z80_MAX_RAM_SIZE:
 * Address space for a Z80 ends at 64K (some emulations might use less)
 */
#if !defined (KiB)		/* from qemu/units.h in QEmu v3+ */
#define KiB			1024
#endif
#define	Z80_MAX_RAM_SIZE	(64 * KiB)



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

/* zaphod.c */
void zaphod_set_inkey(void *opaque, uint8_t val, bool is_data);
uint8_t zaphod_get_inkey(void *opaque, bool read_and_clear);


int zaphod_has_feature(ZaphodState *zs, zaphod_feature_t n);
void zaphod_putchar(ZaphodState *zs, const unsigned char ch);

#endif	/*  HW_Z80_ZAPHOD_H  */
