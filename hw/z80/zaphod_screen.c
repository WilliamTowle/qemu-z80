/*
 * QEmu Zaphod board - screen support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "qemu/osdep.h"
#include "zaphod.h"

#include "ui/console.h"
//#include "qemu-timer.h"

/* Implements a CGI-like display at 25x80 text resolution [ie. one
 * more line than QEmu's serial console]. Phil Brown calls his
 * Amiga emulator screen "medium-res two-colour 24x80" [Amiga NTSC
 * is usually 25x80 at 640x200 and PAL 32x80 at 640x256]. Grant
 * Searle's 2KiB RAM ATmega328 design is 25x80 with per-line
 * attributes for bold/size/graphics effects, where in graphics mode
 * each character cell maps to a 2x4 grid [for 160x100 pixels]
 */

#ifdef ZAPHOD_DEBUG
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif


//#include "vgafont.h"		/* vgafont16 - 16x8 */
#define FONT_HEIGHT	16
#define FONT_WIDTH	8
#define TEXT_ROWS	25
#define TEXT_COLS	80

#define ZAPHOD_TEXT_CURSOR_PERIOD_MS       (1000 * 2 * 16 / 60)


uint8_t zaphod_rgb_palette[][3]= {
	{ 0x00, 0x00, 0x00 },	/* background - black */
	{ 0x05, 0xf3, 0x05 },	/* foreground - green */
	{ 0xff, 0x91, 0x00 }	/* foreground - amber */
};


static void zaphod_screen_update_display(void *opaque)
{
    ZaphodScreenState *zss= (ZaphodScreenState *)opaque;

    /* TODO:
     *   Previously we considered the display surface dirty if flagged
     * as invalid [through the _invalidate_display() callback] or if
     * executed code wrote new text somewhere.
     *   Upstream code at repo.or.cz compares stored s->twidth and
     * s->theight values to latest ds_get_{width|height}() values
     * and forces qemu_console_resize() if different (prevents user
     * window resize? Later QEmu versions scale the window
     *   vga_update_display() does:
     *   - nothing if surface_bits_per_pixel(surface) == 0
     *   - sets 'full_update' if graphic mode changes (this resets blink timer)
     *   - passes 'full_update' to vga_draw_{text|graphic|blank}()
     */

#if 1
    {
        int64_t now= qemu_clock_get_ms(QEMU_CLOCK_REALTIME);

        if (now >= zss->curs_blink_time)
        {
            zss->curs_blink_time= now + ZAPHOD_TEXT_CURSOR_PERIOD_MS / 2;
            zss->curs_visible= !zss->curs_visible;
            DPRINTF("INFO: Cursor visible -> %s\n", zss->curs_visible?"ON":"OFF");

            /* since visibility changed - was zaphod_consolegui_blink_cursor() */
            {
                DisplaySurface *surface = qemu_console_surface(zss->screen_con);
                int       bypp= (surface_bits_per_pixel(surface) + 7) >> 3;
                uint8_t *dmem;
                int ix, iy;

                dmem= surface_data(surface);
                /* TODO: adjust dmem for cursor position */

                for (ix= 0; ix < FONT_HEIGHT; ix++)
                {
                    /* for bypp = 4 */
                    for (iy= 0; iy < FONT_WIDTH * bypp; iy+= bypp)
                    {
                        *(dmem + iy)^= zss->rgb_bg[2] ^ zss->rgb_fg[2];
                        *(dmem + iy+1)^= zss->rgb_bg[1] ^ zss->rgb_fg[1];
                        *(dmem + iy+2)^= zss->rgb_bg[0] ^ zss->rgb_fg[0];
                    }
                    dmem+= surface_stride(surface);
                }

                /* current update region is where the cursor is */
                dpy_gfx_update(zss->screen_con,
                        0, 0,                       /* ulx, uly */
                        FONT_WIDTH, FONT_HEIGHT);   /* xsz, ysz */
            }
        }
    }
#endif
}

static void zaphod_screen_invalidate_display(void *opaque)
{
;DPRINTF("[%s:%d] Reached UNIMPLEMENTED %s()\n", __FILE__, __LINE__, __func__);
    /* FIXME:
     * Calls here mean the window needs to be fully redrawn as
     * earlier content from our DisplayState is no longer visible
     * for some reason. This is particularly noticeable if QEmu
     * "suddenly" [? bug?] applies scaling to everything, at which
     * point we need to do work to account for graphical damage the
     * HMI or serial console causes.
     * Given that our dpy_update() for the cursor seems to behave,
     * the buffer in zss->ds is intact and we just need to put
     * content back on screen here.
     */

    /* TODO: VGACommonState tracks 'last_{width|height}' and
     * sets both to -1 here [...which forces a full update in
     * vga_update_text()]
     */
}

static const GraphicHwOps zaphod_screen_ops= {
    .invalidate     = zaphod_screen_invalidate_display,
    .gfx_update     = zaphod_screen_update_display,
};


DeviceState *zaphod_screen_new(ZaphodState *super)
{
    DeviceState         *dev= DEVICE(object_new(TYPE_ZAPHOD_SCREEN));
    ZaphodScreenState   *zss= ZAPHOD_SCREEN(dev);

    zss->super= super;

    //qdev_prop_set_chr(dev, "chardev", chr);

    qdev_init_nofail(dev);
    return dev;
}


static void zaphod_screen_realizefn(DeviceState *dev, Error **errp)
{
    ZaphodScreenState *zss= ZAPHOD_SCREEN(dev);

    /* Strictly speaking we are a CGA text display with custom
     * graphics features and 160x100 resolution, but might still
     *  be able to make use of vga_common_init()?
     */
	zss->screen_con= graphic_console_init(
		NULL,	/* no ISA bus bus to emulate */
		&zaphod_screen_ops,
		zss);

    /* TODO: calls to zaphod_consolegui_invalidate_display() (eg.
     * on switch to HMI and back) need to prompt a full redraw of
     * previous output
     */

    /* Distinguish machine type by text color */
    zss->rgb_bg= zaphod_rgb_palette[0];
    if (zaphod_has_feature(zss->super, ZAPHOD_SIMPLE_SCREEN))
        zss->rgb_fg= zaphod_rgb_palette[1];
    else
        zss->rgb_fg= zaphod_rgb_palette[2];
    zss->curs_visible= 0;
    zss->curs_blink_time= 0;

    qemu_console_resize(zss->screen_con,
        FONT_WIDTH * TEXT_COLS, FONT_HEIGHT * TEXT_ROWS);
}

#if 0  /* TODO: v1 init support */
//static Property zaphod_screen_properties[] = {
//    /* properties can be set with '-global zaphod-screen.VAR=VAL' */
//    DEFINE_PROP_CHR("chardev", ZaphodScreenState, chr),
//    DEFINE_PROP_END_OF_LIST(),
//};
#endif

static void zaphod_screen_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = zaphod_screen_realizefn;
#if 0  /* TODO: v1 init support */
    /* TODO: initialisation in dc->reset? */
    dc->props = zaphod_screen_properties;
#endif
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static void zaphod_screen_instance_init(Object *obj)
{
  //ZaphodScreenState *zms= ZAPHOD_SCREEN(obj);

    /* FIXME: reset behaviours belong in a function? */
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
