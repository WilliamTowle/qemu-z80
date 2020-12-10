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
            break;      /* to exit() beyond switch */
        case EXCP_KERNEL_TRAP:
            /* "magic ramtop" reached - exit and show CPU state */
            printf("cpu_exec() encountered EXCP_KERNEL_TRAP (trapnr=%d) - stopping emulation\n", trapnr);
            break;
        default:
            printf("cpu_exec() exited abnormally (with unexpected trapnr=%d) - aborting emulation\n", trapnr);
        }

        /* PARTIAL:
         * For a system that can generate interrupts and signals
         * them here, process_pending_signals() is required here,
         * and we should restart the loop. Abnormal exit drops here
         */
#if 1   /* WmT - PARTIAL */
;DPRINTF("INFO: %s(): got 'trapnr' %d from cpu_exec() - state dump (env at %p) follows:\n", __func__, trapnr, env);
        /* TODO:
         * 'trapnr' tells us why translation has stopped. Our key
         * exception cases are EXCP_ILLOP (bad/unsupported
         * instruction) and KERNEL_TRAP (end of usermode program).
         */
        //cpu_dump_state(cs, stderr, fprintf, 0);
        cpu_dump_state(cs, stderr, 0);
;exit(1);
#else   /* TODO: as per target-i386? */
        process_pending_signals(env);
#endif
    }
}
