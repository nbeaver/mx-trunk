/*
 * Name:    d_merlin_medipix.h
 *
 * Purpose: Header file for the Merlin Medipix series of detectors
 *          from Quantum Detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MERLIN_MEDIPIX_H__
#define __D_MERLIN_MEDIPIX_H__

#define MXU_MPX_SENSOR_LAYOUT_LENGTH	20

/* Bit flags for the 'merlin_flags' field. */

#define MXF_MERLIN_DEBUG_COMMAND_PORT	0x1
#define MXF_MERLIN_DEBUG_DATA_PORT	0x2

#define MXF_MERLIN_CONFIGURE_DETECTOR	0x10

typedef struct {
	MX_RECORD *record;

	char hostname[MXU_HOSTNAME_LENGTH+1];
	unsigned long command_port;
	unsigned long data_port;
	unsigned long merlin_flags;
	unsigned long num_image_buffers;

	double command_socket_timeout;			/* in seconds */

	unsigned long gain;
	unsigned long threshold0;
	unsigned long threshold1;
	unsigned long high_voltage;

	unsigned long merlin_software_version;

	MX_SOCKET *command_socket;
	MX_SOCKET *data_socket;

	MX_THREAD *monitor_thread;

	unsigned long acquisition_header_length;
	char *acquisition_header;

	unsigned long image_message_length;
	unsigned long image_message_array_length;
	char *image_message_array;

	unsigned long image_data_offset;
	unsigned long number_of_chips;
	char sensor_layout[ MXU_MPX_SENSOR_LAYOUT_LENGTH+1 ];
	unsigned long chip_select;
	unsigned long counter;
	unsigned long color_mode;
	unsigned long gain_mode;

	double external_trigger_debounce_time;		/* in seconds */

	MX_CALLBACK_MESSAGE *status_callback_message;
	double status_callback_interval;		/* in seconds */

	mx_bool_type detector_just_started;

	long old_detector_status;

	int32_t old_total_num_frames_at_start;
	int32_t old_total_num_frames;

	MX_RECORD_FIELD *vctest_field_array[4];

	/* The following values are managed via MX atomic ops. */

	/* total_num_frames_at_start is only written to by the
	 * mxd_merlin_medipix_arm() function.
	 *
	 * total_num_frames is only written to by the 
	 * mxd_merlin_medipix_monitor_thread() function.
	 */

	int32_t total_num_frames_at_start;
	int32_t total_num_frames;
} MX_MERLIN_MEDIPIX;

#define MXLV_MERLIN_MEDIPIX_STATUS_CALLBACK_INTERVAL	932001

#define MXD_MERLIN_MEDIPIX_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "command_port", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, command_port), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "data_port", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, data_port), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "merlin_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, merlin_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "num_image_buffers", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, num_image_buffers), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "command_socket_timeout", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MERLIN_MEDIPIX, command_socket_timeout), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "gain", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, gain), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "threshold0", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, threshold0), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "threshold1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, threshold1), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "high_voltage", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, high_voltage), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "merlin_software_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MERLIN_MEDIPIX, merlin_software_version), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "acquisition_header_length", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MERLIN_MEDIPIX, acquisition_header_length), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "acquisition_header", MXFT_STRING, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, acquisition_header), \
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS) }, \
  \
  {-1, -1, "image_message_length", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MERLIN_MEDIPIX, image_message_length), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "image_message_array_length", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MERLIN_MEDIPIX, image_message_array_length), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "image_message_array", MXFT_STRING, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, image_message_array), \
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS) }, \
  \
  {-1, -1, "image_data_offset", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, image_data_offset), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "number_of_chips", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, number_of_chips), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "sensor_layout", MXFT_STRING, \
	  			NULL, 1, {MXU_MPX_SENSOR_LAYOUT_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, sensor_layout), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "chip_select", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, chip_select), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "counter", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, counter), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "color_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, color_mode), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "gain_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, gain_mode), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "external_trigger_debounce_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MERLIN_MEDIPIX, external_trigger_debounce_time), \
	{0}, NULL, 0 }, \
  \
  {MXLV_MERLIN_MEDIPIX_STATUS_CALLBACK_INTERVAL, \
		-1, "status_callback_interval", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MERLIN_MEDIPIX, status_callback_interval), \
	{0}, NULL, 0 }

MX_API mx_status_type mxd_merlin_medipix_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_merlin_medipix_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_merlin_medipix_open( MX_RECORD *record );
MX_API mx_status_type mxd_merlin_medipix_special_processing_setup(
						MX_RECORD *record );

MX_API mx_status_type mxd_merlin_medipix_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_get_last_and_total_frame_numbers(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_get_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_transfer_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_load_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_save_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_copy_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_set_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_measure_correction(
							MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_merlin_medipix_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_merlin_medipix_ad_function_list;

extern long mxd_merlin_medipix_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_merlin_medipix_rfield_def_ptr;

/* Flag values for mxd_merlin_medipix_command(). */

MX_API mx_status_type mxd_merlin_medipix_command( MX_MERLIN_MEDIPIX *pilatus,
					char *command,
					char *response,
					size_t response_buffer_length,
					mx_bool_type suppress_output );

#endif /* __D_MERLIN_MEDIPIX_H__ */

