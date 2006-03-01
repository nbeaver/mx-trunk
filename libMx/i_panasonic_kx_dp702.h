/*
 * Name:    i_panasonic_kx_dp702.h
 *
 * Purpose: MX header file for Panasonic KX-DP702 PTZ units.
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

#ifndef __I_PANASONIC_KX_DP702_H__
#define __I_PANASONIC_KX_DP702_H__

#define MXU_PANASONIC_KX_DP702_MAX_CMD_LENGTH	14

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;

	long num_cameras;
	long last_camera_number;
} MX_PANASONIC_KX_DP702;

#define MXI_PANASONIC_KX_DP702_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANASONIC_KX_DP702, rs232_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_panasonic_kx_dp702_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_panasonic_kx_dp702_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_panasonic_kx_dp702_record_function_list;

extern long mxi_panasonic_kx_dp702_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_panasonic_kx_dp702_rfield_def_ptr;

MX_API mx_status_type
mxi_panasonic_kx_dp702_cmd( MX_PANASONIC_KX_DP702 *panasonic_kx_dp702,
			long camera_number,
			unsigned char *command,
			size_t command_length );

MX_API mx_status_type
mxi_panasonic_kx_dp702_raw_cmd( MX_PANASONIC_KX_DP702 *panasonic_kx_dp702,
			long camera_number,
			unsigned char *command,
			size_t command_length,
			unsigned char *response,
			size_t response_length,
			size_t *actual_response_length );

#endif /* __I_PANASONIC_KX_DP702_H__ */

