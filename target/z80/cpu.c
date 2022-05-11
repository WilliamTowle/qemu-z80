/*
 * Minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Ported/provisional reimplementations by William Towle 2018-2022
 *  All versions under GPL
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"

#include "qemu/error-report.h"
#include "exec/exec-all.h"

#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 cpu: " fmt , ## __VA_ARGS__); } while(0)


/* list registered CPU models */

static void z80_cpu_list_entry(gpointer data, gpointer user_data)
{
    ObjectClass *oc = data;
    CPUListState *s = user_data;
    const char *typename= object_class_get_name(oc);
    char *suffix= strstr(typename, "-cpu");
    char *name;

    assert(suffix != NULL);
    name = g_strndup(typename, suffix - typename);
    (*s->cpu_fprintf)(s->file, "  %s\n", name);
    g_free(name);
}

void z80_cpu_list(FILE *f, fprintf_function cpu_fprintf)
{
    CPUListState s = {
        .file = f,
        .cpu_fprintf = cpu_fprintf,
    };
    GSList *list;

    (*cpu_fprintf)(f, "Available CPUs:\n");
    list = object_class_get_list_sorted(TYPE_Z80_CPU, false);
    g_slist_foreach(list, z80_cpu_list_entry, &s);
    g_slist_free(list);
}


static void z80_cpu_initfn(Object *obj)
{
    CPUState *cs = CPU(obj);
    Z80CPU *cpu = Z80_CPU(obj);

    cs->env_ptr = &cpu->env;
}

static void z80_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    Z80CPUClass *zcc = Z80_CPU_GET_CLASS(dev);
    Error *local_err = NULL;

    cpu_exec_realizefn(cs, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }

#ifndef CONFIG_USER_ONLY
;DPRINTF("DEBUG: %s() INCOMPLETE -> BAIL\n", __func__);
;exit(1);
#endif  /* TODO: set machine reset callback here */

    qemu_init_vcpu(cs);

    cpu_reset(cs);

    zcc->parent_realize(dev, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }
}

/* CPUClass::reset() */
static void z80_cpu_reset(CPUState *s)
{
    Z80CPU *cpu = Z80_CPU(s);
    Z80CPUClass *zcc = Z80_CPU_GET_CLASS(cpu);
    CPUZ80State *env = &cpu->env;
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: Reached %s() ** PARTIAL **\n", __func__);
#endif

    zcc->parent_reset(s);

    memset(env, 0, offsetof(CPUZ80State, end_reset_fields));

    /* init to reset state */
    env->pc= 0x0000;
    //env->iff1= 0;
    //env->iff2= 0;
    //env->imode= 0;
    //env->regs[R_A]= 0xff;
    //env->regs[R_F]= 0xff;
    env->regs[R_SP]= 0xffff;

    env->hflags= 0;
}


/* Return type name for a given CPU model name
 * Caller is responsible for freeing the returned string.
 */
static char *z80_cpu_type_name(const char *model_name)
{
    /* NB: must generate a name corresponding to a non-abstract
     * TypeInfo struct below. Direct reference to TYPE_Z80_CPU
     * suits an implementation that doesn't distinguish between
     * Zilog's Z80 and ASCII-Mitsui's R800
     */
    return g_strdup_printf(TYPE_Z80_CPU);
    //return g_strdup_printf(Z80_CPU_TYPE_NAME("%s"), model_name);
}

static ObjectClass *z80_cpu_class_by_name(const char *cpu_model)
{
#if 1   /* redo like i386 */
    g_autofree char *typename= z80_cpu_type_name(cpu_model);
    return object_class_by_name(typename);
#else
    ObjectClass *oc;

    oc = object_class_by_name(cpu_model);
;DPRINTF("DEBUG: %s() got initial oc=%p for cpu_model '%s'\n", __func__, oc);
    if (object_class_dynamic_cast(oc, TYPE_Z80_CPU) == NULL ||
        object_class_is_abstract(oc)) {
;DPRINTF("DEBUG: Failed cast - reason %s\n",
        (object_class_dynamic_cast(oc, TYPE_Z80_CPU) == NULL)?"cast was NULL":"object_class_is_abstract() problem");
        oc = NULL;
    }
;DPRINTF("DEBUG: %s() returning oc %p\n", __func__, oc);
    return oc;
#endif
}


