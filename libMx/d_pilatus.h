/*
 * Name:    d_pilatus.h
 *
 * Purpose: Header file for Dectris Pilatus detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PILATUS_H__
#define __D_PILATUS_H__

#define MXU_PILATUS_TVX_VERSION_LENGTH	80
#define MXU_PILATUS_CAMERA_NAME_LENGTH	80

/* Values for the 'pilatus_flags' field. */

#define MXF_PILATUS_DEBUG_SERIAL			0x1
#define MXF_PILATUS_LOAD_FRAME_AFTER_ACQUISITION	0x2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long pilatus_flags;

	mx_bool_type pilatus_debug_flag;

	char tvx_version[MXU_PILATUS_TVX_VERSION_LENGTH+1];
	char camera_name[MXU_PILATUS_CAMERA_NAME_LENGTH+1];
	unsigned long camserver_pid;

	mx_bool_type exposure_in_progress;
} MX_PILATUS;


#define MXD_PILATUS_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "pilatus_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, pilatus_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "pilatus_debug_flag", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, pilatus_debug_flag), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "tvx_version", MXFT_STRING, NULL, \
			1, {MXU_PILATUS_TVX_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, tvx_version), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "camera_name", MXFT_STRING, NULL, \
			1, {MXU_PILATUS_CAMERA_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, camera_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "camserver_pid", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, camserver_pid), \
	{0}, NULL, MXFF_READ_ONLY }

MX_API mx_status_type mxd_pilatus_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_pilatus_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_pilatus_open( MX_RECORD *record );

MX_API mx_status_type mxd_pilatus_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_last_frame_number( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_total_num_frames( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_transfer_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_load_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_save_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_copy_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_set_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_measure_correction( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_setup_oscillation( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_trigger_oscillation( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_pilatus_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST
			mxd_pilatus_ad_function_list;

extern long mxd_pilatus_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pilatus_rfield_def_ptr;

MX_API mx_status_type mxd_pilatus_command( MX_PILATUS *pilatus,
					char *command,
					char *response,
					size_t response_buffer_length,
					unsigned long *pilatus_return_code,
					unsigned long debug_flag );

#endif /* __D_PILATUS_H__ */

