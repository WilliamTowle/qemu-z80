/*
 * QEmu Zaphod board - IOCore support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "qemu/osdep.h"
#include "zaphod.h"

#include "qemu/error-report.h"

#define DPRINTF(fmt, ...) \
    do { if (ZAPHOD_DEBUG) error_printf("zaphod_iocore: " fmt , ## __VA_ARGS__); } while(0)


/* PARTIAL
 * Missing here:
 *  1. for input - _can_read and _read callbacks
 *  2. for output - putchar
 *
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
static void zaphod_iocore_instance_init(Object *obj)
{
    /* Nothing to do here - handlers are set in realize function */
}

ZaphodIOCoreState *zaphod_iocore_init(void)
{
    DeviceState *dev;

    dev = DEVICE(object_new(TYPE_ZAPHOD_IOCORE));

    qdev_init_nofail(dev);
    return ZAPHOD_IOCORE(dev);
}


static const TypeInfo zaphod_iocore_info= {
	.name= TYPE_ZAPHOD_IOCORE,
	.parent= TYPE_DEVICE,
	/* For ZaphodIOCoreClass with virtual functions:
	.class_size= sizeof(ZaphodIOCoreClass),
	.class_init= zaphod_iocore_class_init,
	 */
	.instance_size= sizeof(ZaphodIOCoreState),
	.instance_init= zaphod_iocore_instance_init
};

static void zaphod_iocore_register_types(void)
{
	type_register_static(&zaphod_iocore_info);
}

type_init(zaphod_iocore_register_types)
