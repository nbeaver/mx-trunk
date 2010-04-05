/*
 * Name:    d_limited_move.h 
 *
 * Purpose: Header file for MX pseudomotor driver that limits the magnitude
 *          of relative moves.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_LIMITED_MOVE_H__
#define __D_LIMITED_MOVE_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *real_motor_record;
	double relative_negative_limit;
	double relative_positive_limit;
} MX_LIMITED_MOVE;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_limited_move_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_limited_move_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_limited_move_open( MX_RECORD *record );
MX_API mx_status_type mxd_limited_move_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_limited_move_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_limited_move_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_limited_move_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_limited_move_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_limited_move_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_limited_move_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_limited_move_constant_velocity_move(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_limited_move_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_limited_move_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_limited_move_get_status( MX_MOTOR *motor );
MX_API mx_status_type mxd_limited_move_get_extended_status( MX_MOTOR *motor );

MX_API mx_status_type mxd_limited_move_get_pointers( MX_MOTOR *motor,
				MX_LIMITED_MOVE **limited_move,
				const char *calling_fname );

extern MX_RECORD_FUNCTION_LIST mxd_limited_move_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_limited_move_motor_function_list;

extern long mxd_limited_move_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_limited_move_rfield_def_ptr;

#define MXD_LIMITED_MOVE_STANDARD_FIELDS \
  {-1, -1, "real_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LIMITED_MOVE, real_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "relative_negative_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT,offsetof(MX_LIMITED_MOVE, relative_negative_limit),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "relative_positive_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT,offsetof(MX_LIMITED_MOVE, relative_positive_limit),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_LIMITED_MOVE_H__ */

