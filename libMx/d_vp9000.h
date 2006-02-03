/*
 * Name:    d_vp9000.h
 *
 * Purpose: Header file for MX drivers for Velmex VP9000 motor controllers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_VP9000_H__
#define __D_VP9000_H__

#include "i_vp9000.h"

/* ============ Motor channels ============ */

#define MXF_VP9000_DISABLE_ENCODER_CHECK	0x1

typedef struct {
	MX_RECORD *interface_record;
	int controller_number;
	int motor_number;
	long vp9000_speed;
	long vp9000_acceleration;
	unsigned long vp9000_flags;

	int motor_is_moving;
	long last_move_direction;

	int positive_limit_latch;
	int negative_limit_latch;

	long last_start_position;
	time_t last_start_time;
} MX_VP9000_MOTOR;

MX_API mx_status_type mxd_vp9000_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_vp9000_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_vp9000_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_vp9000_open( MX_RECORD *record );
MX_API mx_status_type mxd_vp9000_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_vp9000_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_vp9000_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_vp9000_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_vp9000_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_vp9000_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_vp9000_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_vp9000_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_vp9000_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_vp9000_find_home_position( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_vp9000_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_vp9000_motor_function_list;

extern mx_length_type mxd_vp9000_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_vp9000_rfield_def_ptr;

#define MXD_VP9000_STANDARD_FIELDS \
  {-1, -1, "interface_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_VP9000_MOTOR, interface_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "controller_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VP9000_MOTOR, controller_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "motor_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VP9000_MOTOR, motor_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "vp9000_speed", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VP9000_MOTOR, vp9000_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "vp9000_acceleration", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VP9000_MOTOR, vp9000_acceleration), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "vp9000_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VP9000_MOTOR, vp9000_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_VP9000_H__ */
