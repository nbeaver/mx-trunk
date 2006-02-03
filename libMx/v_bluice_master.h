/*
 * Name:    v_bluice_master.h
 *
 * Purpose: Header file for Blu-Ice control variables.
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

#ifndef __V_BLUICE_MASTER_H__
#define __V_BLUICE_MASTER_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bluice_server_record;
} MX_BLUICE_MASTER;

#define MXV_BLUICE_MASTER_STANDARD_FIELDS \
  {-1, -1, "bluice_server_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_MASTER, bluice_server_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API_PRIVATE mx_status_type mxv_bluice_master_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_bluice_master_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_bluice_master_receive_variable(
						MX_VARIABLE *variable );

extern mx_length_type mxv_bluice_master_num_record_fields;
extern MX_RECORD_FUNCTION_LIST mxv_bluice_master_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_bluice_master_variable_function_list;
extern MX_RECORD_FIELD_DEFAULTS *mxv_bluice_master_rfield_def_ptr;

#endif /* __V_BLUICE_MASTER_H__ */

