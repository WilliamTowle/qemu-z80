/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle, under GPL...]
 */

#include <stdio.h>
#include <errno.h>

#include "qemu.h"


#if 1	/* debug */
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif


int load_raw_binary(struct bblbrx_binprm *bprm)
{
    abi_ulong	code_start, code_size;

    code_start= 0x0000;
    code_size= bprm->filesize;

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing load (%u bytes from file %s) to guest addr 0x%04x\n", __func__, code_size, bprm->filename, code_start);
#endif
    /* PARTIAL: typical algorithm runs as follows
     * 1. call load function; normally:
    ...there are functions specific to file types
    ...files are in sections, with size information encapsulated
     * 2. determine code entry point and initialise stack/registers
     * 3. return an appropriate status to the calling function
     */
    /* TODO:
     * 1. memory model has:
	    ...code in low RAM addresses
	    ...stack growing downward from RAMTOP (64K)
     * 2. file size tells us how many bytes to load
     * 3. might want to set ROM/RAM split based on arguments
     */

#if 1
;return fprintf(stderr, "%s(): PARTIAL - load routines incomplete\n", __func__);
#else
    return -ENOEXEC;
#endif
}
