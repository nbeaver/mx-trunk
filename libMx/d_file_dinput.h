/*
 * Name:    d_file_dinput.h
 *
 * Purpose: Header file for MX keyboard digital input device.  This driver
 *          uses values read from a file to simulate a digital input.
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

#ifndef __D_FILE_DINPUT_H__
#define __D_FILE_DINPUT_H__

#include "mx_digital_input.h"

typedef struct {
	mx_bool_type debug;
	char filename[MXU_FILENAME_LENGTH+1];
} MX_FILE_DINPUT;

#define MXD_FILE_DINPUT_STANDARD_FIELDS \
  {-1, -1, "debug", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FILE_DINPUT, debug), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FILE_DINPUT, filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

/* Define all of the interface functions. */

MX_API mx_status_type mxd_file_dinput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_file_dinput_read( MX_DIGITAL_INPUT *dinput );
MX_API mx_status_type mxd_file_dinput_clear( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_file_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_file_dinput_digital_input_function_list;

extern long mxd_file_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_file_dinput_rfield_def_ptr;

#endif /* __D_FILE_DINPUT_H__ */

