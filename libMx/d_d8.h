/*
 * Name:    d_d8.h
 *
 * Purpose: Header file for MX drivers for Bruker D8 controlled motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_D8_H__
#define __D_D8_H__

#include "i_d8.h"

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *d8_record;
	long drive_number;
	double d8_speed;
} MX_D8_MOTOR;

MX_API mx_status_type mxd_d8_motor_initialize_type( long type );
MX_API mx_status_type mxd_d8_motor_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_d8_motor_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_d8_motor_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_d8_motor_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_d8_motor_read_parms_from_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_d8_motor_write_parms_to_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_d8_motor_open( MX_RECORD *record );
MX_API mx_status_type mxd_d8_motor_close( MX_RECORD *record );

MX_API mx_status_type mxd_d8_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_d8_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_d8_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_d8_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_d8_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_d8_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_d8_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_d8_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_d8_motor_find_home_position( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_d8_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_d8_motor_motor_function_list;

extern long mxd_d8_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_d8_motor_rfield_def_ptr;

#define MXD_D8_MOTOR_STANDARD_FIELDS \
  {-1, -1, "d8_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_D8_MOTOR, d8_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "drive_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_D8_MOTOR, drive_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "d8_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_D8_MOTOR, d8_speed), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_D8_H__ */

