/*
 * Name:    d_itc503_control.h
 *
 * Purpose: Header file for setting control parameters in an
 *          Oxford Instruments ITC503 temperature controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ITC503_CONTROL_H__
#define __D_ITC503_CONTROL_H__

#include "mx_analog_output.h"

typedef struct {
	MX_RECORD *itc503_motor_record;
	char parameter_type;
} MX_ITC503_CONTROL;

/* The value of 'parameter_type' is the letter that starts the ITC503 command
 * that will be sent.  The currently supported values are:
 *
 * 'A' - Set auto/manual for heater and gas
 * 'C' - Set local/remote/lock status
 * 'G' - Set gas flow ( in manual only )
 * 'O' - Set heater output volts ( in manual only )
 *
 * There are several other ITC503 control commands, but only the ones likely
 * to be used in routine operation are supported.
 *
 */

/* Define all of the interface functions. */

MX_API mx_status_type mxd_itc503_control_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_itc503_control_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_itc503_control_read( MX_ANALOG_OUTPUT *ainput );
MX_API mx_status_type mxd_itc503_control_write( MX_ANALOG_OUTPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_itc503_control_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
			mxd_itc503_control_analog_output_function_list;

extern long mxd_itc503_control_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_itc503_control_rfield_def_ptr;

#define MXD_ITC503_CONTROL_STANDARD_FIELDS \
  {-1, -1, "itc503_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_CONTROL, itc503_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "parameter_type", MXFT_CHAR, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_CONTROL, parameter_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_ITC503_CONTROL_H__ */

