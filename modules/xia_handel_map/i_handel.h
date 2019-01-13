/*
 * Name:   i_handel.h
 *
 * Purpose: Header for MX driver for a generic interface to the X-Ray 
 *          Instrumentation Associates Handel library used by the 
 *          DXP series of multichannel analyzers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2009-2010, 2012, 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_HANDEL_H__
#define __I_HANDEL_H__

#define MX_IGNORE_HANDEL_NULL_STRING	TRUE

#define MX_HANDEL_ACTIVE_DETECTOR_CHANNEL_SET	(-2)

#define MXU_HANDEL_PARAMETER_NAME_LENGTH		80
#define MXU_HANDEL_ACQUISITION_VALUE_NAME_LENGTH	80

#define MXU_HANDEL_MCA_LABEL_LENGTH			120

/* The following flags are used by the 'handel_flags' field. */

#define MXF_HANDEL_IGNORE_HARDWARE_SCAS			0x1
#define MXF_HANDEL_WRITE_LOG_OUTPUT_TO_FILE		0x2
#define MXF_HANDEL_DISPLAY_CONFIGURATION_AT_STARTUP	0x4

#define MXU_HANDEL_MODULE_TYPE_LENGTH	20

#define MXU_HANDEL_MAX_MODULES		4

/* The available values for 'mapping_mode' (expressed as an integer). */

#define MXF_HANDEL_MAP_NORMAL_MODE	0
#define MXF_HANDEL_MAP_MCA_MODE		1
#define MXF_HANDEL_MAP_SCA_MODE		2
#define MXF_HANDEL_MAP_LIST_MODE	3

/* The available values for 'pixel_advance_mode' (expressed as an integer). */

#define MXF_HANDEL_ADV_NONE_MODE	0
#define MXF_HANDEL_ADV_GATE_MODE	1
#define MXF_HANDEL_ADV_SYNC_MODE	2
#define MXF_HANDEL_ADV_HOST_MODE	3

/* Define the data structures used by the Handel driver. */

typedef struct {
	MX_RECORD *record;

	unsigned long handel_flags;
	int handel_log_level;		/* As defined in Handel's md_generic.h
					 *
					 *  MD_ERROR   = 1
					 *  MD_WARNING = 2
					 *  MD_INFO    = 3
					 *  MD_DEBUG   = 4
					 */

	char config_filename[ MXU_FILENAME_LENGTH + 1 ];

	char log_filename[ MXU_FILENAME_LENGTH + 1 ];

	char save_filename[ MXU_FILENAME_LENGTH + 1 ];

	char parameter_name[MXU_HANDEL_PARAMETER_NAME_LENGTH+1];
	char acquisition_value_name[MXU_HANDEL_ACQUISITION_VALUE_NAME_LENGTH+1];

	long num_active_detector_channels;
	long *active_detector_channel_array;

	unsigned long num_mcas;
	MX_RECORD **mca_record_array;

	unsigned long num_detectors;
	unsigned long num_modules;
	unsigned long mcas_per_module;

	MX_RECORD ***module_array;

	double last_measurement_interval;

	/* The hardware uses 'double' for mapping_mode and
	 * pixel_advance_mode, but in MX we round to the
	 * nearest integer so that we can do equality tests
	 * simply in the MX drivers.
	 */

	long mapping_mode;
	long pixel_advance_mode;
	long sync_count;

	mx_bool_type debug_flag;

	mx_bool_type use_module_statistics_2;

	MX_MUTEX *mutex;

	MX_THREAD *monitor_thread;
} MX_HANDEL;

/* WARNING: The following macro _EXPECTS_ that the variables 'handel'
 *          and 'xia_status' are present in the function block that
 *          this macro is used in.  In an ideal world, we could use
 *          an inline function here, but the function specified by
 *          the (x) macro argument do not always have the same
 *          function calling signature, so we can't easily go the
 *          inline function route.
 * 
 * Note:    The if() test is to avoid using the mutex if the monitor
 *          thread is not running.
 */

#define MX_XIA_SYNC(x) \
    do { \
        if ( handel->monitor_thread == NULL ) { \
            xia_status = (x); \
        } else { \
            mx_mutex_lock( handel->mutex ); \
            xia_status = (x); \
            mx_mutex_unlock( handel->mutex ); \
        } \
    } while (0)

/* The following flags are used by the "PRESET" MCA parameter. */

#define MXF_HANDEL_PRESET_NONE		0
#define MXF_HANDEL_PRESET_REAL_TIME	1
#define MXF_HANDEL_PRESET_LIVE_TIME	2
#define MXF_HANDEL_PRESET_OUTPUT_EVENTS	3
#define MXF_HANDEL_PRESET_INPUT_COUNTS	4

