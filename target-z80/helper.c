/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPLv2+...]
 */


#include <stdlib.h>
#include <string.h>

#include "cpu.h"

static int cpu_z80_find_by_name(const char *name);

CPUZ80State *cpu_z80_init(const char *model)
{
    CPUZ80State *env;
    int id;

    id = cpu_z80_find_by_name(model);
    if (id == 0) {
        return NULL;
    }

    env= calloc(1, sizeof *env);

    /* PARTIAL: cpu_z80_init() continues (requiring enhanced
     * CPUZ80State?) with:
     * 1. cpu_exec_init() for the 'env'
     * 2. z80_translate_init() call, if not already done
     * 3. store id in env->model
     * 4. calls to cpu_reset(), qemu_init_vcpu()
     */

    return env;
}

/* Legacy cpu_*_find_by_name()
 * This function returns 0 to signify "not found"; it uses the
 * Z80_CPU_* constants in cpu.h for success values. In target-i386
 * for QEmu v1+ an index into an array is returned, with -1 on error
 */
static int cpu_z80_find_by_name(const char *name)
{
    /* PARTIAL: qemu-z80 iterates around z80_cpu_names[] list,
     * containing "z80", "r800", an end-of-list sentinel, and
     * the Z80_CPU_* constants used as return values
     */
    if (strcmp(name, "z80") == 0)
    {
        return 1;	/* value of Z80_CPU_Z80 */
    }

  return 0;
}
