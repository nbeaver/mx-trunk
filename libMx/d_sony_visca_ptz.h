/*
 * Name:    d_sony_visca_ptz.h
 *
 * Purpose: MX header file for Sony Pan/Tilt/Zoom camera supports that use
 *          the VISCA protocol for communication.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SONY_VISCA_PTZ_H__
#define __D_SONY_VISCA_PTZ_H__

#define MXU_SONY_VISCA_PTZ_MAX_CMD_LENGTH	14

typedef struct {
	MX_RECORD *record;

	MX_RECORD *visca_record;
	int32_t camera_number;
} MX_SONY_VISCA_PTZ;

#define MXD_SONY_VISCA_PTZ_STANDARD_FIELDS \
  {-1, -1, "visca_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SONY_VISCA_PTZ, visca_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "camera_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SONY_VISCA_PTZ, camera_number),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the device functions. */

MX_API mx_status_type mxd_sony_visca_ptz_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_sony_visca_ptz_command( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_sony_visca_ptz_get_status( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_sony_visca_ptz_get_parameter( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_sony_visca_ptz_set_parameter( MX_PAN_TILT_ZOOM *ptz );

extern MX_RECORD_FUNCTION_LIST mxd_sony_visca_ptz_record_function_list;
extern MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_sony_visca_ptz_ptz_function_list;

extern mx_length_type mxd_sony_visca_ptz_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sony_visca_ptz_rfield_def_ptr;

#endif /* __D_SONY_VISCA_PTZ_H__ */

