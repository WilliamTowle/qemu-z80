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


static int prepare_binprm(struct bblbrx_binprm *bprm)
{
    struct stat	st;

    /* values in case of error */
    bprm->filesize= 0;
    bprm->magic_ramloc= 0;

    if (fstat(bprm->fd, &st) < 0)
    {
        printf("%s(): fstat() failed on filename '%s'\n", __func__, bprm->filename);
        return -errno;
    }
    if (!S_ISREG(st.st_mode))
    {   /* Require a (readable) regular file. Whether the host
         * considers this to be executable is unimportant.
         */

        printf("%s(): %s not a regular file\n", __func__, bprm->filename);
        return -EACCES;
    }

    bprm->filesize= st.st_size;
    bprm->magic_ramloc= 0xfffe;

    return 0;
}

/* handle raw binary loading */
int bblbrx_exec(const char *filename, struct bblbrx_binprm *bprm)
{
    int                     ret;

    ret= open(filename, O_RDONLY);
    if (ret < 0)
    {
        printf("%s(): open() failed on filename '%s'\n", __func__, filename);
        return -errno;
    }

    bprm->filename= filename;
    bprm->fd= ret;		/* for subsequent fstat() */

    ret= prepare_binprm(bprm);
    if (ret >= 0)
        ret= load_raw_binary(bprm);

    /* Store an invalid file descriptor if loading failed */
    if (ret <= 0)
    {
        close(bprm->fd);
        bprm->fd= -1;
    }
    return ret;
}
