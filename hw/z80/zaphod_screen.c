/*
 * QEmu Zaphod board - screen support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "qemu/osdep.h"
#include "zaphod.h"

#include "hw/hw.h"
#include "ui/console.h"


/* Implements a monochrome display at 25x80 text resolution (Phil
 * Brown calls his Amiga emulator screen "medium-res two-colour
 * 24x80" [Amiga is usually 25x80 at 640x200 for NTSC and 32x80 at
 * 640x256 for PAL].
 * Skeleton support for Grant Searle's ATmega328 per-line attributes
 * (normally bold/size/graphics) is present. For graphics, each byte
 * of the line maps to a 2x4 pixel subgroup of all 160x100 pixels
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

#define ZAPHOD_TEXT_CURSOR_PERIOD_MS       (1000 * 2 * 16 / 60)


uint8_t zaphod_rgb_palette[][3]= {
	{ 0x00, 0x00, 0x00 },	/* background - black */
	{ 0x05, 0xf3, 0x05 },	/* foreground - green */
	{ 0xff, 0x91, 0x00 }	/* foreground - amber */
};


#ifdef ZAPHOD_HAS_KEYBIO
/* EXPERIMENTAL */
static const unsigned char keycode_to_asciilc[128]= {
    /* keymap for UK QWERTY keyboard - NB. repo.or.cz uses its
     * callback to translate keycode to row and column [to suit the
     * ZX Spectrum's keyboard electronics]. When feeding input to
     * our stdio or ACIA input streams, we want ASCII.
     * (TODO: this is (unintentionally) partial, and the handler
     * (also) lacks code to sense/track/apply modifier keys)
     */
      0,  0,'1','2','3','4','5','6',
    '7','8','9','0',  0,  0,  0,  0,
    'q','w','e','r','t','y','u','i',
    'o','p',  0,  0, 13,  0,'a','s',
    'd','f','g','h','j','k','l',  0,
      0,  0,  0,  0,'z','x','c','v',
    'b','n','m',  0,  0,  0,  0,  0,
      0,' ',  0,  0,  0,  0,  0,  0,

      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,

      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
};


static void zaphod_put_keycode(void *opaque, int keycode)
{
    ZaphodScreenState	*zss= (ZaphodScreenState *)opaque;
    int	release= keycode & 0x80;

    if (release)
    {
        /* FIXME: handle XT scancode 0xe0 properly */
        /* TODO: resetting cs_inkey on key release is risky if
         * systems without an IRQ sent on keypress don't poll
         * the port promptly enough. Do we want to implement key
         * repeat (in which case we start a timer elsewhere that
         * should get stopped here)?
         */
        zaphod_set_inkey(zss->super, 0, false);
    }
    else
    {
      int	ch= keycode_to_asciilc[keycode & 0x7f];
            zaphod_set_inkey(zss->super, ch, (ch != 0)?true : false);
#ifdef ZAPHOD_DEBUG
;DPRINTF("DEBUG: %s() stored keycode %d to inkey as ch=%02x\n", __func__, keycode, ch);
#endif
#ifdef ZAPHOD_HAS_RXINT_IRQ
        if (zss->rxint_irq)
            qemu_irq_raise(*zss->rxint_irq);
#endif
    }
}
#endif	/* ZAPHOD_HAS_KEYBIO */


static
void zaphod_screen_draw_char(void *opaque, int row, int col, char ch)
{
    ZaphodScreenState   *zss= (ZaphodScreenState *)opaque;
    DisplaySurface      *surface = qemu_console_surface(zss->display);
    int                 bypp= (surface_bits_per_pixel(surface) + 7) >> 3;
    int                 r_incr, c_incr, ix;
    uint8_t             *dmem_start;
    const uint8_t       *font_ptr;

    /* TODO: logic to bail on unexpected state here?
     * - if is_graphic_console() is false (ie. HMI/serial console shown)
     * [...this possibility looks like a bug, now fixed?]
       if (!is_graphic_console())
       {       /X* Avoid rendering onto the HMI/serial console *X/
               return;
       }
     * - if bypp is unexpected (since we assume BGRA format):
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
}

static
void zaphod_screen_draw_graphic(void *opaque, int row, int col, uint8_t data)
{
    ZaphodScreenState  *zss= (ZaphodScreenState *)opaque;
    DisplaySurface *surface = qemu_console_surface(zss->display);
    int                 bypp= (surface_bits_per_pixel(surface) + 7) >> 3;
    int                 r_incr, c_incr, ix;
    uint8_t             *dmem_start;

    /* TODO: bail on unexpected state? see zaphod_screen_draw_char() */

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
}

