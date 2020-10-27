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

#endif /* !defined (QEMU_Z80_CPU_QOM_H) */
