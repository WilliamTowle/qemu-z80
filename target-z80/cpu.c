/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPL...]
 */

#include "cpu.h"

#include "qemu/error-report.h"

#ifndef CONFIG_USER_ONLY
#include "hw/hw.h"
#endif
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


static void z80_cpu_instance_init(Object *obj)
{
    CPUState *cs = CPU(obj);
    Z80CPU *cpu = Z80_CPU(obj);
    CPUZ80State *env = &cpu->env;
    static int inited;

    cs->env_ptr = env;
    cpu_exec_init(env);

    /* x86 sets family/model/stepping etc here */

    /* init various static tables used in TCG mode */
    if (tcg_enabled() && !inited) {
        inited = 1;
#if 0   /* x86-specific? */
        optimize_flags_init();
#endif
#ifndef CONFIG_USER_ONLY
#if 0	/* omit? unimplemented */
        cpu_set_debug_excp_handler(breakpoint_handler);
#endif
#endif
    }
//X{
//X    Z80CPU *cpu = Z80_CPU(obj);
//X    cpu_set_cpustate_pointers(cpu);
//X}
}

#ifndef CONFIG_USER_ONLY
/* TODO: remove me, when reset over QOM tree is implemented */
static void z80_cpu_machine_reset_cb(void *opaque)
{
    Z80CPU *cpu = opaque;
    cpu_reset(CPU(cpu));
}
#endif

static void z80_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    Z80CPUClass *xcc = Z80_CPU_GET_CLASS(dev);
    //XCPUZ80State *env = &cpu->env;
    Error *local_err = NULL;
#ifndef CONFIG_USER_ONLY
    Z80CPU *cpu = Z80_CPU(dev);

    qemu_register_reset(z80_cpu_machine_reset_cb, cpu);
#endif

    qemu_init_vcpu(cs);

    cpu_reset(cs);

    xcc->parent_realize(dev, &local_err);

//Xout:
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }
//X{
//X    CPUState *cs = CPU(dev);
//X    Z80CPUClass *zcc = Z80_CPU_GET_CLASS(dev);
//X    Error *local_err = NULL;
//X
//X    cpu_exec_realizefn(cs, &local_err);
//X    if (local_err != NULL) {
//X        error_propagate(errp, local_err);
//X        return;
//X    }
//X    qemu_init_vcpu(cs);
//X    cpu_reset(cs);
//X
//X    zcc->parent_realize(dev, errp);
//X}
}



static void z80_cpu_class_init(ObjectClass *oc, void *data)
{
    Z80CPUClass *xcc = Z80_CPU_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    DeviceClass *dc = DEVICE_CLASS(oc);

    xcc->parent_realize = dc->realize;
    dc->realize = z80_cpu_realizefn;
    //Xdc->bus_type = TYPE_ICC_BUS;
    //Xdc->props = z80_cpu_properties;

    xcc->parent_reset = cc->reset;
    cc->reset = z80_cpu_reset;
    //cc->reset_dump_flags = CPU_DUMP_FPU | CPU_DUMP_CCOP;
    cc->reset_dump_flags = 0;

    cc->do_interrupt = z80_cpu_do_interrupt;
    cc->dump_state = z80_cpu_dump_state;
    cc->set_pc = z80_cpu_set_pc;
    cc->synchronize_from_tb = z80_cpu_synchronize_from_tb;
#if 0	/* FIXME: gdb interface unimplemented */
    cc->gdb_read_register = z80_cpu_gdb_read_register;
    cc->gdb_write_register = z80_cpu_gdb_write_register;
#endif
    //Xcc->get_arch_id = z80_cpu_get_arch_id;
    //Xcc->get_paging_enabled = z80_cpu_get_paging_enabled;
#ifndef CONFIG_USER_ONLY
    //Xcc->get_memory_mapping = z80_cpu_get_memory_mapping;
    //Xcc->get_phys_page_debug = z80_cpu_get_phys_page_debug;
    //Xcc->write_elf64_note = z80_cpu_write_elf64_note;
    //Xcc->write_elf64_qemunote = z80_cpu_write_elf64_qemunote;
    //Xcc->write_elf32_note = z80_cpu_write_elf32_note;
    //Xcc->write_elf32_qemunote = z80_cpu_write_elf32_qemunote;
    //Xcc->vmsd = &vmstate_z80_cpu;
#endif
#if 0	/* FIXME: gdb interface unimplemented (and probably wrong) */
    cc->gdb_num_core_regs = CPU_NB_REGS * 2 + 25;
#endif
}

static const TypeInfo z80_cpu_type_info = {
	/* NB: default to .abstract=false and emulate regular Zilog Z80
     * for now (TODO: base class definition can use
     * .name=TYPE_Z80_CPU directly)
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
