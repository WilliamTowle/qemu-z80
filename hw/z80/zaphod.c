/*
 * QEmu Zaphod machine family
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "zaphod.h"         /* for config */
#include "qemu-common.h"
#include "qemu/error-report.h"

#include "hw/boards.h"
#include "hw/loader.h"
#include "sysemu/sysemu.h"
#include "sysemu/char.h"

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


void zaphod_putchar(ZaphodState *zs, const unsigned char ch)
{
#ifdef ZAPHOD_HAS_SERCON
    if (zs->sercon)
        zaphod_sercon_putchar(zs->sercon, ch);
#endif
#ifdef ZAPHOD_HAS_SCREEN
    if (zs->screen)
        zaphod_screen_putchar(zs->screen, ch);
#endif
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


static void zaphod_init_common(ZaphodState *zs, QEMUMachineInitArgs *args)
{
    const char *cpu_model = args->cpu_model;
    const char *kernel_filename = args->kernel_filename;

    zs->cpu= zaphod_init_cpu(cpu_model);
    cpu_reset(ENV_GET_CPU(zs->cpu));

    zs->ram= zaphod_init_ram();

    zs->inkey= 0;

    zaphod_load_kernel(kernel_filename);

#ifdef ZAPHOD_HAS_SERCON
    zs->sercon= zaphod_new_sercon(zs, serial_hds[0]);
#endif
#ifdef ZAPHOD_HAS_MC6850
    zs->mc6850= zaphod_new_mc6850(zs);
#endif
#ifdef ZAPHOD_HAS_SCREEN
    zs->screen= zaphod_new_screen(zs);
#endif
}


void zaphod_set_inkey(void *opaque, uint8_t val, bool is_data)
{
    ZaphodState *zs= (ZaphodState *)opaque;
    zs->inkey= val;
#ifdef ZAPHOD_HAS_SCREEN
;if (is_data) DPRINTF("INFO: %s() raising interrupt %s (zs=%p, new char 0x%02x)...\n", __func__, is_data?"Y":"N", zs, val);
    if (is_data && zs->screen->rxint_irq)
        qemu_irq_raise(*zs->screen->rxint_irq);
#endif
}

uint8_t zaphod_get_inkey(void *opaque, bool read_and_clear)
{
    ZaphodState *zs= (ZaphodState *)opaque;
    uint8_t val= zs->inkey;

    if (read_and_clear) zs->inkey= 0;
#ifdef ZAPHOD_HAS_SCREEN
    if (zs->screen->rxint_irq)
        qemu_irq_lower(*zs->screen->rxint_irq);
#endif

  return val;
}

#ifdef ZAPHOD_HAS_SCREEN
void zaphod_interrupt(void *opaque, int source, int level)
{
    ZaphodState  *zs= (ZaphodState *)opaque;
    CPUState *cs = CPU(z80_env_get_cpu(zs->cpu));
    cpu_interrupt(cs, CPU_INTERRUPT_HARD);
}
#endif

static void zaphod_add_feature(ZaphodState *zs, zaphod_feature_t n)
{
    switch(n)
    {
#ifdef ZAPHOD_HAS_SCREEN
    case ZAPHOD_SIMPLE_SCREEN:
#endif
#ifdef ZAPHOD_HAS_MC6850
    case ZAPHOD_FEATURE_MC6850:
#endif
        zs->features|= 1 << n;
        break;
    default:
        DPRINTF("%s(): unsupported/missing feature with n=%d requested\n", __func__, n);
    }
}

int zaphod_has_feature(ZaphodState *zs, zaphod_feature_t n)
{
    return zs->features & (1 << n);
}

/* Development machine init
 * Implemented for testing purposes - no fixed features list
 */

static void zaphod_init_dev_machine(QEMUMachineInitArgs *args)
{
    ZaphodState *zs= g_new0(ZaphodState, 1);
    zaphod_add_feature(zs, ZAPHOD_FEATURE_MC6850);
    zaphod_init_common(zs, args);
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

static void zaphod_init_pb_machine(QEMUMachineInitArgs *args)
{
    ZaphodState *zs= g_new0(ZaphodState, 1);
    zaphod_add_feature(zs, ZAPHOD_SIMPLE_SCREEN);
    zaphod_init_common(zs, args);
}

static QEMUMachine zaphod_pb_machine= {
    .name=	"zaphod-pb",
    .alias=	"zaphod",
    .desc=	"Zaphod 1 (Phil Brown Emulator)",
    .init=	zaphod_init_pb_machine,

    .max_cpus= 1,
    .no_parallel= 1
};


/* "Grant Searle SBC" init
 * Features:
 *  Input: 'stdio' emulation, plus mc6850 emulation (with IRQ)
 *  Output: screen enabled, with extended control codes
 */

static void zaphod_init_gs_machine(ram_addr_t ram_size,
                     const char *boot_device,
                     const char *kernel_filename, const char *kernel_cmdline,
                     const char *initrd_filename, const char *cpu_model)
{
    ZaphodState *zs= g_new0(ZaphodState, 1);
    zaphod_add_feature(zs, ZAPHOD_FEATURE_MC6850);
    zaphod_init_common(zs, kernel_filename, cpu_model);
}

static QEMUMachine zaphod_gs_machine= {
    .name=	"zaphod-gs",
    .desc=	"Zaphod 2 (Grant Searle SBC)",
    .init=	zaphod_init_gs_machine,

    .max_cpus= 1,
    .no_parallel= 1
};


static void zaphod_machine_init(void)
{
	qemu_register_machine(&zaphod_dev_machine);
	qemu_register_machine(&zaphod_pb_machine);
	qemu_register_machine(&zaphod_gs_machine);
}

machine_init(zaphod_machine_init);
