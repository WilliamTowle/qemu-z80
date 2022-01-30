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
        exit(1);
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

static void zaphod_init_machine(QEMUMachineInitArgs *args)
{
    const char *cpu_model = args->cpu_model;
    ZaphodState *zs= g_new(ZaphodState, 1);

    zs->cpu= zaphod_init_cpu(cpu_model);
    cpu_reset(ENV_GET_CPU(zs->cpu));

    zs->ram= zaphod_init_ram();

#if 1   /* TRACE */
;DPRINTF("DEBUG: %s() INCOMPLETE - will execute 'nop's from empty RAM\n", __func__);
#endif
    /* TODO
     * Board-specific init, with:
     * - state variable with CPU and RAM handle (passed to callbacks)
     * - separate functions for initialising state
     * - loading of "kernel" image if filename specified
     * - basic "inkey" I/O mechanism
     */
}

static QEMUMachine zaphod_machine= {
    /* sample machine: Z80 CPU, 64KiB RAM, basic I/O */
    .name=	"zaphod",
    .desc=	"Zaphod 1",
    .init=	zaphod_init_machine,

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