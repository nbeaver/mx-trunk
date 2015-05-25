/*
 * Name:    v_newport_xps_motor.h
 *
 * Purpose: Header file for custom Newport XPS motor configuration.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_NEWPORT_XPS_MOTOR_H__
#define __V_NEWPORT_XPS_MOTOR_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *newport_xps_motor_record;
	char newport_xps_motor_config_name[
			MXU_NEWPORT_XPS_CONFIG_NAME_LENGTH + 1];
} MX_NEWPORT_XPS_MOTOR_CONFIG;

#define MXV_NEWPORT_XPS_MOTOR_CONFIG_STANDARD_FIELDS \
  {-1, -1, "newport_xps_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_NEWPORT_XPS_MOTOR_CONFIG, newport_xps_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "newport_xps_motor_config_name", MXFT_STRING, NULL, \
				1, {MXU_NEWPORT_XPS_CONFIG_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
	  offsetof(MX_NEWPORT_XPS_MOTOR_CONFIG, newport_xps_motor_config_name),\
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API_PRIVATE mx_status_type
	mxv_newport_xps_motor_config_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_newport_xps_motor_config_open(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_newport_xps_motor_config_send_variable(
							MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_newport_xps_motor_config_receive_variable(
							MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST
	mxv_newport_xps_motor_config_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST
	mxv_newport_xps_motor_config_variable_function_list;

extern long mxv_newport_xps_motor_config_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_newport_xps_motor_config_rfield_def_ptr;

#endif /* __V_NEWPORT_XPS_MOTOR_H__ */
