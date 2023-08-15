/*
 * QEmu Zaphod sample board
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


/* Machine state initialisation: simple machine */

static void zaphod_sample_board_init(MachineState *ms)
{
    const char *kernel_filename = ms->kernel_filename;
    Z80CPU *cpu;
    MemoryRegion *address_space_mem;
    MemoryRegion *ram;
    CPUState *cs;


    /* Init CPU/set reset callback */
;DPRINTF("DEBUG: ms has cpu_type: %s (is '%s')\n", ms->cpu_type?"y":"n", ms->cpu_type);

    cs= cpu_create(ms->cpu_type);
    cpu= Z80_CPU(cs);

    qemu_register_reset(main_cpu_reset, cpu);

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
    {   /* QEmu's serial console exists */
#if 1   /* WmT - TRACE */
;DPRINTF("TODO: non-NULL serial0 is available - do UART init!\n");
#endif
    }


    /* Populate RAM */

    zaphod_load_kernel(kernel_filename);
}


/* Machine class initialisation: zaphod_machine_class_init() */

static void zaphod_machine_class_init(MachineClass *mc)
{
    /* Set description, init function, default CPU */

    mc->desc= "Zaphod sample board";
    mc->init= zaphod_sample_board_init;
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

DEFINE_MACHINE("zaphod", zaphod_machine_class_init)
