/*
 * Name:    d_galil_gclib_doutput.h
 *
 * Purpose: Header file for Galil controller uncommitted digital output devices.
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

#ifndef __D_GALIL_GCLIB_DOUTPUT_H__
#define __D_GALIL_GCLIB_DOUTPUT_H__

#include "mx_digital_output.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *galil_gclib_record;
	unsigned long doutput_number;
} MX_GALIL_GCLIB_DOUTPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_galil_gclib_doutput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_galil_gclib_doutput_read(
					MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_galil_gclib_doutput_write(
					MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_galil_gclib_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_galil_gclib_doutput_digital_output_function_list;

extern long mxd_galil_gclib_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_doutput_rfield_def_ptr;

#define MXD_GALIL_GCLIB_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "galil_gclib_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_GALIL_GCLIB_DOUTPUT, galil_gclib_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "doutput_number", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB_DOUTPUT, doutput_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_GALIL_GCLIB_DOUTPUT_H__ */

