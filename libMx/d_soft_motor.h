/*
 * Name:    d_soft_motor.h 
 *
 * Purpose: Header file for MX motor driver for a software emulated
 *          stepping motor.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2006, 2009, 2013
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_MOTOR_H__
#define __D_SOFT_MOTOR_H__

#include "mx_motor.h"

typedef struct {
	MX_DEAD_RECKONING dead_reckoning;

	double default_speed;
	double default_base_speed;
	double default_acceleration;

	double simulated_home_position;
	long raw_simulated_home_position;

	mx_bool_type home_search_succeeded;

	double simulated_negative_limit;
	double simulated_positive_limit;
} MX_SOFT_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_soft_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_soft_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_soft_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_soft_motor_special_processing_setup(
					MX_RECORD *record );

MX_API mx_status_type mxd_soft_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_soft_motor_simultaneous_start( long num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						unsigned long flags );
MX_API mx_status_type mxd_soft_motor_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_soft_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_soft_motor_motor_function_list;

extern long mxd_soft_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_motor_rfield_def_ptr;

#define MXLV_SOFT_MOTOR_SIMULATED_HOME_POSITION		77701
#define MXLV_SOFT_MOTOR_RAW_SIMULATED_HOME_POSITION	77702

#define MXD_SOFT_MOTOR_STANDARD_FIELDS \
  {-1, -1, "default_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MOTOR, default_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "default_base_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MOTOR, default_base_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "default_acceleration", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MOTOR, default_acceleration), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_SOFT_MOTOR_SIMULATED_HOME_POSITION, -1, "simulated_home_position", \
					MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MOTOR, simulated_home_position), \
	{0}, NULL, 0 }, \
  \
  {MXLV_SOFT_MOTOR_RAW_SIMULATED_HOME_POSITION, \
		-1, "raw_simulated_home_position", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SOFT_MOTOR, raw_simulated_home_position), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "simulated_negative_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MOTOR, simulated_negative_limit),\
	{0}, NULL, 0 }, \
  \
  {-1, -1, "simulated_positive_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MOTOR, simulated_positive_limit),\
	{0}, NULL, 0 }

#endif /* __D_SOFT_MOTOR_H__ */
