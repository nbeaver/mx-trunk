/*
 * Name:    d_ainput_as_dinput.h
 *
 * Purpose: Header file for treating an MX analog input as a digital input.
 *          The driver uses an ADC voltage threshold to determine whether
 *          to return a 0 or 1 as digital input.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AINPUT_AS_DINPUT_H__
#define __D_AINPUT_AS_DINPUT_H__

#include "mx_digital_input.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *analog_input_record;
	double low_voltage;
	double high_voltage;
	double threshold_voltage;
} MX_AINPUT_AS_DINPUT;

#define MXD_AINPUT_AS_DINPUT_STANDARD_FIELDS \
  {-1, -1, "analog_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AINPUT_AS_DINPUT, analog_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "low_voltage", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AINPUT_AS_DINPUT, low_voltage), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "high_voltage", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AINPUT_AS_DINPUT, high_voltage), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "threshold_voltage", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AINPUT_AS_DINPUT, threshold_voltage), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_ainput_as_dinput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_ainput_as_dinput_read( MX_DIGITAL_INPUT *dinput );
MX_API mx_status_type mxd_ainput_as_dinput_clear( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_ainput_as_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_ainput_as_dinput_digital_input_function_list;

extern long mxd_ainput_as_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ainput_as_dinput_rfield_def_ptr;

#endif /* __D_AINPUT_AS_DINPUT_H__ */

