/*
 * Name:    d_network_scaler.h
 *
 * Purpose: Header file for MX network scalers.
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_SCALER_H__
#define __D_NETWORK_SCALER_H__

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD busy_nf;
	MX_NETWORK_FIELD clear_nf;
	MX_NETWORK_FIELD dark_current_nf;
	MX_NETWORK_FIELD mode_nf;
	MX_NETWORK_FIELD overflow_set_nf;
	MX_NETWORK_FIELD raw_value_nf;
	MX_NETWORK_FIELD stop_nf;
	MX_NETWORK_FIELD value_nf;
} MX_NETWORK_SCALER;

MX_API mx_status_type mxd_network_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_scaler_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_network_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_network_scaler_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_network_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_network_scaler_read_raw( MX_SCALER *scaler );
MX_API mx_status_type mxd_network_scaler_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_network_scaler_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_network_scaler_stop( MX_SCALER *scaler );
MX_API mx_status_type mxd_network_scaler_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_network_scaler_set_parameter( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_network_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_network_scaler_scaler_function_list;

extern mx_length_type mxd_network_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_scaler_rfield_def_ptr;

#define MXD_NETWORK_SCALER_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_SCALER, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_SCALER, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_NETWORK_SCALER_H__ */
