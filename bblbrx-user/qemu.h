/* bblbrx - barebones layer for binary execution */

#ifndef QEMU_H
#define QEMU_H

#include "cpu.h"

#include "qemu-types.h"

typedef struct TaskState {
    int				used;
    struct bblbrx_binprm	*bprm;
} TaskState;	/* aligmnent is useful here, for the linux-user case */

struct bblbrx_binprm {
    const char			*filename;
    int				fd;
    long			filesize;
    target_ulong		magic_ramloc;
};

#define VERIFY_READ 0
#define VERIFY_WRITE 1 /* implies read access */

/* XXX: todo protect every memory access */
#define lock_user(x,a,b,c)	(void*)(x)
#define unlock_user(x,y,z)

int bblbrx_exec(const char *filename, struct bblbrx_binprm *bprm);

int load_raw_binary(struct bblbrx_binprm *bprm);

void cpu_loop(CPUState *env);

void mmap_lock(void);
void mmap_unlock(void);

void cpu_list_lock(void);
void cpu_list_unlock(void);

#endif /* QEMU_H */
