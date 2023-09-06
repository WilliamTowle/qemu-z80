/*
 * QEmu Zaphod board - IOCore support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#ifndef HW_Z80_ZAPHOD_IOCORE_H
#define HW_Z80_ZAPHOD_IOCORE_H

#include "zaphod.h"
#include "zaphod_screen.h"
#include "zaphod_uart.h"

#include "exec/ioport.h"
#include "hw/irq.h"
#if 1   /* IOCORE-KEYBIO */
#include "ui/input.h"
#endif

typedef struct ZaphodMachineState ZaphodMachineState;

typedef DeviceClass ZaphodIOCoreClass;

typedef struct {
    DeviceState     parent;

    ZaphodMachineState  *board;

    bool                has_stdio, has_acia;
    PortioList          *ioports_stdio;
    PortioList          *ioports_acia;
    qemu_irq            *irq_acia;
#ifdef CONFIG_ZAPHOD_HAS_UART
    ZaphodUARTState     *uart_stdio;
    ZaphodUARTState     *uart_acia;
#endif
    /* FIXME: stdio/ACIA/KEYBIO muxing configuration is missing */
#if 1   /* IOCORE-KEYBIO */
    QemuInputHandlerState *ihs;
    int             modifiers;
#endif
    /* TODO: correlate screen(s) to stdio/acia input */
    ZaphodScreenState   *screen;
} ZaphodIOCoreState;


#define TYPE_ZAPHOD_IOCORE "zaphod-iocore"

#define ZAPHOD_IOCORE_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodIOCoreClass, obj, TYPE_ZAPHOD_IOCORE)
#define ZAPHOD_IOCORE_CLASS(oc) \
    OBJECT_CLASS_CHECK(ZaphodIOCoreClass, oc, TYPE_ZAPHOD_IOCORE)
#define ZAPHOD_IOCORE(obj) \
    OBJECT_CHECK(ZaphodIOCoreState, obj, TYPE_ZAPHOD_IOCORE)


ZaphodScreenState *zaphod_iocore_get_screen(ZaphodIOCoreState *zis);

ZaphodUARTState *zaphod_iocore_get_stdio_uart(ZaphodIOCoreState *zis);
ZaphodUARTState *zaphod_iocore_get_acia_uart(ZaphodIOCoreState *zis);


#endif  /* HW_Z80_ZAPHOD_IOCORE_H */
