/*
 * QEmu Zaphod board - Motorola MC6850 ACIA support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef HW_Z80_ZAPHOD_MC6850_H
#define HW_Z80_ZAPHOD_MC6850_H

#include "zaphod.h"

//#include "chardev/char-fe.h"
//#include "sysemu/char.h"

typedef struct ZaphodState ZaphodState;

typedef DeviceClass ZaphodMC6850Class;

typedef struct {
    DeviceState     parent;

    ZaphodState     *super;
    PortioList      *ports;
    //CharDriverState *chr;
    //uint8_t         inkey;
} ZaphodMC6850State;


#define TYPE_ZAPHOD_MC6850 "zaphod-mc6850"

#define ZAPHOD_MC6850_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodMC6850Class, obj, TYPE_ZAPHOD_MC6850)
#define ZAPHOD_MC6850_CLASS(klass) \
    OBJECT_CLASS_CHECK(ZaphodMC6850Class, klass, TYPE_ZAPHOD_MC6850)
#define ZAPHOD_MC6850(obj) \
    OBJECT_CHECK(ZaphodMC6850State, obj, TYPE_ZAPHOD_MC6850)


//DeviceState *zaphod_mc6850_new(CharDriverState *cd);
DeviceState *zaphod_mc6850_new(ZaphodState *super);

#endif  /* HW_Z80_ZAPHOD_MC6850_H */
