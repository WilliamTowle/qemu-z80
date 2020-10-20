/*
 * QEmu Zaphod sample board
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "zaphod.h"

#include "qemu/error-report.h"
#include "hw/boards.h"


#define DPRINTF(fmt, ...) \
    do { if (ZAPHOD_DEBUG) error_printf("zaphod: " fmt , ## __VA_ARGS__); } while(0)


/* zaphod_machine_init() */

static void zaphod_machine_init(MachineClass *mc)
{
#if 1	/* WmT - TRACE */
;DPRINTF("** Reached %s() - INCOMPLETE **\n", __func__);
;exit(1);
#endif
    /* TODO: set description, init function, default CPU */
}

DEFINE_MACHINE("zaphod", zaphod_machine_init)
