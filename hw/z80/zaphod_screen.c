/* "Zaphod" Z80 machine family for QEmu
 *
 * Support for screen emulation
 *
 * Wm. Towle c. 2013-2021
 */

#include "zaphod.h"

#include "ui/console.h"
//#include "qemu-timer.h"

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

#define ZAPHOD_TEXT_CURSOR_PERIOD_MS       (1000 * 2 * 16 / 60)


uint8_t zaphod_rgb_palette[][3]= {
	{ 0x00, 0x00, 0x00 },	/* background - black */
#if 0	/* TODO: board-specific foreground colour */
	{ 0x05, 0xf3, 0x05 }	/* foreground - green */
#else
	{ 0xff, 0x91, 0x00 }	/* foreground - amber */
#endif
};


static void zaphod_screen_update_display(void *opaque)
{
    ZaphodScreenState *zss= (ZaphodScreenState *)opaque;

    /* TODO:
     *   Previously we considered the display surface dirty if flagged
     * as invalid [through the _invalidate_display() callback] or if
     * executed code wrote new text somewhere.
     *   Upstream code at repo.or.cz compares stored s->twidth and
     * s->theight values to latest ds_get_{width|height}() values
     * and forces qemu_console_resize() if different (prevents user
     * window resize? Later QEmu versions scale the window
     *   vga_update_display() does:
     *   - nothing if surface_bits_per_pixel(surface) == 0
     *   - sets 'full_update' if graphic mode changes (this resets blink timer)
     *   - passes 'full_update' to vga_draw_{text|graphic|blank}()
     */

#if 1
    {
        int64_t now= qemu_clock_get_ms(QEMU_CLOCK_REALTIME);

        if (now >= zss->curs_blink_time)
        {
            zss->curs_blink_time= now + ZAPHOD_TEXT_CURSOR_PERIOD_MS / 2;
            zss->curs_visible= !zss->curs_visible;
            DPRINTF("INFO: Cursor visible -> %s\n", zss->curs_visible?"ON":"OFF");

            /* since visibility changed - was zaphod_consolegui_blink_cursor() */
            {
                int       bypp= (ds_get_bits_per_pixel(zss->ds) + 7) >> 3;
                uint8_t *dmem;
                int ix, iy;

                dmem= ds_get_data(zss->ds);
                /* TODO: adjust dmem for cursor position */

                for (ix= 0; ix < FONT_HEIGHT; ix++)
                {
                    /* for bypp = 4 */
                    for (iy= 0; iy < FONT_WIDTH * bypp; iy+= bypp)
                    {
                        *(dmem + iy)^= zaphod_rgb_palette[0][2] ^ zaphod_rgb_palette[1][2];
                        *(dmem + iy+1)^= zaphod_rgb_palette[0][1] ^ zaphod_rgb_palette[1][1];
                        *(dmem + iy+2)^= zaphod_rgb_palette[0][0] ^ zaphod_rgb_palette[1][0];
                    }
                    dmem+= ds_get_linesize(zss->ds);
                }

                /* current update region is where the cursor is */
                dpy_update(zss->ds,
                        0, 0,                       /* ulx, uly */
                        FONT_WIDTH, FONT_HEIGHT);   /* xsz, ysz */
            }
        }
    }
#endif
}

static void zaphod_screen_invalidate_display(void *opaque)
{
;DPRINTF("[%s:%d] Reached UNIMPLEMENTED %s()\n", __FILE__, __LINE__, __func__);
    /* FIXME:
     * Calls here mean the window needs to be fully redrawn as
     * earlier content from our DisplayState is no longer visible
     * for some reason. This is particularly noticeable if QEmu
     * "suddenly" [? bug?] applies scaling to everything, at which
     * point we need to do work to account for graphical damage the
     * HMI or serial console causes.
     * Given that our dpy_update() for the cursor seems to behave,
     * the buffer in zss->ds is intact and we just need to put
     * content back on screen here.
     */

    /* TODO: VGACommonState tracks 'last_{width|height}' and
     * sets both to -1 here [...which forces a full update in
     * vga_update_text()]
     */
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

    /* TODO: calls to zaphod_consolegui_invalidate_display() (eg.
     * on switch to HMI and back) need to prompt a full redraw of
     * previous output
     */

    zss->curs_visible= 0;
    zss->curs_blink_time= 0;


	qemu_console_resize(zss->ds,
		FONT_WIDTH * TEXT_COLS, FONT_HEIGHT * TEXT_ROWS);

    return zss;
}
