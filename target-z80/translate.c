/*
 * Minimal QEmu Z80 CPU - instruction translation
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting by William Towle 2018-2022
 */


//#if 1	/* debug */
//	/* TODO: version with error_printf() needs CPU headers */
//#define DPRINTF(fmt, ...) \
//	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
//#else
//#define DPRINTF(fmt, ...) \
//	do { } while (0)
//#endif
