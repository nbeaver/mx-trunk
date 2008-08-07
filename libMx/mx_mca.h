/*
 * Name:    mx_mca.h
 *
 * Purpose: MX system header file for multichannel analyzers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2004-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MCA_H__
#define __MX_MCA_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
			    * to this MCA.
			    */

	long maximum_num_channels;
	long current_num_channels;

	unsigned long *channel_array;

	mx_bool_type start;
	mx_bool_type stop;
	mx_bool_type clear;
	mx_bool_type busy;
	mx_bool_type new_data_available;

	long preset_type;
	long parameter_type;

	double start_with_preset[2];

	unsigned long mca_flags;

	long maximum_num_rois;
	long current_num_rois;

	unsigned long **roi_array;
	unsigned long *roi_integral_array;

	unsigned long roi[2];
	unsigned long roi_integral;

	unsigned long channel_number;
	unsigned long channel_value;

	unsigned long roi_number;

	double real_time;
	double live_time;
	unsigned long counts;

	double preset_real_time;
	double preset_live_time;
	unsigned long preset_count;

	double last_measurement_interval;

	unsigned long preset_count_region[2];

	long num_soft_rois;
	unsigned long soft_roi_number;

	unsigned long soft_roi[2];
	unsigned long soft_roi_integral;

	unsigned long **soft_roi_array;
	unsigned long *soft_roi_integral_array;

	double energy_scale;
	double energy_offset;

	double input_count_rate;
	double output_count_rate;
} MX_MCA;

/* Preset types */

#define MXF_MCA_PRESET_NONE		0
#define MXF_MCA_PRESET_LIVE_TIME	1
#define MXF_MCA_PRESET_REAL_TIME	2
#define MXF_MCA_PRESET_COUNT		3

/* Bit definitions for the 'mca_flags' field. */

#define MXF_MCA_NO_READ_OPTIMIZATION	0x1

#define MXLV_MCA_CHANNEL_ARRAY			1001
#define MXLV_MCA_MAXIMUM_NUM_CHANNELS		1002
#define MXLV_MCA_CURRENT_NUM_CHANNELS		1003
#define MXLV_MCA_START				1004
#define MXLV_MCA_STOP				1005
#define MXLV_MCA_READ				1006
#define MXLV_MCA_CLEAR				1007
#define MXLV_MCA_BUSY				1008
#define MXLV_MCA_NEW_DATA_AVAILABLE		1009
#define MXLV_MCA_PRESET_TYPE			1010
#define MXLV_MCA_START_WITH_PRESET		1011
#define MXLV_MCA_MAXIMUM_NUM_ROIS		1012
#define MXLV_MCA_CURRENT_NUM_ROIS		1013
#define MXLV_MCA_ROI_ARRAY   			1014
#define MXLV_MCA_ROI_INTEGRAL_ARRAY   		1015
#define MXLV_MCA_ROI				1016
#define MXLV_MCA_ROI_INTEGRAL			1017
#define MXLV_MCA_CHANNEL_NUMBER			1018
#define MXLV_MCA_CHANNEL_VALUE			1019
#define MXLV_MCA_ROI_NUMBER			1020
#define MXLV_MCA_REAL_TIME			1021
#define MXLV_MCA_LIVE_TIME			1022
#define MXLV_MCA_COUNTS				1023
#define MXLV_MCA_PRESET_REAL_TIME		1024
#define MXLV_MCA_PRESET_LIVE_TIME		1025
#define MXLV_MCA_PRESET_COUNT			1026
#define MXLV_MCA_PRESET_COUNT_REGION		1027
#define MXLV_MCA_NUM_SOFT_ROIS			1028
#define MXLV_MCA_SOFT_ROI_NUMBER		1029
#define MXLV_MCA_SOFT_ROI			1030
#define MXLV_MCA_SOFT_ROI_INTEGRAL		1031
#define MXLV_MCA_SOFT_ROI_ARRAY			1032
#define MXLV_MCA_SOFT_ROI_INTEGRAL_ARRAY	1033
#define MXLV_MCA_ENERGY_SCALE			1034
#define MXLV_MCA_ENERGY_OFFSET			1035
#define MXLV_MCA_INPUT_COUNT_RATE		1036
#define MXLV_MCA_OUTPUT_COUNT_RATE		1037

