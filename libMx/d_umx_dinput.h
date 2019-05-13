/*
 * Name:    d_umx_dinput.h
 *
 * Purpose: Header file for UMX-based microcontroller digital inputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_UMX_DINPUT_H__
#define __D_UMX_DINPUT_H__

#include "mx_digital_input.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *umx_record;
	char dinput_name[MXU_RECORD_NAME_LENGTH+1];
} MX_UMX_DINPUT;

MX_API mx_status_type mxd_umx_dinput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_umx_dinput_read(MX_DIGITAL_INPUT *dinput);

extern MX_RECORD_FUNCTION_LIST mxd_umx_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
		mxd_umx_dinput_digital_input_function_list;

extern long mxd_umx_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_umx_dinput_rfield_def_ptr;

#define MXD_UMX_DINPUT_STANDARD_FIELDS \
  {-1, -1, "umx_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_UMX_DINPUT, umx_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "dinput_name", MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_DINPUT, dinput_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_UMX_DINPUT_H__ */
