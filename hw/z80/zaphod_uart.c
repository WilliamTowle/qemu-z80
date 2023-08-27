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


//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("zaphod_uart: " fmt , ## __VA_ARGS__); } while(0)


#if defined(CONFIG_ZAPHOD_HAS_IOCORE)
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
    zus->inkey_valid= true;
}
#endif

void zaphod_uart_putchar(ZaphodUARTState *zus, const unsigned char ch)
{
;DPRINTF("INFO: %s() write of character '%c' (connected? %s)\n", __func__, ch, qemu_chr_fe_backend_connected(&zus->chr)?"y":"n");
    if (unlikely(!qemu_chr_fe_backend_connected(&zus->chr)))
        return;     /* CPU IOPort write without console window */

    if (isprint(ch))
    {
        qemu_chr_write(zus->chr.chr, &ch, 1, true);
    }
    else
    {   /* HACK: Show non-printable characters as corresponding hex */
        uint8_t nyb_hi, nyb_lo;

        nyb_hi= (ch & 0xf0) >> 4;
        nyb_lo= ch & 0x0f;

        zaphod_uart_putchar(zus, '[');
        nyb_hi+= (nyb_hi > 9)? 'A' - 10 : '0';
        //zus->sercon->chr_write(zus->sercon, &nyb_hi, 1);
        zaphod_uart_putchar(zus, nyb_hi);
        nyb_lo+= (nyb_lo > 9)? 'A' - 10 : '0';
        //zus->sercon->chr_write(zus->sercon, &nyb_lo, 1);
        zaphod_uart_putchar(zus, nyb_lo);
        zaphod_uart_putchar(zus, ']');
        zaphod_uart_putchar(zus, '*');
    }
}

uint8_t zaphod_uart_get_inkey(void *opaque, bool read_and_clear)
{
    ZaphodUARTState *zus= (ZaphodUARTState *)opaque;
    uint8_t val= zus->inkey;

    if (read_and_clear)
    {
        zus->inkey= '\0';
        zus->inkey_valid= false;
    }

    return val;
}

void zaphod_uart_set_inkey(void *opaque, uint8_t val, bool is_data)
{
;DPRINTF("DEBUG: Reached %s() - opaque is %p, chval is 0x%02x \n", __func__, opaque, val);
    ZaphodUARTState *zus= (ZaphodUARTState *)opaque;
    zus->inkey= val;

    /* TODO: raise interrupt logic here?
     * stdin mode (sercon) case: no interrupt to be raised
     * ACIA mode (mc6850) case: raise interrupt if this is data
     */
}


static void zaphod_uart_realizefn(DeviceState *dev, Error **errp)
{
    /* Nothing to do here */
}

static void zaphod_uart_reset(DeviceState *dev)
{
    ZaphodUARTState   *zus= ZAPHOD_UART(dev);

    zus->inkey= '\0';
    zus->inkey_valid= false;
}


static Property zaphod_uart_properties[]= {
    /* properties can be set with '-global zaphod-uart.VAR=VAL' */
    DEFINE_PROP_CHR("chardev",  ZaphodUARTState, chr),
    DEFINE_PROP_END_OF_LIST()
};

static void zaphod_uart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc= DEVICE_CLASS(klass);

    dc->desc= "Zaphod UART device";
    dc->realize= zaphod_uart_realizefn;
    dc->reset= zaphod_uart_reset;
    dc->props= zaphod_uart_properties;
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static void zaphod_uart_instance_init(Object *obj)
{
    /* Nothing to do here */
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
