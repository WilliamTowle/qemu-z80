/*
 * QEmu Zaphod machine family
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "qemu-common.h"
#include "zaphod.h"         /* for config */

#include "qemu/error-report.h"
#include "hw/boards.h"
#include "hw/loader.h"


/* ZAPHOD_RAM_SIZE:
 * Ensure Zaphod boards have 64KiB RAM
 * Other system emulations might want more or less RAM
 */
#define ZAPHOD_RAM_SIZE         Z80_MAX_RAM_SIZE

#ifdef ZAPHOD_DEBUG
#define DPRINTF(fmt, ...) \
    do { error_printf( "zaphod: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

static CPUZ80State *zaphod_init_cpu(const char *cpu_model)
{
    CPUZ80State *cpu;

    /* QEmu v1 doesn't record a default CPU; ours is Z80 */
    if (!cpu_model)
        cpu_model= "z80";

    cpu= cpu_init(cpu_model);
    if (!cpu) {
        fprintf(stderr, "Unable to find '%s' CPU definition\n", cpu_model);
        abort();
    }

    return cpu;
}

static MemoryRegion *zaphod_init_ram(void)
{
    MemoryRegion *address_space_mem= get_system_memory();
    MemoryRegion *ram= g_new(MemoryRegion, 1);

    /* QEmu v1 bases ram_size on '-m megs' option. Override this */
    memory_region_init_ram(ram, NULL, "zaphod.ram", ZAPHOD_RAM_SIZE);
    /* TODO: for ROM we can memory_region_set_readonly() */
    memory_region_add_subregion(address_space_mem, 0, ram);

    return ram;
}

static void zaphod_load_kernel(const char *kernel_file)
{
    if (!kernel_file || !kernel_file[0])
    {
        hw_error("No code to run - missing 'kernel' argument\n");
    }
    else
    {
      int    kernel_size;
        kernel_size= get_image_size(kernel_file);
        if (kernel_size <= 0)
        {
            hw_error("Kernel with bad size specified - %s\n", (kernel_size == 0)? "file empty" : "file missing?");
        }
        load_image_targphys(kernel_file, 0, kernel_size);
#ifdef ZAPHOD_DEBUG
DPRINTF("INFO: %s(): Kernel size %d bytes\n", __func__, kernel_size);
#endif
    }
}

static void zaphod_init_dev_machine(QEMUMachineInitArgs *args)
{
    const char *cpu_model = args->cpu_model;
    const char *kernel_filename = args->kernel_filename;
    ZaphodState *zs= g_new(ZaphodState, 1);

    zs->cpu= zaphod_init_cpu(cpu_model);
    cpu_reset(ENV_GET_CPU(zs->cpu));

    zs->ram= zaphod_init_ram();

    zaphod_load_kernel(kernel_filename);

    /* TODO
     * Board-specific init, with:
     * - basic "stdio" I/O mechanism, with 'inkey' state variable
     * - hardware emulation and machine "features" bitmap
     */
}

static QEMUMachine zaphod_machine= {
    /* sample machine: Z80 CPU, 64KiB RAM, basic I/O */
    .name=	"zaphod-dev",
    .desc=	"Zaphod 0 (Test System)",
    .init=	zaphod_init_dev_machine,

    .max_cpus= 1,
    .no_parallel= 1,
    .is_default = 1
    /* all options for v1:
     *  .use_scsi= 0,
     *  .max_cpus= 1,
     *  .no_serial= 0,
     *  .no_parallel= 1,
     *  .no_vga= 1,
     *  .no_floppy= 1,
     *  .no_cdrom= 1,
     *  .no_sdcard= 1,
     *  .is_default = 1
     */
};


static void zaphod_machine_init(void)
{
;DPRINTF("DEBUG: Enter %s() { calling qemu_register_machine() }\n", __func__);

	qemu_register_machine(&zaphod_machine);

;DPRINTF("DEBUG: Exit %s()\n", __func__);
}

machine_init(zaphod_machine_init);
