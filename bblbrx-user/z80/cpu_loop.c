/*
 * QEmu user cpu loop
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle, under GPL...]
 */

#include "qemu/osdep.h"
#include "qemu.h"


#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("bblbrx-user cpu_loop: " fmt , ## __VA_ARGS__); } while(0)


void cpu_loop(CPUZ80State *env)
{
    CPUState *cs= env_cpu(env);
    int trapnr;

    for(;;) {
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: %s() calling cpu_exec_*()...\n", __func__);
#endif
        cpu_exec_start(cs);
        trapnr= cpu_exec(cs);
        cpu_exec_end(cs);
        //process_queued_cpu_work(cs);

        switch(trapnr)
        {
        case EXCP_ILLOP:
            /* instruction parser is incomplete - bailing is normal */
            printf("%s() encountered EXCP_ILLOP (trapnr=%d) - aborting emulation\n", __func__, trapnr);
            break;      /* to loop-exit 'break' */
        case EXCP_KERNEL_TRAP:
            /* "magic ramtop" reached - exit and show CPU state */
            printf("Program exit. Register dump follows:\n");
            break;      /* to loop-exit 'break' */
        default:
            printf("qemu: cpu_exec() returned unhandled exception 0x%x at PC=0x%04x - aborting emulation\n", trapnr, env->pc);
            abort();
        }

#if 0	/* target-i386: loop continues */
        process_pending_signals(env);
#else	/* z80: ILLOP (incomplete parser) or KERNEL_TRAP */
        //cpu_dump_state(cs, stderr, fprintf, 0);
        cpu_dump_state(cs, stderr, 0);
        break;	/* exit loop */
#endif
    }
}
