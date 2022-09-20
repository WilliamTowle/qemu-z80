/*
 * QEmu Zaphod board - UART support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "qemu/osdep.h"
#include "zaphod.h"

#include "qemu/error-report.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"


#define DPRINTF(fmt, ...) \
    do { if (ZAPHOD_DEBUG) error_printf("zaphod_uart: " fmt , ## __VA_ARGS__); } while(0)


#if defined(CONFIG_ZAPHOD_HAS_IOCORE)
int zaphod_uart_portstatus(void *opaque)
{
    ZaphodUARTState *zus= ZAPHOD_UART(opaque);
    int value;

    value= zus->inkey? 0x01 : 0;       /* RxDataReady */
    value|= 0x02;                      /* TxDataEmpty (always) */
    value|= 0x04;                      /* DTD [Data Carrier Detect] */
    value|= 0x08;                      /* CTS [Clear to Send] */
    /* FrameErr|Overrun|ParityErr|IrqReq not emulated */

    return value;
}

int zaphod_uart_can_receive(void *opaque)
{
    /* With no FIFO, we can always store something to 'inkey' (and
     * the device-specific 'opaque' value is redundant
     */
    return 1;
}
#else
static
int zaphod_uart_can_receive(void *opaque)
{
    /* Maybe implement a FIFO queue? */
    return 1;
}

static
void zaphod_uart_receive(void *opaque, const uint8_t *buf, int len)
{
    ZaphodUARTState *zus= ZAPHOD_UART(opaque);

    zus->inkey= buf[0];
}
#endif

void zaphod_uart_putchar(ZaphodUARTState *zus, const unsigned char ch)
{
;DPRINTF("INFO: %s() write of character '%c' (connected? %s)\n", __func__, ch, qemu_chr_fe_backend_connected(&zus->chr)?"y":"n");
    if (unlikely(!qemu_chr_fe_backend_connected(&zus->chr)))
        return;     /* CPU IOPort write without console window */

    /* TODO: standard control codes are handled by the serial
     * console - non-standard ones will need to be treated specially
     */
    qemu_chr_write(zus->chr.chr, &ch, 1, true);
}

uint8_t zaphod_uart_get_inkey(void *opaque, bool read_and_clear)
{
    ZaphodUARTState *zus= (ZaphodUARTState *)opaque;
    uint8_t val= zus->inkey;

    if (read_and_clear) zus->inkey= 0;

    return val;
}

void zaphod_uart_set_inkey(void *opaque, uint8_t val, bool is_data)
{
;DPRINTF("DEBUG: Reached %s() - opaque is %p, chval is 0x%02x \n", __func__, opaque, val);
    ZaphodUARTState *zus= (ZaphodUARTState *)opaque;
    zus->inkey= val;

    /* TODO: raise interrupt logic here?
     * stdin mode (sercon) case: no interrupt to be raised
     * acai mode (mc6850) case: raise interrupt if this is data
     */
}


DeviceState *zaphod_uart_new(Chardev *chr_fallback)
{
    DeviceState         *dev= DEVICE(object_new(TYPE_ZAPHOD_UART));
    ZaphodUARTState     *zus= ZAPHOD_UART(dev);

    if (!qemu_chr_fe_backend_connected(&zus->chr))
        qdev_prop_set_chr(DEVICE(zus), "chardev", chr_fallback);

    qdev_init_nofail(dev);
    return dev;
}


static void zaphod_uart_realizefn(DeviceState *dev, Error **errp)
{
    //ZaphodUARTState *zus= ZAPHOD_UART(dev);
}

static void zaphod_uart_reset(DeviceState *dev)
{
    ZaphodUARTState   *zus= ZAPHOD_UART(dev);

    zus->inkey= '\0';
}


static Property zaphod_uart_properties[] = {
    /* properties can be set with '-global zaphod-uart.VAR=VAL' */
    DEFINE_PROP_CHR("chardev",  ZaphodUARTState, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void zaphod_uart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = zaphod_uart_realizefn;
    dc->reset= zaphod_uart_reset;
    dc->props = zaphod_uart_properties;
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static void zaphod_uart_instance_init(Object *obj)
{
#if 0   /* TODO: v1 init support */
    ZaphodUARTState *zms= ZAPHOD_UART(obj);
    /* ... */
#endif
}


static const TypeInfo zaphod_uart_info= {
	.name= TYPE_ZAPHOD_UART,
	.parent= TYPE_DEVICE,
	/* For ZaphodUARTClass with virtual functions:
	.class_size= sizeof(ZaphodUARTClass),
	 */
	.class_init= zaphod_uart_class_init,
	.instance_size= sizeof(ZaphodUARTState),
	.instance_init= zaphod_uart_instance_init
};

static void zaphod_uart_register_types(void)
{
	type_register_static(&zaphod_uart_info);
}

type_init(zaphod_uart_register_types)
