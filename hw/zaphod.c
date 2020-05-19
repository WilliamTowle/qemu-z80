/* "Zaphod" Z80 machine family for QEmu */
/* Wm. Towle c. 2013-2020 */

#include "hw/zaphod.h"

#ifdef ZAPHOD_DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>

#include "sysemu.h"		/* bios_name and other externs */

#include "hw/boards.h"		/* QEMUMachine */
#include "hw/hw.h"
#include "hw/isa.h"
#include "hw/loader.h"


/* RAM: maximum for a Z80 is 64K */
#define	RAM_SIZE	(64 * 1024)

/*  Zaphod-1 has a "medium-res two-colour 24x80 screen" and its
 *  console responds with "An IN from port 0 will respond with the
 *  ASCII code of the last key pressed, or 0 if none available. An
 *  OUT to port 1 will display the appropriate character on the
 *  console screen". The documented intention is to have the screen
 *  support VT52 escape sequences, but for temporary:
 *	ESC 0     - clears the console screen
 *	ESC 1 x y - moves the cursor position to x,y
 *	ESC 2     - clears to end of line from the current position
 */

#ifdef ZAPHOD_HAS_MACHINESPEC
#define	ZAPHOD_SPEC_DEVEL	0	/* misc config */
#define ZAPHOD_SPEC_ZAPHODPB	1	/* Phil Brown */
#endif

#ifdef ZAPHOD_HAS_IOPORTS
static uint32_t zaphod_io_read(void *opaque, uint32_t addr)
{
  ZaphodState	*zs= (ZaphodState *)opaque;
  int		value;
//#ifdef ZAPHOD_DEBUG
//	fprintf(stderr, "DEBUG: Enter %s()\n", __func__);
////;fprintf(stderr, "*** IN (%s) with inkey value %d ***\n", __func__, value);
//#endif
	switch (addr)
	{
	case 0x00:		/* Zaphod: STDIN */
		value= zs->console->cs_inkey;
		zs->console->cs_inkey= 0;
#ifdef ZAPHOD_HAS_RXINT_IRQ
		if (zs->console->rxint_irq)
		    qemu_irq_lower(*zs->console->rxint_irq);
#endif	/* ZAPHOD_HAS_RXINT_IRQ */
		return value;
#ifdef ZAPHOD_HAS_SERIALIO
	case 0x80:		/* read mc6850 PortStatus */
		value= (zs->console->cs_inkey)? 0x01 : 0; /* RxDataReady */
		value|= 0x02;		/* TxDataEmpty (always) */
		value|= 0x04;		/* DTD [Data Carrier Detect] */
		value|= 0x08;		/* CTS [Clear to Send] */
			/* FrameErr|Overrun|ParityErr|IrqReq not emulated */
#ifdef ZAPHOD_DEBUG
		//fprintf(stderr, "DEBUG: %s() read mc6850 PortStatus (port 0x%02x) -> status %02x\n", __func__, addr, value);
#endif
		return value;
	case 0x81:		/* read mc6850 RxData */
		value= zs->console->cs_inkey;
		zs->console->cs_inkey= 0;
#ifdef ZAPHOD_HAS_RXINT_IRQ
		if (zs->console->rxint_irq)
			qemu_irq_lower(*zs->console->rxint_irq);
#endif	/* ZAPHOD_HAS_RXINT_IRQ */
#ifdef ZAPHOD_DEBUG
		//fprintf(stderr, "DEBUG: %s() read mc6850 RXData (port 0x%02x) -> ch-value %d\n", __func__, addr, value);
#endif
		//return value;
		return value? value : 0xff;
#endif	/* ZAPHOD_HAS_SERIALIO */
	default:
#ifdef ZAPHOD_DEBUG
		//fprintf(stderr, "DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
#endif
		return 0x00;
	}
//#ifdef ZAPHOD_DEBUG
//	fprintf(stderr, "DEBUG: Exit %s()\n", __func__);
//#endif
}