static bool z80_cpu_has_work(CPUState *cs)
{
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: Reached %s() ** PARTIAL **\n", __func__)
;if (cs->interrupt_request & CPU_INTERRUPT_HARD)
{
;DPRINTF("INFO: ** implementation required ** CPU_INTERRUPT_HARD is flagged -> BAIL\n");
;exit(1);
}
;return 0;
#else   /* TODO: implement */
    /* For i386, INTERRUPT_HARD is only flagged if eflags has
     * IF_MASK unset or interrupts are not inhibited if it is.
     * Zilog's Z80 also has NMI type interrupts [not implemented]
     */
    return cs->interrupt_request & CPU_INTERRUPT_HARD;
#endif
}

/* TODO:
 * z80_cpu_do_interrupt() [helper.c]
 * z80_cpu_exec_interrupt() [helper.c]
 */


static void z80_cpu_set_pc(CPUState *cs, vaddr value)
{
    Z80CPU *cpu = Z80_CPU(cs);

    cpu->env.pc = value;
}

/* TODO:
 * z80_cpu_get_phys_page_debug() via helper.c
 */

//static void z80_cpu_disas_set_info(CPUState *cs, disassemble_info *info)
//{
//;DPRINTF("DEBUG: Reached %s() ** PARTIAL **\n", __func__);
//    /* TODO: switch to bfd_mach_z80_r800 where applicable */
//    info->mach = bfd_mach_z80_z80;
//    info->print_insn = print_insn_z80;
//}

static void z80_cpu_synchronize_from_tb(CPUState *cs, TranslationBlock *tb)
{
    Z80CPU *cpu = Z80_CPU(cs);
    CPUZ80State *env = &cpu->env;

    env->pc= tb->pc;
}

static void z80_cpu_class_init(ObjectClass *oc, void *data)
{
    Z80CPUClass *zcc = Z80_CPU_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    DeviceClass *dc = DEVICE_CLASS(oc);

    /* v2's target-i386 common class init */
    device_class_set_parent_realize(dc, z80_cpu_realizefn,
                                    &zcc->parent_realize);
    /* TODO: device_class_set_parent_unrealize(...); */
//    device_class_set_parent_unrealize(dc, z80_cpu_unrealizefn,
//                                    &zcc->parent_unrealize);

    zcc->parent_reset= cc->reset;
    cc->reset= z80_cpu_reset;
    cc->reset_dump_flags = 0;   /* i386: CPU_DUMP_FPU | CPU_DUMP_CCOP */

    cc->class_by_name = z80_cpu_class_by_name;

    cc->has_work = z80_cpu_has_work;
//#ifdef CONFIG_TCG
//    cc->do_interrupt = z80_cpu_do_interrupt;
//    cc->cpu_exec_interrupt = z80_cpu_exec_interrupt;
//#endif
    cc->dump_state = z80_cpu_dump_state;
    /* TODO: cc->get_crash_info() useful? or x86-specific? */
    cc->set_pc = z80_cpu_set_pc;
    cc->synchronize_from_tb = z80_cpu_synchronize_from_tb;
//    cc->gdb_read_register = z80_cpu_gdb_read_register;
//    cc->gdb_write_register = z80_cpu_gdb_write_register;
//    cc->gdb_num_core_regs = 35;	/* NUMBER_OF_CPU_REGISTERS? */
//    cc->gdb_core_xml_file = "z80-cpu.xml";
    /* TODO: set get_phys_page_debug if !CONFIG_USER_ONLY */
//    cc->get_phys_page_debug = z80_cpu_get_phys_page_debug;
//    cc->vmsd = &vmstate_z80_cpu;
    /* TODO? i386 adjusts CPU eflags with:
     * cc->cpu_exec_enter
     * cc->cpu_exec_exit
     */
//#ifdef CONFIG_TCG
//    cc->tcg_initialize = tcg_z80_init;
//#endif
//    cc->disas_set_info = z80_cpu_disas_set_info;
}

static const TypeInfo z80_cpu_type_info = {
    /* NB: default to .abstract=false and regular Zilog Z80 for
     * now (TODO: base class definition can use .name=TYPE_Z80_CPU
     * directly)
     */
    .name = TYPE_Z80_CPU,
    //.name = Z80_CPU_TYPE_NAME(TYPE_Z80_CPU),
    .parent = TYPE_CPU,
    .instance_size = sizeof(Z80CPU),
    .instance_init = z80_cpu_initfn,
    //.abstract = true;
    .class_size = sizeof(Z80CPUClass),
    .class_init = z80_cpu_class_init,
};

static void z80_cpu_register_types(void)
{
    type_register_static(&z80_cpu_type_info);
}

/* TODO: support CPU variants:
 * - Zilog Z80 CPU
 * - Mitsui R800 CPU
 */
type_init(z80_cpu_register_types)
