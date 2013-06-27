/*
 * Name:    d_rdi_mbc_filename.h
 *
 * Purpose: MX video input driver header for the Vimba C API
 *          used by cameras from Allied Vision Technologies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_RDI_MBC_FILENAME_H__
#define __D_RDI_MBC_FILENAME_H__

#define MXU_RDI_MBC_FILENAME_TYPE_LENGTH	20

typedef struct {
	MX_RECORD *record;

	unsigned long rdi_mbc_filename_type;
	char rdi_mbc_filename_type_name[MXU_RDI_MBC_FILENAME_TYPE_LENGTH+1];

} MX_RDI_MBC_FILENAME_CAMERA;


#define MXD_RDI_MBC_FILENAME_STANDARD_FIELDS \
  {-1, -1, "rdi_mbc_filename_type_name", MXFT_STRING, \
			NULL, 1, {MXU_RDI_MBC_FILENAME_TYPE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_RDI_MBC_FILENAME_CAMERA, rdi_mbc_filename_type_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }

MX_API mx_status_type mxd_rdi_mbc_filename_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_rdi_mbc_filename_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_rdi_mbc_filename_open( MX_RECORD *record );
MX_API mx_status_type mxd_rdi_mbc_filename_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxd_rdi_mbc_filename_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_rdi_mbc_filename_video_function_list;

extern long mxd_rdi_mbc_filename_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_rdi_mbc_filename_rfield_def_ptr;

#endif /* __D_RDI_MBC_FILENAME_H__ */

