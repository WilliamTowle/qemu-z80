/*
 * QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Ported/provisional reimplementations by William Towle 2018-2022
 *  All versions under GPL
 */

#ifndef QEMU_Z80_CPU_QOM_H
#define QEMU_Z80_CPU_QOM_H

#include "qom/cpu.h"

#define TYPE_Z80_CPU "z80-cpu"

#define Z80_CPU_CLASS(class) \
    OBJECT_CLASS_CHECK(Z80CPUClass, (class), TYPE_Z80_CPU)
#define Z80_CPU(obj) \
    OBJECT_CHECK(Z80CPU, (obj), TYPE_Z80_CPU)
#define Z80_CPU_GET_CLASS(obj) \
    OBJECT_GET_CLASS(Z80CPUClass, (obj), TYPE_Z80_CPU)

/**
 *  Z80CPUClass
 */
typedef struct Z80CPUClass {
    /*< private >*/
    CPUClass parent_class;

    /*< public >*/
    DeviceRealize parent_realize;
    DeviceUnrealize parent_unrealize;
    void (*parent_reset)(CPUState *cpu);
} Z80CPUClass;

typedef struct Z80CPU Z80CPU;

#endif /* !QEMU_Z80_CPU_QOM_H */
