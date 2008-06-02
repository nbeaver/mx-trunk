/*
 * Name:    v_bluice_operation.h
 *
 * Purpose: Header file for handling Blu-Ice 'operation' messages.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_BLUICE_OPERATION_H__
#define __V_BLUICE_OPERATION_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bluice_server_record;
	char bluice_name[MXU_BLUICE_NAME_LENGTH+1];

	MX_BLUICE_FOREIGN_DEVICE *foreign_device;
} MX_BLUICE_OPERATION;

#define MXV_BLUICE_OPERATION_STANDARD_FIELDS \
  {-1, -1, "bluice_server_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_OPERATION, bluice_server_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "bluice_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_NAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_OPERATION, bluice_name), \
        {sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API_PRIVATE mx_status_type mxv_bluice_operation_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_bluice_operation_finish_record_initialization(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_bluice_operation_open( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_bluice_operation_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_bluice_operation_receive_variable(
						MX_VARIABLE *variable );

extern long mxv_bluice_operation_num_record_fields;
extern MX_RECORD_FUNCTION_LIST mxv_bluice_operation_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_bluice_operation_variable_function_list;
extern MX_RECORD_FIELD_DEFAULTS *mxv_bluice_operation_rfield_def_ptr;

#endif /* __V_BLUICE_OPERATION_H__ */

