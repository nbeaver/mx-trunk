/*
 * Name:    d_keithley199_ainput.h
 *
 * Purpose: Header file for reading voltages, currents, etc. from a
 *          Keithley 199 dmm/scanner.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KEITHLEY199_AINPUT_H__
#define __D_KEITHLEY199_AINPUT_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *keithley199_record;
} MX_KEITHLEY199_AINPUT;

MX_API mx_status_type mxd_keithley199_ainput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_keithley199_ainput_open( MX_RECORD *record );
MX_API mx_status_type mxd_keithley199_ainput_close( MX_RECORD *record );

MX_API mx_status_type mxd_keithley199_ainput_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_keithley199_ainput_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_keithley199_ainput_analog_input_function_list;

extern long mxd_keithley199_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_keithley199_ainput_rfield_def_ptr;

#define MXD_KEITHLEY199_AINPUT_STANDARD_FIELDS \
  {-1, -1, "keithley199_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_KEITHLEY199_AINPUT, keithley199_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_KEITHLEY199_AINPUT_H__ */
