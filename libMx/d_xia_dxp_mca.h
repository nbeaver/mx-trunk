/* 
 * Name:    d_xia_dxp_mca.h
 *
 * Purpose: Header file for the X-Ray Instrumentation Associates Mesa2x
 *          Data Server used by the DXP-2X multichannel analyzer.
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

#ifndef __D_XIA_DXP_MCA_H__
#define __D_XIA_DXP_MCA_H__

#include "mx_mca.h"

#define MX_XIA_DXP_CLOCK_TICK		(400.0e-9)	/* in seconds */

#define MXU_XIA_DXP_PARAMETER_NAME_LENGTH		80
#define MXU_XIA_DXP_ACQUISITION_VALUE_NAME_LENGTH	80

#define MXU_XIA_DXP_LABEL_LENGTH			120

#define MX_XIA_DXP_MCA_MAX_SCAS 			16
#define MX_XIA_DXP_MCA_MAX_BINS				8192

/* The following defines are used by the 'statistics' record field. */

#define MX_XIA_DXP_NUM_STATISTICS	8

#define MX_XIA_DXP_REAL_TIME		0
#define MX_XIA_DXP_LIVE_TIME		1
#define MX_XIA_DXP_INPUT_COUNT_RATE	2
#define MX_XIA_DXP_OUTPUT_COUNT_RATE	3
#define MX_XIA_DXP_NUM_FAST_PEAKS	4
#define MX_XIA_DXP_NUM_EVENTS		5
#define MX_XIA_DXP_NUM_UNDERFLOWS	6
#define MX_XIA_DXP_NUM_OVERFLOWS	7

/* The following flags are used by the "PRESET" MCA parameter. */

#define MXF_XIA_PRESET_NONE		0
#define MXF_XIA_PRESET_REAL_TIME	1
#define MXF_XIA_PRESET_LIVE_TIME	2
#define MXF_XIA_PRESET_OUTPUT_EVENTS	3
#define MXF_XIA_PRESET_INPUT_COUNTS	4

/* Old versions of Xerxes defined the baseline array using unsigned shorts
 * while Handel used unsigned longs.  They now both use unsigned longs.
 */

#if ((HANDEL_MAJOR_VERSION == 0) && (HANDEL_MINOR_VERSION < 6))

#  define XIA_HAVE_OLD_DXP_READOUT_DETECTOR_RUN	TRUE
#else

#  define XIA_HAVE_OLD_DXP_READOUT_DETECTOR_RUN	FALSE
#endif

typedef struct {
	MX_RECORD *record;

	MX_RECORD *xia_dxp_record;
	char mca_label[MXU_XIA_DXP_LABEL_LENGTH + 1];

	double statistics[ MX_XIA_DXP_NUM_STATISTICS ];
	char parameter_name[ MXU_XIA_DXP_PARAMETER_NAME_LENGTH + 1 ];
	unsigned long parameter_value;
	unsigned long param_value_to_all_channels;

	unsigned long baseline_length;
	unsigned long *baseline_array;

#if XIA_HAVE_OLD_DXP_READOUT_DETECTOR_RUN
	unsigned short *xerxes_baseline_array;
#endif
	
	double gain_change;
	double gain_calibration;
	char acquisition_value_name
		[ MXU_XIA_DXP_ACQUISITION_VALUE_NAME_LENGTH + 1 ];
	double acquisition_value;
	double adc_trace_step_size;		/* in nanoseconds */
	unsigned long adc_trace_length;
	unsigned long *adc_trace_array;

	unsigned long baseline_history_length;
	unsigned long *baseline_history_array;

	mx_bool_type sca_has_been_initialized[ MX_XIA_DXP_MCA_MAX_SCAS ];

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

	double preset_clock_tick; /* Used for calculation of time presets. */

	double runtime_clock_tick; /* Used for real time and live time
				    * calculations. */

	double input_count_rate;
	double output_count_rate;

	unsigned long num_fast_peaks;
	unsigned long num_events;
	unsigned long num_underflows;
	unsigned long num_overflows;

	unsigned long old_preset_type;
	unsigned long old_preset_high_order;
	unsigned long old_preset_low_order;

	mx_status_type (*is_busy)( MX_MCA *mca,
					mx_bool_type *busy_flag,
					int debug_flag);

	mx_status_type (*read_parameter)( MX_MCA *mca,
					char *parameter_name,
					unsigned long *value_ptr,
					int debug_flag );

	mx_status_type (*write_parameter)( MX_MCA *mca,
					char *parameter_name,
					unsigned long value,
					int debug_flag );

	mx_status_type (*write_parameter_to_all_channels)( MX_MCA *mca,
					char *parameter_name,
					unsigned long value,
					int debug_flag );

	mx_status_type (*start_run)( MX_MCA *mca,
					mx_bool_type clear_flag,
					int debug_flag );

	mx_status_type (*stop_run)( MX_MCA *mca,
					int debug_flag );

	mx_status_type (*read_spectrum)( MX_MCA *mca,
					int debug_flag );

	mx_status_type (*read_statistics)( MX_MCA *mca,
					int debug_flag );

	mx_status_type (*get_baseline_array)( MX_MCA *mca,
					int debug_flag );

	mx_status_type (*set_gain_change)( MX_MCA *mca,
					int debug_flag );

	mx_status_type (*set_gain_calibration)( MX_MCA *mca,
					int debug_flag );

	mx_status_type (*get_acquisition_value)( MX_MCA *mca,
					int debug_flag );

	mx_status_type (*set_acquisition_value)( MX_MCA *mca,
					int debug_flag );

	mx_status_type (*get_adc_trace_array)( MX_MCA *mca,
					int debug_flag );

	mx_status_type (*get_baseline_history_array)( MX_MCA *mca,
					int debug_flag );

	mx_status_type (*get_mx_parameter)( MX_MCA *mca );

	mx_status_type (*set_mx_parameter)( MX_MCA *mca );

#if HAVE_XIA_HANDEL
	mx_bool_type use_mca_channel_array;

	unsigned int num_spectrum_bins;
	unsigned long *spectrum_array;

	char *detector_alias;
	char *module_alias;
	int module_channel;

	char *module_type;

#endif /* HAVE_XIA_HANDEL */

} MX_XIA_DXP_MCA;


