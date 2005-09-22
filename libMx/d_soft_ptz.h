/*
 * Name:    d_soft_ptz.h
 *
 * Purpose: MX software-emulated Pan/Tilt/Zoom driver.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_PTZ_H__
#define __D_SOFT_PTZ_H__

typedef struct {
	MX_RECORD *record;
} MX_SOFT_PTZ;

#define MXD_SOFT_PTZ_STANDARD_FIELDS

/* Define all of the device functions. */

MX_API mx_status_type mxd_soft_ptz_create_record_structures(MX_RECORD *record);
MX_API mx_status_type mxd_soft_ptz_open( MX_RECORD *record );

MX_API mx_status_type mxd_soft_ptz_command( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_soft_ptz_get_status( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_soft_ptz_get_parameter( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_soft_ptz_set_parameter( MX_PAN_TILT_ZOOM *ptz );

extern MX_RECORD_FUNCTION_LIST mxd_soft_ptz_record_function_list;
extern MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_soft_ptz_ptz_function_list;

extern long mxd_soft_ptz_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_ptz_rfield_def_ptr;

#endif /* __D_SOFT_PTZ_H__ */

