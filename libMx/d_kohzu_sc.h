/*
 * Name:    d_kohzu_sc.h
 *
 * Purpose: Header file for MX drivers for Kohzu SC-200, SC-400, and SC-800
 *          motor controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KOHZU_SC_H__
#define __D_KOHZU_SC_H__

/* Values for the flags variable. */

#define MXF_KOHZU_SC_MOTOR_USE_ENCODER_POSITION		0x0001
#define MXF_KOHZU_SC_MOTOR_DISABLE_HARDWARE_LIMITS	0x1000

typedef struct {
	MX_RECORD *kohzu_sc_record;
	int axis_number;
	long kohzu_sc_flags;
} MX_KOHZU_SC_MOTOR;

MX_API mx_status_type mxd_kohzu_sc_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_kohzu_sc_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_kohzu_sc_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_kohzu_sc_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_kohzu_sc_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_kohzu_sc_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_kohzu_sc_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_kohzu_sc_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_kohzu_sc_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_kohzu_sc_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_kohzu_sc_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_kohzu_sc_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_kohzu_sc_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_kohzu_sc_simultaneous_start( int num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						int flags );
MX_API mx_status_type mxd_kohzu_sc_get_status( MX_MOTOR *motor );

MX_API mx_status_type mxd_kohzu_sc_enable_continuous_mode(
				MX_KOHZU_SC_MOTOR *kohzu_sc_motor,
				MX_KOHZU_SC *kohzu_sc,
				int enable_flag );

extern MX_RECORD_FUNCTION_LIST mxd_kohzu_sc_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_kohzu_sc_motor_function_list;

extern long mxd_kohzu_sc_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_kohzu_sc_rfield_def_ptr;

#define MXD_KOHZU_SC_MOTOR_STANDARD_FIELDS \
  {-1, -1, "kohzu_sc_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_KOHZU_SC_MOTOR, kohzu_sc_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KOHZU_SC_MOTOR, axis_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "kohzu_sc_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KOHZU_SC_MOTOR, kohzu_sc_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_KOHZU_SC_H__ */
