/*
 * Name:    d_elapsed_time.h 
 *
 * Purpose: Header file for MX motor driver that allows one to scan using
 *          the wall clock elapsed time as the independent variable.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ELAPSED_TIME_H__
#define __D_ELAPSED_TIME_H__

#include "mx_motor.h"

typedef struct {
	double time_of_last_reset;      /* in seconds */
} MX_ELAPSED_TIME_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_elapsed_time_initialize_type( long type );
MX_API mx_status_type mxd_elapsed_time_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_elapsed_time_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_elapsed_time_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_elapsed_time_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_elapsed_time_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_elapsed_time_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_elapsed_time_open( MX_RECORD *record );
MX_API mx_status_type mxd_elapsed_time_close( MX_RECORD *record );
MX_API mx_status_type mxd_elapsed_time_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_elapsed_time_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_elapsed_time_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_elapsed_time_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_elapsed_time_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_elapsed_time_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_elapsed_time_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_elapsed_time_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_elapsed_time_find_home_position( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_elapsed_time_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_elapsed_time_motor_function_list;

extern long mxd_elapsed_time_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_elapsed_time_rfield_def_ptr;

#define MXD_ELAPSED_TIME_MOTOR_STANDARD_FIELDS \
  {-1, -1, "time_of_last_reset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_ELAPSED_TIME_MOTOR, time_of_last_reset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_ELAPSED_TIME_H__ */

