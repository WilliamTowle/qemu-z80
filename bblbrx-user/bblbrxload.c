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

#if 1	/* debug */
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif


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
    {	/* Check for a regular file only - we don't also expect to
         * find a file mode that tells us this is executable
         */
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
        printf("%s(): open() failed on filename '%s'\n", __func__, filename);
        return -errno;
    }

    bprm->filename= filename;
    bprm->fd= ret;		/* for subsequent fstat() */

    ret= prepare_binprm(bprm);
    if (ret >= 0)
        ret= load_raw_binary(bprm);

    /* Don't store a file descriptor if loading failed */
    if (ret <= 0)
    {
        close(ret);
        bprm->fd= -1;
    }
    return ret;
}
