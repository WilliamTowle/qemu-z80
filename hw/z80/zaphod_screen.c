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
 * 640x256 for PAL].
 * Skeleton support for Grant Searle's ATmega328 per-line attributes
 * (normally bold/size/graphics) is present. For graphics, each byte
 * of the line maps to a 2x4 pixel subgroup of all 160x100 pixels
 */

#define DPRINTF(fmt, ...) \
    do { if (ZAPHOD_DEBUG) error_printf("zaphod_screen: " fmt , ## __VA_ARGS__); } while(0)

#define ZAPHOD_TEXT_ROWS 25
#define ZAPHOD_TEXT_COLS 80

#define ZAPHOD_TEXT_CURSOR_PERIOD_MS       (1000 * 2 * 16 / 60)

#if 0   /* FIXME: implement? */
#include "vgafont.h"           /* vgafont16 - 16x8 */
#endif
#define FONT_HEIGHT    16
#define FONT_WIDTH     8

uint8_t zaphod_rgb_palette[][3]= {
    { 0x00, 0x00, 0x00 },   /* background - black */
#if 1   /* select green or amber screen (TODO: make machine-specific) */
    { 0x05, 0xf3, 0x05 }    /* foreground - green */
#else
    { 0xff, 0x91, 0x00 }    /* foreground - amber */
#endif
};


static void zaphod_screen_invalidate_display(void *opaque)
{
;DPRINTF("[%s:%d] Reached UNIMPLEMENTED %s()\n", __FILE__, __LINE__, __func__);
    /* TODO: implement full redraw of the window.
     * QEmu's VGACommonState tracks 'last_{width|height}' and sets
     * both to -1 here [...which leads to a full update in
     * vga_update_text()]
     */
}

static void zaphod_screen_update_display(void *opaque)
{
    ZaphodScreenState *zss= ZAPHOD_SCREEN(opaque);

    /* TODO:
     *   Previously we considered the display surface dirty if flagged
     * as invalid [through the _invalidate_display() callback] or if
     * executed code wrote new text somewhere.
     *   Upstream code at repo.or.cz compares stored s->twidth and
     * s->theight values to latest ds_get_{width|height}() values
     * and forces qemu_console_resize() if different (prevents user
     * window resize? Later QEmu versions scale the window)
     *   vga_update_display() does:
     *   - nothing if surface_bits_per_pixel(surface) == 0
     *   - sets 'full_update' if graphic mode changes (this resets blink timer)
     *   - passes 'full_update' to vga_draw_{text|graphic|blank}()
     */

#if 1
    {   /* TODO: vga.c has:
         * - vga_draw_text() cares if text resolution changed;
         *   ...this triggers qemu_console_resize()
         *   ...and full_update gets set (sets s->full_update_text)
         *   ...cursor update happens here
         * - vga_draw_graphic() is passed 'full_update', and:
         *   ...forces it true if recorded width/height differ (mode change?)
         *   ...triggers qemu_console_resize() [or equivalent] if true
         */
        //int64_t now = qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL);
        int64_t now = qemu_clock_get_ms(QEMU_CLOCK_REALTIME);

        if (now >= zss->cursor_blink_time)
        {
            zss->cursor_blink_time= now + ZAPHOD_TEXT_CURSOR_PERIOD_MS / 2;
            zss->cursor_visible= !zss->cursor_visible;
            DPRINTF("INFO: Cursor visible -> %s\n", zss->cursor_visible?"ON":"OFF");

            /* since visibility changed - was zaphod_consolegui_blink_cursor() */
            {
                DisplaySurface *ds = qemu_console_surface(zss->display);
                int       bypp= (surface_bits_per_pixel(ds) + 7) >> 3;
                uint8_t *dmem;
                int ix, iy;

                dmem= surface_data(ds);
                /* TODO: adjust dmem for cursor position */

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

                /* redraw cursor in its present location */
                dpy_gfx_update(zss->display,
                        0, 0,                       /* ulx, uly */
                        FONT_WIDTH, FONT_HEIGHT);   /* xsz, ysz */
            }
        }
    }
#endif
}

static const GraphicHwOps zaphod_screen_ops= {
    .invalidate     = zaphod_screen_invalidate_display,
    .gfx_update     = zaphod_screen_update_display,
};


DeviceState *zaphod_screen_new(void)
{
    DeviceState *dev;

    dev = DEVICE(object_new(TYPE_ZAPHOD_SCREEN));

    //qdev_prop_set_chr(dev, "chardev", chr);

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

    zss->cursor_visible= 0;
    zss->cursor_blink_time= 0;

    qemu_console_resize(zss->display,
                        FONT_WIDTH * ZAPHOD_TEXT_COLS,
                        FONT_HEIGHT * ZAPHOD_TEXT_ROWS);
}


//static Property zaphod_screen_properties[] = {
//    /* properties can be set with '-global zaphod-screen.VAR=VAL' */
//    DEFINE_PROP_CHR("chardev", ZaphodScreenState, chr),
//    DEFINE_PROP_END_OF_LIST(),
//};

static void zaphod_screen_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = zaphod_screen_realizefn;
#if 0   /* TODO: v1 init support */
    /* TODO: initialisation in dc->reset? */
    dc->props = zaphod_screen_properties;
#endif
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static void zaphod_screen_instance_init(Object *obj)
{
#if 0   /* TODO: v1 init support */
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
