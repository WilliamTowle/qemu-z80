/* bblbrx - barebones layer for binary execution */

#ifndef QEMU_H
#define QEMU_H

#include "cpu.h"

#include "qemu-types.h"

struct bblbrx_binprm {
	const char	*filename;
	int		fd;
	long		filesize;
};

int bblbrx_exec(const char *filename);

int load_raw_binary(struct bblbrx_binprm *bprm);

#endif /* QEMU_H */
