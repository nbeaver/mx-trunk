/*
 * Name:    d_compumotor.h
 *
 * Purpose: Header file for MX drivers for Compumotor 6000 and 6K series
 *          motor controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_COMPUMOTOR_H__
#define __D_COMPUMOTOR_H__

#include "i_compumotor.h"

/* Values for the flags variable. */

#define MXF_COMPUMOTOR_USE_ENCODER_POSITION			0x0001

#define MXF_COMPUMOTOR_ROUND_POSITION_OUTPUT_TO_NEAREST_INTEGER	0x0002

#define MXF_COMPUMOTOR_DISABLE_HARDWARE_LIMITS			0x1000

typedef struct {
	MX_RECORD *compumotor_interface_record;
	int32_t controller_number;
	int32_t axis_number;
	mx_hex_type flags;

	int32_t controller_index;
	bool continuous_mode_enabled;
	bool is_servo;
	double axis_resolution;
} MX_COMPUMOTOR;

MX_API mx_status_type mxd_compumotor_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_open( MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_close( MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_compumotor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_simultaneous_start( int num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						int flags );
MX_API mx_status_type mxd_compumotor_get_status( MX_MOTOR *motor );

MX_API mx_status_type mxd_compumotor_enable_continuous_mode(
				MX_COMPUMOTOR *compumotor,
				MX_COMPUMOTOR_INTERFACE *compumotor_interface,
				bool enable_flag );

extern MX_RECORD_FUNCTION_LIST mxd_compumotor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_compumotor_motor_function_list;

extern mx_length_type mxd_compumotor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_compumotor_rfield_def_ptr;

#define MXD_COMPUMOTOR_STANDARD_FIELDS \
  {-1, -1, "compumotor_interface_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR, compumotor_interface_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "controller_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, controller_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, axis_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_COMPUMOTOR_H__ */
