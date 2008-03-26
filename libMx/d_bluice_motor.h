/*
 * Name:    d_bluice_motor.h
 *
 * Purpose: Header file for Blu-Ice controlled motors.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2005, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BLUICE_MOTOR_H__
#define __D_BLUICE_MOTOR_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bluice_server_record;
	char bluice_name[MXU_BLUICE_NAME_LENGTH+1];

	double bluice_scale;
	double bluice_speed;
	double bluice_acceleration_time;
	double bluice_backlash;

	MX_BLUICE_FOREIGN_DEVICE *foreign_device;
} MX_BLUICE_MOTOR;

MX_API mx_status_type mxd_bluice_motor_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_bluice_motor_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_bluice_motor_print_structure( FILE *file,
						MX_RECORD *record );

MX_API mx_status_type mxd_bluice_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_bluice_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_bluice_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_bluice_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_bluice_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_bluice_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_bluice_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_bluice_motor_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_bluice_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_bluice_motor_motor_function_list;

extern long mxd_bluice_dcss_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bluice_dcss_motor_rfield_def_ptr;

extern long mxd_bluice_dhs_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bluice_dhs_motor_rfield_def_ptr;

#define MXD_BLUICE_DCSS_MOTOR_STANDARD_FIELDS \
  {-1, -1, "bluice_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_MOTOR, bluice_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bluice_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_MOTOR, bluice_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#define MXD_BLUICE_DHS_MOTOR_STANDARD_FIELDS \
  {-1, -1, "bluice_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_MOTOR, bluice_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bluice_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_MOTOR, bluice_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bluice_scale", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_MOTOR, bluice_scale), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "bluice_speed", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_MOTOR, bluice_speed), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "bluice_acceleration_time", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_MOTOR, bluice_acceleration_time), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "bluice_backlash", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_MOTOR, bluice_backlash), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_BLUICE_MOTOR_H__ */

