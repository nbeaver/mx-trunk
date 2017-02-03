/*
 * Name:    d_net_sample_changer.h
 *
 * Purpose: Header for network controlled sample changers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NET_SAMPLE_CHANGER_H__
#define __D_NET_SAMPLE_CHANGER_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD cooldown_nf;
	MX_NETWORK_FIELD deice_nf;
	MX_NETWORK_FIELD grab_sample_nf;
	MX_NETWORK_FIELD idle_nf;
	MX_NETWORK_FIELD immediate_abort_nf;
	MX_NETWORK_FIELD initialize_nf;
	MX_NETWORK_FIELD mount_sample_nf;
	MX_NETWORK_FIELD requested_sample_holder_nf;
	MX_NETWORK_FIELD requested_sample_id_nf;
	MX_NETWORK_FIELD reset_changer_nf;
	MX_NETWORK_FIELD select_sample_holder_nf;
	MX_NETWORK_FIELD shutdown_nf;
	MX_NETWORK_FIELD soft_abort_nf;
	MX_NETWORK_FIELD status_nf;
	MX_NETWORK_FIELD ungrab_sample_nf;
	MX_NETWORK_FIELD unmount_sample_nf;
	MX_NETWORK_FIELD unselect_sample_holder_nf;
} MX_NET_SAMPLE_CHANGER;

#define MXD_NET_SAMPLE_CHANGER_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NET_SAMPLE_CHANGER, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
		NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NET_SAMPLE_CHANGER, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_net_sample_changer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_net_sample_changer_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_net_sample_changer_initialize(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_shutdown(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_mount_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_unmount_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_grab_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_ungrab_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_select_sample_holder(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_unselect_sample_holder(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_soft_abort(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_immediate_abort(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_idle(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_reset_changer(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_get_status(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_get_parameter(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_net_sample_changer_set_parameter(
						MX_SAMPLE_CHANGER *changer );

extern MX_RECORD_FUNCTION_LIST mxd_net_sample_changer_record_function_list;

extern MX_SAMPLE_CHANGER_FUNCTION_LIST
			mxd_net_sample_changer_sample_changer_function_list;

extern long mxd_net_sample_changer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_net_sample_changer_rfield_def_ptr;

#endif /* __D_NET_SAMPLE_CHANGER_H__ */