static void zaphod_screen_redraw_row(ZaphodScreenState *zss,
                            int row, int minc, int maxc)
{
    int col;

    /* TODO: Assume a fixed display size for now (ie. omit attribute
     * support until later). Grant Searle documents attributes as
     * follows:
     * - 0x80: graphics characters (bit 0 top left, bit 7 bottom right)
     * - 0x04: double height (internally, top half/bottom half)
     * - 0x02: bold
     * - 0x01: use 80 chars (40 otherwise)
     */

;DPRINTF("INFO: Reached %s(), with row=%d/col=%d...%d\n", __func__, row, minc, maxc);
    for (col= minc; col <= maxc; col++)
        switch(zss->row_attr[row])
        {
        case ZAPHOD_SCREEN_ATTR_80COL:
            zaphod_screen_draw_char(zss, row, col, zss->char_grid[row][col]);
            break;
        case ZAPHOD_SCREEN_ATTR_GRAPH:

            zaphod_screen_draw_graphic(zss, row, col, zss->char_grid[row][col]);
            break;
        /* INCOMPLETE */
        }
}


static void zaphod_screen_invalidate_display(void *opaque)
{
    ZaphodScreenState  *zss= (ZaphodScreenState *)opaque;
    /* Implement full redraw of the window.
     * Earlier content from our DisplayState is no longer visible
     * for some reason. Set state to trigger full update in
     * zaphod_screen_update_display()
     */
    zss->dirty_minr= zss->dirty_minc= 0;
    zss->dirty_maxr= ZAPHOD_TEXT_ROWS-1;
    zss->dirty_maxc= ZAPHOD_TEXT_COLS-1;
}

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
     * window resize? Later QEmu versions scale the window)
     *   vga_update_display() does:
     *   - nothing if surface_bits_per_pixel(surface) == 0
     *   - sets 'full_update' if graphic mode changes (this resets blink timer)
     *   - passes 'full_update' to vga_draw_{text|graphic|blank}()
     */

    if (zss->dirty_minr > -1)
    {
        int row;

        /* FIXME: calls to the repaint functions for the dirty
         * region belong here [ie. immediately prior to our
         * dpy_update()]; we risk blanking the screen otherwise. If
         * this obliterates the cursor we may need to repaint it too
         */
        for (row= zss->dirty_minr; row <= zss->dirty_maxr; row++)
            zaphod_screen_redraw_row(zss, row,
                                zss->dirty_minc, zss->dirty_maxc);

        dpy_gfx_update(zss->display,
                zss->dirty_minc * FONT_WIDTH,
                zss->dirty_minr * FONT_HEIGHT,
                (zss->dirty_maxc - zss->dirty_minc + 1) * FONT_WIDTH,
                (zss->dirty_maxr - zss->dirty_minr + 1) * FONT_HEIGHT
                );
        if ( (zss->curs_posr >= zss->dirty_minr)
            && (zss->curs_posr <= zss->dirty_maxr)
            && (zss->curs_posc >= zss->dirty_minc)
            && (zss->curs_posc <= zss->dirty_maxc)
            )
        {
            zss->curs_dirty|= zss->cursor_visible;    /* was erased? */
        }

        zss->dirty_minr= zss->dirty_maxr= -1;
        zss->dirty_minc= zss->dirty_maxc= -1;
    }

