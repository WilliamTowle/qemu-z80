/*
 * QEmu Zaphod board - screen support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "zaphod.h"

#include "qemu/error-report.h"
#include "ui/console.h"


/* Implements a monochrome display at 25x80 text resolution (Phil
 * Brown calls his Amiga emulator screen "medium-res two-colour
 * 24x80" [Amiga is usually 25x80 at 640x200 for NTSC and 32x80 at
 * 640x256 for PAL]).
 * Skeleton support for Grant Searle's ATmega328 per-line attributes
 * (normally bold/size/graphics) is present. For graphics, each byte
 * of the line maps to a 2x4 pixel subgroup of all 160x100 pixels
 */

//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("zaphod_screen: " fmt , ## __VA_ARGS__); } while(0)


#if 0   /* FIXME: implement? */
#include "vgafont.h"           /* vgafont16 - 16x8 */
#endif
#define FONT_HEIGHT    16
#define FONT_WIDTH     8


/* TODO: palette with black and amber/green [foreground] options */


static void zaphod_screen_invalidate_display(void *opaque)
{
;DPRINTF("[%s:%d] Reached UNIMPLEMENTED %s()\n", __FILE__, __LINE__, __func__);
    /* TODO: trigger full redraw of the window by setting state
     * appropriately here - marking all cell locations as dirty
     */
}

static void zaphod_screen_update_display(void *opaque)
{
;DPRINTF("[%s:%d] Reached UNIMPLEMENTED %s()\n", __FILE__, __LINE__, __func__);
    /* align QEmu window content with the simulated display */
}

static const GraphicHwOps zaphod_screen_ops= {
    .invalidate     = zaphod_screen_invalidate_display,
    .gfx_update     = zaphod_screen_update_display,
};


DeviceState *zaphod_screen_new(void)
{
    DeviceState *dev= DEVICE(object_new(TYPE_ZAPHOD_SCREEN));

    qdev_init_nofail(dev);
    return dev;
}

static void zaphod_screen_realizefn(DeviceState *dev, Error **errp)
{
    ZaphodScreenState *zss= ZAPHOD_SCREEN(dev);

    /* NB. our text mode is essentially VGA-like; is QEmu's
     * common display code useful?
     */
    zss->display= graphic_console_init(
                        NULL,   /* no ISA bus to emulate */
                        0, &zaphod_screen_ops, zss);

    qemu_console_resize(zss->display,
                        FONT_WIDTH * ZAPHOD_TEXT_COLS,
                        FONT_HEIGHT * ZAPHOD_TEXT_ROWS);

    /* TODO: register reset function */
}


static void zaphod_screen_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc= DEVICE_CLASS(klass);

    dc->desc= "Zaphod screen device";
    dc->realize= zaphod_screen_realizefn;
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static void zaphod_screen_instance_init(Object *obj)
{
#if 0   /* TODO: implement init support */
    ZaphodScreenState *zms= ZAPHOD_SCREEN(obj);
    /* ... */
#endif
}


static const TypeInfo zaphod_screen_info= {
    .name= TYPE_ZAPHOD_SCREEN,
    .parent= TYPE_DEVICE,
    /* For ZaphodScreenClass with virtual functions:
     .class_size= sizeof(ZaphodScreenClass),
     */
    .class_init= zaphod_screen_class_init,
    .instance_size= sizeof(ZaphodScreenState),
    .instance_init= zaphod_screen_instance_init
};

static void zaphod_screen_register_types(void)
{
    type_register_static(&zaphod_screen_info);
}

type_init(zaphod_screen_register_types)
