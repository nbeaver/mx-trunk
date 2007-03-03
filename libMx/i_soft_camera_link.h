/*
 * Name:    i_soft_camera_link.h
 *
 * Purpose: Header for MX driver for EPIX Camera Link serial I/O.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SOFT_CAMERA_LINK_H__
#define __I_SOFT_CAMERA_LINK_H__

#if defined(IS_MX_DRIVER)

MX_API mx_status_type mxi_soft_camera_link_create_record_structures(
						MX_RECORD *record );

MX_API mx_status_type mxi_soft_camera_link_open( MX_RECORD *record );

MX_API mx_status_type mxi_soft_camera_link_close( MX_RECORD *record );

typedef struct {
	MX_RECORD *record;
	MX_CAMERA_LINK *camera_link;
	struct mx_soft_camera_link_port_type *port;
} MX_SOFT_CAMERA_LINK;

typedef struct mx_soft_camera_link_port_type {
	MX_RECORD *record;
	MX_CAMERA_LINK *camera_link;
	MX_SOFT_CAMERA_LINK *soft_camera_link;
} MX_SOFT_CAMERA_LINK_PORT;

extern MX_CAMERA_LINK_API_LIST mxi_soft_camera_link_api_list;

MX_API INT32 MX_CLCALL mxi_soft_camera_link_get_num_bytes_avail(
							hSerRef serial_ref,
							UINT32 *num_bytes );

MX_API void  MX_CLCALL mxi_soft_camera_link_serial_close( hSerRef serial_ref );

MX_API INT32 MX_CLCALL mxi_soft_camera_link_serial_init( UINT32 serial_index,
						      hSerRef *serial_ref_ptr );

MX_API INT32 MX_CLCALL mxi_soft_camera_link_serial_read( hSerRef serial_ref,
							INT8 *buffer,
							UINT32 *num_bytes,
							UINT32 serial_timeout );

MX_API INT32 MX_CLCALL mxi_soft_camera_link_serial_write( hSerRef serial_ref,
							INT8 *buffer,
							UINT32 *num_bytes,
							UINT32 serial_timeout );

MX_API INT32 MX_CLCALL mxi_soft_camera_link_set_baud_rate( hSerRef serial_ref,
							UINT32 baud_rate );

MX_API INT32 MX_CLCALL mxi_soft_camera_link_set_cc_line( hSerRef serial_ref,
							UINT32 cc_line_number,
							UINT32 cc_line_state );
#endif /* IS_MX_DRIVER */

extern MX_RECORD_FUNCTION_LIST mxi_soft_camera_link_record_function_list;

extern long mxi_soft_camera_link_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_soft_camera_link_rfield_def_ptr;

#define MXI_SOFT_CAMERA_LINK_STANDARD_FIELDS

#endif /* __I_SOFT_CAMERA_LINK_H__ */

