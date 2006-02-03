/*
 * Name:    d_keithley2400_doutput.h
 *
 * Purpose: Header file for setting the output of Keithley 2400 multimeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KEITHLEY2400_DOUTPUT_H__
#define __D_KEITHLEY2400_DOUTPUT_H__

#include "mx_digital_output.h"

typedef struct {
	MX_RECORD *controller_record;
} MX_KEITHLEY2400_DOUTPUT;

MX_API mx_status_type mxd_keithley2400_doutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_keithley2400_doutput_open( MX_RECORD *record );
MX_API mx_status_type mxd_keithley2400_doutput_close( MX_RECORD *record );

MX_API mx_status_type mxd_keithley2400_doutput_write(MX_DIGITAL_OUTPUT *doutput);

extern MX_RECORD_FUNCTION_LIST mxd_keithley2400_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_keithley2400_doutput_digital_output_function_list;

extern mx_length_type mxd_keithley2400_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_keithley2400_doutput_rfield_def_ptr;

#define MXD_KEITHLEY2400_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_KEITHLEY2400_DOUTPUT, controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_KEITHLEY2400_DOUTPUT_H__ */
