/*
 * QEmu Zaphod board - screen support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "zaphod.h"

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


#define ZAPHOD_TEXT_CURSOR_PERIOD_MS       (1000 * 2 * 16 / 60)

#if 0   /* FIXME: implement? */
#include "vgafont.h"           /* vgafont16 - 16x8 */
#endif
#define FONT_HEIGHT    16
#define FONT_WIDTH     8


uint8_t zaphod_rgb_palette[][3]= {
    { 0x00, 0x00, 0x00 },   /* background - black */
    /* TODO: configuration-specific green or amber screen */
    { 0x05, 0xf3, 0x05 }    /* foreground 1 - green */
    //{ 0xff, 0x91, 0x00 }    /* foreground 2 - amber */
};


static
void zaphod_screen_toggle_cursor(void *opaque, int row, int col)
{
    ZaphodScreenState   *zss= ZAPHOD_SCREEN(opaque);
    DisplaySurface      *ds = qemu_console_surface(zss->display);
    int                 bypp= (surface_bits_per_pixel(ds) + 7) >> 3;
    uint8_t             *dmem;
    int                 ix, iy;

    dmem= surface_data(ds);
    dmem+= col * FONT_WIDTH * bypp;
    dmem+= row * FONT_HEIGHT * surface_stride(ds);

    for (ix= 0; ix < FONT_HEIGHT; ix++)
    {
        /* for bypp = 4 */
        for (iy= 0; iy < FONT_WIDTH * bypp; iy+= bypp)
        {
            *(dmem + iy)^= zaphod_rgb_palette[0][2] ^ zaphod_rgb_palette[1][2];
            *(dmem + iy+1)^= zaphod_rgb_palette[0][1] ^ zaphod_rgb_palette[1][1];
            *(dmem + iy+2)^= zaphod_rgb_palette[0][0] ^ zaphod_rgb_palette[1][0];
        }
        dmem+= surface_stride(ds);
    }

    /* update display to redraw cursor in given location */
    dpy_gfx_update(zss->display,
            col * FONT_WIDTH,
            row * FONT_HEIGHT,
            FONT_WIDTH, FONT_HEIGHT);
}

static void zaphod_screen_invalidate_display(void *opaque)
{
#if 1   /* WmT - TRACE */
;DPRINTF("[%s:%d] Reached UNIMPLEMENTED %s()\n", __FILE__, __LINE__, __func__);
#endif
    /* TODO: trigger full redraw of the window by setting state
     * appropriately here - marking all cell locations as dirty
     */
}

static void zaphod_screen_update_display(void *opaque)
{
    ZaphodScreenState *zss= ZAPHOD_SCREEN(opaque);
    int64_t now;

    /* Handle cursor blink if its timer expired */

    //now = qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL);
    now = qemu_clock_get_ms(QEMU_CLOCK_REALTIME);

    if (now >= zss->cursor_blink_time)
    {
        zss->cursor_blink_time= now + ZAPHOD_TEXT_CURSOR_PERIOD_MS / 2;
        zss->cursor_visible= !zss->cursor_visible;

        /* effect "blink" step (toggle cursor visibility) */
        zaphod_screen_toggle_cursor(opaque, 0, 0);
    }
}

static const GraphicHwOps zaphod_screen_ops= {
    .invalidate     = zaphod_screen_invalidate_display,
    .gfx_update     = zaphod_screen_update_display,
};


/* character processing */

void zaphod_screen_putchar(void *opaque, uint8_t ch)
{
#if 1   /* WmT - TRACE */
;DPRINTF("*** INFO: zaphod_screen_putchar() - ch 0x%02x ***\n", ch);
#endif
    /* TODO: implement */
}


static void zaphod_screen_reset(void *opaque)
{
    ZaphodScreenState *zss= ZAPHOD_SCREEN(opaque);

    zss->cursor_visible= false;
    zss->cursor_blink_time= 0;
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

    qemu_register_reset(zaphod_screen_reset, zss);
}


/* TODO: zaphod_screen_properties[] */

static void zaphod_screen_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc= DEVICE_CLASS(oc);

    dc->desc= "Zaphod screen device";
    dc->realize= zaphod_screen_realizefn;
    /* TODO: set properties */
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
