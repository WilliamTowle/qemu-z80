/*
 * QEmu Zaphod machine family
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "zaphod.h"
#include "cpu.h"

#include "qemu/config-file.h"
#include "qemu/option.h"
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

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static QemuOptsList zaphod_io_opts = {
    .name = "zaphod-io-config",
    .implied_opt_name = "mode",
    .head = QTAILQ_HEAD_INITIALIZER(zaphod_io_opts.head),
    .desc = {
        {
            .name = "mode",
            .type = QEMU_OPT_STRING,
            .help = "Specify 'stdio' or 'acia' configuration options",
        },
        {
            .name = "chardev",
            .type = QEMU_OPT_STRING,
            .help = "Character device ID",
        },
        { /* end of list */ }
    }
};

void zaphod_interrupt_request(void *opaque, int source, int level)
{   /* ACIA has received input */
    ZaphodMachineState  *zms= (ZaphodMachineState *)opaque;
    //CPUState            *cs= CPU(z80_env_get_cpu(zms->cpu));
    CPUState            *cs= CPU(zms->cpu);

    if (level)
    {
        cpu_interrupt(cs, CPU_INTERRUPT_HARD);
    }
    else
    {
        cpu_reset_interrupt(cs, CPU_INTERRUPT_HARD);
    }
}


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


/* Per-board feature configuration */

static bool zaphod_board_has_acia(int board_type)
{
    switch (board_type)
    {
    case ZAPHOD_BOARD_TYPE_ZAPHOD_2:
    case ZAPHOD_BOARD_TYPE_ZAPHOD_DEV:
        return true;
    default:
        return false;
    }
}

static bool zaphod_board_has_stdio(int board_type)
{
    switch (board_type)
    {
    case ZAPHOD_BOARD_TYPE_ZAPHOD_1:
    case ZAPHOD_BOARD_TYPE_ZAPHOD_DEV:
        return true;
    case ZAPHOD_BOARD_TYPE_ZAPHOD_2:
    default:
        return false;
    }
}

/* Initialise UART object */
static void zaphod_uart_init(ZaphodUARTState *zus, Chardev *chr_fallback, const char *label)
{
    if (chr_fallback == NULL)
    {
#if QEMU_VERSION_MAJOR < 5
        chr_fallback= qemu_chr_new(label, "vc:" TOSTRING(ZAPHOD_TEXT_COLS) "Cx" TOSTRING(ZAPHOD_TEXT_ROWS) "C");
#else
        chr_fallback= qemu_chr_new(label, "vc:" TOSTRING(ZAPHOD_TEXT_COLS) "Cx" TOSTRING(ZAPHOD_TEXT_ROWS) "C", NULL);
#endif
    }

    qdev_prop_set_chr(DEVICE(zus), "chardev", chr_fallback);
}

