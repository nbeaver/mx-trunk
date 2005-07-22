/*
 * Name:    d_cryostream600_status.h
 *
 * Purpose: Header file for reading status values from an Oxford Cryosystems
 *          600 series Cryostream controller.
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

#ifndef __D_CRYOSTREAM600_STATUS_H__
#define __D_CRYOSTREAM600_STATUS_H__

#include "mx_analog_input.h"

#define MXT_CRYOSTREAM600_TEMPERATURE			1
#define MXT_CRYOSTREAM600_SET_TEMPERATURE		2
#define MXT_CRYOSTREAM600_TEMPERATURE_ERROR		3
#define MXT_CRYOSTREAM600_FINAL_TEMPERATURE		4
#define MXT_CRYOSTREAM600_RAMP_RATE			5
#define MXT_CRYOSTREAM600_EVAPORATOR_TEMPERATURE	6
#define MXT_CRYOSTREAM600_ICE_BLOCK			7

typedef struct {
	MX_RECORD *cryostream600_motor_record;
	int parameter_type;
} MX_CRYOSTREAM600_STATUS;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_cryostream600_status_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_cryostream600_status_resynchronize(
							MX_RECORD *record );

MX_API mx_status_type mxd_cryostream600_status_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_cryostream600_status_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
			mxd_cryostream600_status_analog_input_function_list;

extern long mxd_cryostream600_status_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_cryostream600_status_rfield_def_ptr;

#define MXD_CRYOSTREAM600_STATUS_STANDARD_FIELDS \
  {-1, -1, "cryostream600_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_CRYOSTREAM600_STATUS, cryostream600_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "parameter_type", MXFT_INT, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_CRYOSTREAM600_STATUS, parameter_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_CRYOSTREAM600_STATUS_H__ */

