/*
 * QEmu Zaphod board - screen support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "qemu/osdep.h"
#include "zaphod.h"

//#include "qemu/error-report.h"


//#ifdef ZAPHOD_DEBUG
//	/* TODO: version with error_printf() needs CPU headers */
//#define DPRINTF(fmt, ...) \
//	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
//#else
//#define DPRINTF(fmt, ...) \
//	do { } while (0)
//#endif


DeviceState *zaphod_screen_new(void)
{
    DeviceState *dev;

    dev = DEVICE(object_new(TYPE_ZAPHOD_SCREEN));

    //qdev_prop_set_chr(dev, "chardev", chr);

    qdev_init_nofail(dev);
    return dev;
}

static void zaphod_screen_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

#if 0  /* TODO: v1 init support */
    dc->realize = zaphod_screen_realizefn;
    /* TODO: initialisation in dc->reset? */
    dc->props = zaphod_screen_properties;
#endif
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static void zaphod_screen_instance_init(Object *obj)
{
    //ZaphodScreenState *zms= ZAPHOD_SCREEN(obj);

    /* TODO: reset behaviours belong in a function? */
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
