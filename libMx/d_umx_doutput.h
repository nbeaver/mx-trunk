/*
 * Name:    d_umx_doutput.h
 *
 * Purpose: Header file for UMX-based microcontroller digital outputs.
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

#ifndef __D_UMX_DOUTPUT_H__
#define __D_UMX_DOUTPUT_H__

#include "mx_digital_output.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *umx_record;
	char doutput_name[MXU_RECORD_NAME_LENGTH+1];
} MX_UMX_DOUTPUT;

MX_API mx_status_type mxd_umx_doutput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_umx_doutput_write(MX_DIGITAL_OUTPUT *doutput);

extern MX_RECORD_FUNCTION_LIST mxd_umx_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_umx_doutput_digital_output_function_list;

extern long mxd_umx_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_umx_doutput_rfield_def_ptr;

#define MXD_UMX_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "umx_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_UMX_DOUTPUT, umx_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "doutput_name", MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_DOUTPUT, doutput_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_UMX_DOUTPUT_H__ */
