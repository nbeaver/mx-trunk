/*
 * Name:    d_network_relay.h
 *
 * Purpose: MX header file for network relay support.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2002, 2004, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_RELAY_H__
#define __D_NETWORK_RELAY_H__

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD pulse_duration_nf;
	MX_NETWORK_FIELD pulse_on_value_nf;
	MX_NETWORK_FIELD pulse_off_value_nf;
	MX_NETWORK_FIELD relay_command_nf;
	MX_NETWORK_FIELD relay_status_nf;
} MX_NETWORK_RELAY;

#define MXD_NETWORK_RELAY_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_RELAY, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_RELAY, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }


/* Define all of the device functions. */

MX_API mx_status_type mxd_network_relay_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_relay_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_network_relay_relay_command( MX_RELAY *relay );
MX_API mx_status_type mxd_network_relay_get_relay_status( MX_RELAY *relay );
MX_API mx_status_type mxd_network_relay_pulse( MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_network_relay_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_network_relay_relay_function_list;

extern long mxd_network_relay_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_relay_rfield_def_ptr;

#endif /* __D_NETWORK_RELAY_H__ */

