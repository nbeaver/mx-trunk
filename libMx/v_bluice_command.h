/*
 * Name:    v_bluice_command.h
 *
 * Purpose: Header file for Blu-Ice sending commands to a Blu-Ice server.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_BLUICE_COMMAND_H__
#define __V_BLUICE_COMMAND_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bluice_server_record;
} MX_BLUICE_COMMAND;

#define MXV_BLUICE_COMMAND_STANDARD_FIELDS \
  {-1, -1, "bluice_server_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_COMMAND, bluice_server_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API_PRIVATE mx_status_type mxv_bluice_command_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_bluice_command_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_bluice_command_receive_variable(
						MX_VARIABLE *variable );

extern mx_length_type mxv_bluice_command_num_record_fields;
extern MX_RECORD_FUNCTION_LIST mxv_bluice_command_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_bluice_command_variable_function_list;
extern MX_RECORD_FIELD_DEFAULTS *mxv_bluice_command_rfield_def_ptr;

#endif /* __V_BLUICE_COMMAND_H__ */

