/* 
 * Name:    d_handel_mca.h
 *
 * Purpose: Header file for multichannel analyzers controlled by the
 * X-Ray Instrumentation Associates Handel library.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001-2006, 2009-2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_HANDEL_MCA_H__
#define __D_HANDEL_MCA_H__

#include "mx_mca.h"

#define MXU_HANDEL_MCA_PARAMETER_NAME_LENGTH		80
#define MXU_HANDEL_MCA_ACQUISITION_VALUE_NAME_LENGTH	80

#define MXU_HANDEL_MCA_LABEL_LENGTH			120

#define MX_HANDEL_MCA_MAX_SCAS 				256
#define MX_HANDEL_MCA_MAX_BINS				65536

/* The following defines are used by the 'statistics' record field. */

#define MX_HANDEL_MCA_NUM_STATISTICS	8

#define MX_HANDEL_MCA_REAL_TIME		0
#define MX_HANDEL_MCA_LIVE_TIME		1
#define MX_HANDEL_MCA_INPUT_COUNT_RATE	2
#define MX_HANDEL_MCA_OUTPUT_COUNT_RATE	3
#define MX_HANDEL_MCA_NUM_TRIGGERS	4
#define MX_HANDEL_MCA_NUM_EVENTS	5

/* The following flags are used by the "PRESET" MCA parameter. */

#define MXF_HANDEL_MCA_PRESET_NONE		0
#define MXF_HANDEL_MCA_PRESET_REAL_TIME		1
#define MXF_HANDEL_MCA_PRESET_LIVE_TIME		2
#define MXF_HANDEL_MCA_PRESET_OUTPUT_EVENTS	3
#define MXF_HANDEL_MCA_PRESET_INPUT_COUNTS	4

typedef struct {
	MX_RECORD *record;

	MX_RECORD *handel_record;
	char mca_label[MXU_HANDEL_MCA_LABEL_LENGTH + 1];

	double statistics[ MX_HANDEL_MCA_NUM_STATISTICS ];
	char parameter_name[ MXU_HANDEL_MCA_PARAMETER_NAME_LENGTH + 1 ];
	unsigned long parameter_value;
	unsigned long param_value_to_all_channels;

	unsigned long baseline_length;
	unsigned long *baseline_array;

	double gain_change;
	double gain_calibration;

	char acquisition_value_name
		[ MXU_HANDEL_MCA_ACQUISITION_VALUE_NAME_LENGTH + 1 ];
	char old_acquisition_value_name
		[ MXU_HANDEL_MCA_ACQUISITION_VALUE_NAME_LENGTH + 1 ];

	double acquisition_value;
	double acquisition_value_to_all;

	mx_bool_type apply_flag;
	mx_bool_type apply_to_all;

	double adc_trace_step_size;		/* in nanoseconds */
	unsigned long adc_trace_length;
	unsigned long *adc_trace_array;

	unsigned long baseline_history_length;
	unsigned long *baseline_history_array;

	unsigned long show_parameters;

	mx_bool_type sca_has_been_initialized[ MX_HANDEL_MCA_MAX_SCAS ];

	long mca_record_array_index;

	long dxp_module;

	long detector_channel;
	long crate;
	long slot;
	long dxp_channel;

	unsigned long firmware_code_variant;
	unsigned long firmware_code_revision;

	mx_bool_type hardware_scas_are_enabled;
	mx_bool_type new_statistics_available;

	/* Clock tick intervals in seconds. */

	double energy_live_time;

	unsigned long num_triggers;
	unsigned long num_events;

	double input_count_rate;
	double output_count_rate;

	unsigned long old_preset_type;
	double old_preset_time;

	unsigned long num_underflows;
	unsigned long num_overflows;

	mx_bool_type debug_flag;

	mx_bool_type use_mca_channel_array;

	unsigned int num_spectrum_bins;
	unsigned long *spectrum_array;

	char *detector_alias;
	char *module_alias;
	unsigned long module_number;

	char *module_type;

	mx_bool_type use_double_roi_integral_array;
	double *double_roi_integral_array;

} MX_HANDEL_MCA;


