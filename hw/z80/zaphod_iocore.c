/*
 * QEmu Zaphod board - IOCore support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "zaphod.h"

#include "qemu/error-report.h"

//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("zaphod_iocore: " fmt , ## __VA_ARGS__); } while(0)


/* PARTIAL
 * Missing here:
 *  1. for input - _can_receive and _receive callbacks
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

static void zaphod_iocore_realizefn(DeviceState *dev, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(dev);

    zis->ioports_stdio = g_new(PortioList, 1);
    portio_list_init(zis->ioports_stdio, OBJECT(zis), zaphod_iocore_portio_stdio,
                    zis, "zaphod.stdio");
    portio_list_add(zis->ioports_stdio, get_system_io(), 0x00);
;DPRINTF("INFO: Done stdio init/add\n");

;DPRINTF("INFO: About to set chardev handlers for stdio... (chardev connected %s)\n", qemu_chr_fe_backend_connected(&zis->board->uart_stdio->chr)?"y":"n");
    qemu_chr_fe_set_handlers(&zis->board->uart_stdio->chr,
                    zaphod_iocore_can_receive_stdio, zaphod_iocore_receive_stdio,
                    NULL,
                    NULL, zis, NULL, true);
}

#if 0   /* 'chardev' removed (see sercon/mc6850 devices) */
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
