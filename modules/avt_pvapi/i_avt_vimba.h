/*
 * Name:    i_avt_vimba.h
 *
 * Purpose: Header file for the Vimba API used by cameras from 
 *          Allied Vision Technologies.
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

#ifndef __I_AVT_VIMBA_H__
#define __I_AVT_VIMBA_H__

#include <VimbaC.h>

typedef struct {
	MX_RECORD *record;

	unsigned long avt_vimba_flags;
	unsigned long vimba_version;

	unsigned long num_cameras;
	VmbCameraInfo_t *camera_info;
} MX_AVT_VIMBA;

#define MXI_AVT_VIMBA_STANDARD_FIELDS \
  {-1, -1, "avt_vimba_flags", MXFT_HEX, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_AVT_VIMBA, avt_vimba_flags), \
        {0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "vimba_version", MXFT_ULONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_AVT_VIMBA, vimba_version), \
        {0}, NULL, 0 }

MX_API mx_status_type mxi_avt_vimba_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_avt_vimba_open( MX_RECORD *record );
MX_API mx_status_type mxi_avt_vimba_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_avt_vimba_record_function_list;

extern long mxi_avt_vimba_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_avt_vimba_rfield_def_ptr;

#endif /* __I_AVT_VIMBA_H__ */

