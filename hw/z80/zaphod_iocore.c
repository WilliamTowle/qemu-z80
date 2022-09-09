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
    return zaphod_uart_can_receive(opaque);
}

static
void zaphod_iocore_receive_stdio(void *opaque, const uint8_t *buf, int len)
{
    ZaphodIOCoreState *zis= (ZaphodIOCoreState *)opaque;

    zaphod_uart_set_inkey(zis->board->uart_stdio, buf[0], true);
}


/* ACIA chardev handlers */

static
int zaphod_iocore_can_receive_acia(void *opaque)
{
    return zaphod_uart_can_receive(opaque);
}

static
void zaphod_iocore_receive_acia(void *opaque, const uint8_t *buf, int len)
{
    ZaphodIOCoreState *zis= (ZaphodIOCoreState *)opaque;

    /* TODO: QEmu's SDL v2 support introduces sdl2_process_key(),
     * which injects '\n' in the input stream for Q_KEY_CODE_RET,
     * effectively performing CR -> NL translation on input compared
     * to QEmu v1. Previously we received '\r' in the buffer here.
     */
    zaphod_uart_set_inkey(zis->board->uart_acia, buf[0], true);
    if (zis->irq_acia)
        qemu_irq_raise(*zis->irq_acia);
}


/* stdio ioport handlers */

