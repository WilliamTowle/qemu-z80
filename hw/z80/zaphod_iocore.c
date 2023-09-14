/*
 * QEmu Zaphod board - IOCore support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "zaphod.h"
#include "zaphod_uart.h"

#include "qemu/error-report.h"
#include "qapi/error.h"
#include "exec/address-spaces.h"


//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("zaphod_iocore: " fmt , ## __VA_ARGS__); } while(0)


/* PARTIAL
 * Missing here:
 *  1. for input - _can_receive and _receive callbacks
 *  2. for output - putchar
 *
 * Handles IO ports for 'stdio' and ACIA input, as expected by the
 * "teletype" ROM for the Phil Brown emulator and the BASIC ROM for
 * the Grant Searle board emulation respectively.
 * The IRQ for the MC6850 UART is managed here, and the 'inkey'
 * value shared by UART and corresponding screen is part of the UART
 * code.
 */

/* stdio chardev handlers */

static
int zaphod_iocore_can_receive_stdio(void *opaque)
{
    /* Maybe implement a FIFO queue? */
    return 1;
}

static
void zaphod_iocore_receive_stdio(void *opaque, const uint8_t *buf, int len)
{
    ZaphodIOCoreState *zis= (ZaphodIOCoreState *)opaque;

    zaphod_uart_set_inkey(zis->board->uart_stdio, buf[0], true);
}


/* stdio ioport handlers */

static uint32_t zaphod_iocore_read_stdio(void *opaque, uint32_t addr)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(opaque);
    int                 value;

    switch (addr)
    {
    case 0x00:      /* stdin */
        value= zaphod_uart_get_inkey(zis->board->uart_stdio, true);
        return value;
    default:
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
#endif
        return 0x00;
    }
}

static
void zaphod_iocore_putchar_stdio(ZaphodIOCoreState *zis, const unsigned char ch)
{
#ifdef CONFIG_ZAPHOD_HAS_UART
        zaphod_uart_putchar(zis->board->uart_stdio, ch);
#endif
}

static void zaphod_iocore_write_stdio(void *opaque, uint32_t addr, uint32_t value)
{
    ZaphodIOCoreState   *zis= (ZaphodIOCoreState *)opaque;

    switch (addr)
    {
    case 0x01:      /* stdout */
        zaphod_iocore_putchar_stdio(zis, value & 0xff);
        break;
    default:
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
#endif
        break;
    }
}

static const MemoryRegionPortio zaphod_iocore_portio_stdio[] = {
    { 0x00, 1, 1, .read = zaphod_iocore_read_stdio },     /* stdin */
    { 0x01, 1, 1, .write = zaphod_iocore_write_stdio, },  /* stdout */
    PORTIO_END_OF_LIST()
};

static void zaphod_iocore_realizefn(DeviceState *dev, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(dev);

    if (!zis->board)
    {
        error_setg(errp, "initialisation error - zis->board NULL");
        return;
    }

    /* stdio setup */

#if 1   /* WmT - TRACE */
;DPRINTF("INFO: %s() about to do stdio init/add...\n", __func__);
#endif
    zis->ioports_stdio = g_new(PortioList, 1);
    portio_list_init(zis->ioports_stdio, OBJECT(zis), zaphod_iocore_portio_stdio,
                    zis, "zaphod.stdio");
    portio_list_add(zis->ioports_stdio, get_system_io(), 0x00);

    /* TODO: configuration of stdin and ACIA inputs from command line */

    qemu_chr_fe_set_handlers(&zis->board->uart_stdio->chr,
                    zaphod_iocore_can_receive_stdio, zaphod_iocore_receive_stdio,
                    NULL,
                    NULL, zis, NULL, true);

    /* TODO: correlate screen(s) to stdio/acia input; enable (and
     * configure) screen to suit command line arguments
     */
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: %s() about to do screen init/add...\n", __func__);
#endif
    zis->screen= ZAPHOD_SCREEN(object_new(TYPE_ZAPHOD_SCREEN));
    qdev_init_nofail(DEVICE(zis->screen));
}

#if 0
static Property zaphod_iocore_properties[]= {
    DEFINE_PROP_BOOL("has-stdio",  ZaphodIOCoreState, has_stdio, false),
    DEFINE_PROP_BOOL("has-acia",  ZaphodIOCoreState, has_acia, false),
    DEFINE_PROP_END_OF_LIST()
};
#endif

static void zaphod_iocore_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc= DEVICE_CLASS(oc);

    dc->desc= "Zaphod IOCore subsystem";
    dc->realize= zaphod_iocore_realizefn;
#if 0
    /* TODO: initialisation in dc->reset? */
    dc->props= zaphod_iocore_properties;
#endif
}

static void zaphod_iocore_instance_init(Object *obj)
{
    /* Nothing to do here - handlers are set in realize function */
}


static const TypeInfo zaphod_iocore_info= {
    .name= TYPE_ZAPHOD_IOCORE,
    .parent= TYPE_DEVICE,
    /* For ZaphodIOCoreClass with virtual functions:
    .class_size= sizeof(ZaphodIOCoreClass),
     */
    .class_init= zaphod_iocore_class_init,
    .instance_size= sizeof(ZaphodIOCoreState),
    .instance_init= zaphod_iocore_instance_init
};

static void zaphod_iocore_register_types(void)
{
    type_register_static(&zaphod_iocore_info);
}

type_init(zaphod_iocore_register_types)
