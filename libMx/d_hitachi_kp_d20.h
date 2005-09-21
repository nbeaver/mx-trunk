/*
 * Name:    d_hitachi_kp_d20.h
 *
 * Purpose: MX Pan/Tilt/Zoom driver for Hitachi KP-D20A/B cameras.
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

#ifndef __D_HITACHI_KP_D20_H__
#define __D_HITACHI_KP_D20_H__

#define MXU_HITACHI_KP_D20_MAX_CMD_LENGTH	14

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
} MX_HITACHI_KP_D20;

#define MXD_HITACHI_KP_D20_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HITACHI_KP_D20, rs232_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the device functions. */

MX_API mx_status_type mxd_hitachi_kp_d20_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_hitachi_kp_d20_open( MX_RECORD *record );

MX_API mx_status_type mxd_hitachi_kp_d20_command( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_hitachi_kp_d20_get_status( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_hitachi_kp_d20_get_parameter( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_hitachi_kp_d20_set_parameter( MX_PAN_TILT_ZOOM *ptz );

extern MX_RECORD_FUNCTION_LIST mxd_hitachi_kp_d20_record_function_list;
extern MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_hitachi_kp_d20_ptz_function_list;

extern long mxd_hitachi_kp_d20_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_hitachi_kp_d20_rfield_def_ptr;

#endif /* __D_HITACHI_KP_D20_H__ */