static uint32_t zaphod_iocore_read_stdio(void *opaque, uint32_t addr)
{
    static ZaphodUARTState  *zus= NULL;
    static bool             vc_present= false;
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(opaque);
    int                 value;

    if (!vc_present)
    {   /* check a VC exists for input; simulate one otherwise */
        if ( (zus= zis->board->uart_stdio) )
        {
            vc_present= true;
        }
    }

    switch (addr)
    {
    case 0x00:      /* stdin */
        value= 0x00;
        if (vc_present)
            value= zaphod_uart_get_inkey(zus, true);
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
        if (zis->board->uart_stdio)
            zaphod_uart_putchar(zis->board->uart_stdio, ch);
#endif
#ifdef CONFIG_ZAPHOD_HAS_SCREEN
        /* mux to screen (TODO: not if ACIA set? make configurable?) */
        zaphod_screen_putchar(zis->screen, ch);
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


ZaphodScreenState *zaphod_iocore_get_screen(ZaphodIOCoreState *zis)
{
    if (zis->screen) return zis->screen;

    return zis->screen= ZAPHOD_SCREEN(object_new(TYPE_ZAPHOD_SCREEN));
}

/* ACIA ioport handlers */

static uint32_t zaphod_iocore_read_acia(void *opaque, uint32_t addr)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(opaque);
    int                 value;

    switch (addr)
    {
    case 0x80:      /* ACIA: read UART PortStatus */
        return zaphod_uart_portstatus(zis->board->uart_acia);

    case 0x81:      /* ACIA: read UART RxData */
        value= zaphod_uart_get_inkey(zis->board->uart_acia, true);
        if (zis->irq_acia)
            qemu_irq_lower(*zis->irq_acia);
;DPRINTF("DEBUG: %s() read ACIA UART RXData (port 0x%02x) -> inkey ch-value %d\n", __func__, addr, value);
        return value? value : 0xff;

    default:
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
#endif
        return 0x00;
    }
}

static
void zaphod_iocore_putchar_acia(ZaphodIOCoreState *zis, const unsigned char ch)
{
#ifdef CONFIG_ZAPHOD_HAS_UART
        zaphod_uart_putchar(zis->board->uart_acia, ch);
#endif
#ifdef CONFIG_ZAPHOD_HAS_SCREEN
        /* mux to screen (TODO: make configurable) */
        zaphod_screen_putchar(zis->screen, ch);
#endif
}

static void zaphod_iocore_write_acia(void *opaque, uint32_t addr, uint32_t value)
{
    ZaphodIOCoreState   *zis= (ZaphodIOCoreState *)opaque;

    switch (addr)
    {
    case 0x80:      /* ACIA: write UART PortControl */
        /* Received byte controls baud rate, encoding, and which
         * status events trigger the interrupt. The Zaphod machines do
         * not need these to be emulated
         */
//;DPRINTF("DEBUG: %s() write ACIA PortControl (port 0x%02x) is a NOP <- value %d\n", __func__, addr, value);
        break;
    case 0x81:      /* ACIA: write UART TxData */
//;DPRINTF("DEBUG: %s() write ACIA UART TxData (port 0x%02x) -> ch-value=%d\n", __func__, addr, value);
        zaphod_iocore_putchar_acia(zis, value & 0xff);
        break;
    default:
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
#endif
        break;
    }
}

static const MemoryRegionPortio zaphod_iocore_portio_acia[] = {
    /* TODO: 0x80-0x81 apply to Grant Searle BASIC ROM but hardware
     * decodes all of 0x80-0xbf [with even/odd ports equivalent]
     */
    { 0x80, 2, 1,
                .read = zaphod_iocore_read_acia,
                .write = zaphod_iocore_write_acia
                },
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

    if (zis->has_stdio)
    {
        qemu_chr_fe_set_handlers(&zis->board->uart_stdio->chr,
                    zaphod_iocore_can_receive_stdio, zaphod_iocore_receive_stdio,
                    NULL,
                    NULL, zis, NULL, true);
    }

    /* ACIA setup */

    zis->ioports_acia = g_new(PortioList, 1);
    portio_list_init(zis->ioports_acia, OBJECT(zis), zaphod_iocore_portio_acia,
                    zis, "zaphod.acia");
    portio_list_add(zis->ioports_acia, get_system_io(), 0x00);

    if (zis->has_acia)
    {
        qemu_chr_fe_set_handlers(&zis->board->uart_acia->chr,
                    zaphod_iocore_can_receive_acia, zaphod_iocore_receive_acia,
                    NULL,
                    NULL, zis, NULL, true);

        zis->irq_acia= qemu_allocate_irqs(zaphod_interrupt_request, zis->board, 1);
    }

    /* TODO: correlate screen(s) to stdio/acia input; enable (and
     * configure) screen to suit command line arguments
     */
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: %s() about to do screen init/add...\n", __func__);
#endif
    if (zis->screen)
        qdev_init_nofail(DEVICE(zis->screen));
}

#if 0
static Property zaphod_iocore_properties[]= {
    DEFINE_PROP_BOOL("has-stdio",  ZaphodIOCoreState, has_stdio, false),
    DEFINE_PROP_BOOL("has-acia",  ZaphodIOCoreState, has_acia, false),
    DEFINE_PROP_END_OF_LIST()
};
#endif

static bool zaphod_iocore_get_has_stdio(Object *obj, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(obj);

    return zis->has_stdio;
}

static void zaphod_iocore_set_has_stdio(Object *obj, bool value, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(obj);

    /* TODO: bail if inited and new setting is off */
    zis->has_stdio= value;
}

static bool zaphod_iocore_get_has_acia(Object *obj, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(obj);

    /* TODO: bail if inited and new setting is off? */
    return zis->has_acia;
}

static void zaphod_iocore_set_has_acia(Object *obj, bool value, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(obj);

    /* TODO: bail if inited and new setting is off? */
    zis->has_acia= value;
}

static void zaphod_iocore_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc= DEVICE_CLASS(oc);

    dc->desc= "Zaphod IOCore subsystem";
    dc->realize= zaphod_iocore_realizefn;
#if 0
    /* TODO: initialisation in dc->reset? */
    dc->props= zaphod_iocore_properties;
#endif

    object_class_property_add_bool(oc, "has-stdio",
        zaphod_iocore_get_has_stdio, zaphod_iocore_set_has_stdio, NULL);
    object_class_property_set_description(oc, "has-stdio",
        "Configure IOCore with stdio devices", NULL);
    object_class_property_add_bool(oc, "has-acia",
        zaphod_iocore_get_has_acia, zaphod_iocore_set_has_acia, NULL);
    object_class_property_set_description(oc, "has-acia",
        "Configure IOCore with ACIA devices", NULL);
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
