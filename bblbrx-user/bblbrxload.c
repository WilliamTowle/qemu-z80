/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2019-2022 William Towle <william_towle@yahoo.co.uk>
 *  [...under GPL...]
 */

#include "qemu/osdep.h"
#include "qemu.h"

#include "qemu/error-report.h"

#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("bblbrx-user bblbrxload: " fmt , ## __VA_ARGS__); } while(0)


/* handle raw binary loading */
int bblbrx_exec(const char *filename)
{
    struct stat	st;

    if (stat(filename, &st) < 0)
    {
        printf("%s(): stat() failed on %s\n", __func__, filename);
        return -errno;
    }
    if (!S_ISREG(st.st_mode))
    {   /* Require a (readable) regular file. Whether the host
         * considers this to be executable is unimportant.
         */

        printf("%s(): %s not a regular file\n", __func__, filename);
        return -EACCES;
    }

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): INFO - file %s has %llu bytes\n", __func__, filename, st.st_size);
#endif
    /* PARTIAL.
     * Some formats have sections and a code entry point; for bblbrx:
     *  - we have a flat file format (no sections)
     *	- we always load the file to 0x0000
     *	- we always execute from 0x000 (as per initial PC)
     *	- we always assume a stack growing down from 64K (initial SP)
     */

    return fprintf(stderr, "%s(): exec %s: PARTIAL - load routines absent\n", __func__, filename);
}
