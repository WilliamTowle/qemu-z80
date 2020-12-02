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
#if 1   /* WmT - TRACE */
;DPRINTF( "%s(): PARTIAL - empty infinite loop (TODO: cpu_exec() call missing) follows\n", __func__);
#endif
    for(;;) {
        /* TODO: v5 implementation for x86 has
         * 1. cpu_exec_start(), cpu_exec(), cpu_exec_end() calls
         * 2. possible process_queued_cpu_work()
         * 3. handling the 'trapnr' returned by cpu_exec()
         * 4. calls to process_{queued_cpu_work|pending_signals}()
         */
    }
}
