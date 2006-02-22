/*
 * Name:    d_segmented_move.h 
 *
 * Purpose: Header file for MX motor driver for motors that break up
 *          long moves into short segments.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SEGMENTED_MOVE_H__
#define __D_SEGMENTED_MOVE_H__

#include "mx_motor.h"
#include "mx_net.h"

typedef struct {
	MX_RECORD *real_motor_record;
	double segment_length;

	int more_segments_needed;
	double final_destination;
} MX_SEGMENTED_MOVE;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_segmented_move_initialize_type( long type );
MX_API mx_status_type mxd_segmented_move_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_segmented_move_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_segmented_move_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_segmented_move_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_segmented_move_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_segmented_move_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_segmented_move_open( MX_RECORD *record );
MX_API mx_status_type mxd_segmented_move_close( MX_RECORD *record );
MX_API mx_status_type mxd_segmented_move_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_segmented_move_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_segmented_move_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_segmented_move_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_segmented_move_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_segmented_move_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_segmented_move_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_segmented_move_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_segmented_move_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_segmented_move_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_segmented_move_constant_velocity_move(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_segmented_move_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_segmented_move_set_parameter( MX_MOTOR *motor );

MX_API mx_status_type mxd_segmented_move_get_pointers( MX_MOTOR *motor,
				MX_SEGMENTED_MOVE **segmented_move,
				const char *calling_fname );

extern MX_RECORD_FUNCTION_LIST mxd_segmented_move_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_segmented_move_motor_function_list;

extern long mxd_segmented_move_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_segmented_move_rfield_def_ptr;

#define MXD_SEGMENTED_MOVE_STANDARD_FIELDS \
  {-1, -1, "real_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SEGMENTED_MOVE, real_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "segment_length", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SEGMENTED_MOVE, segment_length), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_SEGMENTED_MOVE_H__ */
