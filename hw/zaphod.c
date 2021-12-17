/*
 * QEmu Zaphod boards
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2021, under GPL...]
 */

#include "zaphod.h"         /* for config */
#include "qemu-common.h"
#include "qemu-error.h"

#include "boards.h"
#include "hw/loader.h"
#include "sysemu.h"         /* for serial_hds[] */

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

static CPUState *zaphod_init_cpu(const char *cpu_model)
{
    CPUState *cpu;

    /* QEmu v1 doesn't record a default CPU; ours is Z80 */
    if (!cpu_model)
        cpu_model= "z80";

    cpu= cpu_init(cpu_model);
    if (!cpu) {
        hw_error("Unable to find '%s' CPU definition\n", cpu_model);
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

static void zaphod_load_kernel(const char *filename)
{
    if (!filename || !filename[0])
    {
        hw_error("%s(): No code to run\n", __func__);
    }
    else
    {
      int    kernel_size;
        kernel_size= get_image_size(filename);
        if (kernel_size <= 0)
        {
            hw_error("%s(): Kernel with bad size specified - %s\n", __func__, (kernel_size == 0)? "file empty" : "file missing?");
        }
        load_image_targphys(filename, 0, kernel_size);
#ifdef ZAPHOD_DEBUG
DPRINTF("INFO: %s(): Kernel size %d bytes\n", __func__, kernel_size);
#endif
    }
}

static void zaphod_init_common(ZaphodState *zs, const char *kernel_filename, const char *cpu_model)
{
    zs->cpu= zaphod_init_cpu(cpu_model);
    cpu_reset(zs->cpu);

    zs->ram= zaphod_init_ram();

    zaphod_load_kernel(kernel_filename);

    /* TODO
     * Board-specific init, with:
     * - basic "stdio" I/O mechanism, with 'inkey' state variable
     */
#ifdef ZAPHOD_HAS_SERCON
    zs->sercon= zaphod_new_sercon(zs, serial_hds[0]);
#endif
#ifdef ZAPHOD_HAS_SCREEN
    zs->screen= zaphod_new_screen();
#endif
}

static void zaphod_add_feature(ZaphodState *zs, zaphod_feature_t n)
{
    zs->features|= 1 << n;
}

int zaphod_has_feature(ZaphodState *zs, zaphod_feature_t n)
{
    return zs->features & ( 1 << n);
}


/* Development machine init
 * Implemented for testing purposes - no fixed features list
 */

static void zaphod_init_dev_machine(ram_addr_t ram_size,
                     const char *boot_device,
                     const char *kernel_filename, const char *kernel_cmdline,
                     const char *initrd_filename, const char *cpu_model)
{
    ZaphodState *zs= g_new(ZaphodState, 1);
    /* No board-specific "features" at this time */
    zaphod_init_common(zs, kernel_filename, cpu_model);
}

static QEMUMachine zaphod_dev_machine= {
    .name=	"zaphod-dev",
    .desc=	"Zaphod 0 (Test System)",
    .init=	zaphod_init_dev_machine,

    .max_cpus= 1,
    .no_parallel= 1
};


/* "Phil Brown Emulator" init
 * Features:
 *  Input: 'stdio' emulation
 *  Output: screen enabled, with simple control codes
 */

static void zaphod_init_pb_machine(ram_addr_t ram_size,
                     const char *boot_device,
                     const char *kernel_filename, const char *kernel_cmdline,
                     const char *initrd_filename, const char *cpu_model)
{
    ZaphodState *zs= g_new(ZaphodState, 1);
    zaphod_add_feature(zs, ZAPHOD_SIMPLE_SCREEN);
    zaphod_init_common(zs, kernel_filename, cpu_model);
}

static QEMUMachine zaphod_pb_machine= {
    .name=	"zaphod-pb",
    .alias=	"zaphod",
    .desc=	"Zaphod 1 (Phil Brown Emulator)",
    .init=	zaphod_init_pb_machine,

    .max_cpus= 1,
    .no_parallel= 1
};


/* TODO: "Grant Searle SBC" init to feature:
 *  Input: 'stdio' emulation, plus mc6850 emulation (with IRQ)
 *  Output: screen enabled, with extended control codes
 *  // static void zaphod_init_pb_machine([...])
 */



static void zaphod_machine_init(void)
{
	qemu_register_machine(&zaphod_dev_machine);
	qemu_register_machine(&zaphod_pb_machine);
}

machine_init(zaphod_machine_init);
