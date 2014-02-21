/*
 * Name:   o_network.h
 *
 * Purpose: MX operation header for operations controlled by a remote MX server.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __O_NETWORK_H__
#define __O_NETWORK_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD start_nf;
	MX_NETWORK_FIELD status_nf;
	MX_NETWORK_FIELD stop_nf;

} MX_NETWORK_OPERATION;

#define MXO_NETWORK_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_OPERATION, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
		NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_OPERATION,remote_record_name),\
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxo_network_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxo_network_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxo_network_get_status( MX_OPERATION *op );
MX_API_PRIVATE mx_status_type mxo_network_start( MX_OPERATION *op );
MX_API_PRIVATE mx_status_type mxo_network_stop( MX_OPERATION *op );

extern MX_RECORD_FUNCTION_LIST mxo_network_record_function_list;
extern MX_OPERATION_FUNCTION_LIST mxo_network_operation_function_list;

extern long mxo_network_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxo_network_rfield_def_ptr;

#endif /* __O_NETWORK_H__ */
