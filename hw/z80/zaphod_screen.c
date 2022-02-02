/* "Zaphod" Z80 machine family for QEmu
 *
 * Support for screen emulation
 *
 * Wm. Towle c. 2013-2021
 */

#include "zaphod.h"

#ifdef ZAPHOD_DEBUG
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif


ZaphodScreenState *zaphod_new_screen(void)
{
    ZaphodScreenState *zss= NULL; //g_new(ZaphodScreenState, 1);

;DPRINTF("[%s:%d] Reached UNIMPLEMENTED %s()\n", __FILE__, __LINE__, __func__);

    return zss;
}
