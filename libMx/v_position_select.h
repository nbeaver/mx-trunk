/*
 * Name:    v_position_select.h
 *
 * Purpose: Header file for the motor position select record.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_POSITION_SELECT_H__
#define __V_POSITION_SELECT_H__

typedef struct {
	MX_RECORD *motor_record;
	long num_positions;
	double *position_array;
} MX_POSITION_SELECT;

#define MX_POSITION_SELECT_STANDARD_FIELDS \
  {-1, -1, "motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POSITION_SELECT, motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_positions", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POSITION_SELECT, num_positions), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "position_array", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POSITION_SELECT, position_array), \
	{sizeof(double)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

MX_API_PRIVATE mx_status_type mxv_position_select_initialize_type( long );
MX_API_PRIVATE mx_status_type mxv_position_select_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_position_select_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_position_select_receive_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_position_select_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_position_select_variable_function_list;

extern mx_length_type mxv_position_select_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_position_select_rfield_def_ptr;

#endif /* __V_POSITION_SELECT_H__ */

