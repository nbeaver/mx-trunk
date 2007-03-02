/*
 * Name:    i_camera_link_api.h
 *
 * Purpose: Header for using the generic Camera Link API.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_CAMERA_LINK_API_H__
#define __I_CAMERA_LINK_API_H__

#include "mx_dynamic_library.h"

#if defined(IS_MX_DRIVER)

typedef struct {
	MX_RECORD *record;
	MX_CAMERA_LINK *camera_link;

	MX_DYNAMIC_LIBRARY *library;

	char library_filename[MXU_FILENAME_LENGTH+1];
} MX_CAMERA_LINK_API;

extern MX_CAMERA_LINK_API_LIST mxi_camera_link_api_api_list;

#endif /* IS_MX_DRIVER */

MX_API mx_status_type mxi_camera_link_api_create_record_structures(
						MX_RECORD *record );

MX_API mx_status_type mxi_camera_link_api_open( MX_RECORD *record );

MX_API mx_status_type mxi_camera_link_api_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_camera_link_api_record_function_list;

extern long mxi_camera_link_api_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_camera_link_api_rfield_def_ptr;

#define MXI_CAMERA_LINK_API_STANDARD_FIELDS \
  {-1, -1, "library_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CAMERA_LINK_API, library_filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_CAMERA_LINK_API_H__ */

