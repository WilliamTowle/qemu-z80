/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle, under GPL...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "config-target.h"
#include "qemu.h"
#include "cpu.h"
#include "tcg.h"

#if 1	/* debug */
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif


const char *cpu_model= NULL;
#if defined(CONFIG_USE_GUEST_BASE)
unsigned long guest_base;
#endif

int singlestep;

static void usage(int exitcode)
{
    /* Makefile builds QEMU_PROG, set to 'qemu-'$(TARGET_NAME)
     */
    printf(//"Usage: qemu-" TARGET_ARCH " [options] program [arguments...]\n"
            "Usage: qemu-" TARGET_NAME " [options] program\n"
            );
    exit(exitcode);
}

static void handle_arg_singlestep(void)
{
    singlestep = 1;
}

static int parse_args(int argc, char **argv)
{
    int		optind;
    const char	*r;

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
        else if (strcmp(r, "-singlestep") == 0)
        {
            handle_arg_singlestep();
        }
        /* TODO: support "cpu" option */
        /* TODO: support "singlestep" option */
        else
        {
            fprintf(stderr, "Unexpected option '%s'\n", &r[1]);
            usage(EXIT_FAILURE);
        }
    }

    return optind;
}

void cpu_list_lock(void)
{
/* Nothing to lock without multiple CPUs? */
}

void cpu_list_unlock(void)
{
/* Nothing to lock without multiple CPUs? */
}

#if defined(TARGET_Z80)
void cpu_loop(CPUZ80State *env)
{
    //CPUState *cs = CPU(z80_env_get_cpu(env));
    int	trapnr;

    /* PARTIAL:
     * Temporary indication we're doing something
     */

    for (;;)
    {
        /* PARTIAL:
         * Key exception cases are for ILLOP (bad/unsupported
         * instruction) and KERNEL_TRAP ("magic ramtop") cases.
         * The EXCP_KERNEL_TRAP in particular works like a ROM
         * system call, resulting in program exit in the
         * simplest case.
         */

        /* TODO: we are not SMP. Can we avoid v1.7.x mutexes and
         * cpu_exec_{start|end}()?
         */
        trapnr= cpu_z80_exec(env);
        switch(trapnr)
        {
        /* Identify ILLOP (bail) and KERNEL_TRAP (clean exit) cases.
         * On return from the emulated program (or jump to the "magic
         * ramtop" address) we should exit if there is no ROM code to
         * run at this point
         */
        case EXCP06_ILLOP:
            printf("cpu_exec() encountered EXCP06_ILLOP (trapnr=%d) - aborting emulation\n", trapnr);
            /* TODO: bailing is normal response here */
            break;	/* to exit() beyond switch */
        case EXCP_KERNEL_TRAP:
            printf("cpu_exec() encountered EXCP_KERNEL_TRAP (trapnr=%d) - stopping emulation\n", trapnr);
            /* TODO: show CPU state dump on "successful" exit */
            break;	/* to exit() beyond switch */
        default:
            printf("cpu_exec() exited abnormally (with unexpected trapnr=%d) - aborting emulation\n", trapnr);
        }

        /* PARTIAL:
         * For a system that can generate interrupts and signals
         * them here, process_pending_signals() is required here,
         * and we should restart the loop. Abormal exit drops here
         */
        exit(1);
    }
}
#else	/* non-z80 CPUs */
void cpu_loop(CPUZ80State *env)
{
    CPUState *cs = CPU(z80_env_get_cpu(env));

	printf("cpu_loop() for unimplemented CPU type\n");
	//cpu_dump_state(env, stderr, fprintf, 0);
	cpu_dump_state(cs, stderr, fprintf, 0);
	exit(1);
}
#endif