#if 1
    /* Handle cursor blink. If redrawing the screen blanked its
     * location we will also need to make it reappear.
     */
    {
        int64_t now= qemu_clock_get_ms(QEMU_CLOCK_REALTIME);

        if (now >= zss->cursor_blink_time)
        {
            zss->cursor_blink_time= now + ZAPHOD_TEXT_CURSOR_PERIOD_MS / 2;
            zss->cursor_visible= !zss->cursor_visible;
            zss->curs_dirty= true;
        }
    }

    if (zss->curs_dirty)
    {
        /* cursor moved, was erased, or changed visibility status */
        DisplaySurface *surface = qemu_console_surface(zss->display);
        int       bypp= (surface_bits_per_pixel(surface) + 7) >> 3;
        uint8_t *dmem;
        int ix, iy;

        dmem= surface_data(surface);
        dmem+= zss->curs_posc * FONT_WIDTH * bypp;
        dmem+= zss->curs_posr * FONT_HEIGHT * surface_stride(surface);

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

            dpy_gfx_update(zss->display,
                zss->curs_posc * FONT_WIDTH,
                zss->curs_posr * FONT_HEIGHT,
                FONT_WIDTH, FONT_HEIGHT);
        zss->curs_dirty= false;
    }
#endif
}


static const GraphicHwOps zaphod_screen_ops= {
    .invalidate     = zaphod_screen_invalidate_display,
    .gfx_update     = zaphod_screen_update_display,
};


void zaphod_screen_putchar(void *opaque, uint8_t ch)
{
    ZaphodScreenState *zss= opaque;

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
#if 0   /* HACK */
    if (zaphod_has_feature(zss->super, ZAPHOD_SIMPLE_SCREEN))
    {
        uint8_t nyb_hi, nyb_lo;

        nyb_hi= (ch & 0xf0) >> 4;
        nyb_lo= ch & 0x0f;
        nyb_hi+= (nyb_hi > 9)? 'A' - 10 : '0';
        nyb_lo+= (nyb_lo > 9)? 'A' - 10 : '0';

        zss->dirty_minr= zss->dirty_minc= 0;
        zss->char_grid[0][0]= nyb_hi;
        zss->char_grid[0][1]= nyb_lo;

        /* fall through for 'ch' at 0,2 */
        zss->curs_posr= 0;
        zss->curs_posc= 2;
    }
    else
    {
        uint8_t nyb_hi, nyb_lo;

        nyb_hi= (ch & 0xf0) >> 4;
        nyb_lo= ch & 0x0f;
        nyb_hi+= (nyb_hi > 9)? 'A' - 10 : '0';
        nyb_lo+= (nyb_lo > 9)? 'A' - 10 : '0';

        /* graphics not included in automatic redraw :( */
        zaphod_screen_draw_graphic(zss, ZAPHOD_TEXT_ROWS-1,0, ch);
        dpy_gfx_update(zss->display,
                0, (ZAPHOD_TEXT_ROWS - 1) * FONT_HEIGHT,
                FONT_WIDTH, FONT_HEIGHT);

        zss->dirty_minr= zss->dirty_minc= 0;
        zss->char_grid[0][0]= nyb_hi;

        /* fall through for 'nyb_lo' at 0,1 */
        ch= nyb_lo;
        zss->curs_posr= 0;
        zss->curs_posc= 1;
    }
#endif

    /* update grid and state for dirty region */
DPRINTF("INFO: Reached putchar for ch=0x%02x at r=%d,c=%d\n", ch, zss->curs_posr, zss->curs_posc);
    zss->row_attr[zss->curs_posr]= ZAPHOD_SCREEN_ATTR_80COL;
    zss->char_grid[zss->curs_posr][zss->curs_posc]= ch;
    if (zss->dirty_maxr < zss->curs_posr)
        zss->dirty_maxr= zss->curs_posr;
    if (zss->dirty_maxc < zss->curs_posc)
        zss->dirty_maxc= zss->curs_posc;
    if ((zss->dirty_minr > zss->curs_posr) || (zss->dirty_minr == -1))
        zss->dirty_minr= zss->curs_posr;
    if ((zss->dirty_minc > zss->curs_posc) || (zss->dirty_minc == -1))
        zss->dirty_minc= zss->curs_posc;
DPRINTF("INFO: Screen dirty from %d,%d to %d,%d\n", zss->dirty_minr, zss->dirty_minc, zss->dirty_maxr, zss->dirty_maxc);

    /* move cursor and ensure it gets redrawn */
    if (++zss->curs_posc == ZAPHOD_TEXT_COLS)
    {
        zss->curs_posc= 0;
        if (++zss->curs_posr == ZAPHOD_TEXT_ROWS)
        {
            int row, col;

            /* Display full - scroll and repaint it */
            for (col= 0; col < ZAPHOD_TEXT_COLS; col++)
            {
                for (row= 0; row < ZAPHOD_TEXT_ROWS - 1; row++)
                    zss->char_grid[row][col]= zss->char_grid[row+1][col];
                zss->char_grid[ZAPHOD_TEXT_ROWS-1][col]= 0;
            }

            zss->curs_posr= ZAPHOD_TEXT_ROWS-1;
            zaphod_screen_invalidate_display(zss);
        }
    }
}


