/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPL...]
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

#if 0	/* X86 specific */
    bool hyperv_vapic;
    bool hyperv_relaxed_timing;
    int hyperv_spinlock_attempts;

    /* if true the CPUID code directly forward host cache leaves to the guest */
    bool cache_info_passthrough;

    /* Features that were filtered out because of missing host capabilities */
    uint32_t filtered_features[FEATURE_WORDS];

    /* Enable PMU CPUID bits. This can't be enabled by default yet because
     * it doesn't have ABI stability guarantees, as it passes all PMU CPUID
     * bits returned by GET_SUPPORTED_CPUID (that depend on host CPU and kernel
     * capabilities) directly to the guest.
     */
    bool enable_pmu;
#endif
} Z80CPU;

static inline Z80CPU *z80_env_get_cpu(CPUZ80State *env)
{
    return container_of(env, Z80CPU, env);
}


#define ENV_GET_CPU(e) CPU(z80_env_get_cpu(e))

#endif /* !defined (QEMU_Z80_CPU_QOM_H) */