int main(int argc, char **argv)
{
    char *filename;
    CPUArchState *env;
    void  *target_ram;
    int optind;
    int ret;

    if (argc <= 1)
        usage(EXIT_SUCCESS);    /* effectively "--help" */

#if 1
    module_call_init(MODULE_INIT_QOM);
#endif

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 1/3...\n", __func__);
#endif
    /* PARTIAL - prior to argument parse v1.7.2 has:
     * 1. module_call_init() for QOM
     * 2. qemu_cache_utils_init()
     * 3. initialisation for target's environment/stack
     * 4. global cpu_model is set to NULL
     * 5. cpudef_setup() called [if defined - for x86 support]
     */

    cpu_model= NULL;

#if 0	/* x86 support */
#if defined(cpudef_setup)
    cpudef_setup(); /* parse cpu definitions in target config file (TBD) */
#endif
#endif

    optind= parse_args(argc, argv);
    if (optind >= argc)
        usage(EXIT_FAILURE);
    filename= argv[optind];

    if (cpu_model == NULL)
    {
#if defined(TARGET_Z80)
        cpu_model = "z80";	/* TODO: support specifying "r800"? */
#else
#error unsupported target CPU
#endif
    }

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 2/3...\n", __func__);
#endif
    /* PARTIAL - following argument parse v1.7.5 has:
     * 1. CPU register, binary image info, paths are prepared
     * 2. 'cpu_model' is defaulted based on target CPU
     * 3. TCG and exec subsystems are initialised:
     *    1. with call to tcg_exec_init(0);
     *    2. with call to cpu_exec_init_all();
     */

    tcg_exec_init(0);
    /* TODO: cpu_exec_init_all(); here */

    env = cpu_init(cpu_model);
    if (!env) {
        fprintf(stderr, "Unable to find definition for cpu_model '%s'\n", cpu_model);
        exit(1);
    }
#if 1  /* WmT - TRACE */
;DPRINTF("INFO: After CPU init in %s() - 'env' at %p; reset to follow\n", __func__, env);
#endif

    /* cpu_reset() can contain a tlb_flush() for some platforms. Based
     * on linux-user/main.c, side effects apply on I386/SPARC/PPC
     */
    cpu_reset(ENV_GET_CPU(env));
#if 1    /* WmT - TRACE */
;DPRINTF("%s(): INFO - CPU has reset; initial state follows...\n", __func__);
;cpu_dump_state(ENV_GET_CPU(env), stderr, fprintf, 0);
#endif

    /* Following cpu_reset():
     * - Local variable 'thread_cpu' is set
     * - 'do_strace' is enabled, if environment variable set
     * - There is final environment configuration
     */

#if !defined(CONFIG_USE_GUEST_BASE)
    /* cpu-all.h requires us to define CONFIG_USE_GUEST_BASE if parts of
     * the guest address space are reserved on the host.
     */
#error "CONFIG_USE_GUEST_BASE not defined"
#else
    /* TODO:
     * We have no MMU, and therefore no paging/memory protection.
     * Nevertheless, we *may* want to to use the facilities of the host
     * to ensure any ROM regions can be protected from writes. In
     * the meantime we get to have self-modifying code anywhere.
     */
    target_ram= mmap(0, 64*1024,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (target_ram == MAP_FAILED)
    {
        perror("MAP_FAILED");
        exit(-1);
    }

    /* Giving guest_base a value causes ldub_code() to retrieve bytes
     * from addresses that won't segfault
     */
    guest_base= (unsigned long)target_ram;
#if 1	/* WmT - TRACE */
;DPRINTF("%s(): INFO - set guest_base=%p\n", __func__, (void *)guest_base);
#endif
#endif

    ret= bblbrx_exec(filename);
    if (ret != 0) {
        if (ret > 0)
            printf("%s(): BAILING - unexpected bblbrx_exec() retval %d while loading %s\n", __func__, ret, filename);
        else
            printf("Error while loading %s: %s\n", filename, strerror(-ret));
        exit(1);
    }

#if defined(CONFIG_USE_GUEST_BASE)
	/* loading the executable above allows us to assume GUEST_BASE
	 * has a fixed value at this point...
	 */
	/* tcg_ctx is an extern in tcg/tcg.h, from translate-all.c.
	 * cpu_gen_init() calls an init function for it
	 */
    tcg_prologue_init(&tcg_ctx);
#endif

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 3/3...\n", __func__);
#endif
    /* PARTIAL - v1.7.5 has:
     * 1. before loader_exec():
     * - guest_base reservation is made, logged
     * - command line for target is managed
     * - TaskState is initialised
     * 2. prior to cpu_loop() call:
     * - calls to target_set_brk(), {syscall|signal}_init()
     * - call to tcg_prologue_init() is made if using GUEST_BASE
     * - registers/flags/interrupts are set up based on loaded file
     * - stack and heap settings are recorded in the TaskState
     * - GDB stub initialisation happens if port configured
     */

    /* PARTIAL. May now want/need to:
     * free memory for any redundant data structures
     * log information about the program
     * initialise brk and syscall/signal handlers
     * further initialise 'env' (esp. registers)
     * further initialise TaskState
     * manage any GDB stub
     */
#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - run filename=%s via local cpu_loop() (%d bit '%s' CPU, env %p)\n", __func__, filename, TARGET_LONG_BITS, cpu_model, env);
#endif
    cpu_loop(env); /* NB: doesn't exit to our 'return' statement below */
  return 0;
}