DeviceState *zaphod_screen_new(ZaphodState *super)
{
    DeviceState         *dev= DEVICE(object_new(TYPE_ZAPHOD_SCREEN));
    ZaphodScreenState   *zss= ZAPHOD_SCREEN(dev);

    zss->super= super;

    //qdev_prop_set_chr(dev, "chardev", chr);

    qdev_init_nofail(dev);
    return dev;
}

static void zaphod_screen_reset(void *opaque)
{
    ZaphodScreenState *zss= ZAPHOD_SCREEN(opaque);
    int row, col;

    zss->curs_posr= zss->curs_posc= 0;
    zss->cursor_visible= zss->curs_dirty= false;
    zss->cursor_blink_time= 0;

    zss->dirty_minr= zss->dirty_maxr= -1;
    zss->dirty_minc= zss->dirty_maxc= -1;

    /* TODO: "screen clear" escape should reset everything too */
    for (row= 0; row < ZAPHOD_TEXT_ROWS; row++)
    {
        zss->row_attr[row]= ZAPHOD_SCREEN_ATTR_80COL;
        for (col= 0; col < ZAPHOD_TEXT_COLS; col++)
            zss->char_grid[row][col]= '\0';
    }
}


static void zaphod_screen_realizefn(DeviceState *dev, Error **errp)
{
    ZaphodScreenState *zss= ZAPHOD_SCREEN(dev);

    /* NB. our text mode is essentially VGA-like; is QEmu's
     * common display code useful?
     */
    zss->display= graphic_console_init(
                NULL,	/* no ISA bus bus to emulate */
                &zaphod_screen_ops,
                zss);

    /* Distinguish machine type by text color */
    zss->rgb_bg= zaphod_rgb_palette[0];
    if (zaphod_has_feature(zss->super, ZAPHOD_SIMPLE_SCREEN))
        zss->rgb_fg= zaphod_rgb_palette[1];
    else
        zss->rgb_fg= zaphod_rgb_palette[2];

    zss->rxint_irq= qemu_allocate_irqs(zaphod_interrupt, zss->super, 1);

    qemu_console_resize(zss->display,
        FONT_WIDTH * ZAPHOD_TEXT_COLS, FONT_HEIGHT * ZAPHOD_TEXT_ROWS);

#ifdef ZAPHOD_HAS_KEYBIO
    /* provide keycode to ASCII translation for zaphod_io_read() */
    qemu_add_kbd_event_handler(zaphod_put_keycode, zss);
#endif	/* ZAPHOD_HAS_KEYBIO */

    qemu_register_reset(zaphod_screen_reset, zss);
}

static void zaphod_screen_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = zaphod_screen_realizefn;
#if 0  /* TODO: v1 init support */
    dc->props = zaphod_screen_properties;
#endif
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static void zaphod_screen_instance_init(Object *obj)
{
    //ZaphodScreenState *zms= ZAPHOD_SCREEN(obj);

    /* NB. see zaphod_screen_reset() */
}


static const TypeInfo zaphod_screen_info= {
	.name= TYPE_ZAPHOD_SCREEN,
	.parent= TYPE_DEVICE,
	/* For ZaphodScreenClass with virtual functions:
	.class_size= sizeof(ZaphodScreenClass),
	 */
	.class_init= zaphod_screen_class_init,
	.instance_size= sizeof(ZaphodScreenState),
	.instance_init= zaphod_screen_instance_init
};

static void zaphod_screen_register_types(void)
{
	type_register_static(&zaphod_screen_info);
}

type_init(zaphod_screen_register_types)
