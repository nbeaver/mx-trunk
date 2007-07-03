/*
 * Name:    d_file_vinput.h
 *
 * Purpose: MX header file for an emulated video input device that reads
 *          image frames from a directory of existing image files.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_FILE_VINPUT_H__
#define __D_FILE_VINPUT_H__

typedef struct {
	MX_RECORD *record;
	char file_format_name[MXU_IMAGE_FILE_NAME_LENGTH+1];
	char directory_name[MXU_FILENAME_LENGTH+1];

	long file_format;

	long num_files;
	char **filename_array;
	long current_filenum;
} MX_FILE_VINPUT;


#define MXD_FILE_VINPUT_STANDARD_FIELDS \
  {-1, -1, "file_format_name", MXFT_STRING, \
				NULL, 1, {MXU_IMAGE_FILE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FILE_VINPUT, file_format_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "directory_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FILE_VINPUT, directory_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API mx_status_type mxd_file_vinput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_file_vinput_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_file_vinput_open( MX_RECORD *record );
MX_API mx_status_type mxd_file_vinput_close( MX_RECORD *record );

MX_API mx_status_type mxd_file_vinput_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_file_vinput_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_file_vinput_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_file_vinput_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_file_vinput_get_last_frame_number(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_file_vinput_get_total_num_frames(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_file_vinput_get_status( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_file_vinput_get_frame( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_file_vinput_get_parameter(MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_file_vinput_set_parameter(MX_VIDEO_INPUT *vinput);

extern MX_RECORD_FUNCTION_LIST mxd_file_vinput_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_file_vinput_video_function_list;

extern long mxd_file_vinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_file_vinput_rfield_def_ptr;

#endif /* __D_FILE_VINPUT_H__ */