static void zaphod_io_write(void *opaque, uint32_t addr, uint32_t value)
{
  ZaphodState	*zs= (ZaphodState *)opaque;

	switch (addr)
	{
	case 0x01:		/* Zaphod: STDOUT */
#ifdef ZAPHOD_HAS_CONSOLEGUI
	/* TODO: how to manage/display text in non-GUI mode? */
#ifdef ZAPHOD_DEBUG
;fprintf(stderr, "%s: character value: %d\n", __func__, value);
#endif
#ifdef ZAPHOD_CONSOLE_PSEUDO_CURPOS /* forcibly show capitals */
	    zaphod_consolegui_putchar(zs->console, toupper(value & 0xff));
#else
	    zaphod_consolegui_putchar(zs->console, value & 0xff);
#endif
#endif	/* ZAPHOD_HAS_CONSOLEGUI */
#ifdef ZAPHOD_HAS_SERIALIO
	    zaphod_serio_putchar(/*zs->console, */value & 0xff);
#endif
	    break;
#ifdef ZAPHOD_HAS_SERIALIO
	case 0x80:		/* write -> mc6850 PortControl */
		/* ignore since baud rate change etc. not emulated? */
#ifdef ZAPHOD_DEBUG
		fprintf(stderr, "DEBUG: %s() write m6850 PortControl (port 0x%02x) <- value %d\n", __func__, addr, value);
#endif
		break;
	case 0x81:		/* write -> mc6850 TxData */
#ifdef ZAPHOD_DEBUG
		//fprintf(stderr, "DEBUG: %s() write mc6850 TxData (port 0x%02x) -> ch-value=%d\n", __func__, addr, value);
#endif
#ifdef ZAPHOD_HAS_CONSOLEGUI
		zaphod_consolegui_putchar(zs->console, toupper(value & 0xff));
#endif
		zaphod_serio_putchar(/*zs->console, */value & 0xff);
		break;
#endif	/* ZAPHOD_HAS_SERIALIO */
	default:
#ifdef ZAPHOD_DEBUG
		fprintf(stderr, "DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
#endif
	    break;
	}
}
#endif	/* ZAPHOD_HAS_IOPORTS */


static CPUState* zaphod_new_cpu(const char *cpu_model)
{
  CPUState	*cpu;

	if (!cpu_model)
	{
	    cpu_model= "z80";
	}
	cpu= cpu_init(cpu_model);
	if (!cpu)
	{
	    hw_error("Unable to find CPU definition '%s'\n", cpu_model);
	}
	/* TODO: zx_spectrum.c has register_{savevm|reset}() and 
	 * main_cpu_reset() calls
	 */

  return cpu;
}

#ifdef ZAPHOD_HAS_RXINT_IRQ
static
void zaphod_interrupt(void *opaque, int source, int level)
{
#ifdef ZAPHOD_DEBUG
  ZaphodState	*zs= (ZaphodState *)opaque;
	fprintf(stderr, "DEBUG: %s() with source=%d, level=%d\n", __func__, source, level);
#endif
	cpu_interrupt(zs->cpu, CPU_INTERRUPT_HARD);
}
#endif	/* ZAPHOD_HAS_RXINT_IRQ */

static void zaphod_init_common(int zaphodspec,
				const char *kernel_filename,
				const char *cpu_model)
{
  ZaphodState	*zs= qemu_mallocz(sizeof(ZaphodState));
#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Enter %s()\n", __func__);
#endif

	/*  allocate CPU and attach some RAM  */
	zs->cpu= zaphod_new_cpu(cpu_model);
	zs->ram_handle= qemu_ram_alloc(RAM_SIZE);
	cpu_register_physical_memory(0, RAM_SIZE,
					zs->ram_handle | IO_MEM_RAM);

#ifdef ZAPHOD_HAS_BIOS
	/* Load ROM binary
	 *	{ TODO: fixed or variable ROM/RAM split point? Paging? }
	 *   At present, code that assumes ROM can be paged out of low
	 * addresses may "work" since read-only markers are not applied.
	 *   Since empty RAM is full of NOPs, not loading a ROM won't
	 * result in catastrophic failue (and `info registers` in the
	 * monitor lets us see the CPU running)
	 */
	if (kernel_filename && kernel_filename[0])
	{
	  int	kernel_size;
	    kernel_size= get_image_size(kernel_filename);
	    if (kernel_size <= 0)
	    {
		hw_error("Kernel with bad size specified - %s\n", (kernel_size == 0)?"file empty?":"file missing?");
	    }
	    load_image_targphys(kernel_filename, 0, kernel_size);
	}
#if 0
	/* Load a second image/program?
	 * (NB: cannot use '-initrd' without '-kernel')
	 */
	if (initrd_filename && initrd_filename[0])
	{
	  int	initrd_size;
	    //filepath= qemu_find_file(QEMU_FILE_TYPE_BIOS, initrd_filename);
	    initrd_size= get_image_size(initrd_filename);
	    /* make reference to 'initrd_size' to validate? */
	    //qemu_free(filepath);
	}
#endif
#endif	/*  ZAPHOD_HAS_BIOS  */

#ifdef ZAPHOD_HAS_IOPORTS
#ifdef ZAPHOD_DEBUG
//;fprintf(stderr, "INFO: %s: register io read/write funcs...\n", __func__);
#endif

#if !defined(ZAPHOD_HAS_RXINT_IRQ)
	if (1)
#else
	if (zaphodspec == ZAPHOD_SPEC_ZAPHODPB)
#endif
	{
	    /* No IRQs, or not required */
	    zs->console= zaphod_console_init(NULL);
	}
#if defined(ZAPHOD_HAS_RXINT_IRQ)
	else	/* we can support (and want) an IRQ on keypress */
	{
	    zs->irqs= qemu_allocate_irqs(zaphod_interrupt, zs, 1);
	    zs->console= zaphod_console_init(&zs->irqs[0]);
	}
#endif

#if defined(ZAPHOD_HAS_MACHINESPEC)
	if (zaphodspec == ZAPHOD_SPEC_ZAPHODPB)
#elif defined(ZAPHOD_HAS_CONSOLEGUI)
	if (1)
#endif
	{   /* Phil Brown's Zaphod reserves ports 0 and 1 for normal I/O:
	     *	to-addr 0x00 for input (ie. stdin)
	     *	to-addr 0x01 for output (ie. stdout)
	     */
	    register_ioport_read(0x00, 1,sizeof(char), zaphod_io_read, zs);
	    register_ioport_write(0x01, 1,sizeof(char), zaphod_io_write, zs);
	}

#if defined(ZAPHOD_HAS_MACHINESPEC)
	if (zaphodspec != ZAPHOD_SPEC_ZAPHODPB)
#elif defined(ZAPHOD_HAS_SERIALIO)
	if (1)
#endif
	{
	    /* ...Grant Searle's SimpleZ80 reserves 0x80 to 0xbf for serial
	     * I/O (although circuitry makes all even/odd ports equivalent)
	     */
	    /* 0x80: mc6850 PortStatus (read) and PortControl (write) */
	    register_ioport_read(0x80, 1,sizeof(char), zaphod_io_read, zs);
	    register_ioport_write(0x80, 1,sizeof(char), zaphod_io_write, zs);
	    /* 0x81: mc6850 RxData (read) and TxData (write) */
	    register_ioport_read(0x81, 1,sizeof(char), zaphod_io_read, zs);
	    register_ioport_write(0x81, 1,sizeof(char), zaphod_io_write, zs);
	}
#endif	/* ZAPHOD_HAS_IOPORTS */

#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Exit %s()\n", __func__);
#endif
}

#ifdef ZAPHOD_HAS_MACHINESPEC
static void zaphod_init_pb(ram_addr_t ram_size,
                               const char *boot_device,
                               const char *kernel_filename,
                               const char *kernel_cmdline,
                               const char *initrd_filename,
                               const char *cpu_model)
{
	zaphod_init_common(ZAPHOD_SPEC_ZAPHODPB,
			kernel_filename, cpu_model);
}

static QEMUMachine zaphodpb_machine= {
	.name=	"zaphodpb",
	.desc=	"Zaphod 1 (Phil Brown)",
	.init=	zaphod_init_pb
};
#endif	/* ZAPHOD_HAS_MACHINESPEC */

static void zaphod_init_dev(ram_addr_t ram_size,
                               const char *boot_device,
                               const char *kernel_filename,
                               const char *kernel_cmdline,
                               const char *initrd_filename,
                               const char *cpu_model)
{
	zaphod_init_common(ZAPHOD_SPEC_DEVEL,
			kernel_filename, cpu_model);
}

static QEMUMachine zaphod_machine= {
#ifdef ZAPHOD_HAS_MACHINESPEC
	.name=		"zaphoddev",
	.is_default=	1,
#else
	.name=		"zaphod",
#endif
	.desc=		"Zaphod Development Platform",
	.init=		zaphod_init_dev
};


static void zaphod_machine_init(void)
{
#ifdef ZAPHOD_HAS_MACHINESPEC
	qemu_register_machine(&zaphodpb_machine);
#endif	/* ZAPHOD_HAS_MACHINESPEC */
	qemu_register_machine(&zaphod_machine);
}

machine_init(zaphod_machine_init);
