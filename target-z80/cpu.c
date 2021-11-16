/*
 * Minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPL...]
 *  [...all versions under GPL...]
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
     * v1.7.2 does cpu_*_find_by_name() and fills in a struct
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


static void z80_cpu_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    Z80CPUClass *zcc = Z80_CPU_CLASS(oc);

#if 1	/* WmT - PARTIAL */
;fprintf(stderr, "DEBUG: Reached %s() **PARTIAL ONLY**\n", __func__);
#endif
    zcc->parent_realize = dc->realize;
    dc->realize = z80_cpu_realizefn;

//    device_class_set_parent_reset(dc, z80_cpu_reset, &zcc->parent_reset);
//
//    cc->class_by_name = z80_cpu_class_by_name;
//
    cc->has_work = z80_cpu_has_work;
//    cc->do_interrupt = z80_cpu_do_interrupt;
//    cc->cpu_exec_interrupt = z80_cpu_exec_interrupt;
//    cc->dump_state = z80_cpu_dump_state;
    cc->set_pc = z80_cpu_set_pc;
//    cc->memory_rw_debug = z80_cpu_memory_rw_debug;
//    cc->get_phys_page_debug = z80_cpu_get_phys_page_debug;
//    cc->tlb_fill = z80_cpu_tlb_fill;
////    cc->vmsd = &vms_z80_cpu;
//    cc->disas_set_info = z80_cpu_disas_set_info;
//    cc->tcg_initialize = z80_cpu_tcg_init;
    cc->synchronize_from_tb = z80_cpu_synchronize_from_tb;
//    cc->gdb_read_register = z80_cpu_gdb_read_register;
//    cc->gdb_write_register = z80_cpu_gdb_write_register;
//    cc->gdb_num_core_regs = 35;	/* NUMBER_OF_CPU_REGISTERS? */
//    cc->gdb_core_xml_file = "z80-cpu.xml";
}

static const TypeInfo z80_cpu_type_info = {
    /* NB: default to regular Zilog Z80 for now [TODO: use 'name' of
     * TYPE_Z80_CPU for the base class and Z80_CPU_TYPE_NAME(x) for
     * the Z80/R800 subclasses and in z80_cpu_type_name() likewise]
     */
    .name = TYPE_Z80_CPU,
    //.name = Z80_CPU_TYPE_NAME(TYPE_Z80_CPU),
    .parent = TYPE_CPU,
    .instance_size = sizeof(Z80CPU),
    .instance_init = z80_cpu_instance_init,
    //.abstract = true,
    .abstract = false,
    .class_size = sizeof(Z80CPUClass),
    .class_init = z80_cpu_class_init,
};

static void z80_cpu_register_types(void)
{
    type_register_static(&z80_cpu_type_info);
}


type_init(z80_cpu_register_types)
