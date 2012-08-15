/*
 * Name:    d_aoutput_as_doutput.h
 *
 * Purpose: Header file for treating an MX analog output as a digital output.
 *          The driver commands the analog output to put out a high or low
 *          voltage depending on the value of the digital output record (0/1).
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AOUTPUT_AS_DOUTPUT_H__
#define __D_AOUTPUT_AS_DOUTPUT_H__

#include "mx_digital_output.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *analog_output_record;
	double low_voltage;
	double high_voltage;
	double threshold_voltage;
} MX_AOUTPUT_AS_DOUTPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_aoutput_as_doutput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_aoutput_as_doutput_read( MX_DIGITAL_OUTPUT *ainput );
MX_API mx_status_type mxd_aoutput_as_doutput_write( MX_DIGITAL_OUTPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_aoutput_as_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_aoutput_as_doutput_digital_output_function_list;

extern long mxd_aoutput_as_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aoutput_as_doutput_rfield_def_ptr;

#define MXD_AOUTPUT_AS_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "analog_output_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AOUTPUT_AS_DOUTPUT, analog_output_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "low_voltage", MXFT_DOUBLE, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_AOUTPUT_AS_DOUTPUT, low_voltage), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "high_voltage", MXFT_DOUBLE, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_AOUTPUT_AS_DOUTPUT, high_voltage), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "threshold_voltage", MXFT_DOUBLE, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_AOUTPUT_AS_DOUTPUT, threshold_voltage), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_AOUTPUT_AS_DOUTPUT_H__ */

