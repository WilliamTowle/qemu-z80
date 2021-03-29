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


/* TODO: enum for screen attributes
 * Grant Searle's ATmega328 supports:
 * - 0x01: use 80 chars (40 otherwise)
 * - 0x02: bold
 * - 0x04: double height (internally, top half/bottom half)
 * - 0x80: 2x4 graphics block (bit 0 top left, bit 7 bottom right)
 */

typedef struct {
    DeviceState     parent;

    bool            simple_escape_codes;

    QemuConsole     *display;
    bool            cursor_visible;
    int64_t         cursor_blink_time;    /* millisec */
    uint8_t         *rgb_bg, *rgb_fg;
} ZaphodScreenState;

#define TYPE_ZAPHOD_SCREEN "zaphod-screen"

#define ZAPHOD_SCREEN_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodScreenClass, obj, TYPE_ZAPHOD_SCREEN)
#define ZAPHOD_SCREEN_CLASS(klass) \
    OBJECT_CLASS_CHECK(ZaphodScreenClass, klass, TYPE_ZAPHOD_SCREEN)
#define ZAPHOD_SCREEN(obj) \
    OBJECT_CHECK(ZaphodScreenState, obj, TYPE_ZAPHOD_SCREEN)


void zaphod_screen_putchar(ZaphodScreenState *zss, uint8_t ch);


#endif  /* HW_Z80_ZAPHOD_SCREEN_H */
