/*
 * Name:   i_handel_network.h
 *
 * Purpose: Header for an MX network driver for communicating with an XIA DXP
 *          multichannel analyzer system that is controlled by a remote MX
 *          server using the 'handel' driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001-2006, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_HANDEL_NETWORK_H__
#define __I_HANDEL_NETWORK_H__

/* Define the data structures used by the Handel network driver. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	double last_measurement_interval;

	unsigned long num_mcas;
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
} MX_HANDEL_NETWORK;

#define MXI_HANDEL_NETWORK_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_NETWORK, server_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
				NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL_NETWORK, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "last_measurement_interval", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_HANDEL_NETWORK, last_measurement_interval),\
	{0}, NULL, 0 }

MX_API mx_status_type mxi_handel_network_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_handel_network_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_handel_network_open( MX_RECORD *record );
MX_API mx_status_type mxi_handel_network_resynchronize( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_handel_network_record_function_list;

extern long mxi_handel_network_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_handel_network_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_handel_network_restore_configuration(
					MX_RECORD *record,
					char *configuration_filename,
					mx_bool_type debug_flag );

MX_API mx_status_type mxi_handel_network_is_busy( MX_MCA *mca,
					mx_bool_type *busy_flag );

MX_API mx_status_type mxi_handel_network_get_run_data( MX_MCA *mca,
					char *name,
					void *value_ptr );

MX_API mx_status_type mxi_handel_network_get_acquisition_values( MX_MCA *mca,
					char *value_name,
					double *value_ptr );

MX_API mx_status_type mxi_handel_network_set_acquisition_values( MX_MCA *mca,
					char *value_name,
					double *value_ptr,
					mx_bool_type apply_flag );

MX_API mx_status_type mxi_handel_network_set_acq_for_all_channels( MX_MCA *mca,
					char *value_name,
					double *value_ptr,
					mx_bool_type apply_flag );

MX_API mx_status_type mxi_handel_network_apply( MX_MCA *mca,
					long module_number );

MX_API mx_status_type mxi_handel_network_read_parameter( MX_MCA *mca,
					char *parameter_name,
					unsigned long *value_ptr );

MX_API mx_status_type mxi_handel_network_write_parameter( MX_MCA *mca,
					char *parameter_name,
					unsigned long value );

MX_API mx_status_type mxi_handel_network_write_param_to_all_channels(
					MX_MCA *mca,
					char *parameter_name,
					unsigned long value );

MX_API mx_status_type mxi_handel_network_start_run( MX_MCA *mca,
					mx_bool_type clear_flag );

MX_API mx_status_type mxi_handel_network_stop_run( MX_MCA *mca );

MX_API mx_status_type mxi_handel_network_read_spectrum( MX_MCA *mca );

MX_API mx_status_type mxi_handel_network_read_statistics( MX_MCA *mca );

MX_API mx_status_type mxi_handel_network_get_mx_parameter( MX_MCA *mca );

MX_API mx_status_type mxi_handel_network_set_mx_parameter( MX_MCA *mca );

/*------*/

MX_API const char *mxi_handel_network_strerror( int handel_status_code );

#endif /* __I_HANDEL_NETWORK_H__ */
