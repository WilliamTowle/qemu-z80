/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2019-2022 William Towle <william_towle@yahoo.co.uk>
 *  [...under GPL...]
 */


#include "qemu/osdep.h"
#include "qemu.h"

#include "qapi/error.h"
#include "qemu/error-report.h"

#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("bblbrx-user main: " fmt , ## __VA_ARGS__); } while(0)


static void usage(int exitcode)
{
    /* NB: platforms may pass program arguments */
    printf("Usage: qemu-" TARGET_NAME " [options] program\n");
    exit(exitcode);
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

    /* PARTIAL: prior to argument parse v2.12.1 has:
     * 1. init for MODULE_INIT_TRACE, cpu list, MODULE_INIT_QOM
     * 2. initialisation for target's environment and stack
     * 3. cpu_model is defaulted to NULL
     * 4. srand() is called
     * 5. tracing options are noted
     */

    optind= parse_args(argc, argv);
    if (optind >= argc)
        usage(EXIT_FAILURE);
    filename= argv[optind];


    /* PARTIAL - following argument parse, v2.12.1 has:
     * 1. CPU register, binary image info, paths are prepared
     * 2. checking the binary file opens is done
     * 3. cpu_model is defaulted based on the binary, if unset
     * 4. TCG subsystem is initialised with tcg_exec_init(0);
     * 5. CPU is initialised, and reset called
     * 6. Local variable 'thread_cpu' is set
     * 7. 'do_strace' is enabled, if environment variable set
     * 8. 'randseed' is handled, if environment variable set
     * 9. Environment configuration is done, and target RAM set up
     */

    ret= bblbrx_exec(filename);
    if (ret != 0) {
        if (ret > 0)
            printf("%s(): BAILING - unexpected bblbrx_exec() retval %d while loading %s\n", __func__, ret, filename);
        else
            printf("Error while loading %s: %s\n", filename, strerror(-ret));
        exit(EXIT_FAILURE);
    }

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
;return fprintf(stderr, "%s(): INCOMPLETE - need cpu_loop() to execute %s\n", __func__, filename);
#else
    /* NB: cpu_loop() exits on ILLOP/KERNEL_TRAP */

  return EXIT_SUCCESS;
#endif
}
