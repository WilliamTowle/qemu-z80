/* Skeleton Z80 machine for QEmu */
/* Wm. Towle c. 2013-2020 */

#include "hw/zaphod.h"

#ifdef ZAPHOD_DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>

#include "sysemu.h"		/* bios_name and other externs */
#include "hw/boards.h"		/* QEmuMachine */
#include "hw/hw.h"
#include "hw/isa.h"
#include "hw/loader.h"


/* Machine configuration */
#define ZAPHOD_RAM_SIZE		ZAPHOD_MAX_RAMTOP

typedef struct {
	CPUState		*cpu;
	ram_addr_t		ram_handle;
} ZaphodState;


/* pic_info() and irq_info() are monitor functions for
 * hardware we don't have
 */

void pic_info(Monitor *mon)
{
}

void irq_info(Monitor *mon)
{
}

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
#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Enter %s() { initialising 'z80' CPU }\n", __func__);
#endif

	if (!cpu_model)
	{
	    cpu_model= "z80";
	}
	cpu= cpu_init(cpu_model);
	if (!cpu)
	{
	    hw_error("Unable to find CPU definition '%s'\n", cpu_model);
	}

#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Exit %s(): Initialising 'z80' CPU\n", __func__);
#endif
  return cpu;
}

static void zaphod_init(ram_addr_t ram_size,
                               const char *boot_device,
                               const char *kernel_filename,
                               const char *kernel_cmdline,
                               const char *initrd_filename,
                               const char *cpu_model)
{
  ZaphodState	*zs= qemu_mallocz(sizeof(ZaphodState));

#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Enter %s(): Initialise CPU and RAM...\n", __func__);
#endif

	/*  allocate CPU and attach some RAM  */
	zs->cpu= zaphod_new_cpu(cpu_model);
	zs->ram_handle= qemu_ram_alloc(ZAPHOD_RAM_SIZE);
	cpu_register_physical_memory(0, ZAPHOD_RAM_SIZE,
					zs->ram_handle | IO_MEM_RAM);

	if (!kernel_filename || !kernel_filename[0])
	{
	    hw_error("zaphod_init(): No code to run\n");
	}
	else
	{
	  int	kernel_size;
	    kernel_size= get_image_size(kernel_filename);
	    if (kernel_size == -1)
	    {
		hw_error("zaphod_init(): Unexpected kernel_size - is file missing?\n");
	    }
	    load_image_targphys(kernel_filename, 0, kernel_size);
#ifdef ZAPHOD_DEBUG
	    fprintf(stderr, "INFO: %s(): Kernel size %d bytes\n", __func__, kernel_size);
#endif
	}

#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Exit %s()\n", __func__);
#endif
}

static QEMUMachine zaphod_machine= {
	.name=	"zaphod",
	.desc=	"Zaphod 1",
	.init=	zaphod_init,
	.is_default = 1		/* needed? */
};

static void zaphod_machine_init(void)
{
#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Enter %s() { calling qemu_register_machine() }\n", __func__);
#endif

	qemu_register_machine(&zaphod_machine);

#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Exit %s()\n", __func__);
#endif
}

machine_init(zaphod_machine_init);
