/*
 * Name:    d_eiger.h
 *
 * Purpose: Header file for the Dectris Eiger series of detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EIGER_H__
#define __D_EIGER_H__

#define MXU_EIGER_SIMPLON_VERSION_LENGTH	20
#define MXU_EIGER_TRANSFER_MODE_NAME_LENGTH	20

#define MXU_EIGER_STATE_LENGTH			80
#define MXU_EIGER_ERROR_LENGTH			200

#define MXU_EIGER_DESCRIPTION_LENGTH		40
#define MXU_EIGER_SOFTWARE_VERSION_LENGTH	40
#define MXU_EIGER_DETECTOR_NUMBER_LENGTH	40

#define MXU_EIGER_MODULE_NAME_LENGTH		40
#define MXU_EIGER_KEY_NAME_LENGTH		80
#define MXU_EIGER_KEY_VALUE_LENGTH		80
#define MXU_EIGER_KEY_RESPONSE_LENGTH		1024

#define MXU_EIGER_KEY_TYPE_LENGTH		20

/*--- Values for 'trigger_command' --- */

#define MXS_EIGER_CMD_NONE		0
#define MXS_EIGER_CMD_TRIGGER		1

/*--- Values for 'trigger_status' --- */

#define MXS_EIGER_STAT_NOT_INITIALIZED	(-1)
#define MXS_EIGER_STAT_IDLE		0

/*--- Values for 'transfer_mode' ---*/

#define MXF_EIGER_TRANSFER_MONITOR	0x1
#define MXF_EIGER_TRANSFER_STREAM	0x2

/*---*/

typedef struct {
	MX_RECORD *record;

	MX_RECORD *url_server_record;
	MX_RECORD *url_trigger_record;
	char hostname[MXU_HOSTNAME_LENGTH+1];
	char simplon_version[MXU_EIGER_SIMPLON_VERSION_LENGTH+1];
	unsigned long eiger_flags;
	MX_RECORD *photon_energy_record;
	char transfer_mode_name[MXU_EIGER_TRANSFER_MODE_NAME_LENGTH+1];

	MX_THREAD *trigger_thread;

	MX_MUTEX *trigger_thread_init_mutex;
	MX_CONDITION_VARIABLE *trigger_thread_init_cv;

	MX_MUTEX *trigger_thread_command_mutex;
	MX_CONDITION_VARIABLE *trigger_thread_command_cv;

	MX_MUTEX *trigger_thread_status_mutex;
	MX_CONDITION_VARIABLE *trigger_thread_status_cv;

	long trigger_command;
	long trigger_status;

	double trigger_count_time;

	MX_CLOCK_TICK expected_finish_tick;

	unsigned long transfer_mode;

	mx_bool_type monitor_mode;
	mx_bool_type stream_mode;

	unsigned long bit_depth_image;
	unsigned long bit_depth_readout;

	char state[MXU_EIGER_STATE_LENGTH+1];
	char error[MXU_EIGER_ERROR_LENGTH+1];

	uint64_t time;		/* This is converted to POSIX epoch time. */

	double temperature;
	double humidity;
	double dcu_buffer_free;

	char description[MXU_EIGER_DESCRIPTION_LENGTH+1];
	char software_version[MXU_EIGER_SOFTWARE_VERSION_LENGTH+1];
	char detector_number[MXU_EIGER_DETECTOR_NUMBER_LENGTH+1];

	char module_name[MXU_EIGER_MODULE_NAME_LENGTH+1];
	char key_name[MXU_EIGER_KEY_NAME_LENGTH+1];
	char key_value[MXU_EIGER_KEY_VALUE_LENGTH+1];
	char key_response[MXU_EIGER_KEY_RESPONSE_LENGTH+1];

	char key_type[MXU_EIGER_KEY_TYPE_LENGTH+1];

	unsigned long sequence_id;

} MX_EIGER;

