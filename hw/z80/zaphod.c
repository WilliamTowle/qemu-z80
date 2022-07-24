/*
 * QEmu Zaphod machine family
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "qemu-common.h"
#include "zaphod.h"
#include "cpu.h"

#include "qapi/error.h"
#include "qemu/error-report.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/char/serial.h"


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

    qdev_init_nofail(dev);
    return dev;
}


static
void zaphod_screen_init(ZaphodScreenState *zss, int board_type)
{
    switch (board_type)
    {
    case ZAPHOD_BOARD_TYPE_ZAPHOD_2:
    case ZAPHOD_BOARD_TYPE_ZAPHOD_DEV:
        qdev_prop_set_bit(DEVICE(zss), "simple-escape-codes", false);
        break;
    case ZAPHOD_BOARD_TYPE_ZAPHOD_1:
    default:
        qdev_prop_set_bit(DEVICE(zss), "simple-escape-codes", true);
    }
}

/* Prepare the IOCore for configuration (IO ports/IRQ and UARTs) */
static ZaphodIOCoreState *zaphod_iocore_init(ZaphodMachineState *zms)
{
    ZaphodMachineClass *zmc = ZAPHOD_MACHINE_GET_CLASS(zms);
    ZaphodIOCoreState *zis;

    /* create object and set internal board reference */
    zis= ZAPHOD_IOCORE(object_new(TYPE_ZAPHOD_IOCORE));
    zis->board= zms;

    /* initialise screen */
    zaphod_screen_init(zaphod_iocore_get_screen(zis), zmc->board_type);

    qdev_init_nofail(DEVICE(zis));
    return zis;
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

    if (serial_hds[0])
    {   /* QEmu's main serial console is available */
        zms->uart_stdio= ZAPHOD_UART(zaphod_uart_new(serial_hds[0]));
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: UART0 [stdio] created OK - device at %p has chr.chr %p\n", zms->uart_stdio, zms->uart_stdio->chr.chr);
#endif
    }

    if (serial_hds[1]) {
        zms->uart_acia= ZAPHOD_UART(zaphod_uart_new(serial_hds[1]));
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: UART1 [ACIA] created OK - device at %p has chr.chr %p\n", zms->uart_acia, zms->uart_acia->chr.chr);
#endif
    }


#ifdef CONFIG_ZAPHOD_HAS_IOCORE
    /* Initialise IOCore subsystem */
;DPRINTF("*** INFO: %s() - about to do iocore init... ***\n", __func__);
/* NB. we can get a serial0 and a serial1 with:
 *$ ./z80-softmmu/qemu-system-z80 -M zaphod-dev -chardev vc,id=vc0 -chardev vc,id=vc1 -serial chardev:vc0 -serial chardev:vc1 -kernel wills/system/zaphodtt.bin
 */
    zms->iocore= zaphod_iocore_init(zms);
#endif


    /* Populate RAM */

    zaphod_load_kernel(kernel_filename);
}


static void zaphod_machine_state_init(Object *obj)
{
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: Reached %s() [empty]\n", __func__);
#endif
}

static const char *board_type_name(const int board_type)
{
    const char *names[]= {
        [ZAPHOD_BOARD_TYPE_ZAPHOD_1]    = "Zaphod 1 (Phil Brown emulator)",
        [ZAPHOD_BOARD_TYPE_ZAPHOD_2]    = "Zaphod 2 (Grant Searle SBC)",
        [ZAPHOD_BOARD_TYPE_ZAPHOD_DEV]  = "Zaphod Development"
    };

    if (board_type < ARRAY_SIZE(names))
        return names[board_type];

    return names[0];
}

static void zaphod_common_machine_class_init(ObjectClass *oc,
                                    bool set_default, int board_type)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    /* Set description, init function, default CPU */

    mc->desc= g_strdup_printf("Zaphod - %s", board_type_name(board_type));
    mc->init= zaphod_board_init;
    if (set_default)
    {
        mc->alias= "zaphod";
        mc->is_default= 1;
    }

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
#if defined(CONFIG_ZAPHOD_HAS_IOCORE)
    /* Allow QEmu's 'serial0' window for input/output */
    mc->no_serial= 0;
#else
    mc->no_serial= 1;
#endif
    mc->no_sdcard= 1;
}

static void zaphod_pb_machine_class_init(ObjectClass *oc, void *data)
{
    ZaphodMachineClass *zmc= ZAPHOD_MACHINE_CLASS(oc);

    zmc->board_type= ZAPHOD_BOARD_TYPE_ZAPHOD_1;

    zaphod_common_machine_class_init(oc, false, zmc->board_type);
}

static void zaphod_gs_machine_class_init(ObjectClass *oc, void *data)
{
    ZaphodMachineClass *zmc= ZAPHOD_MACHINE_CLASS(oc);

    zmc->board_type= ZAPHOD_BOARD_TYPE_ZAPHOD_2;

    zaphod_common_machine_class_init(oc, false, zmc->board_type);
}

static void zaphod_dev_machine_class_init(ObjectClass *oc, void *data)
{
    ZaphodMachineClass *zmc= ZAPHOD_MACHINE_CLASS(oc);

    zmc->board_type= ZAPHOD_BOARD_TYPE_ZAPHOD_DEV;

    zaphod_common_machine_class_init(oc, true, zmc->board_type);
}


/* TODO: support board variants:
 * - "zaphod-pb" -- Phil Brown machine simulation
 * - "zaphod-gs" -- Grant Searle SBC
 * - "zaphod-dev" machine (includes development features/config)
 */
static const TypeInfo zaphod_machine_types[]= {
    {
        .name           = TYPE_ZAPHOD_MACHINE,
        .parent         = TYPE_MACHINE,
        .abstract       = true,
        .class_size     = sizeof(ZaphodMachineClass),
        //.class_init     = zaphod_machine_class_init,
        .instance_size  = sizeof(ZaphodMachineState),
        .instance_init  = zaphod_machine_state_init,
    }, {    /* Phil Brown emulator */
        .name= MACHINE_TYPE_NAME("zaphod-pb"),
        .parent= TYPE_ZAPHOD_MACHINE,
        .class_size     = sizeof(ZaphodMachineClass),
        .class_init= zaphod_pb_machine_class_init
    }, {    /* Grant Searle SBC sim */
        .name= MACHINE_TYPE_NAME("zaphod-gs"),
        .parent= TYPE_ZAPHOD_MACHINE,
        .class_size     = sizeof(ZaphodMachineClass),
        .class_init= zaphod_gs_machine_class_init
    }, {    /* Sample board for development/testing */
        .name= MACHINE_TYPE_NAME("zaphod-dev"),
        .parent= TYPE_ZAPHOD_MACHINE,
        .class_size     = sizeof(ZaphodMachineClass),
        .class_init= zaphod_dev_machine_class_init
    }
};

DEFINE_TYPES(zaphod_machine_types)