#define MX_MCA_STANDARD_FIELDS \
  {MXLV_MCA_MAXIMUM_NUM_CHANNELS, -1, "maximum_num_channels", \
			MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, maximum_num_channels), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_MCA_CURRENT_NUM_CHANNELS, -1, "current_num_channels", \
			MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, current_num_channels), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_MCA_CHANNEL_ARRAY, -1, "channel_array", \
			MXFT_ULONG, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, channel_array), \
	{sizeof(unsigned long)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_MCA_START, -1, "start", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, start), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, stop), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_CLEAR, -1, "clear", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, clear), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_BUSY, -1, "busy", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, busy), \
	{0}, NULL, MXFF_POLL}, \
  \
  {MXLV_MCA_NEW_DATA_AVAILABLE, -1, "new_data_available", \
						MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, new_data_available), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_PRESET_TYPE, -1, "preset_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, preset_type), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "parameter_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, parameter_type), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_START_WITH_PRESET, -1, "start_with_preset", \
					MXFT_DOUBLE, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, start_with_preset), \
	{sizeof(double)}, NULL, 0}, \
  \
  {-1, -1, "mca_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, mca_flags), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_MAXIMUM_NUM_ROIS, -1, "maximum_num_rois", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, maximum_num_rois), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {MXLV_MCA_CURRENT_NUM_ROIS, -1, "current_num_rois", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, current_num_rois), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_MCA_ROI_ARRAY, -1, "roi_array", \
			MXFT_ULONG, NULL, 2, {MXU_VARARGS_LENGTH, 2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, roi_array), \
	{sizeof(unsigned long), sizeof(unsigned long *)}, \
					NULL, MXFF_VARARGS}, \
  \
  {MXLV_MCA_ROI_INTEGRAL_ARRAY, -1, "roi_integral_array", \
				MXFT_ULONG, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, roi_integral_array), \
	{sizeof(unsigned long)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_MCA_ROI, -1, "roi", MXFT_ULONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, roi), \
	{sizeof(unsigned long)}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_MCA_ROI_INTEGRAL, -1, "roi_integral", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, roi_integral), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_MCA_CHANNEL_NUMBER, -1, "channel_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, channel_number), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_CHANNEL_VALUE, -1, "channel_value", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, channel_value), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_ROI_NUMBER, -1, "roi_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, roi_number), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_REAL_TIME, -1, "real_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, real_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_LIVE_TIME, -1, "live_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, live_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_COUNTS, -1, "counts", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, counts), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_PRESET_REAL_TIME, -1, "preset_real_time", \
					MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, preset_real_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_PRESET_LIVE_TIME, -1, "preset_live_time", \
					MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, preset_live_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_PRESET_COUNT, -1, "preset_count", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, preset_count), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_PRESET_COUNT_REGION, -1, "preset_count_region", \
					MXFT_ULONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, preset_count_region), \
	{sizeof(unsigned long)}, NULL, 0}, \
  \
  {MXLV_MCA_NUM_SOFT_ROIS, -1, "num_soft_rois", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, num_soft_rois), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY)}, \
  \
  {MXLV_MCA_SOFT_ROI_NUMBER, -1, "soft_roi_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, soft_roi_number), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_SOFT_ROI, -1, "soft_roi", MXFT_ULONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, soft_roi), \
	{sizeof(unsigned long)}, NULL, 0}, \
  \
  {MXLV_MCA_SOFT_ROI_INTEGRAL, -1, "soft_roi_integral", \
						MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, soft_roi_integral), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_SOFT_ROI_ARRAY, -1, "soft_roi_array", \
			MXFT_ULONG, NULL, 2, {MXU_VARARGS_LENGTH, 2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, soft_roi_array), \
	{sizeof(unsigned long), sizeof(unsigned long *)}, \
					NULL, MXFF_VARARGS}, \
  \
  {MXLV_MCA_SOFT_ROI_INTEGRAL_ARRAY, -1, "soft_roi_integral_array", \
				MXFT_ULONG, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, soft_roi_integral_array), \
	{sizeof(unsigned long)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_MCA_ENERGY_SCALE, -1, "energy_scale", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, energy_scale), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_ENERGY_OFFSET, -1, "energy_offset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, energy_offset), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_INPUT_COUNT_RATE, -1, "input_count_rate", \
						MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, input_count_rate), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCA_OUTPUT_COUNT_RATE, -1, "output_count_rate", \
						MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCA, output_count_rate), \
	{0}, NULL, 0}

typedef struct {
	mx_status_type ( *start ) ( MX_MCA *mca );
	mx_status_type ( *stop ) ( MX_MCA *mca );
	mx_status_type ( *read ) ( MX_MCA *mca );
	mx_status_type ( *clear ) ( MX_MCA *mca );
	mx_status_type ( *busy ) ( MX_MCA *mca );
	mx_status_type ( *get_parameter ) ( MX_MCA *mca );
	mx_status_type ( *set_parameter ) ( MX_MCA *mca );
} MX_MCA_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mx_mca_get_pointers( MX_RECORD *mca_record,
	MX_MCA **mca, MX_MCA_FUNCTION_LIST **function_list_ptr,
	const char *calling_fname );

MX_API mx_status_type mx_mca_initialize_type(
			long record_type,
			long *num_record_fields,
			MX_RECORD_FIELD_DEFAULTS **record_field_defaults,
			long *maximum_num_channels_varargs_cookie,
			long *maximum_num_rois_varargs_cookie,
			long *num_soft_rois_varargs_cookie );

MX_API_PRIVATE mx_status_type mx_mca_finish_record_initialization(
						MX_RECORD *mca_record );

MX_API mx_status_type mx_mca_start( MX_RECORD *mca_record );
MX_API mx_status_type mx_mca_stop( MX_RECORD *mca_record );
MX_API mx_status_type mx_mca_read( MX_RECORD *mca_record,
					unsigned long *num_channels,
					unsigned long **channel_array );
