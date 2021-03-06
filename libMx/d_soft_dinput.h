/*
 * Name:    d_soft_dinput.h
 *
 * Purpose: Header file for MX soft digital input devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_DINPUT_H__
#define __D_SOFT_DINPUT_H__

#include "mx_digital_input.h"

typedef struct {
	int dummy;
} MX_SOFT_DINPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_soft_dinput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_soft_dinput_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_soft_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_soft_dinput_digital_input_function_list;

extern long mxd_soft_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_dinput_rfield_def_ptr;

#endif /* __D_SOFT_DINPUT_H__ */

