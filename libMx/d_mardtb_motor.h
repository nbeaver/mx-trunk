/*
 * Name:    d_mardtb_motor.h 
 *
 * Purpose: Header file for MX motor driver for motors controlled by the
 *          MarUSA Desktop Beamline goniostat controller.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2004, 2006, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MARDTB_MOTOR_H__
#define __D_MARDTB_MOTOR_H__

/* MX_MARDTB_MOTOR_FUDGE_FACTOR rescales the raw speed. */

#define MX_MARDTB_MOTOR_FUDGE_FACTOR	(3.0)

typedef struct {
	MX_RECORD *mardtb_record;
	long motor_number;
	unsigned long default_speed;
	unsigned long default_acceleration;

	MX_DEAD_RECKONING dead_reckoning;
} MX_MARDTB_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mardtb_motor_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mardtb_motor_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_mardtb_motor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_mardtb_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_mardtb_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_mardtb_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_mardtb_motor_get_extended_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_mardtb_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_mardtb_motor_motor_function_list;

extern long mxd_mardtb_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mardtb_motor_rfield_def_ptr;

#define MXD_MARDTB_MOTOR_STANDARD_FIELDS \
  {-1, -1, "mardtb_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARDTB_MOTOR, mardtb_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "motor_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARDTB_MOTOR, motor_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "default_speed", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARDTB_MOTOR, default_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_acceleration", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARDTB_MOTOR, default_acceleration), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_MARDTB_MOTOR_H__ */
