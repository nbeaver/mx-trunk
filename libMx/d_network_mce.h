/*
 * Name:    d_network_mce.h
 *
 * Purpose: Header file to support MX network multichannel encoders.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2004, 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_MCE_H__
#define __D_NETWORK_MCE_H__

#include "mx_mce.h"

/* ===== MCS mce data structure ===== */

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_RECORD *selected_motor_record;

	MX_NETWORK_FIELD current_num_values_nf;
	MX_NETWORK_FIELD encoder_type_nf;
	MX_NETWORK_FIELD measurement_index_nf;
	MX_NETWORK_FIELD motor_record_array_nf;
	MX_NETWORK_FIELD num_motors_nf;
	MX_NETWORK_FIELD selected_motor_name_nf;
	MX_NETWORK_FIELD value_nf;
	MX_NETWORK_FIELD value_array_nf;
	MX_NETWORK_FIELD use_window_nf;
	MX_NETWORK_FIELD window_nf;
	MX_NETWORK_FIELD window_is_available_nf;
} MX_NETWORK_MCE;

#define MXD_NETWORK_MCE_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_MCE, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
		NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_MCE, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_network_mce_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_network_mce_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_mce_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_mce_delete_record( MX_RECORD *record );

MX_API mx_status_type mxd_network_mce_read( MX_MCE *mce );
MX_API mx_status_type mxd_network_mce_get_current_num_values( MX_MCE *mce );
MX_API mx_status_type mxd_network_mce_read_measurement( MX_MCE *mce );
MX_API mx_status_type mxd_network_mce_get_motor_record_array( MX_MCE *mce );
MX_API mx_status_type mxd_network_mce_connect_mce_to_motor( MX_MCE *mce,
						MX_RECORD *motor_record );
MX_API mx_status_type mxd_network_mce_get_parameter( MX_MCE *mce );
MX_API mx_status_type mxd_network_mce_set_parameter( MX_MCE *mce );

extern MX_RECORD_FUNCTION_LIST mxd_network_mce_record_function_list;
extern MX_MCE_FUNCTION_LIST mxd_network_mce_mce_function_list;

extern long mxd_network_mce_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_mce_rfield_def_ptr;

#endif /* __D_NETWORK_MCE_H__ */

