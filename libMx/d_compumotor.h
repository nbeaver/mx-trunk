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
 * Copyright 1999-2002, 2005-2006, 2013-2014 Illinois Institute of Technology
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

#define MXF_COMPUMOTOR_DETECT_SERVO				0x0010

#define MXF_COMPUMOTOR_IS_STEPPER				0x0020

#define MXF_COMPUMOTOR_IS_SERVO					0x0040

#define MXF_COMPUMOTOR_DISABLE_HARDWARE_LIMITS			0x1000

typedef struct {
	MX_RECORD *record;

	MX_RECORD *compumotor_interface_record;
	long controller_number;
	long axis_number;
	unsigned long flags;

	long controller_index;
	mx_bool_type continuous_mode_enabled;
	mx_bool_type is_servo;

	double motor_resolution;	/* Steps per revolution = DRES */
	double encoder_resolution;	/* Ticks per revolution = ERES */

	/* Velocity resolution is also used for acceleration and deceleration.*/

	double velocity_resolution;	/* Ticks OR steps per revolution. */
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

MX_API mx_status_type mxd_compumotor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_compumotor_simultaneous_start( long num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						unsigned long flags );
MX_API mx_status_type mxd_compumotor_get_status( MX_MOTOR *motor );

MX_API mx_status_type mxd_compumotor_enable_continuous_mode(
				MX_COMPUMOTOR *compumotor,
				MX_COMPUMOTOR_INTERFACE *compumotor_interface,
				int enable_flag );

extern MX_RECORD_FUNCTION_LIST mxd_compumotor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_compumotor_motor_function_list;

extern long mxd_compumotor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_compumotor_rfield_def_ptr;

#define MXD_COMPUMOTOR_STANDARD_FIELDS \
  {-1, -1, "compumotor_interface_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR, compumotor_interface_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "controller_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, controller_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, axis_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "controller_index", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, controller_index), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "continuous_mode_enabled", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, continuous_mode_enabled), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "is_servo", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, is_servo), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "motor_resolution", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, motor_resolution), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "encoder_resolution", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, encoder_resolution), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "velocity_resolution", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR, velocity_resolution), \
	{0}, NULL, MXFF_READ_ONLY }

#endif /* __D_COMPUMOTOR_H__ */
