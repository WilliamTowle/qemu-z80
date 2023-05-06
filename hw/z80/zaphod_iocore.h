/*
 * QEmu Zaphod board - IOCore support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#ifndef HW_Z80_ZAPHOD_IOCORE_H
#define HW_Z80_ZAPHOD_IOCORE_H

#include "zaphod.h"

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

    PortioList      *ioports_stdio;
    PortioList      *ioports_acia;
    qemu_irq        *irq_acia;
    /* FIXME: stdio/ACIA/KEYBIO muxing configuration is missing */
#if 1   /* IOCORE-KEYBIO */
    QemuInputHandlerState *ihs;
    int             modifiers;
#endif
} ZaphodIOCoreState;


#define TYPE_ZAPHOD_IOCORE "zaphod-iocore"

#define ZAPHOD_IOCORE_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodIOCoreClass, obj, TYPE_ZAPHOD_IOCORE)
#define ZAPHOD_IOCORE_CLASS(klass) \
    OBJECT_CLASS_CHECK(ZaphodIOCoreClass, klass, TYPE_ZAPHOD_IOCORE)
#define ZAPHOD_IOCORE(obj) \
    OBJECT_CHECK(ZaphodIOCoreState, obj, TYPE_ZAPHOD_IOCORE)


#endif  /* HW_Z80_ZAPHOD_IOCORE_H */
