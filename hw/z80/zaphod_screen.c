/* "Zaphod" Z80 machine family for QEmu
 *
 * Support for screen emulation
 *
 * Wm. Towle c. 2013-2021
 */

#include "zaphod.h"

#include "ui/console.h"

/* Implements a CGI-like display at 25x80 text resolution [ie. one
 * more line than QEmu's serial console]. Phil Brown calls his
 * Amiga emulator screen "medium-res two-colour 24x80" [Amiga NTSC
 * is usually 25x80 at 640x200 and PAL 32x80 at 640x256]. Grant
 * Searle's 2KiB RAM ATmega328 design is 25x80 with per-line
 * attributes for bold/size/graphics effects, where in graphics mode
 * each character cell maps to a 2x4 grid [for 160x100 pixels]
 */

#ifdef ZAPHOD_DEBUG
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif


//#include "vgafont.h"		/* vgafont16 - 16x8 */
#define FONT_HEIGHT	16
#define FONT_WIDTH	8
#define TEXT_ROWS	25
#define TEXT_COLS	80

static void zaphod_screen_update_display(void *opaque)
{
;DPRINTF("[%s:%d] Reached UNIMPLEMENTED %s()\n", __FILE__, __LINE__, __func__);
}

static void zaphod_screen_invalidate_display(void *opaque)
{
;DPRINTF("[%s:%d] Reached UNIMPLEMENTED %s()\n", __FILE__, __LINE__, __func__);
}

static const GraphicHwOps zaphod_screen_ops= {
    .invalidate     = zaphod_screen_invalidate_display,
    .gfx_update     = zaphod_screen_update_display,
};

ZaphodScreenState *zaphod_new_screen(void)
{
    ZaphodScreenState *zss= g_new(ZaphodScreenState, 1);

	zss->ds= graphic_console_init(
		NULL,	/* no ISA bus bus to emulate */
		&zaphod_screen_ops,
		zss);
	/* TODO: calls to zaphod_consolegui_invalidate_display() (eg. when
	 * user switches to HMI and back) need to prompt a full redraw of
	 * previous output
	 */
	qemu_console_resize(zss->ds,
		FONT_WIDTH * TEXT_COLS, FONT_HEIGHT * TEXT_ROWS);

    return zss;
}
