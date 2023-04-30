/*
 * QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Ported/provisional reimplementations by William Towle 2018-2023
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */


#ifndef QEMU_Z80_CPU_QOM_H
#define QEMU_Z80_CPU_QOM_H

#include "qom/cpu.h"

#define TYPE_Z80_CPU "z80-cpu"

#define Z80_CPU_CLASS(klass) \
    OBJECT_CLASS_CHECK(Z80CPUClass, (klass), TYPE_Z80_CPU)
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
