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


#include "ui/vgafont.h"		/* vgafont16 - 16x8 */
#define FONT_HEIGHT	16
#define FONT_WIDTH	8
#define TEXT_ROWS	25
#define TEXT_COLS	80

#define ZAPHOD_TEXT_CURSOR_PERIOD_MS       (1000 * 2 * 16 / 60)


uint8_t zaphod_rgb_palette[][3]= {
	{ 0x00, 0x00, 0x00 },	/* background - black */
	{ 0x05, 0xf3, 0x05 },	/* foreground - green */
	{ 0xff, 0x91, 0x00 }	/* foreground - amber */
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
                DisplaySurface *surface = qemu_console_surface(zss->screen_con);
                int       bypp= (surface_bits_per_pixel(surface) + 7) >> 3;
                uint8_t *dmem;
                int ix, iy;

                dmem= surface_data(surface);
                /* TODO: adjust dmem for cursor position */

                for (ix= 0; ix < FONT_HEIGHT; ix++)
                {
                    /* for bypp = 4 */
                    for (iy= 0; iy < FONT_WIDTH * bypp; iy+= bypp)
                    {
                        *(dmem + iy)^= zss->rgb_bg[2] ^ zss->rgb_fg[2];
                        *(dmem + iy+1)^= zss->rgb_bg[1] ^ zss->rgb_fg[1];
                        *(dmem + iy+2)^= zss->rgb_bg[0] ^ zss->rgb_fg[0];
                    }
                    dmem+= surface_stride(surface);
                }

                /* current update region is where the cursor is */
                dpy_gfx_update(zss->screen_con,
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

static
void zaphod_screen_draw_char(void *opaque, int row, int col, char ch)
{
    ZaphodScreenState   *zss= (ZaphodScreenState *)opaque;
    DisplaySurface      *surface = qemu_console_surface(zss->screen_con);
    int                 bypp= (surface_bits_per_pixel(surface) + 7) >> 3;
    int                 r_incr, c_incr, ix;
    uint8_t             *dmem_start;
    const uint8_t       *font_ptr;

    /* TODO: this function is for normal height/width characters.
     * The Grant Searle SBC has per-line text attributes as follows:
     * - 0x80: graphics characters (bit 0 top left, bit 7 bottom right)
     * - 0x04: double height (internally, top half/bottom half)
     * - 0x02: bold
     * - 0x01: use 80 chars (40 otherwise)
     */

    /* FIXME: bail on unexpected state:
     * if is_graphic_console() is false (ie. HMI/serial console shown)
     *  ...this possibility looks like a bug, now fixed
       if (!is_graphic_console())
       {       /X* Avoid rendering onto the HMI/serial console *X/
               return;
       }
     * if bypp is unexpected (since we assume BGRA format):
       if (bypp != 4)
       { /X* BAIL *X/ }
       else if (is_surface_bgr(zcs->ds->surface))
       { /X* BAIL *X/ }
     */

    c_incr= FONT_WIDTH * bypp;
    r_incr= FONT_HEIGHT * surface_stride(surface);

    dmem_start= surface_data(surface);
    dmem_start+= col * c_incr;
    dmem_start+= row * r_incr;
    font_ptr= vgafont16 + FONT_HEIGHT * ch;

    for (ix= 0; ix < FONT_HEIGHT; ix++)
    {
        uint8_t *dmem= dmem_start;
        uint8_t bitmask;

        for (bitmask= 0x80; bitmask; bitmask>>= 1)
        {
            /* for depth 32 - sets BGR+A */
            *(dmem++)=      (*font_ptr & bitmask)?
                      zss->rgb_fg[2] : zss->rgb_bg[2];
            *(dmem++)=      (*font_ptr & bitmask)?
                      zss->rgb_fg[1] : zss->rgb_bg[1];
            *(dmem++)=      (*font_ptr & bitmask)?
                      zss->rgb_fg[0] : zss->rgb_bg[0];
            *(dmem++)=      0;
        }

        font_ptr++;
        dmem_start+= surface_stride(surface);
    }

    /* TODO: mark updated coords as dirty and update in callback? */
    dpy_gfx_update(zss->screen_con,
                    col*FONT_WIDTH, row*FONT_HEIGHT,
                    FONT_WIDTH, FONT_HEIGHT);
}

static
void zaphod_screen_draw_graphic(void *opaque, int row, int col, uint8_t data)
{
    ZaphodScreenState  *zss= (ZaphodScreenState *)opaque;
    DisplaySurface      *surface = qemu_console_surface(zss->screen_con);
    int                 bypp= (surface_bits_per_pixel(surface) + 7) >> 3;
    int                 r_incr, c_incr, ix;
    uint8_t             *dmem_start;

    /* FIXME: bail on unexpected state:
     * if is_graphic_console() is false (ie. HMI/serial console shown)
     * if bypp is unexpected - we assume 4-byte BGRA format
     */

    c_incr= FONT_WIDTH * bypp;
    r_incr= FONT_HEIGHT * surface_stride(surface);

    dmem_start= surface_data(surface);
    dmem_start+= col * c_incr;
    dmem_start+= row * r_incr;

    for (ix= 0; ix < FONT_HEIGHT; ix++)
    {
        uint8_t *dmem= dmem_start;
        uint8_t line;
        uint8_t bitmask;

        line= (data & 0x01)? 0xf0 : 0x00;
        line+= (data & 0x02)? 0x0f : 0x00;

        for (bitmask= 0x80; bitmask; bitmask>>= 1)
        {
            /* for depth 32 - assumes BGR+A */
            *(dmem++)=      (line & bitmask)?
                      zss->rgb_fg[2] : zss->rgb_bg[2];
            *(dmem++)=      (line & bitmask)?
                      zss->rgb_fg[1] : zss->rgb_bg[1];
            *(dmem++)=      (line & bitmask)?
                      zss->rgb_fg[0] : zss->rgb_bg[0];
            *(dmem++)=      0;
        }

        dmem_start+= surface_stride(surface);
        if ((ix & 0x03) == 3) data>>= 2;
    }

    /* TODO: mark updated coords as dirty and update in callback? */
    dpy_gfx_update(zss->screen_con,
                    col*FONT_WIDTH, row*FONT_HEIGHT,
                    FONT_WIDTH, FONT_HEIGHT);
}

void zaphod_screen_putchar(void *opaque, uint8_t ch)
{
    ZaphodState *super= (ZaphodState *)((ZaphodScreenState *)opaque)->super;

    /* TODO: need to handle escape sequences sanely - Phil Brown's
     * documentation says "an OUT to port 1 will display the
     * appropriate character on the console screen" and lists
     * "escape codes" of:
     * - ESC 0 - to clear the console screen
     * - ESC 1 x y - to move the cursor position to x,y
     * - ESC 2 - to clear to end of line from the current position
     * Grant Searle has custom codes including changing [per-line]
     * attributes and set/unset/toggle pixels
     */
    if (zaphod_has_feature(super, ZAPHOD_SIMPLE_SCREEN))
    {
        uint8_t nyb_hi, nyb_lo;

        nyb_hi= (ch & 0xf0) >> 4;
        nyb_lo= ch & 0x0f;
        nyb_hi+= (nyb_hi > 9)? 'A' - 10 : '0';
        nyb_lo+= (nyb_lo > 9)? 'A' - 10 : '0';

        zaphod_screen_draw_char(opaque, 0,0, nyb_hi);
        zaphod_screen_draw_char(opaque, 0,1, nyb_lo);
        zaphod_screen_draw_char(opaque, 0,2, ch);
    }
    else
    {
        uint8_t nyb_hi, nyb_lo;

        nyb_hi= (ch & 0xf0) >> 4;
        nyb_lo= ch & 0x0f;
        nyb_hi+= (nyb_hi > 9)? 'A' - 10 : '0';
        nyb_lo+= (nyb_lo > 9)? 'A' - 10 : '0';

        zaphod_screen_draw_char(opaque, 0,0, nyb_hi);
        zaphod_screen_draw_char(opaque, 0,1, nyb_lo);
        zaphod_screen_draw_graphic(opaque, 0,2, ch);
    }

    /* TODO: In our earlier hack, this function *called itself*
     * multiple times to ensure only single printable characters
     * needed to be stored in the character grid at this point;
     * it was then possible to update the cursor position and
     * manage scrolling.
     * Having then noted the region where the grid is dirty, a
     * subsequent dpy_update() in zaphod_screen_update_display()
     * takes care of presentation
     */
}


static const GraphicHwOps zaphod_screen_ops= {
    .invalidate     = zaphod_screen_invalidate_display,
    .gfx_update     = zaphod_screen_update_display,
};

ZaphodScreenState *zaphod_new_screen(ZaphodState *super)
{
    ZaphodScreenState *zss= g_new(ZaphodScreenState, 1);

    zss->super= super;

	zss->screen_con= graphic_console_init(
		NULL,	/* no ISA bus bus to emulate */
		&zaphod_screen_ops,
		zss);

    /* TODO: calls to zaphod_consolegui_invalidate_display() (eg.
     * on switch to HMI and back) need to prompt a full redraw of
     * previous output
     */

    /* Distinguish machine type by text color */
    zss->rgb_bg= zaphod_rgb_palette[0];
    if (zaphod_has_feature(super, ZAPHOD_SIMPLE_SCREEN))
        zss->rgb_fg= zaphod_rgb_palette[1];
    else
        zss->rgb_fg= zaphod_rgb_palette[2];
    zss->curs_visible= 0;
    zss->curs_blink_time= 0;

	qemu_console_resize(zss->screen_con,
		FONT_WIDTH * TEXT_COLS, FONT_HEIGHT * TEXT_ROWS);

    return zss;
}
