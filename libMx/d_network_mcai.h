/*
 * Name:    d_network_mcai.h
 *
 * Purpose: MX network multichannel analog input driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_MCAI_H__
#define __D_NETWORK_MCAI_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD channel_array_nf;
} MX_NETWORK_MCAI;

#define MXD_NETWORK_MCAI_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_MCAI, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, NULL, \
				1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_MCAI, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_network_mcai_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_network_mcai_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_mcai_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_mcai_open( MX_RECORD *record );

MX_API mx_status_type mxd_network_mcai_read( MX_MCAI *mcai );

extern MX_RECORD_FUNCTION_LIST mxd_network_mcai_record_function_list;
extern MX_MCAI_FUNCTION_LIST mxd_network_mcai_mcai_function_list;

extern long mxd_network_mcai_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_mcai_rfield_def_ptr;

#endif /* __D_NETWORK_MCAI_H__ */

