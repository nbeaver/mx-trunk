/*
 * Name:    d_u500.h
 *
 * Purpose: Header file for MX drivers for Aerotech Unidex 500 series motors
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_U500_H__
#define __D_U500_H__

#include "i_u500.h"

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *u500_record;
	int board_number;
	char axis_name;
	double default_speed;

	int motor_number;
} MX_U500_MOTOR;

MX_API mx_status_type mxd_u500_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_u500_finish_record_initialization(MX_RECORD *record);
MX_API mx_status_type mxd_u500_print_structure( FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_u500_open( MX_RECORD *record );
MX_API mx_status_type mxd_u500_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_u500_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_u500_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_u500_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_u500_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_u500_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_u500_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_u500_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_u500_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_u500_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_u500_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_u500_motor_function_list;

extern long mxd_u500_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_u500_rfield_def_ptr;

#define MXD_U500_STANDARD_FIELDS \
  {-1, -1, "u500_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500_MOTOR, u500_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "board_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500_MOTOR, board_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "axis_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500_MOTOR, axis_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "default_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500_MOTOR, default_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* === Driver specific functions === */

#endif /* __D_U500_H__ */

