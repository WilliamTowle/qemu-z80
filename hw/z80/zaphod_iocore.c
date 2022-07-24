/*
 * QEmu Zaphod board - IOCore support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "qemu/osdep.h"
#include "zaphod.h"

#include "qemu/error-report.h"
#include "qapi/error.h"
#include "zaphod_uart.h"
#include "exec/address-spaces.h"

#define DPRINTF(fmt, ...) \
    do { if (ZAPHOD_DEBUG) error_printf("zaphod_iocore: " fmt , ## __VA_ARGS__); } while(0)


/* PARTIAL
 * We need to handle running the BASIC ROM on the Grant Searle board
 * [I/O via MC6850] and running the Zaphod "teletype" ROM on the Phil
 * Brown board. There is an IRQ on input associated with the former.
 *
 * Historically 'inkey' is shared between sercon, mc6850, and KEYBIO
 * input, with putchar() invoking APIs for both "serial" and "screen"
 * devices. When running the BASIC ROM the relevant CPU interrupt mode
 * is enabled explicitly, leading to handlers running appropriately;
 * the teletype ROM works because if the interrupts do fire, the CPU
 * ignores them by default
 */

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

static
int zaphod_iocore_can_receive_acia(void *opaque)
{
    /* Maybe implement a FIFO queue? */
    return 1;
}

static
void zaphod_iocore_receive_acia(void *opaque, const uint8_t *buf, int len)
{
    ZaphodIOCoreState *zis= (ZaphodIOCoreState *)opaque;

    zaphod_uart_set_inkey(zis->board->uart_acia, buf[0], true);
}


static uint32_t zaphod_iocore_read_stdio(void *opaque, uint32_t addr)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(opaque);
    int		value;

    switch (addr)
    {
    case 0x00:		/* stdin */
        value= zaphod_uart_get_inkey(zis->board->uart_stdio, true);
        return value;
    default:
;DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
        return 0x00;
    }
}

static
void zaphod_iocore_putchar_stdio(ZaphodIOCoreState *zis, const unsigned char ch)
{
#ifdef CONFIG_ZAPHOD_HAS_UART
        zaphod_uart_putchar(zis->board->uart_stdio, ch);
#endif
#ifdef CONFIG_ZAPHOD_HAS_SCREEN
        /* mux to screen (TODO: not if ACIA set? make configurable?) */
        zaphod_screen_putchar(zis->board, ch);
#endif
}

static void zaphod_iocore_write_stdio(void *opaque, uint32_t addr, uint32_t value)
{
    ZaphodIOCoreState   *zis= (ZaphodIOCoreState *)opaque;

    switch (addr)
    {
    case 0x01:        /* stdout */
        zaphod_iocore_putchar_stdio(zis, value & 0xff);
        break;
    default:
;DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
        break;
    }
}

static const MemoryRegionPortio zaphod_iocore_portio_stdio[] = {
    { 0x00, 1, 1, .read = zaphod_iocore_read_stdio },     /* stdin */
    { 0x01, 1, 1, .write = zaphod_iocore_write_stdio, },  /* stdout */
    PORTIO_END_OF_LIST(),
};

static uint32_t zaphod_iocore_read_acia(void *opaque, uint32_t addr)
{
    //ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(opaque);
    //int		value;

    switch (addr)
    {
#if 0   /* FIXME: needs 0x80/0x81 cases from zaphod_mc6850.c here */
    case 0x00:		/* stdin */
        value= zaphod_uart_get_inkey(zis->board->uart_acia, true);
        return value;
#endif
    default:
;DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
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
        zaphod_screen_putchar(zis->board, ch);
#endif
}

static void zaphod_iocore_write_acia(void *opaque, uint32_t addr, uint32_t value)
{
    //ZaphodIOCoreState   *zis= (ZaphodIOCoreState *)opaque;

    switch (addr)
    {
#if 0   /* FIXME: needs 0x80/0x81 cases from zaphod_mc6850.c here */
    case 0x01:        /* stdout */
        zaphod_iocore_putchar_acia(zis, value & 0xff);
        break;
#endif
    default:
;DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
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
    PORTIO_END_OF_LIST(),
};

static void zaphod_iocore_realizefn(DeviceState *dev, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(dev);

    if (!zis->board)
    {
        error_setg(errp, "initialisation error - zis->board NULL");
        return;
    }

    zis->ioports_stdio = g_new(PortioList, 1);
    portio_list_init(zis->ioports_stdio, OBJECT(zis), zaphod_iocore_portio_stdio,
                    zis, "zaphod.stdio");
    portio_list_add(zis->ioports_stdio, get_system_io(), 0x00);

    /* TODO: permit disabling stdin [in which case don't bail, but
     * don't set up the handlers either]
     */
    if (!zis->board->uart_stdio)
    {
        error_setg(errp, "initialisation error - zis->board->uart_stdio NULL");
        return;
    }

    qemu_chr_fe_set_handlers(&zis->board->uart_stdio->chr,
                    zaphod_iocore_can_receive_stdio, zaphod_iocore_receive_stdio,
                    NULL,
                    NULL, zis, NULL, true);

    zis->ioports_acia = g_new(PortioList, 1);
    portio_list_init(zis->ioports_acia, OBJECT(zis), zaphod_iocore_portio_acia,
                    zis, "zaphod.acia");
    portio_list_add(zis->ioports_acia, get_system_io(), 0x00);

    /* TODO: permit disabling acia [in which case don't bail, but
     * don't set up the handlers either]
     */
    if (!zis->board->uart_acia)
    {
        error_setg(errp, "initialisation error - zis->board->uart_acia NULL");
        return;
    }

    qemu_chr_fe_set_handlers(&zis->board->uart_acia->chr,
                    zaphod_iocore_can_receive_acia, zaphod_iocore_receive_acia,
                    NULL,
                    NULL, zis, NULL, true);
}

#if 0	/* 'chardev' removed (see sercon/mc6850 devices) */
static Property zaphod_iocore_properties[] = {
    DEFINE_PROP_CHR("chardev",  ZaphodIOCoreState, chr),
    DEFINE_PROP_BOOL("has-acia",  ZaphodIOCoreState, has_acia, false),
    DEFINE_PROP_END_OF_LIST(),
};
#endif

static void zaphod_iocore_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = zaphod_iocore_realizefn;
    /* TODO: initialisation in dc->reset? */
#if 0
    dc->props = zaphod_iocore_properties;
#endif
}

static void zaphod_iocore_instance_init(Object *obj)
{
    /* Nothing to do here - handlers are set in realize function */
}

//ZaphodIOCoreState *zaphod_iocore_new(void)
DeviceState *zaphod_iocore_new(ZaphodMachineState *zms)
{
    DeviceState         *dev= DEVICE(object_new(TYPE_ZAPHOD_IOCORE));
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(dev);

    zis->board= zms;

    qdev_init_nofail(dev);
    return dev;
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
