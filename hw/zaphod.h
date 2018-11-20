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
#define ZAPHOD_HAS_SCREEN
#define ZAPHOD_HAS_MC6850

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

#ifdef ZAPHOD_HAS_SCREEN
typedef struct {
    DisplayState    *ds;
    uint8_t         *rgb_bg, *rgb_fg;
    bool            curs_visible;
    int64_t         curs_blink_time;    /* millisec */
} ZaphodScreenState;
#endif

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
    CPUState        *cpu;
    MemoryRegion    *ram;
    uint8_t         inkey;

#ifdef ZAPHOD_HAS_SERCON
    ZaphodSerConState   *sercon;
#endif
#ifdef ZAPHOD_HAS_SCREEN
    ZaphodScreenState   *screen;
#endif
#ifdef ZAPHOD_HAS_MC6850
    ZaphodMC6850State   *mc6850;
#endif
};

#ifdef ZAPHOD_HAS_SERCON
void zaphod_sercon_putchar(ZaphodSerConState *zss, const unsigned char ch);

ZaphodSerConState *zaphod_new_sercon(ZaphodState *zs, CharDriverState* sercon);
#endif

#ifdef ZAPHOD_HAS_SCREEN
ZaphodScreenState *zaphod_new_screen(ZaphodState *zs);
#endif

#ifdef ZAPHOD_HAS_MC6850
ZaphodMC6850State *zaphod_new_mc6850(ZaphodState *s);
#endif

int zaphod_has_feature(ZaphodState *zs, zaphod_feature_t n);
#endif	/*  _ZAPHOD_H_  */
