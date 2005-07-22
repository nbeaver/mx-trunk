/*
 * Name:    d_network_ccd.h
 *
 * Purpose: Header for MX network CCD devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_CCD_H__
#define __D_NETWORK_CCD_H__

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD bin_size_nf;
	MX_NETWORK_FIELD ccd_flags_nf;
	MX_NETWORK_FIELD correct_nf;
	MX_NETWORK_FIELD data_frame_size_nf;
	MX_NETWORK_FIELD dezinger_nf;
	MX_NETWORK_FIELD header_variable_contents_nf;
	MX_NETWORK_FIELD header_variable_name_nf;
	MX_NETWORK_FIELD preset_time_nf;
	MX_NETWORK_FIELD readout_nf;
	MX_NETWORK_FIELD status_nf;
	MX_NETWORK_FIELD stop_nf;
	MX_NETWORK_FIELD writefile_nf;
	MX_NETWORK_FIELD writefile_name_nf;
} MX_NETWORK_CCD;

#define MXD_NETWORK_CCD_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_CCD, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
					NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_CCD, remote_record_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_network_ccd_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_network_ccd_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_network_ccd_open( MX_RECORD *record );

MX_API mx_status_type mxd_network_ccd_start( MX_CCD *ccd );
MX_API mx_status_type mxd_network_ccd_stop( MX_CCD *ccd );
MX_API mx_status_type mxd_network_ccd_get_status( MX_CCD *ccd );
MX_API mx_status_type mxd_network_ccd_readout( MX_CCD *ccd );
MX_API mx_status_type mxd_network_ccd_dezinger( MX_CCD *ccd );
MX_API mx_status_type mxd_network_ccd_correct( MX_CCD *ccd );
MX_API mx_status_type mxd_network_ccd_writefile( MX_CCD *ccd );
MX_API mx_status_type mxd_network_ccd_get_parameter( MX_CCD *ccd );
MX_API mx_status_type mxd_network_ccd_set_parameter( MX_CCD *ccd );

extern MX_RECORD_FUNCTION_LIST mxd_network_ccd_record_function_list;
extern MX_CCD_FUNCTION_LIST mxd_network_ccd_ccd_function_list;

extern long mxd_network_ccd_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_ccd_rfield_def_ptr;

#endif /* __D_NETWORK_CCD_H__ */

