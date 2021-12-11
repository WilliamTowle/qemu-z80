/*
 * QEmu Z80 virtual CPU header
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting by William Towle 2018-2022
 */

#ifndef CPU_Z80_H
#define CPU_Z80_H

#ifdef CONFIG_USER_ONLY
    /* Temporarily bail from builds of z80-bblbrx-user while partial */
#error "CONFIG_USER_ONLY for z80 has incomplete support"
#endif
#ifdef CONFIG_SOFTMMU
    /* Temporarily bail from builds of z80-softmmu while partial */
#error "z80-softmmu support incomplete"
#endif


/* TODO [v1.7.2]: CPUZ80State, NB_MMU_MODES here */

#endif /* !defined (CPU_Z80_H) */
