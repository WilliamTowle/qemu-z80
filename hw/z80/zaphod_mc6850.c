/*
 * QEmu Zaphod board - Motorola MC6850 ACIA support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "zaphod.h"


//#include "qemu/error-report.h"
//
//#ifdef ZAPHOD_DEBUG
//#define DPRINTF(fmt, ...) \
//    do { error_printf( "zaphod: " fmt , ## __VA_ARGS__); } while (0)
//#else
//#define DPRINTF(fmt, ...) \
//    do { } while (0)
//#endif


//DeviceState *zaphod_mc6850_new(Chardev *chr)
DeviceState *zaphod_mc6850_new(ZaphodState *super)
{
    DeviceState         *dev= DEVICE(object_new(TYPE_ZAPHOD_MC6850));
    ZaphodMC6850State   *zms= ZAPHOD_MC6850(dev);

    zms->super= super;
#if 0	/* FIXME: need a separate console and 'CharDriverState' param? */
    /* TODO: respect any preconfigured chardev, using
     * qdev_prop_set_chr() only to set a fallback
     */
    qdev_prop_set_chr(dev, "chardev", chr);
#endif

    qdev_init_nofail(dev);
    return dev;
}

#if 0	/* TODO: v1 init support */
static void zaphod_mc6850_realizefn(DeviceState *dev, Error **errp)
{
    /* TODO: ports/handlers enable here */
    PortioList *ports = g_new(PortioList, 1);

    portio_list_init(ports, OBJECT(zss), zaphod_sercon_portio, zss, "zaphod.sercon");
    portio_list_add(ports, get_system_io(), 0x00);

    qemu_chr_add_handlers(zss->sercon,
            zaphod_sercon_can_receive, zaphod_sercon_receive,
            NULL, zss);
}

static Property zaphod_mc6850_properties[] = {
    /* properties can be set with '-global zaphod-mc6850.VAR=VAL' */
    DEFINE_PROP_CHR("chardev",  ZaphodMC6850State, chr),
    DEFINE_PROP_END_OF_LIST(),
};
#endif

static void zaphod_mc6850_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

#if 0	/* TODO: v1 init support */
    dc->realize = zaphod_mc6850_realizefn;
    /* TODO: initialisation in dc->reset? */
    dc->props = zaphod_mc6850_properties;
#endif
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static void zaphod_mc6850_instance_init(Object *obj)
{
#if 0	/* TODO: v1 init support */
    ZaphodSerConState *zms= ZAPHOD_SERCON(obj);

    /* FIXME: reset behaviours belong in a function? */
#endif
}


static const TypeInfo zaphod_mc6850_info= {
	.name= TYPE_ZAPHOD_MC6850,
	.parent= TYPE_DEVICE,
	/* For ZaphodMC6850Class with virtual functions:
	.class_size= sizeof(ZaphodMC6850Class),
	 */
	.class_init= zaphod_mc6850_class_init,
	.instance_size= sizeof(ZaphodMC6850State),
	.instance_init= zaphod_mc6850_instance_init
};

static void zaphod_mc6850_register_types(void)
{
	type_register_static(&zaphod_mc6850_info);
}

type_init(zaphod_mc6850_register_types)
