/*
 * QEmu Zaphod sample board/machine family
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef HW_Z80_ZAPHOD_H
#define HW_Z80_ZAPHOD_H


#define ZAPHOD_DEBUG    1


/* Config-related defines - see also z80-softmmu.mak */

#define CONFIG_ZAPHOD_HAS_IOCORE
#define CONFIG_ZAPHOD_HAS_UART


#include "cpu.h"

#include "hw/boards.h"

#ifdef CONFIG_ZAPHOD_HAS_IOCORE
#include "zaphod_iocore.h"
#endif
#ifdef CONFIG_ZAPHOD_HAS_UART
#include "zaphod_uart.h"
#endif


/* Z80_MAX_RAM_SIZE:
 * Address space for a Z80 ends at 64K (some emulations might use less)
 */
#if QEMU_VERSION_MAJOR < 3  /* "KiB" from qemu/units.h in QEmu v3+ */
#define KiB                 1024
#endif
#define Z80_MAX_RAM_SIZE    (64 * KiB)


typedef struct {
    /*< private >*/
    MachineClass parent;

    /*< public >*/
    /* TODO: board configuration here */
} ZaphodMachineClass;

typedef struct {
    /*< private >*/
    MachineState parent;

    /*< public >*/
    Z80CPU              *cpu;
#ifdef CONFIG_ZAPHOD_HAS_IOCORE
    ZaphodIOCoreState        *iocore;
#endif
} ZaphodMachineState;


#define TYPE_ZAPHOD_MACHINE \
    MACHINE_TYPE_NAME("zaphod")
#define ZAPHOD_MACHINE(obj) \
    OBJECT_CHECK(ZaphodMachineState, (obj), TYPE_ZAPHOD_MACHINE)
#define ZAPHOD_MACHINE_CLASS(oc) \
    OBJECT_CLASS_CHECK(ZaphodMachineClass, oc, TYPE_ZAPHOD_MACHINE)
#define ZAPHOD_MACHINE_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodMachineClass, (obj), TYPE_ZAPHOD_MACHINE)


#endif  /* HW_Z80_ZAPHOD_H */
