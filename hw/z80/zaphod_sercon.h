/*
 * QEmu Zaphod board - sercon support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef HW_Z80_ZAPHOD_SERCON_H
#define HW_Z80_ZAPHOD_SERCON_H

//#include "zaphod.h"


typedef DeviceClass ZaphodSerConClass;

typedef struct {
    DeviceState     parent;

    PortioList      *ports;
    CharDriverState *chr;
} ZaphodSerConState;


#define TYPE_ZAPHOD_SERCON "zaphod-sercon"

#define ZAPHOD_SERCON_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodSerConClass, obj, TYPE_ZAPHOD_SERCON)
#define ZAPHOD_SERCON_CLASS(klass) \
    OBJECT_CLASS_CHECK(ZaphodSerConClass, klass, TYPE_ZAPHOD_SERCON)
#define ZAPHOD_SERCON(obj) \
    OBJECT_CHECK(ZaphodSerConState, obj, TYPE_ZAPHOD_SERCON)


DeviceState *zaphod_sercon_new(void);


#endif  /* HW_Z80_ZAPHOD_SERCON_H */
