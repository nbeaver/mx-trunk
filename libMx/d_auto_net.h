/*
 * Name:    d_auto_net.h
 *
 * Purpose: Header file for an autoscale record in a remote server.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _D_AUTO_NET_H_
#define _D_AUTO_NET_H_

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD change_control_nf;
	MX_NETWORK_FIELD enabled_nf;
	MX_NETWORK_FIELD get_change_request_nf;
	MX_NETWORK_FIELD high_deadband_nf;
	MX_NETWORK_FIELD high_limit_nf;
	MX_NETWORK_FIELD low_deadband_nf;
	MX_NETWORK_FIELD low_limit_nf;
	MX_NETWORK_FIELD monitor_offset_array_nf;
	MX_NETWORK_FIELD monitor_offset_index_nf;
	MX_NETWORK_FIELD monitor_value_nf;
	MX_NETWORK_FIELD num_monitor_offsets_nf;
} MX_AUTO_NETWORK;

#define MX_AUTO_NETWORK_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AUTO_NETWORK, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AUTO_NETWORK, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }


MX_API_PRIVATE mx_status_type mxd_auto_network_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_network_finish_record_initialization(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_network_delete_record(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_network_open( MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_network_close( MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_network_dummy_function(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxd_auto_network_read_monitor(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_network_get_change_request(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_network_change_control(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_network_get_offset_index(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_network_set_offset_index(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_network_get_parameter(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_network_set_parameter(
						MX_AUTOSCALE *autoscale );

extern MX_RECORD_FUNCTION_LIST mxd_auto_network_record_function_list;
extern MX_AUTOSCALE_FUNCTION_LIST mxd_auto_network_autoscale_function_list;

extern mx_length_type mxd_auto_network_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_auto_network_rfield_def_ptr;

#endif /* _D_AUTO_NET_H_ */

