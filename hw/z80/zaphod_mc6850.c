/* "Zaphod" Z80 machine family for QEmu
 *
 * Motorola MC6850 serial device emulation
 *
 * Wm. Towle c. 2013-2021
 */

#include "zaphod.h"
#include "qemu/error-report.h"

//#include "qemu-char.h"


#ifdef ZAPHOD_DEBUG
#define DPRINTF(fmt, ...) \
    do { error_printf( "zaphod: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif


static uint32_t zaphod_mc6850_read(void *opaque, uint32_t addr)
{
#if 1
;DPRINTF("INCOMPLETE: Reached %s() OK\n", __func__);
;exit(1);
;return -1;
#else
    ZaphodIOPortState *zis= (ZaphodIOPortState *)opaque;
    //ZaphodMC6850State *zms= (ZaphodMC6850State *)opaque;
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
;DPRINTF("DEBUG: %s() read mc6850 PortStatus (port 0x%02x) -> status %02x\n", __func__, addr, value);
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
#endif
}

static void zaphod_mc6850_write(void *opaque, uint32_t addr, uint32_t value)
{
#if 1
;DPRINTF("INCOMPLETE: Reached %s() OK\n", __func__);
;exit(1);
#else
    ZaphodIOPortState *zis= (ZaphodIOPortState *)opaque;
    //ZaphodMC6850State *zms= (ZaphodMC6850State *)opaque;

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


ZaphodMC6850State *zaphod_new_mc6850(ZaphodState *super)
{
    ZaphodMC6850State *zms;

    if (!zaphod_has_feature(super, ZAPHOD_FEATURE_MC6850))
        return NULL;

    zms= g_new(ZaphodMC6850State, 1);
    zms->ports = g_new(PortioList, 1);

    portio_list_init(zms->ports, OBJECT(zms), zaphod_mc6850_portio, zms, "zaphod.mc6850");
    portio_list_add(zms->ports, get_system_io(), 0x00);

    return zms;
}
