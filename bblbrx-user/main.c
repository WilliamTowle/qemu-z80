/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle, under GPL...]
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

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 1/2...\n", __func__);
#endif
    /* PARTIAL - in v1.0.1 here:
     * 1. There is been an envlist_create() and envlist_setup()
     * 2. Call to cpudef_setup() happens, if defined
     * 3. Argument handling may set cpu_model to non-NULL
     */

    optind= parse_args(argc, argv);
    if (optind >= argc)
        usage(EXIT_FAILURE);
    filename= argv[optind];

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 2/2...\n", __func__);
#endif
    /* PARTIAL - in v1.0.1 here:
     * 1. Logging is initialised
     * 2. Register/image info and paths are prepared
     * 3. TCG and exec subsystems are initialised with:
     *    1. call to tcg_exec_init(0);
     *    2. call to cpu_exec_init_all();
     * 4. Call to cpu_init();
     * 5. Optional call to cpu_reset(env) for I386/SPARC/PPC targets
     * 6. Initialise 'thread_env' (externed via qemu.h)
     * 7. Set 'do_strace', if supported
     * 8. Initialisation of 'target_environ'
     * 9. Handling of CONFIG_USE_GUEST_BASE
     * 10. After loader_exec() we have:
     *    1. logging of guest_base
     *    2. calls to target_set_brk(), {syscall|signal}_init()
     *    3. call to tcg_prologue_init()
     *    4. allocate/initialise any TaskState
     *    5. GDB stub initialisation if port configured
     *    6. cpu_loop() call
     */

    ret= bblbrx_exec(filename);
    if (ret != 0) {
        if (ret > 0)
            printf("%s(): BAILING - unexpected bblbrx_exec() retval %d while loading %s\n", __func__, ret, filename);
        else
            printf("Error while loading %s: %s\n", filename, strerror(-ret));
        exit(1);
    }

#if 1	/* WmT - TODO */
;return fprintf(stderr, "%s(): INCOMPLETE - call cpu_loop() to run %s\n", __func__, filename);
#else
    /* PARTIAL: needs to pass 'env' to cpu_loop() here.
     * NB: cpu_loop() doesn't exit to our 'return' statement below
     */

  return 0;
#endif
}
