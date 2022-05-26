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


#define DPRINTF(fmt, ...) \
    do { if (ZAPHOD_DEBUG) error_printf("zaphod: " fmt , ## __VA_ARGS__); } while(0)


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

static void zaphod_sample_board_init(MachineState *ms)
{
    ram_addr_t ram_size = ms->ram_size;
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
    memory_region_init_ram(ram, NULL, "zaphod.ram",
                            ram_size, &error_fatal);
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

static void zaphod_machine_init(MachineClass *mc)
{
    mc->desc = "Zaphod sample board";
    mc->init = zaphod_sample_board_init;
    mc->is_default = 1;

    mc->default_cpu_type = TYPE_Z80_CPU;

    mc->no_floppy= 1;
    mc->no_cdrom= 1;
    mc->no_parallel= 1;
    mc->no_serial = 1;      /* TODO: set to '0' where serial available */
    mc->no_sdcard = 1;
}

DEFINE_MACHINE("zaphod", zaphod_machine_init)
