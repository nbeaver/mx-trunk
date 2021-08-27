/*
 * Name:    d_motor_relay.h
 *
 * Purpose: MX header file for using an MX motor to drive a mechanical relay.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MOTOR_RELAY_H__
#define __D_MOTOR_RELAY_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *motor_record;
	double closed_position;
	double open_position;
	double position_deadband;
} MX_MOTOR_RELAY;

#define MXD_MOTOR_RELAY_STANDARD_FIELDS \
  {-1, -1, "motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MOTOR_RELAY, motor_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "closed_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MOTOR_RELAY, closed_position), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "open_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MOTOR_RELAY, open_position), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "position_deadband", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MOTOR_RELAY, position_deadband), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

/* Define all of the device functions. */

MX_API mx_status_type mxd_motor_relay_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_motor_relay_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_motor_relay_get_relay_status( MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_motor_relay_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_motor_relay_relay_function_list;

extern long mxd_motor_relay_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_motor_relay_rfield_def_ptr;

#endif /* __D_MOTOR_RELAY_H__ */
