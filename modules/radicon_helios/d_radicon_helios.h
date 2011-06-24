/*
 * Name:    d_radicon_helios.h
 *
 * Purpose: MX header for the Molecular Biology Consortium's NOIR 1 detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_RADICON_HELIOS_H__
#define __D_RADICON_HELIOS_H__

#ifdef __cplusplus

#define MXU_DETECTOR_TYPE_NAME_LENGTH		40

/* --- Values for the 'helios_flags' field. --- */

#define MXF_RADICON_HELIOS_FORCE_BYTESWAP	0x1
#define MXF_RADICON_HELIOS_AUTODETECT_BYTESWAP	0x2

/* --- Values for the 'detector_type' field. --- */

#define MXT_RADICON_HELIOS_10x10		1
#define MXT_RADICON_HELIOS_25x20		2
#define MXT_RADICON_HELIOS_30x30		3

#define MXT_RADICON_HELIOS_TEST			1000

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
	char detector_type_name[MXU_DETECTOR_TYPE_NAME_LENGTH+1];
	MX_RECORD *external_trigger_record;
	char pulse_generator_record_name[MXU_RECORD_NAME_LENGTH+1];
	unsigned long helios_flags;
	char helios_file_format_name[MXU_IMAGE_FORMAT_NAME_LENGTH+1];
	char gain_array_filename[MXU_FILENAME_LENGTH+1];

	long detector_type;
	MX_RECORD *pulse_generator_record;

	mx_bool_type arm_signal_present;
	mx_bool_type acquisition_in_progress;

	mx_bool_type byteswap;
} MX_RADICON_HELIOS;

#define MXD_RADICON_HELIOS_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_HELIOS, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "detector_type_name", MXFT_STRING, \
				NULL, 1, {MXU_DETECTOR_TYPE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_HELIOS, detector_type_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "external_trigger_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_HELIOS, external_trigger_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pulse_generator_record_name", MXFT_STRING, \
		NULL, 1, {MXU_DETECTOR_TYPE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_HELIOS, pulse_generator_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "helios_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_HELIOS, helios_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "helios_file_format_name", MXFT_STRING, \
				NULL, 1, {MXU_IMAGE_FORMAT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_RADICON_HELIOS, helios_file_format_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "gain_array_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_HELIOS, gain_array_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "detector_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_HELIOS, detector_type), \
	{0}, NULL, 0 }

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxd_radicon_helios_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_radicon_helios_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_radicon_helios_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_radicon_helios_open( MX_RECORD *record );

MX_API mx_status_type mxd_radicon_helios_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_transfer_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_radicon_helios_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_radicon_helios_ad_function_list;

extern long mxd_radicon_helios_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_radicon_helios_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_RADICON_HELIOS_H__ */

