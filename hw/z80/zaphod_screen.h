/*
 * QEmu Zaphod board - screen support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef HW_Z80_ZAPHOD_SCREEN_H
#define HW_Z80_ZAPHOD_SCREEN_H

#include "zaphod.h"

typedef struct ZaphodState ZaphodState;

typedef DeviceClass ZaphodScreenClass;

typedef struct {
    DeviceState     parent;

    ZaphodState     *super;
    QemuConsole     *screen_con;
    uint8_t         *rgb_bg, *rgb_fg;
    bool            curs_visible;
    int64_t         curs_blink_time;    /* millisec */
} ZaphodScreenState;

#define TYPE_ZAPHOD_SCREEN "zaphod-screen"

#define ZAPHOD_SCREEN_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodScreenClass, obj, TYPE_ZAPHOD_SCREEN)
#define ZAPHOD_SCREEN_CLASS(klass) \
    OBJECT_CLASS_CHECK(ZaphodScreenClass, klass, TYPE_ZAPHOD_SCREEN)
#define ZAPHOD_SCREEN(obj) \
    OBJECT_CHECK(ZaphodScreenState, obj, TYPE_ZAPHOD_SCREEN)


DeviceState *zaphod_screen_new(ZaphodState *zs);

#endif  /* HW_Z80_ZAPHOD_SCREEN_H */
