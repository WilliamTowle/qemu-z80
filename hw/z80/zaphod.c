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
#define ZAPHOD_RAM_SIZE         Z80_MAX_RAM_SIZE


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

static void zaphod_generic_board_init(MachineState *ms)
{
    const char *kernel_filename = ms->kernel_filename;
    Z80CPU *cpu = NULL;
    MemoryRegion *address_space_mem;
    MemoryRegion *ram;

    /* Init CPU/set reset callback */
    cpu = Z80_CPU(cpu_create(ms->cpu_type));
    qemu_register_reset(main_cpu_reset, cpu);

    /* Init RAM */
    address_space_mem= get_system_memory();
    ram= g_new(MemoryRegion, 1);

    /* Override any '-m <memsize>' option.
     * NB: '-m <n>K' is valid, we should permit <= 64K
     */
    memory_region_init_ram(ram, NULL, "zaphod.ram",
                            ZAPHOD_RAM_SIZE, &error_fatal);
    /* Leaving the entire 64KiB memory space writable supports
     * self-modifying test code. Call memory_region_set_readonly()
     * for ROM regions,
     */
    memory_region_add_subregion(address_space_mem, 0x0000, ram);


    /* Initialise ports/devices */

    if (serial_hds[0]) {
;DPRINTF("TODO: non-NULL serial0 is available - do UART init!\n");
    }


    /* Populate RAM */

    zaphod_load_kernel(kernel_filename);
}


/* zaphod_machine_init() */

static void zaphod_generic_instance_init(Object *obj)
{
;DPRINTF("DEBUG: Reached %s() [empty]\n", __func__);
}

static void zaphod_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    //ZaphodMachineClass *zmc = ZAPHOD_MACHINE_CLASS(oc);
;DPRINTF("DEBUG: Reached %s()\n", __func__);

    /* Set description, CPU, RAM board options */

    mc->desc= "Zaphod development board";
    //mc->default_cpu_type= Z80_CPU_TYPE_NAME("z80");
    mc->default_cpu_type = TARGET_DEFAULT_CPU_TYPE;
    mc->default_cpus = 1;
    mc->min_cpus = mc->default_cpus;
    mc->max_cpus = mc->default_cpus;
    //mc->default_ram_size = ZAPHOD_RAM_SIZE;

    mc->is_default= 1;
    mc->no_floppy= 1;
    mc->no_cdrom= 1;
    mc->no_parallel= 1;
    mc->no_serial = 1;      /* TODO: set to '0' where serial available */
    mc->no_sdcard = 1;

    mc->init= zaphod_generic_board_init;
}

static const TypeInfo zaphod_machine_type_generic = {
    /* Implement a "generic" board until sufficiently diverse
     * feature support is available. Use of TYPE_ZAPHOD_MACHINE
     * means this board name is "zaphod"
     */
    .name           = TYPE_ZAPHOD_MACHINE,
    .parent         = TYPE_MACHINE,
    .abstract       = false,
    .class_init     = zaphod_class_init,
    .class_size     = sizeof(ZaphodMachineClass),
    .instance_init  = zaphod_generic_instance_init,
    .instance_size  = sizeof(ZaphodMachineState)
};

static void zaphod_machine_register_types(void)
{
    type_register_static(&zaphod_machine_type_generic);
}

type_init(zaphod_machine_register_types)
