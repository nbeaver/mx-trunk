/*
 * Name:    d_record_field_motor.h
 *
 * Purpose: Header file for MX record field motor driver.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_RECORD_FIELD_MOTOR_H__
#define __D_RECORD_FIELD_MOTOR_H__

/* Values for the 'record_field_motor_flags' field. */

#define MXF_RFM_SHOW_IN_PROGRESS_DELAY		0x1

typedef struct {
	MX_RECORD *record;

	int command_in_progress;
	MX_HRT_TIMING in_progress_interval;

	unsigned long record_field_motor_flags;

	char disable_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char command_in_progress_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char busy_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char move_absolute_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char get_position_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char set_position_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char soft_abort_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char immediate_abort_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char positive_limit_hit_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char negative_limit_hit_name[MXU_RECORD_FIELD_NAME_LENGTH+1];

	MX_RECORD_FIELD_HANDLER *disable_handler;
	MX_RECORD_FIELD_HANDLER *command_in_progress_handler;
	MX_RECORD_FIELD_HANDLER *busy_handler;
	MX_RECORD_FIELD_HANDLER *move_absolute_handler;
	MX_RECORD_FIELD_HANDLER *get_position_handler;
	MX_RECORD_FIELD_HANDLER *set_position_handler;
	MX_RECORD_FIELD_HANDLER *soft_abort_handler;
	MX_RECORD_FIELD_HANDLER *immediate_abort_handler;
	MX_RECORD_FIELD_HANDLER *positive_limit_hit_handler;
	MX_RECORD_FIELD_HANDLER *negative_limit_hit_handler;
} MX_RECORD_FIELD_MOTOR;

MX_API mx_status_type mxd_record_field_motor_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_record_field_motor_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_record_field_motor_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_record_field_motor_open( MX_RECORD *record );

MX_API mx_status_type mxd_record_field_motor_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_record_field_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_record_field_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_record_field_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_record_field_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_record_field_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_record_field_motor_positive_limit_hit(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_record_field_motor_negative_limit_hit(
							MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_record_field_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_record_field_motor_motor_function_list;

extern mx_length_type mxd_record_field_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_record_field_motor_rfield_def_ptr;

#define MXD_RECORD_FIELD_MOTOR_STANDARD_FIELDS \
  {-1, -1, "record_field_motor_flags", MXFT_HEX, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RECORD_FIELD_MOTOR, record_field_motor_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "disable_name", MXFT_STRING, NULL, \
	  			1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_RECORD_FIELD_MOTOR, disable_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "command_in_progress_name", MXFT_STRING, NULL, \
	  			1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RECORD_FIELD_MOTOR, command_in_progress_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "busy_name", MXFT_STRING, NULL, \
	  			1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_RECORD_FIELD_MOTOR, busy_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "move_absolute_name", MXFT_STRING, NULL, \
	  			1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RECORD_FIELD_MOTOR, move_absolute_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "get_position_name", MXFT_STRING, NULL, \
	  			1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RECORD_FIELD_MOTOR, get_position_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "set_position_name", MXFT_STRING, NULL, \
	  			1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RECORD_FIELD_MOTOR, set_position_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "soft_abort_name", MXFT_STRING, NULL, \
	  			1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_RECORD_FIELD_MOTOR, soft_abort_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "immediate_abort_name", MXFT_STRING, NULL, \
	  			1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RECORD_FIELD_MOTOR, immediate_abort_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "positive_limit_hit_name", MXFT_STRING, NULL, \
	  			1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RECORD_FIELD_MOTOR, positive_limit_hit_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "negative_limit_hit_name", MXFT_STRING, NULL, \
	  			1, {MXU_RECORD_FIELD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RECORD_FIELD_MOTOR, negative_limit_hit_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_RECORD_FIELD_MOTOR_H__ */

