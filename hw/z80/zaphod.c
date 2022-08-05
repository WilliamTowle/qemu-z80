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


#define DPRINTF(fmt, ...) \
    do { if (ZAPHOD_DEBUG) error_printf("zaphod: " fmt , ## __VA_ARGS__); } while(0)


/* ZAPHOD_RAM_SIZE:
 * Ensure Zaphod boards have 64KiB RAM
 * Other system emulations might want more or less RAM
 */
#define ZAPHOD_RAM_SIZE     Z80_MAX_RAM_SIZE


void zaphod_interrupt_request(void *opaque, int source, int level)
{   /* ACIA has received input */
    ZaphodMachineState  *zms= (ZaphodMachineState *)opaque;
    //CPUState            *cs= CPU(z80_env_get_cpu(zms->cpu));
    CPUState            *cs= CPU(zms->cpu);

    cpu_interrupt(cs, CPU_INTERRUPT_HARD);
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


/* Machine state initialisation: simple machine */

static void zaphod_generic_board_init(MachineState *ms)
{
    ZaphodMachineState *zms = ZAPHOD_MACHINE(ms);
    const char *kernel_filename = ms->kernel_filename;
    //Z80CPU *cpu= NULL;
    MemoryRegion *address_space_mem;
    MemoryRegion *ram;
    CPUState *cs;

    /* Init CPU/set reset callback */
    zms->cpu = Z80_CPU(cpu_create(ms->cpu_type));
    qemu_register_reset(main_cpu_reset, zms->cpu);


    /* QEmu v5: reset has happened */
    cs= CPU(zms->cpu);
    //cpu_reset(cs);
    cpu_set_pc(cs, 0x0000);


    /* Init RAM */

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

#ifdef CONFIG_ZAPHOD_HAS_SCREEN
#ifdef ZAPHOD_HAS_MACHINE_SELECTION
    zms->screen= ZAPHOD_SCREEN(zaphod_screen_new());
#else
    if (object_property_get_bool(OBJECT(zmc), "has-simple-screen", &error_fatal))
        zaphod_screen_init();
#endif
#endif

    if (serial_hds[0])
    {   /* QEmu's serial console exists */
;DPRINTF("INFO: initialising UART0 device...\n");
        //zms->uart_stdio= ZAPHOD_UART(zaphod_uart_new());
        zms->uart_stdio= ZAPHOD_UART(zaphod_uart_new(serial_hds[0]));
;DPRINTF("INFO: UART0 device created OK, at %p\n", zms->uart_stdio);
    }

    if (serial_hds[1]) {
;DPRINTF("INFO: initialising UART1 device...\n");
        //zms->uart_stdio= ZAPHOD_UART(zaphod_uart_new());
        zms->uart_acia= ZAPHOD_UART(zaphod_uart_new(serial_hds[1]));
;DPRINTF("INFO: UART1 device created OK, at %p\n", zms->uart_acia);
    }


#ifdef CONFIG_ZAPHOD_HAS_IOCORE
    /* Initialise IOCore subsystem */
;DPRINTF("*** INFO: %s() - about to do iocore init... ***\n", __func__);
/* NB. we can get a serial0 and a serial1 with:
 *$ ./z80-softmmu/qemu-system-z80 -M zaphod-dev -chardev vc,id=vc0 -chardev vc,id=vc1 -serial chardev:vc0 -serial chardev:vc1 -kernel wills/system/zaphodtt.bin
 */
    //zms->iocore= ZAPHOD_IOCORE(zaphod_iocore_new(/* serial_hds[0] */));
    zms->iocore= ZAPHOD_IOCORE(zaphod_iocore_new(zms));
;DPRINTF("INFO: IOCORE device created OK, at %p\n", zms->iocore);
#endif


    /* Populate RAM */

    zaphod_load_kernel(kernel_filename);
}


static void zaphod_generic_instance_init(Object *obj)
{
;DPRINTF("DEBUG: Reached %s() [empty]\n", __func__);
}


/* Machine class initialisation: zaphod_class_init() */

static void zaphod_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    //ZaphodMachineClass *zmc = ZAPHOD_MACHINE_CLASS(oc);

    /* Set description, CPU, RAM board options */

    mc->desc= "Zaphod development board";
    mc->init= zaphod_generic_board_init;

    //mc->default_cpu_type= Z80_CPU_TYPE_NAME("z80");
    mc->default_cpu_type = TARGET_DEFAULT_CPU_TYPE;
    mc->default_cpus = 1;
    mc->min_cpus = mc->default_cpus;
    mc->max_cpus = mc->default_cpus;
    mc->default_ram_size = ZAPHOD_RAM_SIZE;

#ifndef ZAPHOD_HAS_MACHINE_SELECTION
    mc->is_default= 1;
#endif

    mc->no_floppy= 1;
    mc->no_cdrom= 1;
    mc->no_parallel= 1;
#if defined(CONFIG_ZAPHOD_HAS_IOCORE)
    /* forcibly enable a 'serial0' window for stdin input/output */
    mc->no_serial = 0;
#else
    mc->no_serial = 1;
#endif
    mc->no_sdcard = 1;
}

static const TypeInfo zaphod_machine_type_generic = {
    /* Implement a "generic" board until sufficiently diverse
     * feature support is available. Use of TYPE_ZAPHOD_MACHINE
     * means this board name is "zaphod"
     */
    .name           = TYPE_ZAPHOD_MACHINE,
    .parent         = TYPE_MACHINE,
#ifdef ZAPHOD_HAS_MACHINE_SELECTION
    .abstract       = true,
#else
    .abstract       = false,
#endif
    .class_init     = zaphod_class_init,
    .class_size     = sizeof(ZaphodMachineClass),
    .instance_init  = zaphod_generic_instance_init,
    .instance_size  = sizeof(ZaphodMachineState)
};

static void zaphod_machine_register_types(void)
{
    type_register_static(&zaphod_machine_type_generic);
}

static void zaphod_pb_options(MachineClass *mc)
{
    ZaphodMachineClass *zmc= ZAPHOD_MACHINE_CLASS(mc);

    zmc->has_acia= false;
    zmc->has_simple_screen= true;
}

static void zaphod_dev_options(MachineClass *mc)
{
    ZaphodMachineClass *zmc= ZAPHOD_MACHINE_CLASS(mc);

    zmc->has_acia= true;
    zmc->has_simple_screen= false;
}

type_init(zaphod_machine_register_types)
#ifdef ZAPHOD_HAS_MACHINE_SELECTION
/* TYPE_ZAPHOD_MACHINE is abstract; child machine types will encompass:
 * - "zaphod-pb" -- Phil Brown machine simulation
 * - "zaphod-gs" -- Grant Searle SBC
 * - "zaphod-dev" machine (includes development features)
 */
#define DEFINE_ZAPHOD_BOARD(board_id, desc_str, make_default) \
        static void zaphod_##board_id##_class_init(ObjectClass *oc, void *data) \
        { \
            MachineClass *mc= MACHINE_CLASS(oc); \
            zaphod_##board_id##_options(mc); \
            mc->desc= desc_str; \
            if (make_default) { \
                mc->is_default= 1; \
                mc->alias= "zaphod"; \
            } \
        } \
        static const TypeInfo zaphod_##board_id##_info= { \
            .name= MACHINE_TYPE_NAME("zaphod-" # board_id), \
            .parent= TYPE_ZAPHOD_MACHINE, \
            .class_init= zaphod_##board_id##_class_init, \
        }; \
        static void zaphod_##board_id##_init(void) \
        { \
            type_register_static(&zaphod_##board_id##_info); \
        } \
        type_init(zaphod_##board_id##_init);

#define DEFINE_ZAPHOD_MACHINE_OPTION(board_id, desc_str) \
        DEFINE_ZAPHOD_BOARD(board_id, desc_str, false)
#define DEFINE_ZAPHOD_MACHINE_DEFAULT(board_id, desc_str) \
        DEFINE_ZAPHOD_BOARD(board_id, desc_str, true)

DEFINE_ZAPHOD_MACHINE_OPTION(pb, "Zaphod 1 (Phil Brown emulator)")
//DEFINE_ZAPHOD_MACHINE_DEFAULT(gs, "Zaphod 2 (Grant Searle SBC)")
DEFINE_ZAPHOD_MACHINE_DEFAULT(dev, "Zaphod Development")
#endif
