/*
 * Name:    d_si9650_status.h
 *
 * Purpose: Header file for reading status values from Scientific Instruments
 *          9650 temperature controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SI9650_STATUS_H__
#define __D_SI9650_STATUS_H__

#include "mx_analog_input.h"

#define MXT_SI9650_TEMPERATURE_SENSOR1		1
#define MXT_SI9650_TEMPERATURE_SENSOR2		2

typedef struct {
	MX_RECORD *si9650_motor_record;
	int32_t parameter_type;
} MX_SI9650_STATUS;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_si9650_status_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_si9650_status_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_si9650_status_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_si9650_status_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
			mxd_si9650_status_analog_input_function_list;

extern mx_length_type mxd_si9650_status_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_si9650_status_rfield_def_ptr;

#define MXD_SI9650_STATUS_STANDARD_FIELDS \
  {-1, -1, "si9650_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_SI9650_STATUS, si9650_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "parameter_type", MXFT_INT32, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_SI9650_STATUS, parameter_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_SI9650_STATUS_H__ */