MX_API mx_status_type mx_mca_clear( MX_RECORD *mca_record );
MX_API mx_status_type mx_mca_is_busy( MX_RECORD *mca_record,
					mx_bool_type *busy );
MX_API mx_status_type mx_mca_is_new_data_available( MX_RECORD *mca_record,
					mx_bool_type *new_data_available );

MX_API mx_status_type mx_mca_start_without_preset( MX_RECORD *mca_record );
MX_API mx_status_type mx_mca_start_with_preset( MX_RECORD *mca_record,
						long preset_type,
						double preset_value );

MX_API mx_status_type mx_mca_start_for_preset_live_time( MX_RECORD *mca_record,
					double preset_seconds );
MX_API mx_status_type mx_mca_start_for_preset_real_time( MX_RECORD *mca_record,
					double preset_seconds );
MX_API mx_status_type mx_mca_start_for_preset_count( MX_RECORD *mca_record,
					unsigned long preset_count );

MX_API mx_status_type mx_mca_get_parameter( MX_RECORD *mca_record,
						long parameter_type );
MX_API mx_status_type mx_mca_set_parameter( MX_RECORD *mca_record,
						long parameter_type );

MX_API mx_status_type mx_mca_get_preset_type( MX_RECORD *mca_record,
						long *preset_type );
MX_API mx_status_type mx_mca_set_preset_type( MX_RECORD *mca_record,
						long preset_type );
MX_API mx_status_type mx_mca_get_preset_count_region( MX_RECORD *mca_record,
					unsigned long *preset_count_region );
MX_API mx_status_type mx_mca_set_preset_count_region( MX_RECORD *mca_record,
					unsigned long *preset_count_region );
MX_API mx_status_type mx_mca_get_roi( MX_RECORD *mca_record,
					unsigned long roi_number,
					unsigned long *roi );
MX_API mx_status_type mx_mca_set_roi( MX_RECORD *mca_record,
					unsigned long roi_number,
					unsigned long *roi );
MX_API mx_status_type mx_mca_get_roi_integral( MX_RECORD *mca_record,
					unsigned long roi_number,
					unsigned long *roi_integral );

MX_API mx_status_type mx_mca_get_roi_array( MX_RECORD *mca_record,
					unsigned long num_rois,
					unsigned long **roi_array );
MX_API mx_status_type mx_mca_set_roi_array( MX_RECORD *mca_record,
					unsigned long num_rois,
					unsigned long **roi_array );
MX_API mx_status_type mx_mca_get_roi_integral_array( MX_RECORD *mca_record,
					unsigned long num_rois,
					unsigned long *roi_integral_array );

MX_API mx_status_type mx_mca_get_num_channels( MX_RECORD *mca_record,
					unsigned long *num_channels );
MX_API mx_status_type mx_mca_set_num_channels( MX_RECORD *mca_record,
					unsigned long num_channels );
MX_API mx_status_type mx_mca_get_channel( MX_RECORD *mca_record,
					unsigned long channel_number,
					unsigned long *channel_value );
MX_API mx_status_type mx_mca_get_real_time( MX_RECORD *mca_record,
					double *real_time );
MX_API mx_status_type mx_mca_get_live_time( MX_RECORD *mca_record,
					double *live_time );
MX_API mx_status_type mx_mca_get_counts( MX_RECORD *mca_record,
					unsigned long *counts );

MX_API mx_status_type mx_mca_get_soft_roi( MX_RECORD *mca_record,
					unsigned long soft_roi_number,
					unsigned long *soft_roi );
MX_API mx_status_type mx_mca_set_soft_roi( MX_RECORD *mca_record,
					unsigned long soft_roi_number,
					unsigned long *soft_roi );
MX_API mx_status_type mx_mca_get_soft_roi_integral( MX_RECORD *mca_record,
					unsigned long soft_roi_number,
					unsigned long *soft_roi_integral );
MX_API mx_status_type mx_mca_get_soft_roi_integral_array( MX_RECORD *mca_record,
					unsigned long num_rois,
					unsigned long *soft_roi_integral_array);

MX_API mx_status_type mx_mca_get_energy_scale( MX_RECORD *mca_record,
						double *energy_scale );
MX_API mx_status_type mx_mca_set_energy_scale( MX_RECORD *mca_record,
						double energy_scale );
MX_API mx_status_type mx_mca_get_energy_offset( MX_RECORD *mca_record,
						double *energy_offset );
MX_API mx_status_type mx_mca_set_energy_offset( MX_RECORD *mca_record,
						double energy_offset );

MX_API mx_status_type mx_mca_get_energy_axis_array( MX_RECORD *mca_record,
						unsigned long num_channels,
						double *energy_axis_array );

MX_API mx_status_type mx_mca_get_input_count_rate( MX_RECORD *mca_record,
						double *input_count_rate );

MX_API mx_status_type mx_mca_get_output_count_rate( MX_RECORD *mca_record,
						double *output_count_rate );

MX_API mx_status_type mx_mca_default_get_parameter_handler( MX_MCA *mca );

MX_API mx_status_type mx_mca_default_set_parameter_handler( MX_MCA *mca );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MCA_H__ */
