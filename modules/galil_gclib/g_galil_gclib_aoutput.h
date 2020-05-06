/*
 * Name:    d_galil_gclib_aoutput.h
 *
 * Purpose: Header file for MX Galil controller analog outputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_GALIL_GCLIB_AOUTPUT_H__
#define __D_GALIL_GCLIB_AOUTPUT_H__

#include "mx_analog_output.h"

typedef struct {
	int dummy;
} MX_GALIL_GCLIB_AOUTPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_galil_gclib_aoutput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_galil_gclib_aoutput_read( MX_ANALOG_OUTPUT *aoutput );
MX_API mx_status_type mxd_galil_gclib_aoutput_write( MX_ANALOG_OUTPUT *aoutput );

extern MX_RECORD_FUNCTION_LIST mxd_galil_gclib_aoutput_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
			mxd_galil_gclib_aoutput_analog_output_function_list;

extern long mxd_galil_gclib_aoutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_aoutput_rfield_def_ptr;

#define MXD_GALIL_GCLIB_AOUTPUT_STANDARD_FIELDS

#endif /* __D_GALIL_GCLIB_AOUTPUT_H__ */

