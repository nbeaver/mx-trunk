/*
 * Name:    d_keithley2700_ainput.h
 *
 * Purpose: Header file for reading out individual channels for Keithley 2700
 *          switching multimeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KEITHLEY2700_AINPUT_H__
#define __D_KEITHLEY2700_AINPUT_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *controller_record;
	int system_channel;

	int slot;
	int channel;
} MX_KEITHLEY2700_AINPUT;

MX_API mx_status_type mxd_keithley2700_ainput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_keithley2700_ainput_open( MX_RECORD *record );
MX_API mx_status_type mxd_keithley2700_ainput_close( MX_RECORD *record );

MX_API mx_status_type mxd_keithley2700_ainput_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_keithley2700_ainput_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_keithley2700_ainput_analog_input_function_list;

extern long mxd_keithley2700_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_keithley2700_ainput_rfield_def_ptr;

#define MXD_KEITHLEY2700_AINPUT_STANDARD_FIELDS \
  {-1, -1, "controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_KEITHLEY2700_AINPUT, controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "system_channel", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2700_AINPUT, system_channel),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_KEITHLEY2700_AINPUT_H__ */
