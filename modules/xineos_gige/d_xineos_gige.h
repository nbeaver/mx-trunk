/*
 * Name:    d_xineos_gige.h
 *
 * Purpose: MX driver header for the DALSA Xineos GigE series of detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_XINEOS_GIGE_H__
#define __D_XINEOS_GIGE_H__

#include "mx_image_noir.h"

/* Values for the 'xineos_gige_flags' field. */

#define MXF_XINEOS_GIGE_DO_NOT_ROTATE_IMAGE			0x1
#define MXF_XINEOS_GIGE_AUTOMATICALLY_DUMP_PIXEL_VALUES		0x2
#define MXF_XINEOS_GIGE_KEEP_EXPOSURE_TIME_FIXED		0x4

/*---*/

#define MXT_XINEOS_GIGE_MINIMUM_INTERFRAME_GAP		0.055	/* in sec */

/* Values for the 'detector_model' field. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
	unsigned long xineos_gige_flags;
	char pulse_generator_name[MXU_RECORD_NAME_LENGTH+1];

	MX_RECORD *pulse_generator_record;

	mx_bool_type pulse_generator_is_available;
	mx_bool_type start_with_pulse_generator;
	mx_bool_type using_external_video_duration_mode;
	double pulse_generator_time_threshold;
	double minimum_interframe_gap;

	double minimum_pixel_value;
	double saturation_pixel_value;

	mx_bool_type dump_pixel_values;

	MX_IMAGE_NOIR_INFO *image_noir_info;

} MX_XINEOS_GIGE;

#define MXD_XINEOS_GIGE_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_XINEOS_GIGE, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "xineos_gige_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_XINEOS_GIGE, xineos_gige_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pulse_generator_name", MXFT_STRING, NULL, \
					1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_XINEOS_GIGE, pulse_generator_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pulse_generator_is_available", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_XINEOS_GIGE, pulse_generator_is_available), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "start_with_pulse_generator", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_XINEOS_GIGE, start_with_pulse_generator), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "using_external_video_duration_mode", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_XINEOS_GIGE, using_external_video_duration_mode), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "pulse_generator_time_threshold", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_XINEOS_GIGE, pulse_generator_time_threshold), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "minimum_interframe_gap", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XINEOS_GIGE, minimum_interframe_gap), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "minimum_pixel_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_XINEOS_GIGE, minimum_pixel_value), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "saturation_pixel_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_XINEOS_GIGE, saturation_pixel_value), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "dump_pixel_values", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XINEOS_GIGE, dump_pixel_values), \
	{0}, NULL, 0 }

MX_API mx_status_type mxd_xineos_gige_initialize_driver(
							MX_DRIVER *driver );
MX_API mx_status_type mxd_xineos_gige_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_xineos_gige_open( MX_RECORD *record );

MX_API mx_status_type mxd_xineos_gige_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_xineos_gige_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_xineos_gige_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_xineos_gige_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_xineos_gige_readout_frame(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_xineos_gige_correct_frame(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_xineos_gige_get_parameter(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_xineos_gige_set_parameter(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_xineos_gige_measure_correction(
						MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_xineos_gige_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_xineos_gige_ad_function_list;

extern long mxd_xineos_gige_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_xineos_gige_rfield_def_ptr;

#endif /* __D_XINEOS_GIGE_H__ */

