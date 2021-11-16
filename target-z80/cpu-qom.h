/*
 * Minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 *  WmT, 2018-2022 [...after Stuart Brady]
 *  [...all versions under GPL...]
 */

#ifndef QEMU_Z80_QOM_H
#define QEMU_Z80_QOM_H

#include "qom/cpu.h"
#include "cpu.h"
//#include "qapi/error.h"

#define TYPE_Z80_CPU "z80-cpu"

#define Z80_CPU_CLASS(klass) \
    OBJECT_CLASS_CHECK(Z80CPUClass, (klass), TYPE_Z80_CPU)
#define Z80_CPU(obj) \
    OBJECT_CHECK(Z80CPU, (obj), TYPE_Z80_CPU)
#define Z80_CPU_GET_CLASS(obj) \
    OBJECT_GET_CLASS(Z80CPUClass, (obj), TYPE_Z80_CPU)


/**
 * Z80CPUClass:
 * @parent_realize: The parent class' realize handler.
 * @parent_reset: The parent class' reset handler.
 *
 * A Z80 CPU model or family.
 */
typedef struct Z80CPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/

    DeviceRealize parent_realize;
    void (*parent_reset)(CPUState *cpu);
} Z80CPUClass;


/**
 * Z80CPU:
 * @env: #CPUZ80State
 *
 * A z80 CPU.
 */
typedef struct Z80CPU {
    /*< private >*/
    CPUState parent_obj;

    /*< public >*/
    CPUZ80State env;
} Z80CPU;

static inline Z80CPU *z80_env_get_cpu(CPUZ80State *env)
{
    return container_of(env, Z80CPU, env);
}


#define ENV_GET_CPU(e) CPU(z80_env_get_cpu(e))

#define ENV_OFFSET offsetof(Z80CPU, env)


void z80_cpu_do_interrupt(CPUState *cpu);

void z80_cpu_dump_state(CPUState *cs, FILE *f, fprintf_function cpu_fprintf,
                        int flags);


#endif /* !defined (QEMU_Z80_CPU_QOM_H) */
