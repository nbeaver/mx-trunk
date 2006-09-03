/*
 * Name:    mx_camera_link.h
 *
 * Purpose: Header file for Camera Link serial interfaces.
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

#ifndef __MX_CAMERA_LINK_H__
#define __MX_CAMERA_LINK_H__

#include "mxconfig.h"

/* Here we decide whether or not Camera Link is available on this platform. */

#if HAVE_EPIX_XCLIB
#  define HAVE_CAMERA_LINK	TRUE
#else
#  define HAVE_CAMERA_LINK	FALSE
#endif

#if HAVE_CAMERA_LINK

/* Data types for Camera Link functions. */

#define hSerRef		void *
#define INT32		int32_t
#define UINT32		uint32_t
#define INT8		char

/* Status codes for Camera Link. */

#define CL_ERR_NO_ERR			0
#define CL_ERR_BUFFER_TOO_SMALL		(-10001)
#define CL_ERR_MANU_DOES_NOT_EXIST	(-10002)
#define CL_ERR_PORT_IN_USE		(-10003)
#define CL_ERR_TIMEOUT			(-10004)
#define CL_ERR_INVALID_INDEX		(-10005)
#define CL_ERR_INVALID_REFERENCE	(-10006)
#define CL_ERR_ERROR_NOT_FOUND		(-10007)
#define CL_ERR_BAUD_RATE_NOT_SUPPORTED	(-10008)
#define CL_ERR_OUT_OF_MEMORY		(-10009)

#define CL_ERR_UNABLE_TO_LOAD_DLL	(-10098)
#define CL_ERR_FUNCTION_NOT_FOUND	(-10099)

/*------------------------------------------------------------------------*/

typedef struct {
	MX_RECORD *record;

	hSerRef serial_ref;

	long baud_rate;
	double timeout;
	unsigned long serial_index;
} MX_CAMERA_LINK;

#define MX_CAMERA_LINK_STANDARD_FIELDS \
  {-1, -1, "baud_rate", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CAMERA_LINK, baud_rate), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "timeout", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CAMERA_LINK, timeout), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "serial_index", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_CAMERA_LINK, serial_index), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/*
 * Camera Link has a standardized API, but not all functions are supported
 * by all Camera Link implementations.  By invoking the functions through
 * a jump table and setting entries for non-existant functions to NULL, we
 * can avoid the problem of unsatisfied function references at link time.
 *
 * The following MX_CAMERA_LINK_API_LIST has a similar purpose to the
 * various MX_..._FUNCTION_LISTs that you will find for other MX interface
 * and device classes.  However, I call this an API_LIST since it uses the
 * native API style of the Camera Link standard rather than that of MX.
 */

typedef struct {
	INT32 ( *flush_port ) ( hSerRef serialRef );
	INT32 ( *get_error_text ) ( const INT8 *manu_name,
					INT32 error_code,
					INT8 *error_text,
					UINT32 *error_text_size );
	INT32 ( *get_manufacturer_info ) ( INT8 *manufacturer_name,
					UINT32 *buffer_size,
					UINT32 *version );
	INT32 ( *get_num_bytes_avail ) ( hSerRef serial_ref, UINT32 *num_bytes);
	INT32 ( *get_num_ports ) ( UINT32 *num_ports );
	INT32 ( *get_num_serial_ports ) ( UINT32 *num_serial_ports );
	INT32 ( *get_port_info ) ( UINT32 serial_index,
					INT8 *manufacturer_name,
					UINT32 *name_bytes,
					INT8 *port_id,
					UINT32 *id_bytes,
					UINT32 *version );
	INT32 ( *get_serial_port_identifier ) ( UINT32 serial_index,
						INT8 *port_id,
						UINT32 *buffer_size );
	INT32 ( *get_supported_baud_rates ) ( hSerRef serial_ref,
						UINT32 *baud_rates );
	void ( *serial_close ) ( hSerRef serial_ref );
	INT32 ( *serial_init ) ( UINT32 serial_index, hSerRef *serial_ref_ptr );
	INT32 ( *serial_read ) ( hSerRef serial_ref,
					INT8 *buffer,
					UINT32 *buffer_size,
					UINT32 serial_timeout );
	INT32 ( *serial_write ) ( hSerRef serial_ref,
					INT8 *buffer,
					UINT32 *buffer_size,
					UINT32 serial_timeout );
	INT32 ( *set_baud_rate ) ( hSerRef serial_ref, UINT32 baud_rate );

	/* The set_cc_signal() function is used to set the value of one
	 * of the CC1, CC2, CC3, or CC4 signals.  This function has no
	 * equivalent in the standard Camera Link specification, which
	 * I find surprising.
	 */

	mx_status_type ( *set_cc_signal ) ( MX_RECORD *cl_record,
					unsigned long cc_signal_number,
					unsigned long cc_signal_value );
} MX_CAMERA_LINK_API_LIST;

#endif /* HAVE_CAMERA_LINK */

/* ============== Interface function prototypes. ============== */

/* NOTE: The following prototypes use MX style function signatures. */

MX_API mx_status_type mx_camera_link_get_pointers( MX_RECORD *cl_record,
		                        MX_CAMERA_LINK **camera_link,
		                        MX_CAMERA_LINK_API_LIST **api_ptr,
		                        const char *calling_fname );

MX_API mx_status_type mx_camera_link_open( MX_RECORD *cl_record );

MX_API mx_status_type mx_camera_link_close( MX_RECORD *cl_record );

MX_API mx_status_type mx_camera_link_flush_port( MX_RECORD *cl_record );

MX_API mx_status_type mx_camera_link_get_error_text(
					const char *manufacturer_name,
					int error_code, char *error_text,
					size_t *error_text_size );

MX_API mx_status_type mx_camera_link_get_manufacturer_info(
						char *manufacturer_name,
						size_t *buffer_size,
						unsigned long *version );

MX_API mx_status_type mx_camera_link_get_num_bytes_avail( MX_RECORD *cl_record,
						unsigned long *num_bytes );

MX_API mx_status_type mx_camera_link_get_num_ports( unsigned long *num_ports );

MX_API mx_status_type mx_camera_link_get_num_serial_ports(
					unsigned long *num_serial_ports );

MX_API mx_status_type mx_camera_link_get_port_info( unsigned long serial_index,
						char *manufacturer_name,
						size_t *name_bytes,
						char *port_id,
						size_t *id_bytes,
						unsigned long *version );

MX_API mx_status_type mx_camera_link_get_serial_port_identifier(
						unsigned long serial_index,
						char *port_id,
						size_t *buffer_size );

MX_API mx_status_type mx_camera_link_get_supported_baud_rates(
						MX_RECORD *cl_record,
						unsigned long *baud_rates );

MX_API mx_status_type mx_camera_link_serial_read( MX_RECORD *cl_record,
						char *buffer,
						size_t *buffer_size,
						double timeout_in_seconds );

MX_API mx_status_type mx_camera_link_serial_write( MX_RECORD *cl_record,
						char *buffer,
						size_t *buffer_size,
						double timeout_in_seconds );

MX_API mx_status_type mx_camera_link_set_baud_rate( MX_RECORD *cl_record,
						unsigned long baud_rate );

MX_API mx_status_type mx_camera_link_set_cc_signal( MX_RECORD *cl_record,
						unsigned long cc_signal_number,
						unsigned long cc_signal_value );

#endif /* __MX_CAMERA_LINK_H__ */

