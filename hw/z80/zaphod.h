/*
 * QEmu Zaphod sample board/machine family
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef HW_Z80_ZAPHOD_H
#define HW_Z80_ZAPHOD_H


#define ZAPHOD_DEBUG    1

#include "hw/boards.h"

/* TODO: Config-related defines - see also z80-softmmu.mak */


/* Z80_MAX_RAM_SIZE:
 * Address space for a Z80 ends at 64K (some emulations might use less)
 */
#if QEMU_VERSION_MAJOR < 3  /* "KiB" from qemu/units.h in QEmu v3+ */
#define KiB                 1024
#endif
#define Z80_MAX_RAM_SIZE    (64 * KiB)


#ifndef ZAPHOD_HAS_MACHINE_SELECTION
typedef MachineClass ZaphodMachineClass;
#else
typedef struct {
    /*< private >*/
    MachineClass parent;

    /*< public >*/
    /* TODO: board configuration here */
} ZaphodMachineClass;
#endif

typedef struct {
    /*< private >*/
    MachineState parent;

    /*< public >*/
    /* TODO: device state here */
} ZaphodMachineState;


#define TYPE_ZAPHOD_MACHINE \
    MACHINE_TYPE_NAME("zaphod")
#define ZAPHOD_MACHINE(obj) \
    OBJECT_CHECK(ZaphodMachineState, (obj), TYPE_ZAPHOD_MACHINE)
#define ZAPHOD_MACHINE_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodMachineClass, (obj), TYPE_ZAPHOD_MACHINE)


#endif  /* HW_Z80_ZAPHOD_H */