/* Label values for the handel driver. */

#define MXLV_HANDEL_CONFIG_FILENAME		2001
#define MXLV_HANDEL_SAVE_FILENAME		2002
#define MXLV_HANDEL_PARAMETER_NAME		2003
#define MXLV_HANDEL_ACQUISITION_VALUE_NAME	2004
#define MXLV_HANDEL_MAPPING_MODE		2005
#define MXLV_HANDEL_PIXEL_ADVANCE_MODE		2006
#define MXLV_HANDEL_SYNC_COUNT			2007

#define MXI_HANDEL_STANDARD_FIELDS \
  {-1, -1, "handel_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, handel_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "handel_log_level", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, handel_log_level), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_HANDEL_CONFIG_FILENAME, -1, "config_filename", \
				MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, config_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "log_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, log_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_HANDEL_SAVE_FILENAME, -1, "save_filename", \
				MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, save_filename), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_HANDEL_PARAMETER_NAME, -1, "parameter_name", \
		MXFT_STRING, NULL, 1, {MXU_HANDEL_PARAMETER_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, parameter_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_HANDEL_ACQUISITION_VALUE_NAME, -1, "acquisition_value_name", \
	    MXFT_STRING, NULL, 1, {MXU_HANDEL_ACQUISITION_VALUE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, acquisition_value_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {-1, -1, "num_active_detector_channels", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_HANDEL, num_active_detector_channels ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "active_detector_channel_array", MXFT_LONG, \
					NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_HANDEL, active_detector_channel_array ), \
	{sizeof(int)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS) }, \
  \
  {-1, -1, "num_mcas", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, num_mcas ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "num_detectors", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, num_detectors ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "num_modules", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, num_modules ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "last_measurement_interval", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_HANDEL, last_measurement_interval),\
	{0}, NULL, 0 }, \
  \
  {MXLV_HANDEL_MAPPING_MODE, -1, "mapping_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, mapping_mode), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_HANDEL_PIXEL_ADVANCE_MODE, -1, \
		"pixel_advance_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, pixel_advance_mode), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_HANDEL_SYNC_COUNT, -1, "sync_count", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, sync_count), \
	{0}, NULL, MXFF_IN_DESCRIPTION }


MX_API mx_status_type mxi_handel_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxi_handel_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_handel_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_handel_open( MX_RECORD *record );
MX_API mx_status_type mxi_handel_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxi_handel_special_processing_setup(
							MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_handel_record_function_list;

extern long mxi_handel_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_handel_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_handel_restore_configuration( MX_RECORD *record,
					char *configuration_filename,
					int debug_flag );

MX_API mx_status_type mxi_handel_set_acquisition_values_for_all_channels(
					MX_HANDEL *handel,
					char *value_name,
					double *value_ptr,
					mx_bool_type apply_flag );

MX_API mx_status_type mxi_handel_apply_to_all_channels( MX_HANDEL *handel );

MX_API mx_status_type mxi_handel_read_parameter( MX_MCA *mca,
					char *parameter_name,
					unsigned long *value_ptr );

MX_API mx_status_type mxi_handel_write_parameter( MX_MCA *mca,
					char *parameter_name,
					unsigned long value );

MX_API mx_status_type mxi_handel_write_parameter_to_all_channels(
					MX_HANDEL *handel,
					char *parameter_name,
					unsigned long value );

/*------*/

MX_API mx_status_type mxi_handel_start_run( MX_HANDEL *handel,
					mx_bool_type resume_run );

MX_API mx_status_type mxi_handel_stop_run( MX_HANDEL *handel );

MX_API mx_status_type mxi_handel_is_busy( MX_HANDEL *handel,
					mx_bool_type *busy );

MX_API mx_status_type mxi_handel_set_preset( MX_HANDEL *handel,
					double preset_type,
					double preset_value );

/*------*/

MX_API mx_status_type mxi_handel_show_parameter( MX_HANDEL *handel );

MX_API mx_status_type mxi_handel_show_acquisition_value( MX_HANDEL *handel );

/*------*/

MX_API mx_status_type mxi_handel_get_acq_value_as_long( MX_HANDEL *handel,
						char *value_name,
						long *acq_value,
						mx_bool_type apply_all );

MX_API mx_status_type mxi_handel_set_acq_value_as_long( MX_HANDEL *handel,
						char *value_name,
						long acq_value,
						mx_bool_type apply_all );

/*------*/

MX_API const char *mxi_handel_strerror( int handel_status_code );

MX_API mx_status_type mxi_handel_set_data_available_flags(
					MX_HANDEL *handel, int flag_value );

#endif /* __I_HANDEL_H__ */