#define MXLV_EIGER_MONITOR_MODE		94001
#define MXLV_EIGER_STREAM_MODE		94002
#define MXLV_EIGER_BIT_DEPTH_IMAGE		94003
#define MXLV_EIGER_BIT_DEPTH_READOUT		94004
#define MXLV_EIGER_STATE			94005
#define MXLV_EIGER_ERROR			94006
#define MXLV_EIGER_TIME				94007
#define MXLV_EIGER_TEMPERATURE			94008
#define MXLV_EIGER_HUMIDITY			94009
#define MXLV_EIGER_DCU_BUFFER_FREE		94010
#define MXLV_EIGER_KEY_NAME			94011
#define MXLV_EIGER_KEY_VALUE			94012
#define MXLV_EIGER_KEY_RESPONSE			94013

#define MXD_EIGER_STANDARD_FIELDS \
  {-1, -1, "url_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, url_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "url_trigger_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, url_trigger_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "simplon_version", MXFT_STRING, \
			NULL, 1, {MXU_EIGER_SIMPLON_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, simplon_version), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "eiger_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, eiger_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "photon_energy_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, photon_energy_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "transfer_mode_name", MXFT_STRING, \
			NULL, 1, {MXU_EIGER_TRANSFER_MODE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, transfer_mode_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "transfer_mode", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, transfer_mode), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "trigger_count_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, trigger_count_time), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_EIGER_MONITOR_MODE, -1, "monitor_mode", MXFT_BOOL, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, monitor_mode), \
	{0}, NULL, 0 }, \
  \
  {MXLV_EIGER_STREAM_MODE, -1, "stream_mode", MXFT_BOOL, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, stream_mode), \
	{0}, NULL, 0 }, \
  \
  {MXLV_EIGER_BIT_DEPTH_IMAGE, -1, "bit_depth_image", \
					MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, bit_depth_image), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_EIGER_BIT_DEPTH_READOUT, -1, "bit_depth_readout", \
					MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, bit_depth_readout), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_EIGER_STATE, -1, "state", MXFT_STRING, \
	  				NULL, 1, {MXU_EIGER_STATE_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, state), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_EIGER_ERROR, -1, "error", MXFT_STRING, \
	  				NULL, 1, {MXU_EIGER_ERROR_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, state), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_EIGER_TIME, -1, "time", MXFT_UINT64, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, time), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_EIGER_TEMPERATURE, -1, "temperature", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, temperature), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_EIGER_HUMIDITY, -1, "humidity", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, humidity), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_EIGER_DCU_BUFFER_FREE, -1, "dcu_buffer_free", \
	  					MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, dcu_buffer_free), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "description", MXFT_STRING, NULL, 1, {MXU_EIGER_DESCRIPTION_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, description), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "software_version", MXFT_STRING, \
				NULL, 1, {MXU_EIGER_SOFTWARE_VERSION_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, software_version), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "detector_number", MXFT_STRING, NULL, \
	  				1, {MXU_EIGER_DETECTOR_NUMBER_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, detector_number), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "module_name", MXFT_STRING, NULL, 1, {MXU_EIGER_MODULE_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, module_name), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {MXLV_EIGER_KEY_NAME, -1, "key_name", MXFT_STRING, NULL, \
	  				1, {MXU_EIGER_KEY_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, key_name), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {MXLV_EIGER_KEY_VALUE, -1, "key_value", MXFT_STRING, NULL, \
	  				1, {MXU_EIGER_KEY_VALUE_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, key_value), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {MXLV_EIGER_KEY_RESPONSE, -1, "key_response", MXFT_STRING, NULL, \
	  				1, {MXU_EIGER_KEY_RESPONSE_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, key_response), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {-1, -1, "key_type", MXFT_STRING, NULL, 1, {MXU_EIGER_KEY_TYPE_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, key_type), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "sequence_id", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, sequence_id), \
	{0}, NULL, MXFF_READ_ONLY }

MX_API mx_status_type mxd_eiger_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_eiger_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_eiger_open( MX_RECORD *record );
MX_API mx_status_type mxd_eiger_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_eiger_special_processing_setup(
						MX_RECORD *record );

MX_API mx_status_type mxd_eiger_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_get_last_and_total_frame_numbers(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_get_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_transfer_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_eiger_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_eiger_ad_function_list;

extern long mxd_eiger_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_eiger_rfield_def_ptr;

#endif /* __D_EIGER_H__ */

