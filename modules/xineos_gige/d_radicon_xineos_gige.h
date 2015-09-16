/*
 * Name:    d_radicon_xineos_gige.h
 *
 * Purpose: MX driver header for the Radicon Xineos GigE series of detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_RADICON_XINEOS_GIGE_H__
#define __D_RADICON_XINEOS_GIGE_H__

#include "mx_image_noir.h"

/* Values for the 'radicon_xineos_gige_flags' field. */

#define MXF_RADICON_XINEOS_GIGE_DO_NOT_ROTATE_IMAGE		0x1

/* Values for the 'detector_model' field. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
	unsigned long radicon_xineos_gige_flags;
	char pulse_generator_name[MXU_RECORD_NAME_LENGTH+1];

	MX_RECORD *pulse_generator_record;

	mx_bool_type use_pulse_generator;
	double pulse_generator_time_threshold;

	double minimum_pixel_value;
	double saturation_pixel_value;

	MX_IMAGE_NOIR_INFO *image_noir_info;

} MX_RADICON_XINEOS_GIGE;

#define MXD_RADICON_XINEOS_GIGE_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_XINEOS_GIGE, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "radicon_xineos_gige_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_XINEOS_GIGE, radicon_xineos_gige_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pulse_generator_name", MXFT_STRING, NULL, \
					1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_XINEOS_GIGE, pulse_generator_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "minimum_pixel_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_XINEOS_GIGE, minimum_pixel_value), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "saturation_pixel_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_XINEOS_GIGE, saturation_pixel_value), \
	{0}, NULL, MXFF_READ_ONLY }

MX_API mx_status_type mxd_radicon_xineos_gige_initialize_driver(
							MX_DRIVER *driver );
MX_API mx_status_type mxd_radicon_xineos_gige_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_radicon_xineos_gige_open( MX_RECORD *record );

MX_API mx_status_type mxd_radicon_xineos_gige_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_gige_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_gige_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_gige_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_gige_readout_frame(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_gige_correct_frame(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_gige_get_parameter(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_gige_set_parameter(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_gige_measure_correction(
						MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_radicon_xineos_gige_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_radicon_xineos_gige_ad_function_list;

extern long mxd_radicon_xineos_gige_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_radicon_xineos_gige_rfield_def_ptr;

#endif /* __D_RADICON_XINEOS_GIGE_H__ */

