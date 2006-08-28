/*
 * Name:    d_epix_xclib.h
 *
 * Purpose: MX driver header for EPIX, Inc. video inputs via XCLIB.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPIX_XCLIB_H__
#define __D_EPIX_XCLIB_H__

typedef struct {
	MX_RECORD *record;

	long unit_number;

	long unitmap;
} MX_EPIX_XCLIB_VIDEO_INPUT;


#define MXD_EPIX_XCLIB_STANDARD_FIELDS \
  {-1, -1, "unit_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPIX_XCLIB_VIDEO_INPUT, unit_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_epix_xclib_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_open( MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_close( MX_RECORD *record );

MX_API mx_status_type mxd_epix_xclib_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_busy( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_get_status( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_get_frame( MX_VIDEO_INPUT *vinput,
						MX_IMAGE_FRAME **frame );
MX_API mx_status_type mxd_epix_xclib_get_sequence( MX_VIDEO_INPUT *vinput,
						MX_IMAGE_SEQUENCE **sequence );
MX_API mx_status_type mxd_epix_xclib_get_parameter( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_set_parameter( MX_VIDEO_INPUT *vinput );

extern MX_RECORD_FUNCTION_LIST mxd_epix_xclib_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_epix_xclib_video_function_list;

extern long mxd_epix_xclib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epix_xclib_rfield_def_ptr;

#endif /* __D_EPIX_XCLIB_H__ */

