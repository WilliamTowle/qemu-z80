/* bblbrx - barebones layer for binary execution */

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include "qemu.h"

/* handle raw binary loading */
int bblbrx_exec(const char *filename)
{
    struct stat	st;

    /* PARTIAL.
     * Naive check the file exists and is valid (only requires a
     * regular file; our binaries do not need be executable as far
     * as the host OS is concerned)
     * CONSIDER: *if* we target_mmap() the file's permissions affect
     * the parameters passed
     */
    if (stat(filename, &st) < 0)
    {
	printf("%s(): stat() failed on %s\n", __func__, filename);
	return -errno;
    }
    if (!S_ISREG(st.st_mode))
    {
	return -EACCES;
    }

#if 1	/* WmT - TRACE */
;printf("%s(): INFO - file %s has %llu bytes\n", __func__, filename, st.st_size);
#endif
    /* PARTIAL.
     * Normally there are functions specific to file types
     *	...files can have sections
     *	...for our raw files, we can just load 'filesize' bytes
     * Some formats have code entry point, initialise stack/registers
     *	...our code is always in low RAM addresses
     *	...our stack always grows downward from RAMTOP (64K)
     *	...we might want to set ROM/RAM split based on arguments
     */

    return fprintf(stderr, "%s(): exec %s: PARTIAL - load routines absent\n", __func__, filename);
}