static int zaphod_peripherals_init(void *opaque, QemuOpts *opts, Error **errp)
{
    ZaphodMachineState *zms= (ZaphodMachineState *)opaque;
    const char *mode, *chardev;

    mode= qemu_opt_get(opts, "mode");
    chardev= qemu_opt_get(opts, "chardev");

    if (mode)
    {
        Chardev *cd= NULL;

        if (chardev)
        {
            cd= qemu_chr_find(chardev);
            if (!cd)
            {
                error_printf("zaphod: chardev '%s' not found\n", chardev);
                abort();
            }
        }

        if (strcmp(mode, "stdio") == 0)
        {   /* stdio devices requested */
            qdev_prop_set_bit(DEVICE(zms->iocore), "has-stdio", true);
            /* TODO: refactor UART new/realize into IOCore */
            zms->uart_stdio= ZAPHOD_UART(object_new(TYPE_ZAPHOD_UART));
            qdev_prop_set_chr(DEVICE(zms->uart_stdio), "chardev", cd);
        }
        if (strcmp(mode, "acia") == 0)
        {   /* ACIA devices requested */
            qdev_prop_set_bit(DEVICE(zms->iocore), "has-acia", true);
            /* TODO: refactor UART new/realize into IOCore */
            zms->uart_acia= ZAPHOD_UART(object_new(TYPE_ZAPHOD_UART));
            qdev_prop_set_chr(DEVICE(zms->uart_acia), "chardev", cd);
        }
    }

    return 0;
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
static void zaphod_iocore_init(ZaphodMachineState *zms)
{
    ZaphodMachineClass *zmc = ZAPHOD_MACHINE_GET_CLASS(zms);
    int uart_count;
    QemuOptsList *olist;

    /* create object and set internal board reference */
    zms->iocore= ZAPHOD_IOCORE(object_new(TYPE_ZAPHOD_IOCORE));
    zms->iocore->board= zms;

    /* Prepare UARTs */

    qdev_prop_set_bit(DEVICE(zms->iocore), "has-stdio",
                        zaphod_board_has_stdio(zmc->board_type));
    qdev_prop_set_bit(DEVICE(zms->iocore), "has-acia",
                        zaphod_board_has_acia(zmc->board_type));


    /* Enable stdio and ACIA UARTs if relevant opts are used */
    olist= qemu_find_opts("zaphod-io-config");

    qemu_opts_foreach(olist, zaphod_peripherals_init, zms, NULL);

    /* Ensure UART/s are configured and realized */
    uart_count= 0;
    /* FIXME: '-M zaphod-pb -zaphod-io stdio' creates a UART with nothing connected :( */
;DPRINTF("*** DEBUG: uart %p/by-property %s/chardev connected? %s ***\n", zms->uart_stdio, object_property_get_bool(OBJECT(zms->iocore), "has-stdio", NULL)?"y":"n", zms->uart_stdio?(qemu_chr_fe_backend_connected(&zms->uart_stdio->chr)?"y":"n"):"N/A");
    if (object_property_get_bool(OBJECT(zms->iocore), "has-stdio", NULL))
    {   /* Create/assign stdio UART? */
        if (!zms->uart_stdio)
            zms->uart_stdio= ZAPHOD_UART(object_new(TYPE_ZAPHOD_UART));
        if (!qemu_chr_fe_backend_connected(&zms->uart_stdio->chr))
        {
            zaphod_uart_init(zms->uart_stdio,
                        serial_hd(uart_count), "zaphod.uart-stdio");
        }
#if QEMU_VERSION_MAJOR < 5
        qdev_init_nofail(DEVICE(zms->uart_stdio));
#else
        qdev_realize(DEVICE(zms->uart_stdio), NULL, NULL);
#endif
        if (zms->uart_stdio /* ? is connected? */) uart_count++;
    }

    if (object_property_get_bool(OBJECT(zms->iocore), "has-acia", NULL))
    {   /* Create/assign ACIA UART? */
        if (!zms->uart_acia)
            zms->uart_acia= ZAPHOD_UART(object_new(TYPE_ZAPHOD_UART));
        if (!qemu_chr_fe_backend_connected(&zms->uart_acia->chr))
        {
            zaphod_uart_init(zms->uart_acia,
                        serial_hd(uart_count), "zaphod.uart-acia");
        }
#if QEMU_VERSION_MAJOR < 5
        qdev_init_nofail(DEVICE(zms->uart_acia));
#else
        qdev_realize(DEVICE(zms->uart_acia), NULL, NULL);
#endif
        if (zms->uart_acia /* ? is connected? */) uart_count++;
    }


    /* initialise screen */
    zaphod_screen_init(zaphod_iocore_get_screen(zms->iocore), zmc->board_type);
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

#ifdef CONFIG_ZAPHOD_HAS_IOCORE
    /* Initialise IOCore subsystem */
/* NB. we can get a serial0 and a serial1 with:
 *$ ./z80-softmmu/qemu-system-z80 -M zaphod-dev -chardev vc,id=vc0 -chardev vc,id=vc1 -serial chardev:vc0 -serial chardev:vc1 -kernel wills/system/zaphodtt.bin
 */
    zaphod_iocore_init(zms);
    /* early zaphod_iocore_init() leaves realize() to do */
#if QEMU_VERSION_MAJOR < 5
    qdev_init_nofail(DEVICE(zms->iocore));
#else
    qdev_realize(DEVICE(zms->iocore), NULL, NULL);
#endif
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
    mc->default_cpu_type= Z80_CPU_TYPE_NAME("z80");
    //mc->default_cpu_type= TARGET_DEFAULT_CPU_TYPE;
    mc->default_cpus= 1;
    mc->min_cpus= mc->default_cpus;
    mc->max_cpus= mc->default_cpus;
    mc->default_ram_size= ZAPHOD_RAM_SIZE;

    mc->no_floppy= 1;
    mc->no_cdrom= 1;
    mc->no_parallel= 1;
    mc->no_serial= 0;
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


static void zaphod_register_opts(void)
{
    qemu_add_opts(&zaphod_io_opts);
}

opts_init(zaphod_register_opts)
