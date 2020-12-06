/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2019-2022 William Towle <william_towle@yahoo.co.uk>
 *  [...under GPL...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config-target.h"
#include "qemu.h"

#if 1	/* debug */
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif


static void usage(int exitcode)
{
    /* Makefile builds QEMU_PROG, set to 'qemu-'$(TARGET_NAME)
     */
    printf(//"Usage: qemu-" TARGET_ARCH " [options] program [arguments...]\n"
            "Usage: qemu-" TARGET_NAME " [options] program\n"
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
    int optind;
    int ret;

    if (argc <= 1)
        usage(EXIT_SUCCESS);    /* effectively "--help" */

    /* PARTIAL - prior to argument parse v1.7.2 has:
     * 1. module_call_init() for QOM
     * 2. qemu_cache_utils_init()
     * 3. initialisation for target's environment/stack
     * 4. global cpu_model is set to NULL
     * 5. cpudef_setup() called [if defined - for x86 support]
     */

    optind= parse_args(argc, argv);
    if (optind >= argc)
        usage(EXIT_FAILURE);
    filename= argv[optind];

    /* PARTIAL - following argument parse v1.7.5 has:
     * 1. CPU register, binary image info, paths are prepared
     * 2. 'cpu_model' is defaulted based on target CPU
     * 3. TCG and exec subsystems are initialised:
     *    1. with call to tcg_exec_init(0);
     *    2. with call to cpu_exec_init_all();
     * 4. CPU is initialised, and reset called
     * 5. Local variable 'thread_cpu' is set
     * 6. 'do_strace' is enabled, if environment variable set
     * 7. There is final environment configuration
     * 9. Handling of CONFIG_USE_GUEST_BASE/target_ram setup
     */

    ret= bblbrx_exec(filename);
    if (ret != 0) {
        if (ret > 0)
            printf("%s(): BAILING - unexpected bblbrx_exec() retval %d while loading %s\n", __func__, ret, filename);
        else
            printf("Error while loading %s: %s\n", filename, strerror(-ret));
        exit(EXIT_FAILURE);
    }

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
