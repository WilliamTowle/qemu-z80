/* bblbrx - barebones layer for binary execution */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "qemu.h"

static int prepare_binprm(struct bblbrx_binprm *bprm)
{
  struct stat	st;

	/* values in case of error */
	bprm->filesize= 0;

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

	/* TODO: memory model has/code needs:
	 *	1. code in low RAM addresses (pread()s needed)
	 *	2. stack grows downward from RAMTOP (at 64K)
	 *	3. magic RAMTOP will permit clean program exit
	 */

  return 0;
}

/* handle raw binary loading */
int bblbrx_exec(const char *filename)
{
  struct bblbrx_binprm	bprm;
  int		ret;

	ret= open(filename, O_RDONLY);
	if (ret < 0)
	{
		printf("%s(): open() failed on %s\n", __func__, filename);
		return -errno;
	}

	bprm.filename= filename;
	bprm.fd= ret;		/* for subsequent fstat() */

	ret= prepare_binprm(&bprm);
	if (ret >= 0)
		ret= load_raw_binary(&bprm);

	/* If loading something failed, close the file descriptor;
	 * otherwise, a future platform could swap/write to and from it
	 */
	if (ret <= 0)
	{
		close(ret);
		bprm.fd= -1;
	}
	return ret;
}
