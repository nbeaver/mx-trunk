/*
 * Name:    mx_ptz.h
 *
 * Purpose: MX header file for Pan/Tilt/Zoom camera supports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_PTZ_H__
#define __MX_PTZ_H__

#include "mx_record.h"

#define MXU_PTZ_MAX_PARAMETERS		2

#define MXSF_PTZ_IS_BUSY		0x1

/* Flag values for mx_ptz_command(). */

#define MXF_PTZ_DRIVE			0x1000

#define MXF_PTZ_DRIVE_UP		0x1001
#define MXF_PTZ_DRIVE_DOWN		0x1002
#define MXF_PTZ_DRIVE_LEFT		0x1003
#define MXF_PTZ_DRIVE_RIGHT		0x1004
#define MXF_PTZ_DRIVE_UPPER_LEFT	0x1005
#define MXF_PTZ_DRIVE_UPPER_RIGHT	0x1006
#define MXF_PTZ_DRIVE_LOWER_LEFT	0x1007
#define MXF_PTZ_DRIVE_LOWER_RIGHT	0x1008
#define MXF_PTZ_DRIVE_STOP		0x1009
#define MXF_PTZ_DRIVE_HOME		0x100a

#define MXF_PTZ_DRIVE_TO		0x1101

#define MXF_PTZ_DRIVE_OFF		0x1801
#define MXF_PTZ_DRIVE_ON		0x1802

#define MXF_PTZ_ZOOM			0x2000

#define MXF_PTZ_ZOOM_OUT		0x2001
#define MXF_PTZ_ZOOM_IN			0x2002
#define MXF_PTZ_ZOOM_STOP		0x2003

#define MXF_PTZ_ZOOM_TO			0x2101

#define MXF_PTZ_ZOOM_OFF		0x2801
#define MXF_PTZ_ZOOM_ON			0x2802

#define MXF_PTZ_FOCUS			0x4000

#define MXF_PTZ_FOCUS_MANUAL		0x4001
#define MXF_PTZ_FOCUS_AUTO		0x4002
#define MXF_PTZ_FOCUS_FAR		0x4003
#define MXF_PTZ_FOCUS_NEAR		0x4004
#define MXF_PTZ_FOCUS_STOP		0x4005

#define MXF_PTZ_FOCUS_TO		0x4101

#define MXF_PTZ_FOCUS_OFF		0x4801
#define MXF_PTZ_FOCUS_ON		0x4802

typedef struct {
	MX_RECORD *record;

	unsigned long ptz_flags;

	int parameter_type;
	unsigned long parameter_value[MXU_PTZ_MAX_PARAMETERS];

	unsigned long command;
	unsigned long status;
} MX_PAN_TILT_ZOOM;

#define MX_PAN_TILT_ZOOM_STANDARD_FIELDS \
  {-1, -1, "ptz_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, ptz_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/*
 * The structure type MX_PAN_TILT_ZOOM_FUNCTION_LIST contains
 * a list of pointers to functions that vary from driver to driver.
 */

typedef struct {
	mx_status_type ( *command ) ( MX_PAN_TILT_ZOOM *ptz );
	mx_status_type ( *get_status ) ( MX_PAN_TILT_ZOOM *ptz );
	mx_status_type ( *get_parameter ) ( MX_PAN_TILT_ZOOM *ptz );
	mx_status_type ( *set_parameter ) ( MX_PAN_TILT_ZOOM *ptz );
} MX_PAN_TILT_ZOOM_FUNCTION_LIST;

MX_API mx_status_type mx_ptz_command( MX_RECORD *ptz_record,
					unsigned long command );

MX_API mx_status_type mx_ptz_get_status( MX_RECORD *ptz_record,
					unsigned long *status );

/* ----- */

MX_API mx_status_type mx_ptz_get_parameter( MX_RECORD *ptz_record,
					int parameter_type,
					unsigned long *parameter_value );

MX_API mx_status_type mx_ptz_set_parameter( MX_RECORD *ptz_record,
					int parameter_type,
					unsigned long parameter_value );

MX_API mx_status_type mx_ptz_default_command_handler( MX_PAN_TILT_ZOOM *ptz );

MX_API mx_status_type mx_ptz_default_get_parameter_handler(
						MX_PAN_TILT_ZOOM *ptz );

MX_API mx_status_type mx_ptz_default_set_parameter_handler(
						MX_PAN_TILT_ZOOM *ptz );

/* ----- */

MX_API mx_status_type mx_ptz_zoom_in( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_zoom_out( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_zoom_stop( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_zoom_to( MX_RECORD *ptz_record,
					unsigned long zoom_value );

MX_API mx_status_type mx_ptz_zoom_off( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_zoom_on( MX_RECORD *ptz_record );

#endif /* __MX_PTZ_H__ */

