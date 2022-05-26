/*
 * QEmu Zaphod sample board/machine family
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef HW_Z80_ZAPHOD_H
#define HW_Z80_ZAPHOD_H


#define ZAPHOD_DEBUG    1

//#include "hw/boards.h"


/* TODO: Config-related defines - see also z80-softmmu.mak */


/* Z80_MAX_RAM_SIZE:
 * Address space for a Z80 ends at 64K (some emulations might use less)
 */
#if QEMU_VERSION_MAJOR < 3  /* "KiB" from qemu/units.h in QEmu v3+ */
#define KiB                 1024
#endif
#define Z80_MAX_RAM_SIZE    (64 * KiB)


/* TODO: ZaphodMachineClass, ZaphodMachineState types here */

#endif  /* HW_Z80_ZAPHOD_H */