#define MXLV_HANDEL_MCA_STATISTICS			2001
#define MXLV_HANDEL_MCA_PARAMETER_NAME			2002
#define MXLV_HANDEL_MCA_PARAMETER_VALUE			2003
#define MXLV_HANDEL_MCA_PARAM_VALUE_TO_ALL_CHANNELS	2004
#define MXLV_HANDEL_MCA_BASELINE_LENGTH			2005
#define MXLV_HANDEL_MCA_BASELINE_ARRAY			2006
#define MXLV_HANDEL_MCA_GAIN_CHANGE			2007
#define MXLV_HANDEL_MCA_GAIN_CALIBRATION		2008
#define MXLV_HANDEL_MCA_ACQUISITION_VALUE_NAME		2009
#define MXLV_HANDEL_MCA_ACQUISITION_VALUE		2010
#define MXLV_HANDEL_MCA_ACQUISITION_VALUE_TO_ALL	2011
#define MXLV_HANDEL_MCA_APPLY				2012
#define MXLV_HANDEL_MCA_APPLY_TO_ALL			2013
#define MXLV_HANDEL_MCA_ADC_TRACE_STEP_SIZE		2014
#define MXLV_HANDEL_MCA_ADC_TRACE_LENGTH		2015
#define MXLV_HANDEL_MCA_ADC_TRACE_ARRAY			2016
#define MXLV_HANDEL_MCA_BASELINE_HISTORY_LENGTH		2017
#define MXLV_HANDEL_MCA_BASELINE_HISTORY_ARRAY		2018
#define MXLV_HANDEL_MCA_SHOW_PARAMETERS			2019

