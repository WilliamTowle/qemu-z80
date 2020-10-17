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


//#include "qemu/error-report.h"

#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("bblbrx-user main: " fmt , ## __VA_ARGS__); } while(0)


static void usage(int exitcode)
{
    /* NB: platforms may pass program arguments */
    printf(//"Usage: qemu-" TARGET_NAME " [options] program [arguments...]\n"
            "Usage: bblbrx-main [options] program\n"
            );
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

    if (argc <= 1)
        usage(EXIT_SUCCESS);    /* effectively "--help" */

    optind= parse_args(argc, argv);
    if (optind >= argc)
        usage(EXIT_FAILURE);
    filename= argv[optind];

    fprintf(stderr, "%s: Filename '%s' was given...\n", argv[0], filename);
    exit(EXIT_FAILURE);
}
