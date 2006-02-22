/*
 * Name:    d_pm304.h 
 *
 * Purpose: Header file for MX motor driver to control McLennan PM304
 *          DC servo motor controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PM304_H__
#define __D_PM304_H__

#include "mx_motor.h"

/* ===== McLennan PM304 motor data structures ===== */

typedef struct {
	MX_RECORD *rs232_record;

	int axis_number;
	int axis_encoder_number;

	double minimum_event_interval;
} MX_PM304;

#define MX_PM304_POSITION_DISCREPANCY_THRESHOLD		2
#define MX_PM304_ZERO_THRESHOLD    			2

/* Define all of the interface functions. */

MX_API mx_status_type mxd_pm304_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_pm304_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_pm304_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_pm304_open( MX_RECORD *record );
MX_API mx_status_type mxd_pm304_close( MX_RECORD *record );
MX_API mx_status_type mxd_pm304_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_pm304_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_pm304_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_pm304_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_pm304_motor_function_list;

/* Define some extra functions for the use of this driver. */

MX_API mx_status_type mxd_pm304_command( MX_PM304 *pm304, char *command,
			char *response, int response_buffer_length,
			int debug_flag );
MX_API mx_status_type mxd_pm304_getline( MX_PM304 *pm304,
			char *buffer, int buffer_length, int debug_flag );
MX_API mx_status_type mxd_pm304_putline( MX_PM304 *pm304,
			char *buffer, int debug_flag );

extern long mxd_pm304_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pm304_rfield_def_ptr;

#define MXD_PM304_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PM304, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "axis_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PM304, axis_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "axis_encoder_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PM304, axis_encoder_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "minimum_event_interval", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PM304, minimum_event_interval), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_PM304_H__ */
