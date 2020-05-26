/* Skeleton Z80 machine for QEmu */
/* Wm. Towle c. 2013-2018 */

#ifdef ZAPHOD_DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>

#include "sysemu.h"		/* bios_name and other externs */
#include "hw/boards.h"		/* QEmuMachine */
#include "hw/hw.h"
#include "hw/isa.h"
#include "hw/loader.h"

#include "hw/zaphod.h"

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
	zs->ram_handle= qemu_ram_alloc(NULL, "ram", ZAPHOD_RAM_SIZE);
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
