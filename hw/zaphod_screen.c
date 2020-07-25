/* "Zaphod" Z80 machine family for QEmu */
/* Wm. Towle c. 2013-2020 */

#include "hw/zaphod.h"

#ifdef ZAPHOD_HAS_CONSOLEGUI

#ifdef ZAPHOD_CONSOLE_CURSOR_BLINK
#include "qemu-timer.h"
#endif

#include "vgafont.h"		/* vgafont16 - 16x8 */
#define FONT_HEIGHT	16
#define FONT_WIDTH	8

uint8_t zaphod_rgb_palette[][3]= {
	{ 0x00, 0x00, 0x00 },	/* background - black */
#if 0	/* preference: green or amber screen? */
	{ 0xff, 0x91, 0x00 }	/* foreground - amber */
#else
	{ 0x05, 0xf3, 0x05 }	/* foreground - green */
#endif
};

static inline void zaphod_render_pixel8_32(uint8_t *d, int linesize,
			const uint8_t *glyph_ptr, int h)
{
	while (h--)
	{
	  uint8_t *destmem= d;
	  uint8_t bitmask;

	    for (bitmask= 0x80; bitmask; bitmask>>= 1)
	    {
		/* [for DEPTH=32, write 4 bytes (BGR+A) of fg/bg] */
		*destmem++= (*glyph_ptr & bitmask)?
			zaphod_rgb_palette[1][2] : zaphod_rgb_palette[0][2];
		*destmem++= (*glyph_ptr & bitmask)?
			zaphod_rgb_palette[1][1] : zaphod_rgb_palette[0][1];
		*destmem++= (*glyph_ptr & bitmask)?
			zaphod_rgb_palette[1][0] : zaphod_rgb_palette[0][0];
		*destmem++= 0x00;
	    }

	    glyph_ptr++;
	    d+= linesize;
	}
}

static
void zaphod_consolegui_drawchar(void *opaque, char ch, int row, int col)
{
  ZaphodConsoleState	*zcs= (ZaphodConsoleState *)opaque;
  int			bypp= ((ds_get_bits_per_pixel(zcs->ds) + 7) >> 3);
  const	uint8_t		*font_ptr;
  uint8_t		*dmem;
  int			r_incr, c_incr;

#ifdef ZAPHOD_DEBUG
//;fprintf(stderr, "DEBUG: Enter %s()\n", __func__);
#endif

	if (!is_graphic_console())
	{	/* Avoid rendering onto the HMI/serial console */
		return;
	}

	if (bypp != 4)
	{
		fprintf(stderr, "DEBUG: %s() found unexpected bypp %d\n", __func__, bypp);
	}
	else if (is_surface_bgr(zcs->ds->surface))
	{
		fprintf(stderr, "DEBUG: %s() found unexpected BGR surface\n", __func__);
	}

	c_incr= FONT_WIDTH * bypp;
	r_incr= FONT_HEIGHT * ds_get_linesize(zcs->ds);

	dmem= ds_get_data(zcs->ds);
	dmem+= col * c_incr;
	dmem+= row * r_incr;

#ifdef ZAPHOD_CONSOLE_TEST_COLOUR
{
  int			iy;
	for (iy= 0; iy < FONT_HEIGHT; iy++)
	{
	  int ix;
	    for (ix= 128; ix < 256; ix++)
		switch (ix % 4)
		{
		case 0:
			dmem[ix]= ix; break;	/* blue */
		case 1:
			dmem[ix]= ix; break;	/* green */
		case 2:
			dmem[ix]= ix; break;	/* red */
		case 3:
			dmem[ix]= 0x00; break;	/* alpha? */
		}

	    dmem+= ds_get_linesize(zcs->ds);
	}
}
#endif
	font_ptr= vgafont16 + FONT_HEIGHT * ch;
	zaphod_render_pixel8_32(dmem, ds_get_linesize(zcs->ds), font_ptr, FONT_HEIGHT);

#ifdef ZAPHOD_DEBUG
//;fprintf(stderr, "DEBUG: Exit %s()\n", __func__);
#endif
}

