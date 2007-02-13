/*
 * Name:    d_edt.h
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

#ifndef __D_EDT_H__
#define __D_EDT_H__

/* Flag values for the 'edt_flags' field. */

#define MXF_EDT_INVERT_EXPOSE_SIGNAL	0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *edt_record;
	long unit_number;
	long channel_number;
	unsigned long edt_flags;
	unsigned long maximum_num_frames;

#if IS_MX_DRIVER
	PdvDev *pdv_p;
#endif

} MX_EDT_VIDEO_INPUT;


#define MXD_EDT_STANDARD_FIELDS \
  {-1, -1, "edt_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EDT_VIDEO_INPUT, edt_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "unit_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EDT_VIDEO_INPUT, unit_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EDT_VIDEO_INPUT, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "edt_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EDT_VIDEO_INPUT, edt_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "maximum_num_frames", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EDT_VIDEO_INPUT, maximum_num_frames), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_edt_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_edt_finish_record_initialization( MX_RECORD *record );
MX_API mx_status_type mxd_edt_open( MX_RECORD *record );
MX_API mx_status_type mxd_edt_close( MX_RECORD *record );

MX_API mx_status_type mxd_edt_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_edt_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_edt_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_edt_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_edt_get_status( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_edt_get_frame( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_edt_get_parameter( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_edt_set_parameter( MX_VIDEO_INPUT *vinput );

extern MX_RECORD_FUNCTION_LIST mxd_edt_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_edt_video_function_list;

extern long mxd_edt_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_edt_rfield_def_ptr;

#endif /* __D_EDT_H__ */

