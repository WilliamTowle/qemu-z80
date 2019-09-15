/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2019-2022 William Towle <william_towle@yahoo.co.uk>
 *  [...under GPL...]
 */


#include "qemu/osdep.h"
#include "qemu.h"
#include "cpu.h"

#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/help_option.h"
#include "tcg/tcg.h"


#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("bblbrx-user main: " fmt , ## __VA_ARGS__); } while(0)


const char *cpu_model;
unsigned long guest_base;
unsigned long reserved_va;

__thread CPUState *thread_cpu;
int singlestep;


bool qemu_cpu_is_self(CPUState *cpu)
{
    return thread_cpu == cpu;
}

void qemu_cpu_kick(CPUState *cpu)
{
    cpu_exit(cpu);
}


static void usage(int exitcode)
{
    /* NB: platforms may pass program arguments */
    printf("Usage: qemu-" TARGET_NAME " [options] program\n");
    exit(exitcode);
}

static void handle_arg_cpu(char *arg)
{
    cpu_model= arg;

    if (cpu_model == NULL || is_help_option(cpu_model)) {
#if defined(cpu_list)
        cpu_list(stdout, &fprintf);
#endif
        exit(EXIT_FAILURE);
    }
}

static int parse_args(int argc, char **argv)
{
    int         optind;
    const char  *r;

    optind= 1;
    for (;;) {
        if (optind >= argc) {
            break;
        }

        r= argv[optind];
        if (r[0] != '-') {
            break;
        }
        optind++;

        if (r[1] == '-') r++;   /* have '-X' and '--X' equivalent */
        if (strcmp(r, "-help") == 0)
        {
            usage(EXIT_SUCCESS);
        }
        else if (strcmp(r, "-cpu") == 0)
        {
            handle_arg_cpu(argv[optind++]);
        }
        /* TODO: support "singlestep" option */
        else
        {
            fprintf(stderr, "Unexpected option '%s'\n", &r[1]);
            usage(EXIT_FAILURE);
        }
    }

    return optind;
}

#if defined(TARGET_Z80)
void cpu_loop(CPUZ80State *env)
{
    CPUState    *cs = CPU(z80_env_get_cpu(env));
    int         trapnr;

    for (;;)
    {
        cpu_exec_start(cs);
        trapnr= cpu_exec(cs);
        cpu_exec_end(cs);
        process_queued_cpu_work(cs);

        switch(trapnr)
        {
        case EXCP_ILLOP:
            /* instruction parser is incomplete - bailing is normal */
            printf("cpu_exec() encountered EXCP_ILLOP (trapnr=%d) - aborting emulation\n", trapnr);
            break;      /* to loop-exit 'break' */
        case EXCP_KERNEL_TRAP:
            /* "magic ramtop" reached - exit and show CPU state */
            printf("Program exit. Register dump follows:\n");
            break;      /* to loop-exit 'break' */
        default:
            printf("cpu_exec() exited abnormally (with unexpected trapnr=%d) - aborting emulation\n", trapnr);
        }

        //cpu_dump_state(env, stderr, fprintf, 0);
        cpu_dump_state(cs, stderr, fprintf, 0);
        //process_pending_signals(env);
	break;
    }
}
#else   /* non-z80 CPUs */
void cpu_loop(CPUZ80State *env)
{
    CPUState *cs = CPU(z80_env_get_cpu(env));

    printf("Reached cpu_loop() for unimplemented CPU type\n");
    //cpu_dump_state(env, stderr, fprintf, 0);
    cpu_dump_state(cs, stderr, fprintf, 0);
    exit(EXIT_FAILURE);
}
#endif

