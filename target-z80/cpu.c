/*
 * Minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPL...]
 */

#include "cpu.h"

#include "qemu/error-report.h"
#include "hw/qdev-properties.h"


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

static void cpu_z80_register(Z80CPU *cpu, const char *name, Error **errp)
{
    /* FUTURE: distinguish/support CPU types here?
     * v1.7.2 does cpu_*_find_by_name() and sets various 'object'
     * properties and 'env' values
     */
}

Z80CPU *cpu_z80_create(const char *cpu_model, DeviceState *icc_bridge,
                       Error **errp)
{
    Z80CPU *cpu = NULL;
    gchar **model_pieces;
    //char *name, *features;
    char *name;
    char *typename;
    Error *error = NULL;

    model_pieces = g_strsplit(cpu_model, ",", 2);
    if (!model_pieces[0]) {
        error_setg(&error, "Invalid/empty CPU model name");
        goto out;
    }
    name = model_pieces[0];
    //features = model_pieces[1];

    cpu = Z80_CPU(object_new(TYPE_Z80_CPU));
    cpu_z80_register(cpu, name, &error);
    if (error) {
        goto out;
    }

    /* Emulate per-model subclasses for global properties */
    typename = g_strdup_printf("%s-" TYPE_Z80_CPU, name);
    qdev_prop_set_globals_for_type(DEVICE(cpu), typename, &error);
    g_free(typename);
    if (error) {
        goto out;
    }

#if 0   /* FUTURE: distinguish/support CPU types here? */
    cpu_z80_parse_featurestr(cpu, features, &error);
    if (error) {
        goto out;
    }
#endif

out:
    if (error != NULL) {
        error_propagate(errp, error);
        object_unref(OBJECT(cpu));
        cpu = NULL;
    }
    g_strfreev(model_pieces);
    return cpu;
}
