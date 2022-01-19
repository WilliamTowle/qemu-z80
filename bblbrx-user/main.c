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


static void usage(int exitcode)
{
    /* Makefile builds QEMU_PROG, set to 'qemu-'$(TARGET_NAME)
     */
    printf(//"Usage: qemu-" TARGET_ARCH " [options] program [arguments...]\n"
            "Usage: qemu-" TARGET_NAME " program\n"
            );
    exit(exitcode);
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

int main(int argc, char **argv)
{
    char *filename;
    CPUArchState *env;
    void  *target_ram;
    int optind;
    int ret;

    if (argc <= 1)
        usage(EXIT_SUCCESS);    /* effectively "--help" */

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

    env = cpu_init(cpu_model);
    if (!env) {
        fprintf(stderr, "Unable to find definition for cpu_model '%s'\n", cpu_model);
        exit(1);
    }
#if 1  /* WmT - TRACE */
;DPRINTF("INFO: After CPU init in %s() - 'env' at %p; reset to follow\n", __func__, env);
#endif

    /* ...in v1.7.5, after cpu_init()
     * 1. Local variable 'thread_cpu' is set
     * 2. 'do_strace' is enabled, if environment variable set
     * 3. There is final environment configuration
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

#if 1	/* WmT - TODO */
;return fprintf(stderr, "%s(): INCOMPLETE - call cpu_loop() to run %s\n", __func__, filename);
#else
    /* PARTIAL: needs to pass 'env' to cpu_loop() here.
     * NB: cpu_loop() doesn't exit to our 'return' statement below
     */

  return 0;
#endif
}