int main(int argc, char **argv)
{
    const char *cpu_type;
    CPUArchState *env;
    CPUState *cpu;
    char *filename;
    void  *target_ram;
    struct bblbrx_binprm bprm;
    int optind;
    int ret;

    if (argc <= 1)
        usage(EXIT_SUCCESS);    /* effectively "--help" */

    /* PARTIAL: prior to argument parse v2.12.1 has:
     * 1. init for MODULE_INIT_TRACE, cpu list, MODULE_INIT_QOM
     * 2. initialisation for target's environment and stack
     * 3. cpu_model is defaulted to NULL
     * 4. srand() is called
     * 5. tracing options are noted
     */

    //module_call_init(MODULE_INIT_TRACE);
    qemu_init_cpu_list();
    module_call_init(MODULE_INIT_QOM);


    cpu_model= NULL;

    optind= parse_args(argc, argv);
    if (optind >= argc)
        usage(EXIT_FAILURE);
    filename= argv[optind];

    if (cpu_model == NULL) {
        cpu_model= "z80";       /* TODO: respect "cpu" option here */
    }

    cpu_type= parse_cpu_model(cpu_model);

    /* init tcg before creating CPUs and to get qemu_host_page_size */
    tcg_exec_init(0);

    cpu= cpu_create(cpu_type);
    env= cpu->env_ptr;
    cpu_reset(cpu);
#if 1   /* WmT - TRACE */
    /* TODO: don't need both create, reset */
;DPRINTF("%s(): INFO - CPU create/reset OK; initial state dump follows...\n", __func__);
;cpu_dump_state(cpu, stderr, fprintf, 0);
#endif

    /* Following cpu_reset(), v2 has:
     *  1. 'thread_cpu' initialise
     *  2. check QEMU_{STRACE|QEMU_RAND_SEED}
     *  3. initialisation of 'target_environ' [if required]
     */
    //thread_cpu= cpu;

    /* Since we have no MMU, the entirety of target RAM is
     * effectively available to programs at all times without
     * protection (ie. not just where we load any code).
     * Setting guest_base ensures that disas_insn()'s byte fetch
     * doesn't segfault
     */
    target_ram= mmap(0, 64*1024,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (target_ram == MAP_FAILED)
    {
        perror("MAP_FAILED");
        exit(-1);
    }

    guest_base= (unsigned long)target_ram;
#if 1   /* WmT - TRACE */
;DPRINTF("%s(): INFO: guest_base set - using %p\n", __func__, (void *)guest_base);
#endif


    ret= bblbrx_exec(filename, &bprm);
    if (ret != 0) {
        if (ret > 0)
            printf("%s(): BAILING - unexpected bblbrx_exec() retval %d while loading %s\n", __func__, ret, filename);
        else
            printf("Error while loading %s: %s\n", filename, strerror(-ret));
        exit(EXIT_FAILURE);
    }

    /* Now GUEST_BASE is known, generate the prologue so its value
     * can be taken into account
     */
    tcg_prologue_init(tcg_ctx);
    tcg_region_init();

#ifdef TARGET_Z80
    /* In order to execute raw binaries without ROMs present, we
     * designate an address as a "magic ramtop" location and write
     * a relevant sentinel-type value on the stack.
     * Regular programs with a final 'ret' will pop this value, and
     * the jump there can then be intercepted. Similarly, CP/M command
     * binaries using this value for the zero page's "exit program"
     * target will have the same result.
     */
    if (bprm.magic_ramloc)
    {
        env->regs[R_SP]= bprm.magic_ramloc;
        stw_raw(env->regs[R_SP], bprm.magic_ramloc);
    }
#else
#error unsupported target CPU
#endif

    /* PARTIAL - in v2.12.1 here:
     * 1. before loader_exec(),
     * - guest_base reservation is made, logged
     * - command line for target is managed
     * - TaskState is initialised
     * 2. prior to cpu_loop() call:
     * - calls to target_set_brk(), {syscall|signal}_init()
     * - TCG prologue and region are initialised
     * - registers/flags/interrupts are set up based on loaded file
     * - stack and heap settings are recorded in the TaskState
     * - GDB stub initialisation happens if port is configured
     */

#if 1   /* WmT - PARTIAL */
;DPRINTF("%s(): PARTIAL - run filename=%s via cpu_loop() ('%s' CPU, env %p)\n", __func__, filename, cpu_model, env);
#endif
    cpu_loop(env);

    return EXIT_SUCCESS; /* if cpu_loop() exits (ILLOP/KERNEL_TRAP) */
}
