/* bblbrx - barebones layer for binary execution */

#include <stdio.h>
#include <errno.h>

#include "qemu.h"

int load_raw_binary(struct bblbrx_binprm *bprm)
{
  abi_ulong	host_start, host_size;
	host_start= 0x0000;
	host_size= bprm->filesize;

#if 1	/* WmT - TRACE */
;fprintf(stderr, "%s(): PARTIAL - missing load (%u bytes from file %s) to addr 0x%04x\n", __func__, host_size, bprm->filename, host_start);
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
