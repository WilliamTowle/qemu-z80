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
#include "exec-memory.h"


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


static void zaphod_init(ram_addr_t ram_size,
                     const char *boot_device,
                     const char *kernel_filename, const char *kernel_cmdline,
                     const char *initrd_filename, const char *cpu_model)
{
    CPUState *env;
    MemoryRegion *address_space_mem= get_system_memory();
    MemoryRegion *ram= g_new(MemoryRegion, 1);

    /* QEmu v1 doesn't record a default CPU; ours is Z80 */
    if (!cpu_model)
        cpu_model= "z80";
    env= cpu_init(cpu_model);
    if (!env) {
        hw_error("Unable to find '%s' CPU definition\n", cpu_model);
    }

    cpu_reset(env);

    /* QEmu v1 bases ram_size on '-m megs' option. Override this */
    memory_region_init_ram(ram, NULL, "zaphod.ram", ZAPHOD_RAM_SIZE);
    /* Leaving the entire 64KiB memory space writable supports
     * self-modifying test code. Call memory_region_set_readonly()
     * for ROM regions,
     */
    memory_region_add_subregion(address_space_mem, 0, ram);

#if 1   /* TRACE */
;DPRINTF("INCOMPLETE - %s() BAILING\n", __func__);
;exit(1);
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
    .init=	zaphod_init,

#if 1
    .max_cpus= 1,
    .no_parallel= 1,
    .is_default = 1
#else   /* all options for v1 */
    .use_scsi= 0,
    .max_cpus= 1,
    .no_serial= 0,
    .no_parallel= 1,
    .no_vga= 1,
    .no_floppy= 1,
    .no_cdrom= 1,
    .no_sdcard= 1,
    .is_default = 1
#endif
};


static void zaphod_machine_init(void)
{
;DPRINTF("DEBUG: Enter %s() { calling qemu_register_machine() }\n", __func__);

	qemu_register_machine(&zaphod_machine);

;DPRINTF("DEBUG: Exit %s()\n", __func__);
}

machine_init(zaphod_machine_init);
