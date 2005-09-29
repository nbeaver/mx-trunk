/*
 * Name:    d_network_ptz.h
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

#ifndef __D_NETWORK_PTZ_H__
#define __D_NETWORK_PTZ_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD command_nf;
	MX_NETWORK_FIELD status_nf;
	MX_NETWORK_FIELD pan_position_nf;
	MX_NETWORK_FIELD pan_destination_nf;
	MX_NETWORK_FIELD pan_speed_nf;
	MX_NETWORK_FIELD tilt_position_nf;
	MX_NETWORK_FIELD tilt_destination_nf;
	MX_NETWORK_FIELD tilt_speed_nf;
	MX_NETWORK_FIELD zoom_position_nf;
	MX_NETWORK_FIELD zoom_destination_nf;
	MX_NETWORK_FIELD zoom_speed_nf;
	MX_NETWORK_FIELD zoom_on_nf;
	MX_NETWORK_FIELD focus_position_nf;
	MX_NETWORK_FIELD focus_destination_nf;
	MX_NETWORK_FIELD focus_speed_nf;
	MX_NETWORK_FIELD focus_auto_nf;
} MX_NETWORK_PTZ;

#define MXD_NETWORK_PTZ_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_PTZ, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
  					NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_PTZ, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)},

/* Define all of the device functions. */

MX_API mx_status_type mxd_network_ptz_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_ptz_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_network_ptz_command( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_network_ptz_get_status( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_network_ptz_get_parameter( MX_PAN_TILT_ZOOM *ptz );
MX_API mx_status_type mxd_network_ptz_set_parameter( MX_PAN_TILT_ZOOM *ptz );

extern MX_RECORD_FUNCTION_LIST mxd_network_ptz_record_function_list;
extern MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_network_ptz_ptz_function_list;

extern long mxd_network_ptz_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_ptz_rfield_def_ptr;

#endif /* __D_NETWORK_PTZ_H__ */

