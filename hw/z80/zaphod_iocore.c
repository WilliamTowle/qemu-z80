/*
 * QEmu Zaphod board - iocore support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "qemu/osdep.h"
#include "zaphod.h"

#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "exec/address-spaces.h"
#include "ui/console.h"


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
 *
 * TODO: Could 'iocore' simultaneously support stdio (polled ASCII
 * feed) and mc6850 (asynchronous, with interrupts)?  If not, should
 * stdio [sercon] and ACIA need per-device input consoles with
 * 'inkey' state? How do we handle input and output streams/muxing?
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
    /* Maybe implement a FIFO queue? */
    return 1;
}

static
void zaphod_iocore_receive_acia(void *opaque, const uint8_t *buf, int len)
{
    ZaphodIOCoreState *zis= (ZaphodIOCoreState *)opaque;

    zaphod_uart_set_inkey(zis->board->uart_acia, buf[0], true);
    if (zis->irq_acia)
        qemu_irq_raise(*zis->irq_acia);
}


/* stdio ioport handlers */

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


/* ACIA ioport handlers */

static uint32_t zaphod_iocore_read_acia(void *opaque, uint32_t addr)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(opaque);
    int		value;

    switch (addr)
    {
    case 0x80:      /* read ACIA UART PortStatus */
#if 0	/* deprecated - deferred to zaphod_uart.c function */
        value= zaphod_uart_get_inkey(zis->board->uart_acia, false)? 0x01 : 0; /* RxDataReady */
        value|= 0x02;       /* TxDataEmpty (always) */
        value|= 0x04;       /* DTD [Data Carrier Detect] */
        value|= 0x08;       /* CTS [Clear to Send] */
            /* FrameErr|Overrun|ParityErr|IrqReq not emulated */
;DPRINTF("DEBUG: %s() read ACIA UART PortStatus (port 0x%02x) -> status %02x\n", __func__, addr, value);
        return value;
#else
        return zaphod_uart_portstatus(zis->board->uart_acia);
#endif
    case 0x81:      /* read ACIA UART RxData */
        value= zaphod_uart_get_inkey(zis->board->uart_acia, true);
        if (zis->irq_acia)
            qemu_irq_lower(*zis->irq_acia);
;DPRINTF("DEBUG: %s() read ACIA UART RXData (port 0x%02x) -> inkey ch-value %d\n", __func__, addr, value);
        return value? value : 0xff;
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
    ZaphodIOCoreState   *zis= (ZaphodIOCoreState *)opaque;

    switch (addr)
    {
    case 0x80:      /* write -> ACIA UART PortControl */
        /* ignore since baud rate change etc. not emulated? */
;DPRINTF("DEBUG: %s() write m6850 PortControl (port 0x%02x) <- value %d\n", __func__, addr, value);
        break;
    case 0x81:      /* write -> ACIA UART TxData */
;DPRINTF("DEBUG: %s() write ACIA UART TxData (port 0x%02x) -> ch-value=%d\n", __func__, addr, value);
        //zaphod_consolegui_putchar(zms->console, toupper(value & 0xff));
        //zaphod_serio_putchar(/*zms->console, */value & 0xff);
        zaphod_iocore_putchar_acia(zis, value & 0xff);
        break;
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


#if 1	/* keyboard I/O (EXPERIMENTAL) */
static const unsigned char keycode_to_asciilc[128]= {
    /* keymap for UK QWERTY keyboard - NB. repo.or.cz uses its
     * callback to translate keycode to row and column [as per the
     * Spectrum's keyboard electronics]. When feeding input to our
     * sercon or MC6850 input streams, we want ASCII.
     * (FIXME: this is (unintentionally) partial, and the handler
     * (also) lacks code to sense/track/apply modifier keys)
     */
      0,  0,'1','2','3','4','5','6',
    '7','8','9','0',  0,  0,  0,  0,
    'q','w','e','r','t','y','u','i',
    'o','p',  0,  0, 13,  0,'a','s',
    'd','f','g','h','j','k','l',  0,
      0,  0,  0,  0,'z','x','c','v',
    'b','n','m',  0,  0,  0,  0,  0,
      0,' ',  0,  0,  0,  0,  0,  0,

      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,

      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
};

static void zaphod_iocore_put_keycode(void *opaque, int keycode)
{
    //ZaphodIOCoreState *zis= (ZaphodIOCoreState *)opaque;
    int	release= keycode & 0x80;

    if (release)
    {
        /* FIXME: handle XT scancode 0xe0 properly */
        /* TODO: resetting cs_inkey on key release is risky if
         * systems without an IRQ sent on keypress don't poll
         * the port promptly enough. Do we want to implement key
         * repeat (in which case we start a timer elsewhere that
         * should get stopped here)?
         */
#if 1
;DPRINTF("INFO: %s() - key with keycode=0x%02x released\n", __func__, keycode);
#else   /* inkey API missing - TODO: is this stdio or ACAI input? */
        zaphod_set_inkey(zss->super, 0, false);
#endif
    }
    else
    {
      int	ch= keycode_to_asciilc[keycode & 0x7f];
#if 1
;DPRINTF("INFO: %s() - keypress -> BAIL: : keycode=0x%02x (ch %d)\n", __func__, keycode, ch);
;exit(1);
#else
	/* TODO:
	 * - determine the "muxed" UART (ACIA, if defined)
	 * - check can-receive
	 * - calling receive will trigger IRQ as required
	 */
#endif
    }
}
#endif

static void zaphod_iocore_realizefn(DeviceState *dev, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(dev);

    if (!zis->board)
    {
        error_setg(errp, "initialisation error - zis->board NULL");
        return;
    }

    /* stdio setup */

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


    /* ACIA setup */

    if (zis->board->uart_acia)
    {
        zis->ioports_acia = g_new(PortioList, 1);
        portio_list_init(zis->ioports_acia, OBJECT(zis), zaphod_iocore_portio_acia,
                        zis, "zaphod.acia");
        portio_list_add(zis->ioports_acia, get_system_io(), 0x00);


        qemu_chr_fe_set_handlers(&zis->board->uart_acia->chr,
                    zaphod_iocore_can_receive_acia, zaphod_iocore_receive_acia,
                    NULL,
                    NULL, zis, NULL, true);

        zis->irq_acia= qemu_allocate_irqs(zaphod_interrupt_request, zis->board, 1);
    }

#if 1	/* TODO: avoid under legacy KEYBIO? */
    /* FIXME: rework with QemuInputHandler */
    qemu_add_kbd_event_handler(zaphod_iocore_put_keycode, zis);
#endif
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
    //ZaphodIOCoreState *zis= ZAPHOD_IOCORE(obj);

    /* Nothing to do here - handlers are set in
     * zaphod_iocore_realizefn()
     */
}

//DeviceState *zaphod_iocore_new(void)
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
