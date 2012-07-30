/*
 * Name:    d_radicon_taurus.h
 *
 * Purpose: MX driver header for the Radicon Taurus series of CMOS detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_RADICON_TAURUS_H__
#define __D_RADICON_TAURUS_H__

/* Values for the 'radicon_taurus_flags' field. */

#define MXF_RADICON_TAURUS_ACQUIRE_DUAL_FRAMES	0x1
#define MXF_RADICON_TAURUS_MERGE_DUAL_FRAMES	0x2

/* Values for the 'detector_model' field. */

#define MXT_RADICON_TAURUS	1
#define MXT_RADICON_XINEOS	2

#define MXU_RADICON_TAURUS_SERIAL_NUMBER_LENGTH		40

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
	MX_RECORD *serial_port_record;
	unsigned long radicon_taurus_flags;

	unsigned long detector_model;
	char serial_number[MXU_RADICON_TAURUS_SERIAL_NUMBER_LENGTH+1];
	unsigned long firmware_version;

	unsigned long readout_mode;
	uint64_t si1_register;
	uint64_t si2_register;
	double si1_si2_ratio;

	MX_IMAGE_FRAME *raw_frame;

	mx_bool_type bypass_arm;
	mx_bool_type use_raw_frames;
} MX_RADICON_TAURUS;


#define MXD_RADICON_TAURUS_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_TAURUS, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "serial_port_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_TAURUS, serial_port_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "radicon_taurus_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_TAURUS, radicon_taurus_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "detector_model", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, detector_model), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "serial_number", MXFT_STRING, \
			NULL, 1, {MXU_RADICON_TAURUS_SERIAL_NUMBER_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, serial_number), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "firmware_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, firmware_version), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "readout_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, readout_mode), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "si1_register", MXFT_UINT64, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, si1_register), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "si2_register", MXFT_UINT64, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, si2_register), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "si1_si2_ratio", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, si1_si2_ratio), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "bypass_arm", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, bypass_arm), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "use_raw_frames", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, use_raw_frames), \
	{0}, NULL, 0 }

MX_API mx_status_type mxd_radicon_taurus_initialize_driver(
							MX_DRIVER *driver );
MX_API mx_status_type mxd_radicon_taurus_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_radicon_taurus_open( MX_RECORD *record );
MX_API mx_status_type mxd_radicon_taurus_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_radicon_taurus_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_readout_frame(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_get_parameter(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_set_parameter(
						MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_radicon_taurus_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_radicon_taurus_ad_function_list;

extern long mxd_radicon_taurus_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_radicon_taurus_rfield_def_ptr;

MX_API_PRIVATE
mx_status_type mxd_radicon_taurus_command( MX_RADICON_TAURUS *radicon_taurus,
						char *command,
						char *response,
						size_t max_response_length,
						mx_bool_type debug_flag );

#endif /* __D_RADICON_TAURUS_H__ */

