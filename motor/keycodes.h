/*
 * Name:    keycodes.h
 *
 * Purpose: Mnemonic names for single keystrokes for MSDOS and Unix.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __KEYCODES_H__
#define __KEYCODES_H__

#define CTRL_D  0x04
#define CTRL_H	0x08
#define BS	0x08
#define LF	0x0a
#define CTRL_L	0x0c
#define CR	0x0d
#define ESC	0x1b

/* PC scancodes */

#define KEY_LEFT	0x4b00
#define KEY_RIGHT	0x4d00

#define KEY_CTRL_LEFT	0x7300
#define KEY_CTRL_RIGHT	0x7400

#define KEY_UP		0x4800
#define KEY_DOWN	0x5000

#define KEY_INSERT	0x5200
#define KEY_DELETE	0x5300

#endif /* __KEYCODES_H__ */

