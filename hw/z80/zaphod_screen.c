/*
 * QEmu Zaphod board - screen support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "zaphod.h"

#include "qemu/error-report.h"
#include "hw/qdev-properties.h"
#include "ui/console.h"


/* Implements a monochrome display at 25x80 text resolution (Phil
 * Brown calls his Amiga emulator screen "medium-res two-colour
 * 24x80" [Amiga is usually 25x80 at 640x200 for NTSC and 32x80 at
 * 640x256 for PAL]).
 * Skeleton support for Grant Searle's ATmega328 per-line attributes
 * (normally bold/size/graphics) is present. For graphics, each byte
 * of the line maps to a 2x4 pixel subgroup of all 160x100 pixels
 */

//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("zaphod_screen: " fmt , ## __VA_ARGS__); } while(0)


#define ZAPHOD_TEXT_CURSOR_PERIOD_MS       (1000 * 2 * 16 / 60)

#include "ui/vgafont.h"           /* vgafont16 - 16x8 */
#define FONT_HEIGHT    16
#define FONT_WIDTH     8


uint8_t zaphod_rgb_palette[][3]= {
    { 0x00, 0x00, 0x00 },   /* background - black */
    { 0x05, 0xf3, 0x05 },   /* foreground - green */
    { 0xff, 0x91, 0x00 }    /* foreground - amber */
};


static
void zaphod_screen_toggle_cursor(void *opaque, int row, int col)
{
    ZaphodScreenState   *zss= ZAPHOD_SCREEN(opaque);
    DisplaySurface      *ds = qemu_console_surface(zss->display);
    int                 bypp= (surface_bits_per_pixel(ds) + 7) >> 3;
    uint8_t             *dmem;
    int                 ix, iy;

    dmem= surface_data(ds);
    dmem+= col * FONT_WIDTH * bypp;
    dmem+= row * FONT_HEIGHT * surface_stride(ds);

    for (ix= 0; ix < FONT_HEIGHT; ix++)
    {
        /* for bypp = 4 */
        for (iy= 0; iy < FONT_WIDTH * bypp; iy+= bypp)
        {
            *(dmem + iy)^= zss->rgb_bg[2] ^ zss->rgb_fg[2];
            *(dmem + iy+1)^= zss->rgb_bg[1] ^ zss->rgb_fg[1];
            *(dmem + iy+2)^= zss->rgb_bg[0] ^ zss->rgb_fg[0];
        }
        dmem+= surface_stride(ds);
    }

    /* update display to redraw cursor in given location */
    dpy_gfx_update(zss->display,
            col * FONT_WIDTH,
            row * FONT_HEIGHT,
            FONT_WIDTH, FONT_HEIGHT);
}

static
void zaphod_screen_draw_char(void *opaque, int row, int col, char ch)
{
    ZaphodScreenState   *zss= ZAPHOD_SCREEN(opaque);
    DisplaySurface      *ds = qemu_console_surface(zss->display);
    int                 bypp= (surface_bits_per_pixel(ds) + 7) >> 3;
    int                 r_incr, c_incr, ix;
    uint8_t             *dmem_start;
    const uint8_t       *font_ptr;

    /* Render lines of regular text using QEmu's VGA font.
     * TODO: needs attribute (double height/width, bold) support here
     */

    c_incr= FONT_WIDTH * bypp;
    r_incr= FONT_HEIGHT * surface_stride(ds);

    dmem_start= surface_data(ds);
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
        dmem_start+= surface_stride(ds);
    }
}

static
void zaphod_screen_draw_graphic(void *opaque, int row, int col, uint8_t data)
{
    ZaphodScreenState   *zss= ZAPHOD_SCREEN(opaque);
    DisplaySurface      *ds = qemu_console_surface(zss->display);
    int                 bypp= (surface_bits_per_pixel(ds) + 7) >> 3;
    int                 r_incr, c_incr, ix;
    uint8_t             *dmem_start;

    /* For screen lines marked with ZAPHOD_SCREEN_ATTR_GRAPH.
     * The byte for each cell represents a 2x4 pixel block in which
     * bit 0 is top left and bit 7 bottom right)
     */

    c_incr= FONT_WIDTH * bypp;
    r_incr= FONT_HEIGHT * surface_stride(ds);

    dmem_start= surface_data(ds);
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

        dmem_start+= surface_stride(ds);
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
     * - 0x04: double height (has top half/bottom half internally)
     * - 0x02: bold
     * - 0x01: use 80 chars (40 otherwise)
     */

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

    /* set state to trigger full update later */
    zss->dirty_minr= zss->dirty_minc= 0;
    zss->dirty_maxr= ZAPHOD_TEXT_ROWS-1;
    zss->dirty_maxc= ZAPHOD_TEXT_COLS-1;
}