#ifdef ZAPHOD_CONSOLE_CURSOR_BLINK
static void zaphod_consolegui_blink_cursor(void *opaque)
{
  ZaphodConsoleState *zcs= opaque;
  uint8_t	*dmem;
  int		bypp= ((ds_get_bits_per_pixel(zcs->ds) + 7) >> 3);
  int		c_incr, r_incr;
  int		ix, iy;

	if (!is_graphic_console())
	{	/* Avoid rendering onto the HMI/serial console */
		return;
	}

	if (bypp != 4)
	{
		fprintf(stderr, "DEBUG: %s() found unexpected bypp %d\n", __func__, bypp);
	}
	else if (is_surface_bgr(zcs->ds->surface))
	{
		fprintf(stderr, "DEBUG: %s() found unexpected BGR surface\n", __func__);
	}

	c_incr= FONT_WIDTH * bypp;
	r_incr= FONT_HEIGHT * ds_get_linesize(zcs->ds);

	dmem= ds_get_data(zcs->ds);
	dmem+= zcs->cpos_c * c_incr;
	dmem+= zcs->cpos_r * r_incr;

	for (ix= 0; ix < FONT_HEIGHT; ix++)
	{
	    if (bypp == 4)
	    {	/* invert pixels in current character cell */
		for (iy= 0; iy < FONT_WIDTH * bypp; iy+= bypp)
		{
		    *(dmem + iy)^= zaphod_rgb_palette[0][2] ^ zaphod_rgb_palette[1][2];
		    *(dmem + iy+1)^= zaphod_rgb_palette[0][1] ^ zaphod_rgb_palette[1][1];
		    *(dmem + iy+2)^= zaphod_rgb_palette[0][0] ^ zaphod_rgb_palette[1][0];
		}
	    }
	    else
	    {	/* FIXME: modifies RGB values, not pixels */
		for (iy= 0; iy < FONT_WIDTH * bypp; iy++)
		    *(dmem+ iy)^= 0xff;
	    }
	    dmem+= ds_get_linesize(zcs->ds);
	}

	/* track where we wrote to trigger partial update later */
	if (zcs->ds_putchar_maxr < zcs->cpos_r)
	    zcs->ds_putchar_maxr= zcs->cpos_r;
	if (zcs->ds_putchar_maxc < zcs->cpos_c)
	    zcs->ds_putchar_maxc= zcs->cpos_c;
	if ((zcs->ds_putchar_minr == -1) || (zcs->ds_putchar_minr > zcs->cpos_r))
	    zcs->ds_putchar_minr= zcs->cpos_r;
	if ((zcs->ds_putchar_minc == -1) || (zcs->ds_putchar_minc > zcs->cpos_c))
	    zcs->ds_putchar_minc= zcs->cpos_c;
}
#endif	/* ZAPHOD_CONSOLE_CURSOR_BLINK */

static void zaphod_consolegui_update_display(void *opaque)
{
  ZaphodConsoleState	*zcs= (ZaphodConsoleState *)opaque;
  int			dirty;
#ifdef ZAPHOD_DEBUG
	//fprintf(stderr, "DEBUG: Enter %s()\n", __func__);
#endif

	if (!is_graphic_console())
	{	/* Avoid rendering onto the HMI/serial console */
		return;
	}

	dirty= zcs->ds_invalid
		|| (zcs->ds_putchar_minr > -1);

	if (dirty)
	{
	  int	minc= 0, minr= 0,
		maxc, maxr;
	  int	redraw_ulx= 0, redraw_uly= 0,
		redraw_xsz, redraw_ysz;
	  int	row, col;

	    if (zcs->ds_invalid)	/* full blank requested */
	    {
		/* TODO: 'invalid' indicates "damage" (blanking?)
		 * has occurred somewhere non-specific, and a full
		 * redraw is required ...maybe forcibly blank the display
		 * area first in this case?
		    memset(ds_get_data(zcs->ds), 0,
				bypp * ds_get_width(zcs->ds)
					* ds_get_height(zcs->ds));
		 */
#ifdef ZAPHOD_DEBUG
		//fprintf(stderr, "DEBUG: Doing full-screen update because ds_invalid set\n");
#endif
		maxr= TEXT_ROWS - 1;
		maxc= TEXT_COLS - 1;
		redraw_xsz= ds_get_width(zcs->ds);
		redraw_ysz= ds_get_height(zcs->ds);
	    }
	    else
	    {
#ifdef ZAPHOD_DEBUG
		//fprintf(stderr, "DEBUG: Handling 'dirty' screen update with ds_invalid unset\n");
#endif
		minr= zcs->ds_putchar_minr;
		maxr= zcs->ds_putchar_maxr;
		minc= zcs->ds_putchar_minc;
		maxc= zcs->ds_putchar_maxc;
		redraw_ulx= minc * FONT_WIDTH;
		redraw_uly= minr * FONT_HEIGHT;
		redraw_xsz= (maxc - minc + 1) * FONT_WIDTH;
		redraw_ysz= (maxr - minr + 1) * FONT_HEIGHT;
	    }

	    /* Ensure the region between min{c|r} and max{c|r} is a)
	     * (re-)rendered and b) duplicated on screen. Font data
	     * must start with a blank character at index 0
	     */
	    for (row= minr; row <= maxr; row++)
		for (col= minc; col <= maxc; col++)
		{
		    zaphod_consolegui_drawchar(opaque,
						zcs->char_grid[row][col],
						row, col);
#ifdef ZAPHOD_CONSOLE_CURSOR_BLINK
		    if ( (zcs->cs_blink_state == 1)
			&& (row == zcs->cpos_r)
			&& (col == zcs->cpos_c) )
		    {
			zaphod_consolegui_blink_cursor(zcs);
		    }
		}
#endif

	    dpy_update(zcs->ds, redraw_ulx,redraw_uly,
				redraw_xsz,redraw_ysz);

	    zcs->ds_invalid= 0;
	    zcs->ds_putchar_minr= zcs->ds_putchar_maxr= -1;
	    zcs->ds_putchar_minc= zcs->ds_putchar_maxc= -1;
	}

#ifdef ZAPHOD_DEBUG
	//fprintf(stderr, "DEBUG: Exit %s()\n", __func__);
#endif
}

