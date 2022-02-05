/*
 * QEmu Zaphod board - screen support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef HW_Z80_ZAPHOD_SCREEN_H
#define HW_Z80_ZAPHOD_SCREEN_H

#include "zaphod.h"

#define ZAPHOD_TEXT_ROWS	25
#define ZAPHOD_TEXT_COLS	80

typedef DeviceClass ZaphodScreenClass;


typedef enum {
    ZAPHOD_SCREEN_ATTR_80COL = 0x01,
    /* TODO:
     * - 0x02: bold
     * - 0x04: double height (internally, top half/bottom half)
     * - 0x80: 2x4 graphics block (bit 0 top left, bit 7 bottom right)
     */
    ZAPHOD_SCREEN_ATTR_GRAPH = 0x80
} zaphod_screen_attr_t;

typedef struct {
    DeviceState     parent;

    QemuConsole     *display;
    zaphod_screen_attr_t row_attr[ZAPHOD_TEXT_ROWS];
    uint8_t         *rgb_bg, *rgb_fg;
    int             dirty_minr, dirty_maxr;
    int             dirty_minc, dirty_maxc;
    int             curs_posr, curs_posc;
    bool            cursor_visible, cursor_dirty;
    int64_t         cursor_blink_time;    /* millisec */
    uint8_t         char_grid[ZAPHOD_TEXT_ROWS][ZAPHOD_TEXT_COLS];
} ZaphodScreenState;

#define TYPE_ZAPHOD_SCREEN "zaphod-screen"

#define ZAPHOD_SCREEN_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodScreenClass, obj, TYPE_ZAPHOD_SCREEN)
#define ZAPHOD_SCREEN_CLASS(klass) \
    OBJECT_CLASS_CHECK(ZaphodScreenClass, klass, TYPE_ZAPHOD_SCREEN)
#define ZAPHOD_SCREEN(obj) \
    OBJECT_CHECK(ZaphodScreenState, obj, TYPE_ZAPHOD_SCREEN)


void zaphod_screen_putchar(void *opaque, uint8_t ch);


DeviceState *zaphod_screen_new(void);

#endif  /* HW_Z80_ZAPHOD_SCREEN_H */
