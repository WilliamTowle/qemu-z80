/*
 * QEmu Zaphod board - sercon support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "zaphod.h"         /* for config */

#include "qemu/error-report.h"
#include "hw/qdev-properties.h"
#include "sysemu/char.h"


#ifdef ZAPHOD_DEBUG
#define DPRINTF(fmt, ...) \
    do { error_printf( "zaphod: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif


static
int zaphod_sercon_can_receive(void *opaque)
{
    /* Maybe implement a FIFO queue? */
    return 1;
}

static
void zaphod_sercon_receive(void *opaque, const uint8_t *buf, int len)
{
    ZaphodSerConState *zss= ZAPHOD_SERCON(opaque);

    ((ZaphodState *)zss->super)->inkey= buf[0];
}

void zaphod_sercon_putchar(ZaphodSerConState *zss, const unsigned char ch)
{
    if (unlikely (!zss->chr))
    {
        return;
    }

    if (isprint(ch))
    {
        zss->chr->chr_write(zss->chr, &ch, 1);
    }
    else
    {   /* Render non-printable characters as corresponding hex.
         * TODO: pass all characters to device-specific handler to
         * interpret appropriately
         */
        uint8_t nyb_hi, nyb_lo;

        nyb_hi= (ch & 0xf0) >> 4;
        nyb_lo= ch & 0x0f;

        zaphod_sercon_putchar(zss, '[');
        nyb_hi+= (nyb_hi > 9)? 'A' - 10 : '0';
        //zss->sercon->chr_write(zss->sercon, &nyb_hi, 1);
        zaphod_sercon_putchar(zss, nyb_hi);
        nyb_lo+= (nyb_lo > 9)? 'A' - 10 : '0';
        //zss->sercon->chr_write(zss->sercon, &nyb_lo, 1);
        zaphod_sercon_putchar(zss, nyb_lo);
        zaphod_sercon_putchar(zss, ']');
        zaphod_sercon_putchar(zss, '*');
    }
}

static uint32_t zaphod_sercon_read(void *opaque, uint32_t addr)
#if 1
{
    //ZaphodSerConState *zss= (ZaphodSerConState *)opaque;
    ZaphodSerConState *zss= ZAPHOD_SERCON(opaque);
    int		value;

	switch (addr)
	{
	case 0x00:		/* stdin */
		value= ((ZaphodState *)zss->super)->inkey;
		((ZaphodState *)zss->super)->inkey= 0;
		return value;
	default:
;DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
		return 0x00;
	}
}
#else
{
    ZaphodSerConState *zis= (ZaphodSerConState *)opaque;
    int		value;
;DPRINTF("DEBUG: Enter %s()\n", __func__);

	switch (addr)
	{
	case 0x00:		/* stdin */
		value= zis->inkey;
		zis->inkey= 0;
#ifdef ZAPHOD_HAS_RXINT_IRQ
		if (zis->rxint_irq)
		    qemu_irq_lower(*zis->rxint_irq);
#endif	/* ZAPHOD_HAS_RXINT_IRQ */
		return value;
#ifdef ZAPHOD_HAS_SERIALIO
	case 0x80:		/* read mc6850 PortStatus */
		value= (zis->inkey)? 0x01 : 0; /* RxDataReady */
		value|= 0x02;		/* TxDataEmpty (always) */
		value|= 0x04;		/* DTD [Data Carrier Detect] */
		value|= 0x08;		/* CTS [Clear to Send] */
			/* FrameErr|Overrun|ParityErr|IrqReq not emulated */
#ifdef ZAPHOD_DEBUG
;DPRINTF("DEBUG: %s() read mc6850 PortStatus (port 0x%02x) -> status %02x\n", __func__, addr, value);
#endif
		return value;
	case 0x81:		/* read mc6850 RxData */
		value= zis->inkey;
		zis->inkey= 0;
#ifdef ZAPHOD_HAS_RXINT_IRQ
		if (zis->rxint_irq)
			qemu_irq_lower(*zis->console->rxint_irq);
#endif	/* ZAPHOD_HAS_RXINT_IRQ */
DPRINTF("DEBUG: %s() read mc6850 RXData (port 0x%02x) -> ch-value %d\n", __func__, addr, value);
		return value? value : 0xff;
#endif	/* ZAPHOD_HAS_SERIALIO */
	default:
DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
		return 0x00;
	}

;DPRINTF("DEBUG: Exit %s()\n", __func__);
}
#endif

static void zaphod_sercon_write(void *opaque, uint32_t addr, uint32_t value)
#if 1
{
    ZaphodSerConState *zss= (ZaphodSerConState *)opaque;

    switch (addr)
    {
    case 0x01:        /* stdout */
        zaphod_sercon_putchar(zss, value);
        break;
    default:
;DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
        break;
    }
}
#else
{
    ZaphodSerConState *zis= (ZaphodSerConState *)opaque;

	switch (addr)
	{
	case 0x01:		/* stdout */
;DPRINTF("%s: character value: %d\n", __func__, value);
	    zaphod_consolegui_putchar(zis->console, value & 0xff);
#ifdef ZAPHOD_HAS_SERIALIO
	    zaphod_serio_putchar(/*zis->console, */value & 0xff);
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
		zaphod_consolegui_putchar(zis->console, toupper(value & 0xff));
#endif
		zaphod_serio_putchar(/*zis->console, */value & 0xff);
		break;
#endif	/* ZAPHOD_HAS_SERIALIO */
	default:
DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
	    break;
	}
}
#endif


DeviceState *zaphod_sercon_new(ZaphodState *super, CharDriverState *cds)
{
    DeviceState         *dev= DEVICE(object_new(TYPE_ZAPHOD_SERCON));
    ZaphodSerConState   *zis= ZAPHOD_SERCON(dev);

    zis->super= super;
    /* TODO: respect any preconfigured chardev, using
     * qdev_prop_set_chr() only to set a fallback
     */
    qdev_prop_set_chr(dev, "chardev", cds);

    qdev_init_nofail(dev);
    return dev;
}

static const MemoryRegionPortio zaphod_sercon_portio[] = {
    { 0x00, 1, 1, .read = zaphod_sercon_read },     /* stdin */
    { 0x01, 1, 1, .write = zaphod_sercon_write, },  /* stdout */
    PORTIO_END_OF_LIST(),
};

static void zaphod_sercon_realizefn(DeviceState *dev, Error **errp)
{
    ZaphodSerConState *zss= ZAPHOD_SERCON(dev);

    zss->ports = g_new(PortioList, 1);

    portio_list_init(zss->ports, OBJECT(zss),
                    zaphod_sercon_portio, zss, "zaphod.sercon");
    portio_list_add(zss->ports, get_system_io(), 0x00);

    qemu_chr_add_handlers(zss->chr,
            zaphod_sercon_can_receive, zaphod_sercon_receive,
            NULL, zss);
}

static Property zaphod_sercon_properties[] = {
    /* properties can be set with '-global zaphod-sercon.VAR=VAL' */
    DEFINE_PROP_CHR("chardev",  ZaphodSerConState, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void zaphod_sercon_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = zaphod_sercon_realizefn;
    /* TODO: initialisation in dc->reset? */
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
