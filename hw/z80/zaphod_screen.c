/*
 * QEmu Zaphod board - screen support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "zaphod.h"


//#define EMIT_DEBUG ZAPHOD_DEBUG
//#define EMIT_DEBUG 0
//#define DPRINTF(fmt, ...) \
//    do { if (EMIT_DEBUG) error_printf("zaphod_screen: " fmt , ## __VA_ARGS__); } while(0)


#if 0   /* TODO: implement */
static void zaphod_screen_realizefn(DeviceState *dev, Error **errp)
{
    /* ... */
}
#endif

/* TODO: zaphod_screen_properties[] */

static void zaphod_screen_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc= DEVICE_CLASS(oc);

    dc->desc= "Zaphod screen device";
#if 0   /* TODO: implement init support */
    dc->realize= zaphod_screen_realizefn;
    /* TODO: set properties */
#endif
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
