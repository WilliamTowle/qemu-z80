/*
 * QEmu Zaphod board - Motorola MC6850 ACIA support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "zaphod.h"

#include "qemu/error-report.h"
//#include "qemu-char.h"
#include "hw/qdev-properties.h"


#ifdef ZAPHOD_DEBUG
#define DPRINTF(fmt, ...) \
    do { error_printf( "zaphod: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif


//static int zaphod_mc6850_can_receive(void *opaque)
//static void zaphod_mc6850_receive(void *opaque, const uint8_t *buf, int len)
//void zaphod_mc6850putchar(ZaphodMC6850tate *zss, const unsigned char ch)



static uint32_t zaphod_mc6850_read(void *opaque, uint32_t addr)
{
#if 1
;DPRINTF("INCOMPLETE: Reached %s() OK\n", __func__);
;exit(1);
;return -1;
#else
    //ZaphodMC6850State *zms= (ZaphodMC6850State *)opaque;
    ZaphodMC6850State *zms= ZAPHOD_MC6850(opaque);
    int		value;
;DPRINTF("DEBUG: Enter %s()\n", __func__);

	switch (addr)
	{
	case 0x00:		/* stdin */
        /* FIXME: inkey via zms->super? */
		value= zms->inkey;
		zms->inkey= 0;
#ifdef ZAPHOD_HAS_RXINT_IRQ
		if (zms->rxint_irq)
		    qemu_irq_lower(*zms->rxint_irq);
#endif	/* ZAPHOD_HAS_RXINT_IRQ */
		return value;
#ifdef ZAPHOD_HAS_SERIALIO
	case 0x80:		/* read mc6850 PortStatus */
        /* FIXME: inkey via zms->super? */
		value= (zms->inkey)? 0x01 : 0; /* RxDataReady */
		value|= 0x02;		/* TxDataEmpty (always) */
		value|= 0x04;		/* DTD [Data Carrier Detect] */
		value|= 0x08;		/* CTS [Clear to Send] */
			/* FrameErr|Overrun|ParityErr|IrqReq not emulated */
;DPRINTF("DEBUG: %s() read mc6850 PortStatus (port 0x%02x) -> status %02x\n", __func__, addr, value);
		return value;
	case 0x81:		/* read mc6850 RxData */
        /* FIXME: inkey via zms->super? */
		value= zms->inkey;
		zms->inkey= 0;
#ifdef ZAPHOD_HAS_RXINT_IRQ
		if (zms->rxint_irq)
			qemu_irq_lower(*zms->console->rxint_irq);
#endif	/* ZAPHOD_HAS_RXINT_IRQ */
DPRINTF("DEBUG: %s() read mc6850 RXData (port 0x%02x) -> ch-value %d\n", __func__, addr, value);
		return value? value : 0xff;
#endif	/* ZAPHOD_HAS_SERIALIO */
	default:
DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
		return 0x00;
	}

;DPRINTF("DEBUG: Exit %s()\n", __func__);
#endif
}

static void zaphod_mc6850_write(void *opaque, uint32_t addr, uint32_t value)
{
#if 1
;DPRINTF("INCOMPLETE: Reached %s() OK\n", __func__);
;exit(1);
#else
    //ZaphodMC6850State *zms= (ZaphodMC6850State *)opaque;
    ZaphodState *zs= ZAPHOD_MACHINE(qdev_get_machine());
    ZaphodMC6850State *zms= ZAPHOD_MC6850(opaque);

	switch (addr)
	{
	case 0x01:		/* stdout */
;DPRINTF("%s: character value: %d\n", __func__, value);
	    zaphod_consolegui_putchar(zs->console, value & 0xff);
#ifdef ZAPHOD_HAS_SERIALIO
	    zaphod_serio_putchar(/*zms->console, */value & 0xff);
#endif
	    break;
#ifdef ZAPHOD_HAS_SERIALIO
	case 0x80:		/* write -> mc6850 PortControl */
		/* ignore since baud rate change etc. not emulated? */
DPRINTF("DEBUG: %s() write m6850 PortControl (port 0x%02x) <- value %d\n", __func__, addr, value);
		break;
	case 0x81:		/* write -> mc6850 TxData */
DPRINTF("DEBUG: %s() write mc6850 TxData (port 0x%02x) -> ch-value=%d\n", __func__, addr, value);
#ifdef ZAPHOD_HAS_CONSOLEGUI
		zaphod_consolegui_putchar(zms->console, toupper(value & 0xff));
#endif
		zaphod_serio_putchar(/*zms->console, */value & 0xff);
		break;
#endif	/* ZAPHOD_HAS_SERIALIO */
	default:
DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
	    break;
	}
#endif
}

/* zaphod_mc6850_portio
 * TODO: adjust? Grant Searle's documentation talks about 0x80-0xbf
 * being reserved but decoding makes all even/odd ports equivalent
 */
static const MemoryRegionPortio zaphod_mc6850_portio[] = {
    { 0x80, 2, 1,
                .read = zaphod_mc6850_read,
                .write = zaphod_mc6850_write
                },
    PORTIO_END_OF_LIST(),
};


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

/* zaphod_mc6850_portio
 * TODO: adjust? Grant Searle's documentation talks about 0x80-0xbf
 * being reserved but decoding makes all even/odd ports equivalent
 */
static const MemoryRegionPortio zaphod_mc6850_portio[] = {
    { 0x80, 2, 1,
                .read = zaphod_mc6850_read,
                .write = zaphod_mc6850_write
                },
    PORTIO_END_OF_LIST(),
};

static void zaphod_mc6850_realizefn(DeviceState *dev, Error **errp)
{
    ZaphodMC6850State *zms= ZAPHOD_MC6850(dev);

    zms->ports = g_new(PortioList, 1);
    portio_list_init(zms->ports, OBJECT(zms),
                    zaphod_mc6850_portio, zms, "zaphod.mc6850");
    portio_list_add(zms->ports, get_system_io(), 0x00);

#if 0
    qemu_chr_add_handlers(zms->chr,
            zaphod_mc6850_can_receive, zaphod_mc6850_receive,
            NULL, zss);
#endif
}

static Property zaphod_mc6850_properties[] = {
    /* properties can be set with '-global zaphod-mc6850.VAR=VAL' */
    DEFINE_PROP_CHR("chardev",  ZaphodMC6850State, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void zaphod_mc6850_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = zaphod_mc6850_realizefn;
    /* TODO: initialisation in dc->reset? */
    dc->props = zaphod_mc6850_properties;
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static void zaphod_mc6850_instance_init(Object *obj)
{
#if 0	/* TODO: v1 init support */
    ZaphodSerConState *zms= ZAPHOD_MC6850(obj);

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
