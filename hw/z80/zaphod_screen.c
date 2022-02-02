/*
 * QEmu Zaphod board - screen support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "qemu/osdep.h"
#include "zaphod.h"

#include "ui/console.h"

/* Implements a CGI-like display at 25x80 text resolution [ie. one
 * more line than QEmu's serial console]. Phil Brown calls his
 * Amiga emulator screen "medium-res two-colour 24x80" [Amiga NTSC
 * is usually 25x80 (at 640x200) and PAL 32x80 (at 640x256)]. Grant
 * Searle's ATmega328 design is 25x80 with per-line attributes for
 * bold/size/graphics, where each byte in the character map is a
 * 2x4 pixel group [for 160x100 pixels]
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
#define ZAPHOD_TEXT_ROWS	25
#define ZAPHOD_TEXT_COLS	80


static void zaphod_screen_invalidate_display(void *opaque)
{
;DPRINTF("[%s:%d] Reached UNIMPLEMENTED %s()\n", __FILE__, __LINE__, __func__);
}

static void zaphod_screen_update_display(void *opaque)
{
;DPRINTF("[%s:%d] Reached UNIMPLEMENTED %s()\n", __FILE__, __LINE__, __func__);
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

    /* Strictly speaking we are a CGA text display with custom
     * graphics features and 160x100 resolution, but might still
     *  be able to make use of vga_common_init()?
     */
    zss->display= graphic_console_init(
                NULL,	/* no ISA bus bus to emulate */
                &zaphod_screen_ops,
                zss);
	/* TODO: calls to zaphod_consolegui_invalidate_display() (eg. when
	 * user switches to HMI and back) need to prompt a full redraw of
	 * previous output
	 */
    qemu_console_resize(zss->display,
        FONT_WIDTH * ZAPHOD_TEXT_COLS, FONT_HEIGHT * ZAPHOD_TEXT_ROWS);
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
