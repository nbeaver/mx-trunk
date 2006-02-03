/*
 * Name:    d_panasonic_kx_dp702.h
 *
 * Purpose: MX header file for Panasonic KX-DP702 PTZ records.
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

#ifndef __D_PANASONIC_KX_DP702_H__
#define __D_PANASONIC_KX_DP702_H__

#define MXU_PANASONIC_KX_DP702_PTZ_MAX_CMD_LENGTH	14

typedef struct {
	MX_RECORD *record;

	MX_RECORD *kx_dp702_record;
	int camera_number;
} MX_PANASONIC_KX_DP702_PTZ;

#define MXD_PANASONIC_KX_DP702_PTZ_STANDARD_FIELDS \
  {-1, -1, "kx_dp702_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_PANASONIC_KX_DP702_PTZ, kx_dp702_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "camera_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT,offsetof(MX_PANASONIC_KX_DP702_PTZ, camera_number),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the device functions. */

MX_API mx_status_type mxd_panasonic_kx_dp702_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_panasonic_kx_dp702_command( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_panasonic_kx_dp702_get_status(
							MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_panasonic_kx_dp702_get_parameter(
							MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_panasonic_kx_dp702_set_parameter(
							MX_PAN_TILT_ZOOM *ptz );

extern MX_RECORD_FUNCTION_LIST mxd_panasonic_kx_dp702_record_function_list;
extern MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_panasonic_kx_dp702_ptz_function_list;

extern mx_length_type mxd_panasonic_kx_dp702_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_panasonic_kx_dp702_rfield_def_ptr;

#endif /* __D_PANASONIC_KX_DP702_H__ */

