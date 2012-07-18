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
 * Copyright 2003-2006, 2009-2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_HANDEL_H__
#define __I_HANDEL_H__

#define MX_IGNORE_HANDEL_NULL_STRING	TRUE

#define MX_HANDEL_ACTIVE_DETECTOR_CHANNEL_SET	(-2)

/* The following flags are used by the 'handel_flags' field. */

#define MXF_HANDEL_IGNORE_HARDWARE_SCAS			0x1
#define MXF_HANDEL_WRITE_LOG_OUTPUT_TO_FILE		0x2
#define MXF_HANDEL_DISPLAY_CONFIGURATION_AT_STARTUP	0x4

#define MXU_HANDEL_MODULE_TYPE_LENGTH	20

#define MXU_HANDEL_MAX_MODULES		4

/* Define the data structures used by the Handel driver. */

#if ( HAVE_XIA_HANDEL && IS_MX_DRIVER )

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

	long num_active_detector_channels;
	long *active_detector_channel_array;

	unsigned long num_mcas;
	MX_RECORD **mca_record_array;

	unsigned long num_detectors;
	unsigned long num_modules;
	unsigned long mcas_per_module;

	MX_RECORD ***module_array;

	double last_measurement_interval;

	mx_bool_type use_module_statistics_2;
} MX_HANDEL;

#endif /* HAVE_XIA_HANDEL && IS_MX_DRIVER */

/* The following flags are used by the "PRESET" MCA parameter. */

#define MXF_HANDEL_PRESET_NONE		0
#define MXF_HANDEL_PRESET_REAL_TIME	1
#define MXF_HANDEL_PRESET_LIVE_TIME	2
#define MXF_HANDEL_PRESET_OUTPUT_EVENTS	3
#define MXF_HANDEL_PRESET_INPUT_COUNTS	4

/* Label values for the handel driver. */

#define MXLV_HANDEL_CONFIG_FILENAME		2001
#define MXLV_HANDEL_SAVE_FILENAME		2002

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
	{0}, NULL, 0 }


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

MX_API mx_status_type mxi_handel_is_busy( MX_MCA *mca,
					mx_bool_type *busy_flag );

MX_API mx_status_type mxi_handel_get_run_data( MX_MCA *mca,
					char *name,
					void *value_ptr );

MX_API mx_status_type mxi_handel_get_acquisition_values( MX_MCA *mca,
					char *value_name,
					double *value_ptr );

MX_API mx_status_type mxi_handel_set_acquisition_values( MX_MCA *mca,
					char *value_name,
					double *value_ptr,
					mx_bool_type apply_flag );

MX_API mx_status_type mxi_handel_set_acq_for_all_channels(
					MX_MCA *mca,
					char *value_name,
					double *value_ptr,
					mx_bool_type apply_flag );

MX_API mx_status_type mxi_handel_apply( MX_MCA *mca,
					mx_bool_type apply_to_all );

MX_API mx_status_type mxi_handel_read_parameter( MX_MCA *mca,
					char *parameter_name,
					unsigned long *value_ptr );

MX_API mx_status_type mxi_handel_write_parameter( MX_MCA *mca,
					char *parameter_name,
					unsigned long value );

MX_API mx_status_type mxi_handel_write_parameter_to_all_channels(
					MX_MCA *mca,
					char *parameter_name,
					unsigned long value );

MX_API mx_status_type mxi_handel_start_run( MX_MCA *mca,
					mx_bool_type clear_flag );

MX_API mx_status_type mxi_handel_stop_run( MX_MCA *mca );

MX_API mx_status_type mxi_handel_read_spectrum( MX_MCA *mca );

MX_API mx_status_type mxi_handel_read_statistics( MX_MCA *mca );

MX_API mx_status_type mxi_handel_get_baseline_array( MX_MCA *mca );

MX_API mx_status_type mxi_handel_set_gain_change( MX_MCA *mca );

MX_API mx_status_type mxi_handel_set_gain_calibration( MX_MCA *mca );

MX_API mx_status_type mxi_handel_get_adc_trace_array( MX_MCA *mca );

MX_API mx_status_type mxi_handel_get_baseline_history_array( MX_MCA *mca );

MX_API mx_status_type mxi_handel_get_mx_parameter( MX_MCA *mca );

MX_API mx_status_type mxi_handel_set_mx_parameter( MX_MCA *mca );

/*------*/

MX_API const char *mxi_handel_strerror( int handel_status_code );

#if ( HAVE_XIA_HANDEL && IS_MX_DRIVER )

MX_API mx_status_type mxi_handel_set_data_available_flags(
					MX_HANDEL *handel, int flag_value );

#endif

#endif /* __I_HANDEL_H__ */
