/*
 * Name:    d_ptz_motor.h
 *
 * Purpose: Header file for Pan/Tilt/Zoom controller motors.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PTZ_MOTOR_H__
#define __D_PTZ_MOTOR_H__

/* ptz_motor_type values. */

#define MXT_PTZ_MOTOR_PAN	1
#define MXT_PTZ_MOTOR_TILT	2
#define MXT_PTZ_MOTOR_ZOOM	3
#define MXT_PTZ_MOTOR_FOCUS	4

typedef struct {
	MX_RECORD *record;

	MX_RECORD *ptz_record;
	long ptz_motor_type;
} MX_PTZ_MOTOR;

MX_API mx_status_type mxd_ptz_motor_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_ptz_motor_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_ptz_motor_print_structure( FILE *file,
						MX_RECORD *record );

MX_API mx_status_type mxd_ptz_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_ptz_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_ptz_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_ptz_motor_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_ptz_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_ptz_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_ptz_motor_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_ptz_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_ptz_motor_motor_function_list;

extern long mxd_ptz_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ptz_motor_rfield_def_ptr;

#define MXD_PTZ_MOTOR_STANDARD_FIELDS \
  {-1, -1, "ptz_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PTZ_MOTOR, ptz_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "ptz_motor_type", MXFT_LONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_PTZ_MOTOR, ptz_motor_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_PTZ_MOTOR_H__ */
