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
    ZaphodMC6850State *zms= (ZaphodMC6850State *)opaque;
    int		value;

	switch (addr)
	{
	case 0x80:		/* read mc6850 PortStatus */
		value= zaphod_get_inkey(zms->super, false)? 0x01 : 0; /* RxDataReady */
		value|= 0x02;		/* TxDataEmpty (always) */
		value|= 0x04;		/* DTD [Data Carrier Detect] */
		value|= 0x08;		/* CTS [Clear to Send] */
			/* FrameErr|Overrun|ParityErr|IrqReq not emulated */
;DPRINTF("DEBUG: %s() read mc6850 PortStatus (port 0x%02x) -> status %02x\n", __func__, addr, value);
		return value;
	case 0x81:		/* read mc6850 RxData */
		value= zaphod_get_inkey(zms->super, true);
#ifdef ZAPHOD_HAS_RXINT_IRQ
		if (zms->rxint_irq)
			qemu_irq_lower(*zms->rxint_irq);
#endif	/* ZAPHOD_HAS_RXINT_IRQ */
DPRINTF("DEBUG: %s() read mc6850 RXData (port 0x%02x) -> ch-value %d\n", __func__, addr, value);
		return value? value : 0xff;
	default:
DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
		return 0x00;
	}

}

static void zaphod_mc6850_write(void *opaque, uint32_t addr, uint32_t value)
{
    ZaphodMC6850State *zms= (ZaphodMC6850State *)opaque;

	switch (addr)
	{
	case 0x80:		/* write -> mc6850 PortControl */
		/* ignore since baud rate change etc. not emulated? */
DPRINTF("DEBUG: %s() write m6850 PortControl (port 0x%02x) <- value %d\n", __func__, addr, value);
		break;
	case 0x81:		/* write -> mc6850 TxData */
DPRINTF("DEBUG: %s() write mc6850 TxData (port 0x%02x) -> ch-value=%d\n", __func__, addr, value);
		//zaphod_consolegui_putchar(zms->console, toupper(value & 0xff));
		//zaphod_serio_putchar(/*zms->console, */value & 0xff);
		zaphod_putchar((ZaphodState *)zms->super, value & 0xff);
		break;
	default:
DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
	    break;
	}
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
    zms->super= super;

    zms->ports = g_new(PortioList, 1);

    portio_list_init(zms->ports, OBJECT(zms), zaphod_mc6850_portio, zms, "zaphod.mc6850");
    portio_list_add(zms->ports, get_system_io(), 0x00);

    return zms;
}
