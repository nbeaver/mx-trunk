/*
 * Name:    d_galil_gclib_dinput.h
 *
 * Purpose: Header file for Galil controller uncommitted digital input devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_GALIL_GCLIB_DINPUT_H__
#define __D_GALIL_GCLIB_DINPUT_H__

#include "mx_digital_input.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *galil_gclib_record;
	unsigned long dinput_number;
} MX_GALIL_GCLIB_DINPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_galil_gclib_dinput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_galil_gclib_dinput_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_galil_gclib_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_galil_gclib_dinput_digital_input_function_list;

extern long mxd_galil_gclib_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_dinput_rfield_def_ptr;

#define MXD_GALIL_GCLIB_DINPUT_STANDARD_FIELDS \
  {-1, -1, "galil_gclib_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_GALIL_GCLIB_DINPUT, galil_gclib_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "dinput_number", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB_DINPUT, dinput_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_GALIL_GCLIB_DINPUT_H__ */

