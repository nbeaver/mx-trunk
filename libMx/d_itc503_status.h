/*
 * Name:    d_itc503_status.h
 *
 * Purpose: Header file for reading status values from an Oxford Instruments
 *          ITC503 temperature controller.
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

#ifndef __D_ITC503_STATUS_H__
#define __D_ITC503_STATUS_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *itc503_record;
	int32_t parameter_type;
} MX_ITC503_STATUS;

/* The value of 'parameter_type' is used to construct an ITC503 'R' command.
 * Thus, the values of the parameters are as listed in the Oxford manual:
 *
 *  0 - Set temperature
 *  1 - Sensor 1 temperature
 *  2 - Sensor 2 temperature
 *  3 - Sensor 3 temperature
 *  4 - Temperature error
 *  5 - Heater O/P (as % of current limit)
 *  6 - Heater O/P (as Volts, approx.)
 *  7 - Gas flow O/P (arbitrary units)
 *  8 - Proportional band
 *  9 - Integral action time
 * 10 - Derivative action time
 * 11 - Channel 1 freq/4
 * 12 - Channel 2 freq/4
 * 13 - Channel 3 freq/4
 *
 */

/* Define all of the interface functions. */

MX_API mx_status_type mxd_itc503_status_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_itc503_status_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_itc503_status_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_itc503_status_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
			mxd_itc503_status_analog_input_function_list;

extern mx_length_type mxd_itc503_status_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_itc503_status_rfield_def_ptr;

#define MXD_ITC503_STATUS_STANDARD_FIELDS \
  {-1, -1, "itc503_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_STATUS, itc503_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "parameter_type", MXFT_INT32, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_STATUS, parameter_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_ITC503_STATUS_H__ */

