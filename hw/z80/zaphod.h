/*
 * QEmu Zaphod machine family
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef HW_Z80_ZAPHOD_H
#define HW_Z80_ZAPHOD_H


#include "hw/boards.h"


#define ZAPHOD_DEBUG    1

/* TODO: Config-related defines - see also z80-softmmu.mak */
#if 1	/* not fully implemented */
#define ZAPHOD_HAS_MACHINE_SELECTION
#endif
#define CONFIG_ZAPHOD_HAS_IOCORE
#define CONFIG_ZAPHOD_HAS_UART
#define CONFIG_ZAPHOD_HAS_SCREEN

#ifdef CONFIG_ZAPHOD_HAS_IOCORE
#include "zaphod_iocore.h"
#endif
#ifdef CONFIG_ZAPHOD_HAS_UART
#include "zaphod_uart.h"
#endif
#ifdef CONFIG_ZAPHOD_HAS_SCREEN
#include "zaphod_screen.h"
#endif


/* Z80_MAX_RAM_SIZE:
 * Address space for a Z80 ends at 64K (some emulations might use less)
 */
#if !defined (KiB)          /* from qemu/units.h in QEmu v3+ */
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
#ifdef CONFIG_ZAPHOD_HAS_SCREEN
    bool has_simple_screen;
#endif
} ZaphodMachineClass;
#endif

struct ZaphodMachineState {
    /*< private >*/
    MachineState parent;

    /*< public >*/
#ifdef CONFIG_ZAPHOD_HAS_IOCORE
    ZaphodIOCoreState        *iocore;
#endif
#ifdef CONFIG_ZAPHOD_HAS_UART
    ZaphodUARTState     *uart_stdio;
    ZaphodUARTState     *uart_acia;
#endif

#ifdef CONFIG_ZAPHOD_HAS_SCREEN
    ZaphodScreenState    *screen;
#endif
};


#define TYPE_ZAPHOD_MACHINE \
    MACHINE_TYPE_NAME("zaphod")
#define ZAPHOD_MACHINE(obj) \
    OBJECT_CHECK(ZaphodMachineState, (obj), TYPE_ZAPHOD_MACHINE)
#define ZAPHOD_MACHINE_CLASS(oc) \
	OBJECT_CLASS_CHECK(ZaphodMachineClass, oc, TYPE_ZAPHOD_MACHINE)
#define ZAPHOD_MACHINE_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodMachineClass, (obj), TYPE_ZAPHOD_MACHINE)


#endif  /*  HW_Z80_ZAPHOD_H  */
