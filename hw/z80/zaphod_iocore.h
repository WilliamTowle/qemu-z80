/*
 * QEmu Zaphod board - iocore support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef HW_Z80_ZAPHOD_IOCORE_H
#define HW_Z80_ZAPHOD_IOCORE_H

#include "zaphod.h"


typedef DeviceClass ZaphodIOCoreClass;

typedef struct {
    DeviceState     parent;
    //CharBackend   chr;
} ZaphodIOCoreState;


#define TYPE_ZAPHOD_IOCORE "zaphod-iocore"

#define ZAPHOD_IOCORE_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodIOCoreClass, obj, TYPE_ZAPHOD_IOCORE)
#define ZAPHOD_IOCORE_CLASS(klass) \
    OBJECT_CLASS_CHECK(ZaphodIOCoreClass, klass, TYPE_ZAPHOD_IOCORE)
#define ZAPHOD_IOCORE(obj) \
    OBJECT_CHECK(ZaphodIOCoreState, obj, TYPE_ZAPHOD_IOCORE)


ZaphodIOCoreState *zaphod_iocore_init(void);

#endif  /* HW_Z80_ZAPHOD_IOCORE_H */
