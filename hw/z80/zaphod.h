/*
 * QEmu Zaphod machine family
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef _ZAPHOD_H_
#define _ZAPHOD_H_

#define ZAPHOD_DEBUG	1

#include "config.h"

#include "exec/address-spaces.h"
#include "exec/ioport.h"

/* Feature configuration - see also z80-softmmu.mak */

#define ZAPHOD_HAS_SERCON
#define ZAPHOD_HAS_SCREEN
#define ZAPHOD_HAS_MC6850

#ifdef ZAPHOD_HAS_SCREEN
#define ZAPHOD_HAS_KEYBIO
#endif

#ifdef ZAPHOD_HAS_SCREEN
#include "hw/irq.h"
#endif

/* Z80_MAX_RAM_SIZE:
 * Address space for a Z80 ends at 64K (some emulations might use less)
 */
#define	Z80_MAX_RAM_SIZE	(64 * 1024) /* 64KiB */

#define MAX_TEXT_ROWS	25
#define MAX_TEXT_COLS	80

typedef struct ZaphodState ZaphodState;


#ifdef ZAPHOD_HAS_SERCON
typedef struct {
    ZaphodState     *super;
 
    PortioList      *ports;
    CharDriverState *sercon;            /* QEmu serial0 console */
    uint8_t         inkey;
} ZaphodSerConState;
#endif


#ifdef ZAPHOD_HAS_SCREEN
typedef enum {
    ZAPHOD_SCREEN_ATTR_80COL = 0x01,
    ZAPHOD_SCREEN_ATTR_GRAPH = 0x80
} zaphod_screen_attr_t;

#define ROW_ATTR

typedef struct {
    ZaphodState     *super;

    QemuConsole     *screen_con;
    uint8_t         *rgb_bg, *rgb_fg;
    zaphod_screen_attr_t row_attr[MAX_TEXT_ROWS];
    uint8_t         char_grid[MAX_TEXT_ROWS][MAX_TEXT_COLS];
    int             dirty_minr, dirty_maxr;
    int             dirty_minc, dirty_maxc;
    int             curs_posr, curs_posc;
    bool            curs_visible, curs_dirty;
    int64_t         curs_blink_time;    /* millisec */
    qemu_irq        *rxint_irq;
#ifdef ZAPHOD_HAS_KEYBIO
    int             modifiers;
#endif
} ZaphodScreenState;
#endif

#ifdef ZAPHOD_HAS_MC6850
typedef struct {
    ZaphodState     *super;

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
void zaphod_screen_putchar(void *opaque, uint8_t ch);

ZaphodScreenState *zaphod_new_screen(ZaphodState *zs);
#endif

#ifdef ZAPHOD_HAS_MC6850
ZaphodMC6850State *zaphod_new_mc6850(ZaphodState *s);
#endif

void zaphod_set_inkey(void *opaque, uint8_t val, bool is_data);
uint8_t zaphod_get_inkey(void *opaque, bool read_and_clear);


/* zaphod.c */
void zaphod_interrupt(void *opaque, int source, int level);
int zaphod_has_feature(ZaphodState *zs, zaphod_feature_t n);
void zaphod_putchar(ZaphodState *zs, const unsigned char ch);

#endif	/*  _ZAPHOD_H_  */
