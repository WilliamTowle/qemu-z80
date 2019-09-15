/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle, under GPL...]
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "qemu.h"

static int prepare_binprm(struct bblbrx_binprm *bprm)
{
    struct stat	st;

    /* values in case of error */
    bprm->filesize= 0;
    bprm->magic_ramloc= 0;

    /* Check the file exists and is valid (only requires a regular
     * file; our binaries do not need to be executable as far as
     * the host OS is concerned). *If* we target_mmap() later, the
     * file's permissions are relevant too
     */
    if (fstat(bprm->fd, &st) < 0)
    {
        printf("%s(): fstat() failed on %s\n", __func__, bprm->filename);
        return -errno;
    }
    if (!S_ISREG(st.st_mode))
    {
        return -EACCES;
    }

    bprm->filesize= st.st_size;
    bprm->magic_ramloc= 0xfffe;

    return 0;
}

/* handle raw binary loading */
int bblbrx_exec(const char *filename, struct bblbrx_binprm *bprm)
{
    int				ret;

    ret= open(filename, O_RDONLY);
    if (ret < 0)
    {
        printf("%s(): open() failed on %s\n", __func__, filename);
        return -errno;
    }

    bprm->filename= filename;
    bprm->fd= ret;		/* for subsequent fstat() */

    ret= prepare_binprm(bprm);
    if (ret >= 0)
        ret= load_raw_binary(bprm);

    /* If loading something failed, close the file descriptor;
     * otherwise, a future platform could swap/write to and from it
     */
    if (ret <= 0)
    {
        close(ret);
        bprm->fd= -1;
    }
    return ret;
}
