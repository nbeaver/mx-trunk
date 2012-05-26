/* 
 * Name:    d_network_mca.h
 *
 * Purpose: Header file for MX network multichannel analyzers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2004, 2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_MCA_H__
#define __D_NETWORK_MCA_H__

#include "mx_mca.h"

#define MXD_NMCA_TICKS_PER_SECOND	(50.0)

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD busy_nf;
	MX_NETWORK_FIELD channel_array_nf;
	MX_NETWORK_FIELD channel_number_nf;
	MX_NETWORK_FIELD channel_value_nf;
	MX_NETWORK_FIELD clear_nf;
	MX_NETWORK_FIELD current_num_channels_nf;
	MX_NETWORK_FIELD current_num_rois_nf;
	MX_NETWORK_FIELD energy_offset_nf;
	MX_NETWORK_FIELD energy_scale_nf;
	MX_NETWORK_FIELD input_count_rate_nf;
	MX_NETWORK_FIELD live_time_nf;
	MX_NETWORK_FIELD num_soft_rois_nf;
	MX_NETWORK_FIELD output_count_rate_nf;
	MX_NETWORK_FIELD preset_count_nf;
	MX_NETWORK_FIELD preset_live_time_nf;
	MX_NETWORK_FIELD preset_real_time_nf;
	MX_NETWORK_FIELD preset_type_nf;
	MX_NETWORK_FIELD real_time_nf;
	MX_NETWORK_FIELD resynchronize_nf;
	MX_NETWORK_FIELD roi_nf;
	MX_NETWORK_FIELD roi_array_nf;
	MX_NETWORK_FIELD roi_integral_nf;
	MX_NETWORK_FIELD roi_integral_array_nf;
	MX_NETWORK_FIELD roi_number_nf;
	MX_NETWORK_FIELD soft_roi_nf;
	MX_NETWORK_FIELD soft_roi_array_nf;
	MX_NETWORK_FIELD soft_roi_integral_nf;
	MX_NETWORK_FIELD soft_roi_integral_array_nf;
	MX_NETWORK_FIELD soft_roi_number_nf;
	MX_NETWORK_FIELD start_nf;
	MX_NETWORK_FIELD stop_nf;
} MX_NETWORK_MCA;

#define MXD_NETWORK_MCA_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_NETWORK_MCA, server_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_MCA, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API mx_status_type mxd_network_mca_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_network_mca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_mca_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_mca_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_network_mca_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_network_mca_open( MX_RECORD *record );
MX_API mx_status_type mxd_network_mca_close( MX_RECORD *record );
MX_API mx_status_type mxd_network_mca_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_network_mca_start( MX_MCA *mca );
MX_API mx_status_type mxd_network_mca_stop( MX_MCA *mca );
MX_API mx_status_type mxd_network_mca_read( MX_MCA *mca );
MX_API mx_status_type mxd_network_mca_clear( MX_MCA *mca );
MX_API mx_status_type mxd_network_mca_busy( MX_MCA *mca );
MX_API mx_status_type mxd_network_mca_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_network_mca_set_parameter( MX_MCA *mca );

extern MX_RECORD_FUNCTION_LIST mxd_network_mca_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_network_mca_mca_function_list;

extern long mxd_network_mca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_mca_rfield_def_ptr;

#endif /* __D_NETWORK_MCA_H__ */
