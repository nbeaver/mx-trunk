/* 
 * Name:    d_network_sca.h
 *
 * Purpose: Header file for MX network single channel analyzers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_SCA_H__
#define __D_NETWORK_SCA_H__

#include "mx_sca.h"

#define MXD_NSCA_TICKS_PER_SECOND	(50.0)

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD gain_nf;
	MX_NETWORK_FIELD lower_level_nf;
	MX_NETWORK_FIELD resynchronize_nf;
	MX_NETWORK_FIELD sca_mode_nf;
	MX_NETWORK_FIELD time_constant_nf;
	MX_NETWORK_FIELD upper_level_nf;
} MX_NETWORK_SCA;

#define MXD_NETWORK_SCA_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_NETWORK_SCA, server_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_SCA, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API mx_status_type mxd_network_sca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_sca_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_sca_open( MX_RECORD *record );
MX_API mx_status_type mxd_network_sca_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_network_sca_get_parameter( MX_SCA *sca );
MX_API mx_status_type mxd_network_sca_set_parameter( MX_SCA *sca );

extern MX_RECORD_FUNCTION_LIST mxd_network_sca_record_function_list;
extern MX_SCA_FUNCTION_LIST mxd_network_sca_sca_function_list;

extern long mxd_network_sca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_sca_rfield_def_ptr;

#endif /* __D_NETWORK_SCA_H__ */