#define MXLV_XIA_DXP_STATISTICS				2001
#define MXLV_XIA_DXP_PARAMETER_NAME			2002
#define MXLV_XIA_DXP_PARAMETER_VALUE			2003
#define MXLV_XIA_DXP_PARAM_VALUE_TO_ALL_CHANNELS	2004
#define MXLV_XIA_DXP_BASELINE_LENGTH			2005
#define MXLV_XIA_DXP_BASELINE_ARRAY			2006
#define MXLV_XIA_DXP_GAIN_CHANGE			2007
#define MXLV_XIA_DXP_GAIN_CALIBRATION			2008
#define MXLV_XIA_DXP_ACQUISITION_VALUE_NAME		2009
#define MXLV_XIA_DXP_ACQUISITION_VALUE			2010
#define MXLV_XIA_DXP_ADC_TRACE_STEP_SIZE		2011
#define MXLV_XIA_DXP_ADC_TRACE_LENGTH			2012
#define MXLV_XIA_DXP_ADC_TRACE_ARRAY			2013
#define MXLV_XIA_DXP_BASELINE_HISTORY_LENGTH		2014
#define MXLV_XIA_DXP_BASELINE_HISTORY_ARRAY		2015

#define MXD_XIA_DXP_STANDARD_FIELDS \
  {-1, -1, "xia_dxp_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, xia_dxp_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "mca_label", MXFT_STRING, NULL, 1, {MXU_XIA_DXP_LABEL_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, mca_label ), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_XIA_DXP_STATISTICS, -1, "statistics", \
			MXFT_DOUBLE, NULL, 1, {MX_XIA_DXP_NUM_STATISTICS}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, statistics ), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_XIA_DXP_PARAMETER_NAME, -1, "parameter_name", \
		MXFT_STRING, NULL, 1, {MXU_XIA_DXP_PARAMETER_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, parameter_name ), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_XIA_DXP_PARAMETER_VALUE, -1, "parameter_value", \
			MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, parameter_value ), \
	{0}, NULL, 0}, \
  \
  {MXLV_XIA_DXP_PARAM_VALUE_TO_ALL_CHANNELS, -1, \
			"param_value_to_all_channels", \
			MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, \
			param_value_to_all_channels ), \
	{0}, NULL, 0}, \
  \
  {MXLV_XIA_DXP_BASELINE_LENGTH, -1, "baseline_length", \
			MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, baseline_length ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_XIA_DXP_BASELINE_ARRAY, -1, "baseline_array", \
			MXFT_ULONG, NULL, 1, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, baseline_array ), \
	{sizeof(unsigned long)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_XIA_DXP_GAIN_CHANGE, -1, "gain_change", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, gain_change ), \
	{0}, NULL, 0}, \
  \
  {MXLV_XIA_DXP_GAIN_CALIBRATION, -1, "gain_calibration", \
			MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, gain_calibration ), \
	{0}, NULL, 0}, \
  \
  {MXLV_XIA_DXP_ACQUISITION_VALUE_NAME, -1, "acquisition_value_name", \
			MXFT_STRING, NULL, 1, \
			{MXU_XIA_DXP_ACQUISITION_VALUE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof( MX_XIA_DXP_MCA, acquisition_value_name ), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_XIA_DXP_ACQUISITION_VALUE, -1, "acquisition_value", \
			MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof( MX_XIA_DXP_MCA, acquisition_value ), \
	{0}, NULL, 0}, \
  \
  {MXLV_XIA_DXP_ADC_TRACE_STEP_SIZE, -1, "adc_trace_step_size", \
		MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_XIA_DXP_MCA, adc_trace_step_size ), \
	{0}, NULL, 0}, \
  \
  {MXLV_XIA_DXP_ADC_TRACE_LENGTH, -1, "adc_trace_length", \
			MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, adc_trace_length ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_XIA_DXP_ADC_TRACE_ARRAY, -1, "adc_trace_array", \
			MXFT_ULONG, NULL, 1, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, adc_trace_array ), \
	{sizeof(unsigned long)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_XIA_DXP_BASELINE_HISTORY_LENGTH, -1, "baseline_history_length", \
			MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof( MX_XIA_DXP_MCA, baseline_history_length ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_XIA_DXP_BASELINE_HISTORY_ARRAY, -1, "baseline_history_array", \
			MXFT_ULONG, NULL, 1, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof( MX_XIA_DXP_MCA, baseline_history_array ), \
	{sizeof(unsigned long)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "detector_channel", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, detector_channel ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "crate", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, crate ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "slot", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, slot ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "dxp_channel", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, dxp_channel ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "firmware_code_variant", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_DXP_MCA, firmware_code_variant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "firmware_code_revision", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_DXP_MCA, firmware_code_revision), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "hardware_scas_are_enabled", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_XIA_DXP_MCA, hardware_scas_are_enabled ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "new_statistics_available", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_XIA_DXP_MCA, new_statistics_available ), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "preset_clock_tick", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, preset_clock_tick ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "runtime_clock_tick", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, runtime_clock_tick ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY)}

#if HAVE_XIA_HANDEL

#define MXD_XIA_DXP_HANDEL_STANDARD_FIELDS \
  {-1, -1, "detector_alias", MXFT_STRING, NULL, 1, {200}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, detector_alias ), \
	{sizeof(char)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "module_alias", MXFT_STRING, NULL, 1, {200}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, module_alias ), \
	{sizeof(char)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "module_type", MXFT_STRING, NULL, 1, {200}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_XIA_DXP_MCA, module_type ), \
	{sizeof(char)}, NULL, MXFF_VARARGS}

