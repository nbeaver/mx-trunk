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
#define MXF_HANDEL_BREAKPOINT_AT_STARTUP		0x8
#define MXF_HANDEL_CREATE_XIAHOME_VARIABLE		0x10

#define MXU_HANDEL_MODULE_TYPE_LENGTH	20

#define MXU_HANDEL_MAX_MODULES		4

#if !defined( XIA_PRESET_NONE )
#  define XIA_PRESET_NONE		0.0
#  define XIA_PRESET_FIXED_REAL		1.0
#  define XIA_PRESET_FIXED_LIVE		2.0
#  define XIA_PRESET_FIXED_EVENTS	3.0
#  define XIA_PRESET_FIXED_TRIGGERS	4.0
#endif

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

	unsigned long handel_version;

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

	mx_bool_type bypass_xia_preset_type;

	mx_bool_type debug_flag;

	mx_bool_type use_module_statistics_2;
} MX_HANDEL;

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

#define MXI_HANDEL_STANDARD_FIELDS \
  {-1, -1, "handel_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, handel_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "handel_log_level", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, handel_log_level), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "handel_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, handel_version), \
	{0}, NULL, MXFF_READ_ONLY }, \
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
  {-1, -1, "bypass_xia_preset_type", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL, bypass_xia_preset_type), \
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

MX_API const char *mxi_handel_strerror( int handel_status_code );

MX_API mx_status_type mxi_handel_set_data_available_flags(
					MX_HANDEL *handel, int flag_value );

#endif /* __I_HANDEL_H__ */