void zaphod_consolegui_invalidate_display(void *opaque)
{
  ZaphodConsoleState *zcs= (ZaphodConsoleState *)opaque;
#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Enter %s()\n", __func__);
#endif

	zcs->ds_invalid= 1;		/* full redraw on next update */

#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Exit %s()\n", __func__);
#endif
}

void zaphod_consolegui_putchar(void *opaque, char ch)
{
  ZaphodConsoleState	*zcs= (ZaphodConsoleState *)opaque;
#if 1	/* HACK */
	/* TODO: Phil Brown's documentation states a desire to have
	 * ANSI control codes but implements something else; Grant
	 * Searle has a similar idea including the ability to handle
	 * wide/tall characters and plot/unplot/toggle pixels
	 */

	if (!isprint(ch))
	{
	    int hn= (ch & 0xf0) >> 4, ln= (ch & 0x0f);

	    zaphod_consolegui_putchar(opaque, '[');
	    zaphod_consolegui_putchar(opaque, (hn < 10)? '0'+hn : 'A'+hn-10);
	    zaphod_consolegui_putchar(opaque, (ln < 10)? '0'+ln : 'A'+ln-10);
	    zaphod_consolegui_putchar(opaque, ']');
	    ch= '*';
	}
#endif
	zcs->char_grid[zcs->cpos_r][zcs->cpos_c]= ch;
	/* TODO: is this draw important, or does the screen update code
	 * do the right thing anyway now?
	 */
	zaphod_consolegui_drawchar(opaque, ch, zcs->cpos_r, zcs->cpos_c);

	/* track where we wrote in case of partial update later */
	if (zcs->ds_putchar_maxr < zcs->cpos_r)
	    zcs->ds_putchar_maxr= zcs->cpos_r;
	if (zcs->ds_putchar_maxc < zcs->cpos_c)
	    zcs->ds_putchar_maxc= zcs->cpos_c;
	if ((zcs->ds_putchar_minr == -1) || (zcs->ds_putchar_minr > zcs->cpos_r))
	    zcs->ds_putchar_minr= zcs->cpos_r;
	if ((zcs->ds_putchar_minc == -1) || (zcs->ds_putchar_minc > zcs->cpos_c))
	    zcs->ds_putchar_minc= zcs->cpos_c;

	if (++zcs->cpos_c == TEXT_COLS)
	{
	    zcs->cpos_c= 0;
	    if (++zcs->cpos_r == TEXT_ROWS)
	    {
		int row, col;

		/* We filled the display and should scroll/repaint it.
		 * Sane implementation options include leaving the
		 * cursor in the corner and scrolling on next print, or
		 * working out in advance whether we're scrolling the
		 * whole screen vs updating a large region
		 */
		for (row= 0; row < TEXT_ROWS - 1; row++)
		    for (col= 0; col < TEXT_COLS; col++)
			zcs->char_grid[row][col]= zcs->char_grid[row+1][col];
		for (col= 0; col < TEXT_COLS; col++)
			zcs->char_grid[TEXT_ROWS - 1][col]= 0;
		zaphod_consolegui_invalidate_display(zcs);

		zcs->cpos_r= TEXT_ROWS - 1;
	    }
	}

#ifdef ZAPHOD_CONSOLE_CURSOR_BLINK
	if (zcs->cs_blink_state)	/* illuminate new cursor posn? */
	    zaphod_consolegui_blink_cursor(zcs);
#endif
}

