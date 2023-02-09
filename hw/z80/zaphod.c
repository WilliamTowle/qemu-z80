/*
 * QEmu Zaphod sample board
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "zaphod.h"
#include "cpu.h"

#include "exec/address-spaces.h"
#include "hw/hw.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/qdev-properties.h"
#include "sysemu/sysemu.h"
#include "sysemu/reset.h"
#include "qapi/error.h"
#include "qemu/units.h"


//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("zaphod: " fmt , ## __VA_ARGS__); } while(0)


/* ZAPHOD_RAM_SIZE:
 * Ensure Zaphod boards have 64KiB RAM
 * Other system emulations might want more or less RAM
 */
#define ZAPHOD_RAM_SIZE     Z80_MAX_RAM_SIZE


static void zaphod_load_kernel(const char *kernel_filename)
{
    if (!kernel_filename || !kernel_filename[0])
    {
        hw_error("No code to run - missing '-kernel' argument");
    }
    else
    {
        int   kernel_size;

        kernel_size= get_image_size(kernel_filename);
        if (kernel_size <= 0)
        {
            hw_error("%s(): Kernel with bad size specified - %s\n", __func__, (kernel_size == 0)? "file empty" : "file missing?");
        }

        kernel_size= load_image_targphys(kernel_filename, 0, kernel_size);
        if (kernel_size < 0) {
            hw_error("Couldn't load kernel file '%s'", kernel_filename);
        }

#if 1   /* WmT - TRACE */
;DPRINTF("INFO: %s(): Kernel size %d bytes\n", __func__, kernel_size);
#endif
    }
}

static void main_cpu_reset(void *opaque)
{
    Z80CPU *cpu = opaque;

    cpu_reset(CPU(cpu));
}


/* Create UART object. Previously serial0 was always mapped to the
 * stdio UART and serial1 to the MC6850 (if present). This basic UART
 * serves either purpose [with IOCore managing ioports and ACIA IRQ]
 */
static DeviceState *zaphod_uart_new(Chardev *chr_fallback)
{
    DeviceState         *dev= DEVICE(object_new(TYPE_ZAPHOD_UART));
    ZaphodUARTState     *zus= ZAPHOD_UART(dev);

    qdev_prop_set_chr(DEVICE(zus), "chardev", chr_fallback);

#if QEMU_VERSION_MAJOR < 5
    qdev_init_nofail(dev);
#else
    qdev_realize(dev, NULL, NULL);
#endif
    return dev;
}

/* Create IOCore object (to manage IO ports/IRQ) */
static DeviceState *zaphod_iocore_new(ZaphodMachineState *zms)
{
    DeviceState         *dev= DEVICE(object_new(TYPE_ZAPHOD_IOCORE));
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(dev);

    zis->board= zms;

#if QEMU_VERSION_MAJOR < 5
    qdev_init_nofail(dev);
#else
    qdev_realize(dev, NULL, NULL);
#endif
    return dev;
}


/* Machine state initialisation */

static void zaphod_board_init(MachineState *ms)
{
    ZaphodMachineState *zms = ZAPHOD_MACHINE(ms);
    const char *kernel_filename = ms->kernel_filename;
    MemoryRegion *address_space_mem;
    MemoryRegion *ram;
    CPUState *cs;


    /* Init CPU/set reset callback */

    cs= cpu_create(ms->cpu_type);
    zms->cpu = Z80_CPU(cs);
    qemu_register_reset(main_cpu_reset, zms->cpu);

    /* QEmu v5: reset has happened */
    //cpu_reset(cs);
    cpu_set_pc(cs, 0x0000);


    /* Initialise RAM */

    address_space_mem= get_system_memory();
    ram= g_new(MemoryRegion, 1);

    /* Override any '-m <memsize>' option.
     * NB: '-m <n>K' is valid, we should permit ms->ram_size <= 64K
     */
    memory_region_init_ram(ram, NULL, "zaphod.ram",
                            ZAPHOD_RAM_SIZE, &error_fatal);
    /* Leaving the entire 64KiB memory space writable supports
     * self-modifying test code. Call memory_region_set_readonly()
     * for ROM regions,
     */
    memory_region_add_subregion(address_space_mem, 0x0000, ram);


    /* Initialise ports/devices */

    if (serial_hd(0))
    {   /* QEmu's main serial console is available */
        zms->uart_stdio= ZAPHOD_UART(zaphod_uart_new(serial_hd(0)));
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: UART device created OK, at %p\n", zms->uart_stdio);
#endif
    }

#ifdef CONFIG_ZAPHOD_HAS_IOCORE
    /* Initialise IOCore subsystem */
;DPRINTF("*** INFO: %s() - about to do iocore init... ***\n", __func__);
/* NB. we can get a serial0 and a serial1 with:
 *$ ./z80-softmmu/qemu-system-z80 -M zaphod-dev -chardev vc,id=vc0 -chardev vc,id=vc1 -serial chardev:vc0 -serial chardev:vc1 -kernel wills/system/zaphodtt.bin
 */
    zms->iocore= ZAPHOD_IOCORE(zaphod_iocore_new(zms));
#endif


    /* Populate RAM */

    zaphod_load_kernel(kernel_filename);
}


/* Machine class initialisation: zaphod_machine_class_init() */

static void zaphod_machine_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    /* Set description, init function, default CPU */

    mc->desc= "Zaphod development board";
    mc->init= zaphod_board_init;
    //mc->alias= "zaphod";
    mc->is_default= 1;

    /* TODO: Z80, but want R800 option if requested and available */
    //mc->default_cpu_type= Z80_CPU_TYPE_NAME("z80");
    mc->default_cpu_type= TARGET_DEFAULT_CPU_TYPE;
    mc->default_cpus= 1;
    mc->min_cpus= mc->default_cpus;
    mc->max_cpus= mc->default_cpus;
    mc->default_ram_size= ZAPHOD_RAM_SIZE;

    mc->no_floppy= 1;
    mc->no_cdrom= 1;
    mc->no_parallel= 1;
    mc->no_serial= 1;      /* TODO: set to '0' where serial available */
    mc->no_sdcard= 1;
}


static const TypeInfo zaphod_machine_type_generic = {
    /* Provide a "generic" board definition. Use of
     * TYPE_ZAPHOD_MACHINE means this board name is "zaphod"
     * TODO: expand to support board variants when the relevant
     * peripheral types are available
     */
    .name           = TYPE_ZAPHOD_MACHINE,
    .parent         = TYPE_MACHINE,
    .abstract       = false,
    .class_init     = zaphod_machine_class_init,
    .class_size     = sizeof(ZaphodMachineClass),
    //[v2] .instance_init  = zaphod_machine_state_init,
    .instance_size  = sizeof(ZaphodMachineState)
};

static void zaphod_machine_register_types(void)
{
    type_register_static(&zaphod_machine_type_generic);
}

type_init(zaphod_machine_register_types)
