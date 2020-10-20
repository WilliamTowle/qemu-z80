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

static void z80_cpu_reset(DeviceState *ds)
{
    CPUState *cs = CPU(ds);
    Z80CPU *cpu = Z80_CPU(cs);
    Z80CPUClass *zcc = Z80_CPU_GET_CLASS(cpu);
    CPUZ80State *env = &cpu->env;
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: Reached %s() ** PARTIAL **\n", __func__);
#endif

    zcc->parent_reset(ds);

    /* init to reset state */
    memset(env->regs, 0, sizeof(env->regs));
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
     * Zilog's Z80 and ASCII Corp's R800
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
;DPRINTF("INFO: CPU_INTERRUPT_HARD is flagged -> BAIL\n");
;exit(1);
}
;return 0;
#else   /* TODO: implement */
    /* For i386, INTERRUPT_HARD is only flagged if eflags has
     * IF_MASK set; Zilog's Z80 also has NMI type interrupts [not
     * implemented]
     */
    return cs->interrupt_request & CPU_INTERRUPT_HARD;
#endif
}

/* TODO:
 * z80_cpu_do_interrupt() [helper.c]
 * z80_cpu_exec_interrupt() [helper.c]
 * z80_cpu_dump_state() [translate.c]
 */


static void z80_cpu_set_pc(CPUState *cs, vaddr value)
{
    Z80CPU *cpu = Z80_CPU(cs);

    cpu->env.pc = value;
}

/* TODO:
 * z80_cpu_get_phys_page_debug() via helper.c
 */

//static void z80_cpu_disas_set_info(CPUState *cpu, disassemble_info *info)
//{
//;DPRINTF("DEBUG: Reached %s() ** PARTIAL **\n", __func__);
///* TODO: where to clarify Z80A/R800 with bfd_mach_FOO? */
//    info->mach = bfd_arch_z80;
//    info->print_insn = z80_print_insn;
//}

static void z80_cpu_synchronize_from_tb(CPUState *cs, TranslationBlock *tb)
{
    Z80CPU *cpu = Z80_CPU(cs);
    CPUZ80State *env = &cpu->env;

    env->pc= tb->pc;
}

static void z80_cpu_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    Z80CPUClass *zcc = Z80_CPU_CLASS(oc);

#if 1   /* WmT - PARTIAL */
;DPRINTF("DEBUG: Reached %s() ** PARTIAL **\n", __func__);
#endif
    zcc->parent_realize = dc->realize;
    dc->realize = z80_cpu_realizefn;

    device_class_set_parent_reset(dc, z80_cpu_reset, &zcc->parent_reset);

    cc->class_by_name = z80_cpu_class_by_name;

    cc->has_work = z80_cpu_has_work;
//#ifdef CONFIG_TCG
//    cc->do_interrupt = z80_cpu_do_interrupt;
//    cc->cpu_exec_interrupt = z80_cpu_exec_interrupt;
//#endif
//    cc->dump_state = z80_cpu_dump_state;
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

type_init(z80_cpu_register_types)