static void zaphod_screen_update_display(void *opaque)
{
    ZaphodScreenState *zss= ZAPHOD_SCREEN(opaque);
    int64_t now;

    /* align QEmu window content with the simulated display */

    if (zss->dirty_minr > -1)
    {
        int row;

        /* Update the display surface where "dirty" region applies.
         * If this obliterates the cursor we repaint it too.
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

        /* prepare to redraw cursor if it was erased? */
        if ( (zss->curs_posr >= zss->dirty_minr)
            && (zss->curs_posr <= zss->dirty_maxr)
            && (zss->curs_posc >= zss->dirty_minc)
            && (zss->curs_posc <= zss->dirty_maxc)
            )
        {
            zss->cursor_dirty|= zss->cursor_visible;
        }

        zss->dirty_minr= zss->dirty_maxr= -1;
        zss->dirty_minc= zss->dirty_maxc= -1;
    }


    /* Handle cursor blink if its timer expired */

    //now = qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL);
    now = qemu_clock_get_ms(QEMU_CLOCK_REALTIME);

    if (now >= zss->cursor_blink_time)
    {
        zss->cursor_blink_time= now + ZAPHOD_TEXT_CURSOR_PERIOD_MS / 2;
        zss->cursor_visible= !zss->cursor_visible;

        /* Adjust 'cursor_dirty' state here so the "blink" step is
         * performed as needed - ie. if the cursor is:
         *   - in non-dirty state? always trigger state change next
         *   - dirty (was just erased)? unset flag [do nothing below]
         */
        zss->cursor_dirty^= !zss->cursor_dirty || !zss->cursor_visible;
    }

    if (zss->cursor_dirty)
    {
        /* cursor moved, was erased, or changed visibility status */
        zaphod_screen_toggle_cursor(opaque,
                             zss->curs_posr, zss->curs_posc);
        zss->cursor_dirty= false;
    }
}

static const GraphicHwOps zaphod_screen_ops= {
    .invalidate     = zaphod_screen_invalidate_display,
    .gfx_update     = zaphod_screen_update_display,
};


/* character processing */

static void zaphod_screen_scroll(void *opaque)
{
    ZaphodScreenState *zss= (ZaphodScreenState *)opaque;
    int row, col;

    /* Display full - scroll */
    for (col= 0; col < ZAPHOD_TEXT_COLS; col++)
    {
        for (row= 0; row < ZAPHOD_TEXT_ROWS - 1; row++)
            zss->char_grid[row][col]= zss->char_grid[row+1][col];
        zss->char_grid[ZAPHOD_TEXT_ROWS-1][col]= 0;
    }

    /* Adjust attributes - duplicating last row's */
    for (row= 0; row < ZAPHOD_TEXT_ROWS - 1; row++)
        zss->row_attr[row]= zss->row_attr[row+1];

    zaphod_screen_invalidate_display(zss);
}

static void zaphod_screen_mark_dirty(void *opaque, int r,int c)
{
    ZaphodScreenState *zss= (ZaphodScreenState *)opaque;

//    if (zss->dirty_maxr < r)
//        zss->dirty_maxr= r;
//    if (zss->dirty_maxc < c)
//        zss->dirty_maxc= c;
//    if ((zss->dirty_minr > r) || (zss->dirty_minr == -1))
//        zss->dirty_minr= r;
//    if ((zss->dirty_minc > c) || (zss->dirty_minc == -1))
//        zss->dirty_minc= c;
    if (zss->dirty_maxr < zss->curs_posr)
        zss->dirty_maxr= zss->curs_posr;
    if (zss->dirty_maxc < zss->curs_posc)
        zss->dirty_maxc= zss->curs_posc;
    if ((zss->dirty_minr > zss->curs_posr) || (zss->dirty_minr == -1))
        zss->dirty_minr= zss->curs_posr;
    if ((zss->dirty_minc > zss->curs_posc) || (zss->dirty_minc == -1))
        zss->dirty_minc= zss->curs_posc;
}

static void zaphod_screen_clear(ZaphodScreenState *zss)
{
  int row, col;
    for (row= 0; row < ZAPHOD_TEXT_ROWS; row++)
	{
        zss->row_attr[row]= ZAPHOD_SCREEN_ATTR_80COL;
        for (col= 0; col < ZAPHOD_TEXT_COLS; col++)
            zss->char_grid[row][col]= '\0';
    }
    zss->curs_posr= zss->curs_posc= 0;
}

