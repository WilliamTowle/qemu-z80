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


#if 0
static
int zaphod_inkey_can_receive(void *opaque)
{
    /* We can always store in zcs->inkey :) */
    /* Maybe implement a FIFO queue? */
    return 1;
}

static
void zaphod_inkey_receive(void *opaque, const uint8_t *buf, int len)
{
    ZaphodState    *zs= (ZaphodState *)opaque;

    zs->inkey= buf[0];
}

static void zaphod_sercon_putchar(CharDriverState *cd, const unsigned char ch)
{
    if (isprint(ch))
    {
        cd->chr_write(cd, &ch, 1);
    }
    else
    {   /* Render non-printable characters as corresponding hex.
         * TODO: pass all characters to device-specific handler to
         * interpret appropriately
         */
        uint8_t nyb_hi, nyb_lo;

        nyb_hi= (ch & 0xf0) >> 4;
        nyb_lo= ch & 0x0f;

        zaphod_sercon_putchar(cd, '[');
        nyb_hi+= (nyb_hi > 9)? 'A' - 10 : '0';
        zaphod_sercon_putchar(cd, nyb_hi);
        nyb_lo+= (nyb_lo > 9)? 'A' - 10 : '0';
        zaphod_sercon_putchar(cd, nyb_lo);
        zaphod_sercon_putchar(cd, ']');
        zaphod_sercon_putchar(cd, '*');
    }
}

static void zaphod_putchar(ZaphodState *zs, const unsigned char ch)
{
    zaphod_sercon_putchar(zs->sercon, ch);
    /* TODO: "passthrough" for any other supported devices */
}

static uint32_t zaphod_stdio_read(void *opaque, uint32_t addr)
{
    ZaphodState	*zs= (ZaphodState *)opaque;
    int		value;

	switch (addr)
    {
    case 0x00:		/* stdin */
        value= zs->inkey;
        zs->inkey= 0;
        return value;
    default:
;DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
        return 0x00;
    }
}

static void zaphod_stdio_write(void *opaque, uint32_t addr, uint32_t val)
{
    ZaphodState	*zs= (ZaphodState *)opaque;

    switch (addr)
    {
    case 0x01:        /* stdout */
        zaphod_putchar(zs, val);
        break;
    default:
;DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, val);
        break;
    }
}

/* Callbacks for stdin/stdout (port 0x00, 0x01) */
static const MemoryRegionPortio zaphod_stdio_portlist[] = {
    { 0x00, 1, 1, .read = zaphod_stdio_read },
    { 0x01, 1, 1, .write = zaphod_stdio_write, },
    PORTIO_END_OF_LIST(),
};

static void zaphod_init_sercon(ZaphodState *zs, CharDriverState* sercon)
{
    if ((zs->sercon= sercon) != NULL)
    {
        zs->ports= g_new(PortioList, 1);

        portio_list_init(zs->ports, OBJECT(zs), zaphod_stdio_portlist, zs, "zaphod.stdio");
        portio_list_add(zs->ports, get_system_io(), 0x00);

        qemu_chr_add_handlers(zs->sercon,
                zaphod_inkey_can_receive, zaphod_inkey_receive,
                NULL, zs);
    }
}
#endif

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


static void zaphod_init_common(ZaphodState *zs, QEMUMachineInitArgs *args)
{
    const char *cpu_model = args->cpu_model;
    const char *kernel_filename = args->kernel_filename;

    zs->cpu= zaphod_init_cpu(cpu_model);
    cpu_reset(ENV_GET_CPU(zs->cpu));

    zs->ram= zaphod_init_ram();

    zaphod_load_kernel(kernel_filename);

    /* TODO
     * Board-specific init, with:
     * - basic "stdio" I/O mechanism, with 'inkey' state variable
     * - hardware emulation and machine "features" bitmap
     */
#ifdef ZAPHOD_HAS_SERCON
    zs->sercon= ZAPHOD_SERCON(zaphod_sercon_new());
#endif
}

/* Development machine init
 * Implemented for testing purposes - no fixed features list
 */

static void zaphod_init_dev_machine(QEMUMachineInitArgs *args)
{
    ZaphodState *zs= g_new(ZaphodState, 1);
    /* TODO: "features" support required to distinguish this board */
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
    ZaphodState *zs= g_new(ZaphodState, 1);
    /* TODO: "features" support required to distinguish this board */
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
