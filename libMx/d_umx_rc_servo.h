/*
 * Name:    d_umx_rc_servo.h
 *
 * Purpose: Header file for UMX-controlled RC servo motors.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_UMX_RC_SERVO_H__
#define __D_UMX_RC_SERVO_H__

/* Values for the umx_rc_servo_flags variable. */

#define MXF_UMX_RC_SERVO_HOME_TO_LIMIT_SWITCH	0x1
#define MXF_UMX_RC_SERVO_NO_START_CHARACTER	0x2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *umx_record;
	unsigned long servo_number;
	unsigned long servo_pin;
	unsigned long servo_flags;
} MX_UMX_RC_SERVO;

MX_API mx_status_type mxd_umx_rc_servo_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_umx_rc_servo_open( MX_RECORD *record );

MX_API mx_status_type mxd_umx_rc_servo_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_umx_rc_servo_get_position( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_umx_rc_servo_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_umx_rc_servo_motor_function_list;

extern long mxd_umx_rc_servo_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_umx_rc_servo_rfield_def_ptr;

#define MXD_UMX_RC_SERVO_STANDARD_FIELDS \
  {-1, -1, "umx_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_RC_SERVO, umx_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "servo_number", MXFT_LONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_RC_SERVO, servo_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "servo_pin", MXFT_LONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_RC_SERVO, servo_pin), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "servo_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_RC_SERVO, servo_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_UMX_RC_SERVO_H__ */

