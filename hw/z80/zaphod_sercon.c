/*
 * QEmu Zaphod board - sercon support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "qemu/osdep.h"
#include "zaphod.h"

#include "qemu/error-report.h"
#include "hw/qdev-properties.h"


#ifdef ZAPHOD_DEBUG
#define DPRINTF(fmt, ...) \
    do { error_printf( "zaphod: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif


/* TODO:
 * int zaphod_sercon_can_receive(void *opaque)
 * void zaphod_sercon_receive(void *opaque, const uint8_t *buf, int len)
 * void zaphod_sercon_putchar(ZaphodSerConState *zss, const unsigned char ch)
 * uint32_t zaphod_sercon_read(void *opaque, uint32_t addr)
 * void zaphod_sercon_write(void *opaque, uint32_t addr, uint32_t value)
 * const MemoryRegionPortio zaphod_sercon_portio[]
 */
DeviceState *zaphod_sercon_new(ZaphodState *super, CharDriverState *cds)
{
    DeviceState         *dev= DEVICE(object_new(TYPE_ZAPHOD_SERCON));
    ZaphodSerConState   *zis= ZAPHOD_SERCON(dev);

    zis->super= super;
#if 0	/* FIXME: pass and store CharDeviceState? */
    /* TODO: respect any preconfigured chardev, using
     * qdev_prop_set_chr() only to set a fallback
     */
    qdev_prop_set_chr(dev, "chardev", chr);
#endif

    qdev_init_nofail(dev);
    return dev;
}

static const MemoryRegionPortio zaphod_sercon_portio[] = {
    { 0x00, 1, 1, .read = zaphod_sercon_read },     /* stdin */
    { 0x01, 1, 1, .write = zaphod_sercon_write, },  /* stdout */
    PORTIO_END_OF_LIST(),
};

#if 0	/* TODO: v1 init support */
static void zaphod_sercon_realizefn(DeviceState *dev, Error **errp)
{
    /* TODO: ports/handlers enable here */
    PortioList *ports = g_new(PortioList, 1);

    portio_list_init(ports, OBJECT(zss), zaphod_sercon_portio, zss, "zaphod.sercon");
    portio_list_add(ports, get_system_io(), 0x00);

    qemu_chr_add_handlers(zss->sercon,
            zaphod_sercon_can_receive, zaphod_sercon_receive,
            NULL, zss);
}
#endif

static Property zaphod_sercon_properties[] = {
    /* properties can be set with '-global zaphod-sercon.VAR=VAL' */
    DEFINE_PROP_CHR("chardev",  ZaphodSerConState, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void zaphod_sercon_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

#if 0	/* TODO: v1 init support */
    dc->realize = zaphod_sercon_realizefn;
    /* TODO: initialisation in dc->reset? */
#endif
    dc->props = zaphod_sercon_properties;
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static void zaphod_sercon_instance_init(Object *obj)
{
#if 0	/* TODO: v1 init support */
    ZaphodSerConState *zms= ZAPHOD_SERCON(obj);

    /* FIXME: reset behaviours belong in a function? */
#endif
}


static const TypeInfo zaphod_sercon_info= {
	.name= TYPE_ZAPHOD_SERCON,
	.parent= TYPE_DEVICE,
	/* For ZaphodSerConClass with virtual functions:
	.class_size= sizeof(ZaphodSerConClass),
	 */
	.class_init= zaphod_sercon_class_init,
	.instance_size= sizeof(ZaphodSerConState),
	.instance_init= zaphod_sercon_instance_init
};

static void zaphod_sercon_register_types(void)
{
	type_register_static(&zaphod_sercon_info);
}

type_init(zaphod_sercon_register_types)
