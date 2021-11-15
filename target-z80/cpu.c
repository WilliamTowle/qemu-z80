/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPL...]
 */

#include "cpu.h"

#include "qemu/error-report.h"

#define DPRINTF(fmt, ...) \
    do { error_printf( "Z80 cpu.c: " fmt , ## __VA_ARGS__); } while(0)


Z80CPU *cpu_z80_init(const char *cpu_model)
{
    Error *error = NULL;
    Z80CPU *cpu;

    cpu = cpu_z80_create(cpu_model, NULL, &error);
    if (error) {
        goto out;
    }

    object_property_set_bool(OBJECT(cpu), true, "realized", &error);

out:
    if (error) {
        error_report("%s", error_get_pretty(error));
        error_free(error);
        if (cpu != NULL) {
            object_unref(OBJECT(cpu));
            cpu = NULL;
        }
    }
    return cpu;
}
