/*
 * Name:    d_soft_ainput.h
 *
 * Purpose: Header file for MX soft analog input devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_AINPUT_H__
#define __D_SOFT_AINPUT_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *aoutput_record;
} MX_SOFT_AINPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_soft_ainput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_soft_ainput_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_soft_ainput_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
			mxd_soft_ainput_analog_input_function_list;

extern long mxd_soft_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_ainput_rfield_def_ptr;

#define MXD_SOFT_AINPUT_STANDARD_FIELDS \
  {-1, -1, "aoutput_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_AINPUT, aoutput_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_SOFT_AINPUT_H__ */