#endif /* HAVE_XIA_HANDEL */

MX_API mx_status_type mxd_xia_dxp_initialize_type( long type );
MX_API mx_status_type mxd_xia_dxp_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_xia_dxp_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_xia_dxp_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_xia_dxp_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_xia_dxp_open( MX_RECORD *record );
MX_API mx_status_type mxd_xia_dxp_close( MX_RECORD *record );
MX_API mx_status_type mxd_xia_dxp_special_processing_setup( MX_RECORD *record );

MX_API mx_status_type mxd_xia_dxp_start( MX_MCA *mca );
MX_API mx_status_type mxd_xia_dxp_stop( MX_MCA *mca );
MX_API mx_status_type mxd_xia_dxp_read( MX_MCA *mca );
MX_API mx_status_type mxd_xia_dxp_clear( MX_MCA *mca );
MX_API mx_status_type mxd_xia_dxp_busy( MX_MCA *mca );
MX_API mx_status_type mxd_xia_dxp_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_xia_dxp_set_parameter( MX_MCA *mca );

MX_API mx_status_type mxd_xia_dxp_default_get_mx_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_xia_dxp_default_set_mx_parameter( MX_MCA *mca );

MX_API mx_status_type mxd_xia_dxp_get_mca_array( MX_RECORD *xia_dxp_record,
					unsigned long *num_mcas,
					MX_RECORD ***mca_record_array );

MX_API mx_status_type mxd_xia_dxp_get_rate_corrected_roi_integral(
					MX_MCA *mca,
					unsigned long roi_number,
					double *corrected_roi_value );

MX_API mx_status_type mxd_xia_dxp_get_livetime_corrected_roi_integral(
					MX_MCA *mca,
					unsigned long roi_number,
					double *corrected_roi_value );

MX_API mx_status_type mxd_xia_dxp_read_statistics( MX_MCA *mca,
						MX_XIA_DXP_MCA *xia_dxp_mca,
						int debug_flag );

MX_API mx_status_type mxd_xia_dxp_set_gain_change( MX_MCA *mca,
						MX_XIA_DXP_MCA *xia_dxp_mca,
						int debug_flag );

MX_API mx_status_type mxd_xia_dxp_set_gain_calibration( MX_MCA *mca,
						MX_XIA_DXP_MCA *xia_dxp_mca,
						int debug_flag );

MX_API mx_status_type mxd_xia_dxp_get_acquisition_value( MX_MCA *mca,
						MX_XIA_DXP_MCA *xia_dxp_mca,
						int debug_flag );

MX_API mx_status_type mxd_xia_dxp_set_acquisition_value( MX_MCA *mca,
						MX_XIA_DXP_MCA *xia_dxp_mca,
						int debug_flag );

MX_API mx_status_type mxd_xia_dxp_get_adc_trace( MX_MCA *mca,
						MX_XIA_DXP_MCA *xia_dxp_mca,
						int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxd_xia_dxp_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_xia_dxp_mca_function_list;

extern long mxd_xia_dxp_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_xia_dxp_rfield_def_ptr;

#endif /* __D_XIA_DXP_MCA_H__ */
