/*
 * Name:    d_keyboard_dinput.h
 *
 * Purpose: Header file for MX keyboard digital input device.  This driver
 *          uses keypresses on a keyboard to simulate a digital input.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KEYBOARD_DINPUT_H__
#define __D_KEYBOARD_DINPUT_H__

#include "mx_digital_input.h"

typedef struct {
	mx_bool_type debug;
} MX_KEYBOARD_DINPUT;

#define MXD_KEYBOARD_DINPUT_STANDARD_FIELDS \
  {-1, -1, "debug", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEYBOARD_DINPUT, debug), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_keyboard_dinput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_keyboard_dinput_read( MX_DIGITAL_INPUT *dinput );
MX_API mx_status_type mxd_keyboard_dinput_clear( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_keyboard_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_keyboard_dinput_digital_input_function_list;

extern long mxd_keyboard_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_keyboard_dinput_rfield_def_ptr;

#endif /* __D_KEYBOARD_DINPUT_H__ */