void zaphod_screen_putchar(void *opaque, uint8_t ch)
{
    /* this 'opaque' is the MachineState from zaphod_io_write() */
    ZaphodMachineState *zms= ZAPHOD_MACHINE(opaque);
    ZaphodScreenState  *zss= zms->screen;

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
    switch(ch)
    {
    case '\a':  /* BEL (bell, 0x07) */
        /* ignore */
        return;
    case '\b':  /* BS (backspace, 0x08) */
        if (zss->cursor_visible)
            zaphod_screen_mark_dirty(zss, zss->curs_posr, zss->curs_posc);
        if (zss->curs_posc > 0)
            zss->curs_posc--;
        zss->cursor_dirty|= zss->cursor_visible;
        return;
    case '\r':  /* CR (carriage return, 0x0D) */
        if (zss->cursor_visible)
            zaphod_screen_mark_dirty(zss, zss->curs_posr, zss->curs_posc);
        zss->curs_posc= 0;
        zss->cursor_dirty|= zss->cursor_visible;
        return;
    case '\n':  /* NL (newline, 0x0A) */
        if (zss->cursor_visible)
            zaphod_screen_mark_dirty(zss, zss->curs_posr, zss->curs_posc);
        if (++zss->curs_posr == ZAPHOD_TEXT_ROWS)
        {
            zaphod_screen_scroll(zss);
            zss->curs_posr= ZAPHOD_TEXT_ROWS-1;
        }
        return;
    case '\f':  /* FF (formfeed, 0x0C) */
        if (zss->cursor_visible)
            zaphod_screen_mark_dirty(zss, zss->curs_posr, zss->curs_posc);
        zaphod_screen_clear(zss);
        zaphod_screen_mark_dirty(zss, 0,0);
        zaphod_screen_mark_dirty(zss, ZAPHOD_TEXT_ROWS-1,ZAPHOD_TEXT_COLS-1);
        return;
#if 1   /* HACK - reveal unhandled control codes */
    default:
        if (!isprint(ch))
        {
            uint8_t nyb_hi, nyb_lo;

            nyb_hi= (ch & 0xf0) >> 4;
            nyb_lo= ch & 0x0f;
            nyb_hi+= (nyb_hi > 9)? 'A' - 10 : '0';
            nyb_lo+= (nyb_lo > 9)? 'A' - 10 : '0';

            zaphod_screen_putchar(opaque, '[');
            zaphod_screen_putchar(opaque, nyb_hi);
            zaphod_screen_putchar(opaque, nyb_lo);
            zaphod_screen_putchar(opaque, ']');
            zaphod_screen_putchar(opaque, '*');
            return;
        }
#endif
    }

    /* update grid and state for dirty region */
    zss->row_attr[zss->curs_posr]= ZAPHOD_SCREEN_ATTR_80COL;
    zss->char_grid[zss->curs_posr][zss->curs_posc]= ch;
    zaphod_screen_mark_dirty(zss, zss->curs_posr, zss->curs_posc);

    /* TODO: if the cursor position changes when flagged visible,
     * mark it as dirty; the next update will put new text in its
     * old position but it will need rendering immediately where it
     * moved to
     */
    if (++zss->curs_posc == ZAPHOD_TEXT_COLS)
    {
        zss->curs_posc= 0;
        if (++zss->curs_posr == ZAPHOD_TEXT_ROWS)
        {
            zaphod_screen_scroll(zss);
            zss->curs_posr= ZAPHOD_TEXT_ROWS-1;
        }
    }
}


static void zaphod_screen_reset(void *opaque)
{
    ZaphodScreenState *zss= ZAPHOD_SCREEN(opaque);

    zaphod_screen_clear(zss);
    zss->cursor_visible= zss->cursor_dirty= false;
    zss->cursor_blink_time= 0;

    zss->dirty_minr= zss->dirty_maxr= -1;
    zss->dirty_minc= zss->dirty_maxc= -1;
}

static void zaphod_screen_realizefn(DeviceState *dev, Error **errp)
{
    ZaphodScreenState *zss= ZAPHOD_SCREEN(dev);

    /* NB. our text mode is essentially VGA-like; is QEmu's
     * common display code useful?
     */
    zss->display= graphic_console_init(
                        NULL,   /* no ISA bus to emulate */
                        0, &zaphod_screen_ops, zss);

    /* Set text color to distinguish screen type - the simple (green
     * on black) screen has limited escape code support only, whereas
     * the alternative (orange on black) has per-line character
     * attributes including "block mosaic" graphics character mode
     */
    zss->rgb_bg= zaphod_rgb_palette[0];
    if (object_property_get_bool(OBJECT(dev), "simple-escape-codes", NULL))
        zss->rgb_fg= zaphod_rgb_palette[1];
    else
        zss->rgb_fg= zaphod_rgb_palette[2];

    qemu_console_resize(zss->display,
                        FONT_WIDTH * ZAPHOD_TEXT_COLS,
                        FONT_HEIGHT * ZAPHOD_TEXT_ROWS);

    qemu_register_reset(zaphod_screen_reset, zss);
}


static Property zaphod_screen_properties[] = {
    /* Properties for device "zaphod-screen"
     * Can set with '-global zaphod-screen.NAME=VALUE'
     */
    DEFINE_PROP_BOOL("simple-escape-codes", ZaphodScreenState, simple_escape_codes, false),
    DEFINE_PROP_END_OF_LIST()
};

static void zaphod_screen_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc= DEVICE_CLASS(klass);

    dc->desc= "Zaphod screen device";
    dc->realize= zaphod_screen_realizefn;
    dc->props= zaphod_screen_properties;
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static void zaphod_screen_instance_init(Object *obj)
{
    /* Nothing to do - properties to be set on object init */
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