#define MXD_HANDEL_MCA_STANDARD_FIELDS \
  {-1, -1, "handel_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, handel_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "mca_label", MXFT_STRING, NULL, 1, {MXU_HANDEL_MCA_LABEL_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, mca_label ), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_HANDEL_MCA_STATISTICS, -1, "statistics", \
			MXFT_DOUBLE, NULL, 1, {MX_HANDEL_MCA_NUM_STATISTICS}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, statistics ), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_PARAMETER_NAME, -1, "parameter_name", \
		MXFT_STRING, NULL, 1, {MXU_HANDEL_MCA_PARAMETER_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, parameter_name ), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_PARAMETER_VALUE, -1, "parameter_value", \
			MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, parameter_value ), \
	{0}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_PARAM_VALUE_TO_ALL_CHANNELS, -1, \
			"param_value_to_all_channels", \
			MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, \
			param_value_to_all_channels ), \
	{0}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_BASELINE_LENGTH, -1, "baseline_length", \
			MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, baseline_length ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_HANDEL_MCA_BASELINE_ARRAY, -1, "baseline_array", \
			MXFT_ULONG, NULL, 1, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, baseline_array ), \
	{sizeof(unsigned long)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_HANDEL_MCA_GAIN_CHANGE, -1, "gain_change", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, gain_change ), \
	{0}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_GAIN_CALIBRATION, -1, "gain_calibration", \
			MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, gain_calibration ), \
	{0}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_ACQUISITION_VALUE_NAME, -1, "acquisition_value_name", \
			MXFT_STRING, NULL, 1, \
			{MXU_HANDEL_MCA_ACQUISITION_VALUE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof( MX_HANDEL_MCA, acquisition_value_name ), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_ACQUISITION_VALUE, -1, "acquisition_value", \
			MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof( MX_HANDEL_MCA, acquisition_value ), \
	{0}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_ACQUISITION_VALUE_TO_ALL, -1, "acquisition_value_to_all", \
			MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof( MX_HANDEL_MCA, acquisition_value_to_all ), \
	{0}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_APPLY, -1, "apply", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, apply_flag ), \
	{0}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_APPLY_TO_ALL, -1, "apply_to_all", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, apply_to_all ), \
	{0}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_ADC_TRACE_STEP_SIZE, -1, "adc_trace_step_size", \
		MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_HANDEL_MCA, adc_trace_step_size ), \
	{0}, NULL, 0}, \
  \
  {MXLV_HANDEL_MCA_ADC_TRACE_LENGTH, -1, "adc_trace_length", \
			MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, adc_trace_length ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_HANDEL_MCA_ADC_TRACE_ARRAY, -1, "adc_trace_array", \
			MXFT_ULONG, NULL, 1, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, adc_trace_array ), \
	{sizeof(unsigned long)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_HANDEL_MCA_BASELINE_HISTORY_LENGTH, -1, "baseline_history_length", \
			MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof( MX_HANDEL_MCA, baseline_history_length ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_HANDEL_MCA_BASELINE_HISTORY_ARRAY, -1, "baseline_history_array", \
			MXFT_ULONG, NULL, 1, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof( MX_HANDEL_MCA, baseline_history_array ), \
	{sizeof(unsigned long)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_HANDEL_MCA_SHOW_PARAMETERS, -1, "show_parameters", \
			MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, show_parameters ), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "detector_channel", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, detector_channel ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "crate", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, crate ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "slot", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, slot ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "dxp_channel", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, dxp_channel ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "firmware_code_variant", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL_MCA, firmware_code_variant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "firmware_code_revision", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL_MCA, firmware_code_revision), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "hardware_scas_are_enabled", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_HANDEL_MCA, hardware_scas_are_enabled ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "new_statistics_available", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_HANDEL_MCA, new_statistics_available ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "debug_flag", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, debug_flag ), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "detector_alias", MXFT_STRING, NULL, 1, {200}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, detector_alias ), \
	{sizeof(char)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "module_alias", MXFT_STRING, NULL, 1, {200}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, module_alias ), \
	{sizeof(char)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "module_type", MXFT_STRING, NULL, 1, {200}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HANDEL_MCA, module_type ), \
	{sizeof(char)}, NULL, MXFF_VARARGS}

MX_API mx_status_type mxd_handel_mca_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_handel_mca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_handel_mca_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_handel_mca_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_handel_mca_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_handel_mca_open( MX_RECORD *record );
MX_API mx_status_type mxd_handel_mca_close( MX_RECORD *record );
MX_API mx_status_type mxd_handel_mca_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_handel_mca_start( MX_MCA *mca );
MX_API mx_status_type mxd_handel_mca_stop( MX_MCA *mca );
MX_API mx_status_type mxd_handel_mca_read( MX_MCA *mca );
MX_API mx_status_type mxd_handel_mca_clear( MX_MCA *mca );
MX_API mx_status_type mxd_handel_mca_busy( MX_MCA *mca );
MX_API mx_status_type mxd_handel_mca_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_handel_mca_set_parameter( MX_MCA *mca );

MX_API mx_status_type mxd_handel_mca_get_mca_array( MX_RECORD *handel_record,
					unsigned long *num_mcas,
					MX_RECORD ***mca_record_array );

MX_API mx_status_type mxd_handel_mca_get_rate_corrected_roi_integral(
					MX_MCA *mca,
					unsigned long roi_number,
					double *corrected_roi_value );

MX_API mx_status_type mxd_handel_mca_get_livetime_corrected_roi_integral(
					MX_MCA *mca,
					unsigned long roi_number,
					double *corrected_roi_value );

MX_API mx_status_type mxd_handel_mca_read_statistics( MX_MCA *mca );

MX_API mx_status_type mxd_handel_mca_get_run_data( MX_MCA *mca,
					char *name,
					void *value_ptr );

MX_API mx_status_type mxd_handel_mca_get_acquisition_values( MX_MCA *mca,
					char *value_name,
					double *value_ptr );

MX_API mx_status_type mxd_handel_mca_set_acquisition_values( MX_MCA *mca,
					char *value_name,
					double *value_ptr,
					mx_bool_type apply_flag );

MX_API mx_status_type mxd_handel_mca_apply( MX_MCA *mca );

MX_API mx_status_type mxd_handel_mca_start_run( MX_MCA *mca,
					mx_bool_type clear_flag );

MX_API mx_status_type mxd_handel_mca_stop_run( MX_MCA *mca );

MX_API mx_status_type mxd_handel_mca_get_baseline_array( MX_MCA *mca );

MX_API mx_status_type mxd_handel_mca_set_gain_change( MX_MCA *mca );

MX_API mx_status_type mxd_handel_mca_set_gain_calibration( MX_MCA *mca );

MX_API mx_status_type mxd_handel_mca_get_adc_trace_array( MX_MCA *mca );

MX_API mx_status_type mxd_handel_mca_get_baseline_history_array( MX_MCA *mca );

MX_API mx_status_type mxd_handel_mca_show_parameters( MX_MCA *mca );

extern MX_RECORD_FUNCTION_LIST mxd_handel_mca_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_handel_mca_mca_function_list;

extern long mxd_handel_mca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_handel_mca_rfield_def_ptr;

#endif /* __D_HANDEL_MCA_H__ */
