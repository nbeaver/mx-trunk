/*
 * Name:    i_epix_camera_link.h
 *
 * Purpose: Header for MX driver for EPIX Camera Link serial I/O.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_EPIX_CAMERA_LINK_H__
#define __I_EPIX_CAMERA_LINK_H__

#define MX_EPIX_CAMERA_LINK_RECEIVE_BUFFER_SIZE   1024

MX_API mx_status_type mxi_epix_camera_link_create_record_structures(
						MX_RECORD *record );

MX_API mx_status_type mxi_epix_camera_link_open( MX_RECORD *record );

MX_API mx_status_type mxi_epix_camera_link_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_epix_camera_link_record_function_list;

#if defined(IS_MX_DRIVER)

typedef struct {
	MX_RECORD *record;
	MX_CAMERA_LINK *camera_link;
	long unitmap;
} MX_EPIX_CAMERA_LINK;

typedef struct {
        long unit_number;
	int array_index;
	mx_bool_type in_use;
	MX_RECORD *record;
	MX_CAMERA_LINK *camera_link;
	MX_EPIX_CAMERA_LINK *epix_camera_link;
} MX_EPIX_CAMERA_LINK_PORT;

extern MX_CAMERA_LINK_API_LIST mxi_epix_camera_link_api_list;

MX_API INT32 MX_CLCALL mxi_epix_camera_link_get_num_bytes_avail(
							hSerRef serial_ref,
							UINT32 *num_bytes );

MX_API void  MX_CLCALL mxi_epix_camera_link_serial_close( hSerRef serial_ref );

MX_API INT32 MX_CLCALL mxi_epix_camera_link_serial_init( UINT32 serial_index,
						      hSerRef *serial_ref_ptr );

MX_API INT32 MX_CLCALL mxi_epix_camera_link_serial_read( hSerRef serial_ref,
							INT8 *buffer,
							UINT32 *num_bytes,
							UINT32 serial_timeout );

MX_API INT32 MX_CLCALL mxi_epix_camera_link_serial_write( hSerRef serial_ref,
							INT8 *buffer,
							UINT32 *num_bytes,
							UINT32 serial_timeout );

MX_API INT32 MX_CLCALL mxi_epix_camera_link_set_baud_rate( hSerRef serial_ref,
							UINT32 baud_rate );

MX_API mx_status_type mxi_epix_camera_link_set_cc_signal(
						MX_CAMERA_LINK *camera_link,
						unsigned long cc_signal_number,
						unsigned long cc_signal_value );

#endif /* IS_MX_DRIVER */

extern long mxi_epix_camera_link_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_epix_camera_link_rfield_def_ptr;

#define MXI_EPIX_CAMERA_LINK_STANDARD_FIELDS

#endif /* __I_EPIX_CAMERA_LINK_H__ */