#ifdef ZAPHOD_CONSOLE_CURSOR_BLINK
static void zaphod_consolegui_cursor_timer(void *opaque)
{
  ZaphodConsoleState *zcs= opaque;
  int64_t next_time;

	/* TODO: does CPU receive 50HZ interrupt? */

	/* only approximately 50HZ */
	next_time= qemu_get_clock(vm_clock) + muldiv64(1, get_ticks_per_sec(), 50);
	qemu_mod_timer(zcs->cs_blink_timer, next_time);

	if (++zcs->cs_blink_count == 16)	/* arbitrary on/off rate */
	{
#ifdef ZAPHOD_DEBUG
	    //fprintf(stderr, "DEBUG: timer -> cursor blink (state: %s)\n", zcs->cs_blink_state?"ON":"OFF");
#endif
	    zcs->cs_blink_count= 0;
	    /* Now toggle the cursor illumination state (visually) */
	    zaphod_consolegui_blink_cursor(zcs);
	    zcs->cs_blink_state= !zcs->cs_blink_state;
	}
}

static void zaphod_consolegui_cursor_init(ZaphodConsoleState *zcs)
{
  int64_t now= qemu_get_clock(vm_clock);
	zcs->cs_blink_timer= qemu_new_timer(vm_clock, zaphod_consolegui_cursor_timer, zcs);
	qemu_mod_timer(zcs->cs_blink_timer, now);
}
#endif

static void zaphod_consolegui_reset(ZaphodConsoleState *zcs)
{
  int	r, c;

	for (r= 0; r < TEXT_ROWS; r++)
	    for (c= 0; c < TEXT_COLS; c++)
		zcs->char_grid[r][c]= 0;
	zcs->ds_putchar_minr= zcs->ds_putchar_maxr= -1;
	zcs->ds_putchar_minc= zcs->ds_putchar_maxc= -1;
	zcs->cpos_c= 0;
	zcs->cpos_r= 0;

#ifdef ZAPHOD_CONSOLE_CURSOR_BLINK
	zcs->cs_blink_count= 0;
	zcs->cs_blink_state= 1;
#endif
}

void zaphod_consolegui_init(ZaphodConsoleState *zcs)
{
	zcs->ds= graphic_console_init(zaphod_consolegui_update_display,
		zaphod_consolegui_invalidate_display,
		NULL,	/* screen dump via ppm_save() */
		NULL,	/* 'text update' */
		zcs);
	/* TODO: calls to zaphod_consolegui_invalidate_display() (eg. when
	 * user switches to HMI and back) need to prompt a full redraw of
	 * previous output
	 */

	/* TODO: console dimensions vary wildly due to LCD panels and
	 * TV out vs monitor compatibility; Phil Brown calls his screen
	 * "medium-res two-colour 24x80" (Amiga NTSC is usually 25x80
	 * at 640x200 and PAL 32x80 at 640x256); Grant Searle's 2k RAM
	 * ATmega328 is 25x80 at 160x100 (2x4 pixels per character cell).
	 * See also ZX Spectrum (24x32 at 256x192)/PCW (32x90 at 720x256)
	 */
	qemu_console_resize(zcs->ds,
		FONT_WIDTH * TEXT_COLS, FONT_HEIGHT * TEXT_ROWS);

	zaphod_consolegui_reset(zcs);
#ifdef ZAPHOD_CONSOLE_CURSOR_BLINK
	zaphod_consolegui_cursor_init(zcs);
#endif
}
#endif	/* ZAPHOD_HAS_CONSOLEGUI */
