/*
 * Name:   i_xia_network.h
 *
 * Purpose: Header for an MX network driver for communicating with an XIA DXP
 *          multichannel analyzer system that is controlled by a remote MX
 *          server using the 'xia_xerxes' driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_XIA_NETWORK_H__
#define __I_XIA_NETWORK_H__

/* Define the data structures used by the XIA network driver. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	mx_length_type num_mcas;
	MX_RECORD **mca_record_array;

	MX_NETWORK_FIELD num_mcas_nf;
	MX_NETWORK_FIELD resynchronize_nf;

	MX_NETWORK_FIELD *busy_nf;
	MX_NETWORK_FIELD *channel_array_nf;
	MX_NETWORK_FIELD *channel_number_nf;
	MX_NETWORK_FIELD *channel_value_nf;
	MX_NETWORK_FIELD *current_num_channels_nf;
	MX_NETWORK_FIELD *current_num_rois_nf;
	MX_NETWORK_FIELD *hardware_scas_are_enabled_nf;
	MX_NETWORK_FIELD *live_time_nf;
	MX_NETWORK_FIELD *new_data_available_nf;
	MX_NETWORK_FIELD *new_statistics_available_nf;
	MX_NETWORK_FIELD *param_value_to_all_channels_nf;
	MX_NETWORK_FIELD *parameter_name_nf;
	MX_NETWORK_FIELD *parameter_value_nf;
	MX_NETWORK_FIELD *preset_clock_tick_nf;
	MX_NETWORK_FIELD *preset_type_nf;
	MX_NETWORK_FIELD *real_time_nf;
	MX_NETWORK_FIELD *roi_nf;
	MX_NETWORK_FIELD *roi_array_nf;
	MX_NETWORK_FIELD *roi_integral_nf;
	MX_NETWORK_FIELD *roi_integral_array_nf;
	MX_NETWORK_FIELD *roi_number_nf;
	MX_NETWORK_FIELD *soft_roi_nf;
	MX_NETWORK_FIELD *soft_roi_array_nf;
	MX_NETWORK_FIELD *soft_roi_integral_nf;
	MX_NETWORK_FIELD *soft_roi_integral_array_nf;
	MX_NETWORK_FIELD *soft_roi_number_nf;
	MX_NETWORK_FIELD *runtime_clock_tick_nf;
	MX_NETWORK_FIELD *start_with_preset_nf;
	MX_NETWORK_FIELD *statistics_nf;
	MX_NETWORK_FIELD *stop_nf;
} MX_XIA_NETWORK;

#define MXI_XIA_NETWORK_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_NETWORK, server_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
				NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_NETWORK, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API mx_status_type mxi_xia_network_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_xia_network_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_xia_network_open( MX_RECORD *record );
MX_API mx_status_type mxi_xia_network_resynchronize( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_xia_network_record_function_list;

extern mx_length_type mxi_xia_network_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_xia_network_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_xia_network_restore_configuration( MX_RECORD *record,
					char *configuration_filename,
					int debug_flag );

MX_API mx_status_type mxi_xia_network_is_busy( MX_MCA *mca,
					int *busy_flag,
					int debug_flag );

MX_API mx_status_type mxi_xia_network_read_parameter( MX_MCA *mca,
					char *parameter_name,
					uint32_t *value_ptr,
					int debug_flag );

MX_API mx_status_type mxi_xia_network_write_parameter( MX_MCA *mca,
					char *parameter_name,
					uint32_t value,
					int debug_flag );

MX_API mx_status_type mxi_xia_network_write_param_to_all_channels(
					MX_MCA *mca,
					char *parameter_name,
					uint32_t value,
					int debug_flag );

MX_API mx_status_type mxi_xia_network_start_run( MX_MCA *mca,
					int clear_flag,
					int debug_flag );

MX_API mx_status_type mxi_xia_network_stop_run( MX_MCA *mca,
					int debug_flag );

MX_API mx_status_type mxi_xia_network_read_spectrum( MX_MCA *mca,
					int debug_flag );

MX_API mx_status_type mxi_xia_network_read_statistics( MX_MCA *mca,
					int debug_flag );

MX_API mx_status_type mxi_xia_network_get_mx_parameter( MX_MCA *mca );

MX_API mx_status_type mxi_xia_network_set_mx_parameter( MX_MCA *mca );

/*------*/

MX_API const char *mxi_xia_network_strerror( int xia_status_code );

#endif /* __I_XIA_NETWORK_H__ */
