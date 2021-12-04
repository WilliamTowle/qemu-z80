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
#include "qemu-char.h"
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

static void zaphod_sercon_putchar(ZaphodState *zs, const unsigned char ch)
{
    if (isprint(ch))
    {
        zs->sercon->chr_write(zs->sercon, &ch, 1);
    }
    else
    {   /* Render non-printable characters as corresponding hex.
         * TODO: pass all characters to device-specific handler to
         * interpret appropriately
         */
        uint8_t nyb_hi, nyb_lo;

        nyb_hi= (ch & 0xf0) >> 4;
        nyb_lo= ch & 0x0f;

        zaphod_sercon_putchar(zs, '[');
        nyb_hi+= (nyb_hi > 9)? 'A' - 10 : '0';
        zaphod_sercon_putchar(zs, nyb_hi);
        nyb_lo+= (nyb_lo > 9)? 'A' - 10 : '0';
        zaphod_sercon_putchar(zs, nyb_lo);
        zaphod_sercon_putchar(zs, ']');
        zaphod_sercon_putchar(zs, '*');
    }
}

static void zaphod_putchar(ZaphodState *zs, const unsigned char ch)
{
    zaphod_sercon_putchar(zs, ch);
    /* TODO: passthrough for any other supported devices */
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

        portio_list_init(zs->ports, zaphod_stdio_portlist, zs, "zaphod.stdio");
        portio_list_add(zs->ports, get_system_io(), 0x00);

        qemu_chr_add_handlers(zs->sercon,
                zaphod_inkey_can_receive, zaphod_inkey_receive,
                NULL, zs);
    }
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

static void zaphod_init_dev_machine(ram_addr_t ram_size,
                     const char *boot_device,
                     const char *kernel_filename, const char *kernel_cmdline,
                     const char *initrd_filename, const char *cpu_model)
{
    ZaphodState *zs= g_new(ZaphodState, 1);

    zs->cpu= zaphod_init_cpu(cpu_model);
    cpu_reset(zs->cpu);

    zs->ram= zaphod_init_ram();

    zaphod_load_kernel(kernel_filename);

    /* TODO
     * Board-specific init, with:
     * - basic "stdio" I/O mechanism, with 'inkey' state variable
     * - hardware emulation and machine "features" bitmap
     */
    zaphod_init_sercon(zs, serial_hds[0]);
}

static QEMUMachine zaphod_machine= {
    /* sample machine: Z80 CPU, 64KiB RAM, basic I/O */
    .name=	"zaphod-dev",
    .desc=	"Zaphod 0 (Test System)",
    .init=	zaphod_init_dev_machine,

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
