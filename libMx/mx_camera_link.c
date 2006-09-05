/*
 * Name:    mx_camera_link.c
 *
 * Purpose: MX function library of generic Camera Link operations.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef IS_MX_DRIVER
#   define IS_MX_DRIVER
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_camera_link.h"

#if HAVE_CAMERA_LINK

MX_EXPORT mx_status_type
mx_camera_link_get_pointers( MX_RECORD *cl_record,
			MX_CAMERA_LINK **camera_link,
			MX_CAMERA_LINK_API_LIST **api_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_camera_link_get_pointers()";

	if ( cl_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The cl_record pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( cl_record->mx_class != MXI_CAMERA_LINK ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' passed by '%s' is not a CAMERA_LINK interface.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			cl_record->name, calling_fname,
			cl_record->mx_superclass,
			cl_record->mx_class,
			cl_record->mx_type );
	}

	if ( camera_link != (MX_CAMERA_LINK **) NULL ) {
		*camera_link = (MX_CAMERA_LINK *)
				cl_record->record_class_struct;

		if ( *camera_link == (MX_CAMERA_LINK *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_CAMERA_LINK pointer for record '%s' passed by '%s' is NULL.",
				cl_record->name, calling_fname );
		}
	}

	if ( api_ptr != (MX_CAMERA_LINK_API_LIST **) NULL ) {
		*api_ptr = (MX_CAMERA_LINK_API_LIST *)
			cl_record->class_specific_function_list;

		if ( *api_ptr == (MX_CAMERA_LINK_API_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_CAMERA_LINK_API_LIST pointer for record '%s' passed by '%s' is NULL.",
				cl_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_camera_link_open( MX_RECORD *cl_record )
{
	static const char fname[] = "mx_camera_link_open()";

	MX_CAMERA_LINK *camera_link;
	mx_status_type mx_status;

	mx_status = mx_camera_link_get_pointers( cl_record,
						&camera_link, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the port. */

	mx_status = mx_camera_link_serial_init( cl_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the port speed. */

	mx_status = mx_camera_link_set_baud_rate( cl_record,
						camera_link->baud_rate );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_camera_link_close( MX_RECORD *cl_record )
{
	return mx_camera_link_serial_close( cl_record );
}

MX_EXPORT mx_status_type
mx_camera_link_flush_port( MX_RECORD *cl_record )
{
	static const char fname[] = "mx_camera_link_flush_port()";

	MX_CAMERA_LINK *camera_link;
	MX_CAMERA_LINK_API_LIST *api_ptr;
	INT32 ( *flush_port_fn ) ( hSerRef );
	mx_status_type mx_status;

	mx_status = mx_camera_link_get_pointers( cl_record,
						&camera_link, &api_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, cl_record->name));

	flush_port_fn = api_ptr->flush_port;

	if ( flush_port_fn != NULL ) {
		INT32 cl_status;

		/* Invoke the flush port function. */

		cl_status = (*flush_port_fn)( camera_link->serial_ref );

		switch( cl_status ) {
		case CL_ERR_NO_ERR:	/* Success. */
			break;

		case CL_ERR_INVALID_REFERENCE:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Serial reference %p requested for Camera Link "
			"record '%s' is invalid.",
			    camera_link->serial_ref, cl_record->name );
			break;
		default:
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unexpected error code %d returned for the attempt "
			"to flush input for Camera Link record '%s'.",
				cl_status, cl_record->name );
			break;
		}
	} else {
		size_t bytes_available, bytes_to_read;
		char buffer[1024];

		/* This driver does not natively support a flush operation,
		 * so we emulate it by reading the current contents of the
		 * receive buffer and throwing them away.
		 */

		mx_status = mx_camera_link_get_num_bytes_avail( cl_record,
							&bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		for (;;) {
			if ( bytes_available > sizeof(buffer) ) {
				bytes_to_read = sizeof(buffer);
			} else
			if ( bytes_available > 0 ) {
				bytes_to_read = bytes_available;
			} else {
				/* All bytes have been read, so break
				 * out of the for(;;) loop.
				 */

				break;
			}

			mx_status = mx_camera_link_serial_read( cl_record,
						buffer, &bytes_to_read, 1.0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( bytes_available > bytes_to_read ) {
				bytes_available -= bytes_to_read;
			} else {
				bytes_available = 0;
			}
		}
	}

	MX_DEBUG(-2,
	("%s: Camera Link '%s' has successfully flushed unread input.",
		fname, cl_record->name ));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_camera_link_get_num_bytes_avail( MX_RECORD *cl_record,
					size_t *num_bytes_avail )
{
	static const char fname[] = "mx_camera_link_get_num_bytes_avail()";

	MX_CAMERA_LINK *camera_link;
	MX_CAMERA_LINK_API_LIST *api_ptr;
	INT32 ( *get_num_bytes_avail_fn ) ( hSerRef, UINT32 * );
	INT32 cl_status;
	UINT32 bytes_available;
	mx_status_type mx_status;

	mx_status = mx_camera_link_get_pointers( cl_record,
						&camera_link, &api_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, cl_record->name));

	get_num_bytes_avail_fn = api_ptr->get_num_bytes_avail;

	if ( get_num_bytes_avail_fn == NULL ) {
		MX_DEBUG(-2,("%s: Not implemented for record '%s'.",
			fname, cl_record->name ));

		return MX_SUCCESSFUL_RESULT;
	}

	/* Invoke the get_num_bytes_avail function. */

	cl_status = (*get_num_bytes_avail_fn)( camera_link->serial_ref,
						&bytes_available );

	switch( cl_status ) {
	case CL_ERR_NO_ERR:	/* Success. */
		break;

	case CL_ERR_INVALID_REFERENCE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Serial reference %p requested for Camera Link record '%s' "
		"is invalid.", camera_link->serial_ref, cl_record->name );
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unexpected error code %d returned for the attempt "
		"to get the number of available input bytes for "
		"Camera Link record '%s'.",
			cl_status, cl_record->name );
		break;
	}

	*num_bytes_avail = bytes_available;

	MX_DEBUG(-2,("%s: Record '%s', num_bytes_avail = %ld",
		fname, cl_record->name, (long) *num_bytes_avail));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_camera_link_serial_close( MX_RECORD *cl_record )
{
	static const char fname[] = "mx_camera_link_serial_close()";

	MX_CAMERA_LINK *camera_link;
	MX_CAMERA_LINK_API_LIST *api_ptr;
	void ( *serial_close_fn ) ( hSerRef );
	mx_status_type mx_status;

	mx_status = mx_camera_link_get_pointers( cl_record,
						&camera_link, &api_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, cl_record->name));

	serial_close_fn = api_ptr->serial_close;

	if ( serial_close_fn == NULL ) {
		MX_DEBUG(-2,("%s: Not implemented for record '%s'.",
			fname, cl_record->name ));

		return MX_SUCCESSFUL_RESULT;
	}

	/* Invoke the serial close function. */

	(*serial_close_fn)( camera_link->serial_ref );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_camera_link_serial_init( MX_RECORD *cl_record )
{
	static const char fname[] = "mx_camera_link_serial_init()";

	MX_CAMERA_LINK *camera_link;
	MX_CAMERA_LINK_API_LIST *api_ptr;
	INT32 ( *serial_init_fn ) ( UINT32, hSerRef * );
	INT32 cl_status;
	mx_status_type mx_status;

	mx_status = mx_camera_link_get_pointers( cl_record,
						&camera_link, &api_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, cl_record->name));

	serial_init_fn = api_ptr->serial_init;

	if ( serial_init_fn == NULL ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The serial init function for Camera Link record '%s' "
		"is not defined.  This is probably a programmer error, "
		"since no Camera Link driver can operate without the "
		"serial init function.",  cl_record->name );
	}

	/* Invoke the serial init function. */

	cl_status = (*serial_init_fn)( camera_link->serial_index,
					&(camera_link->serial_ref) );

	switch( cl_status ) {
	case CL_ERR_NO_ERR:	/* Success. */
		break;

	case CL_ERR_PORT_IN_USE:
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"Camera Link record '%s' cannot connect to "
		"serial index %ld since it is already in use.",
			cl_record->name, camera_link->serial_index );
		break;
	case CL_ERR_INVALID_INDEX:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Serial index %ld requested for Camera Link record '%s' "
		"is invalid.", camera_link->serial_index, cl_record->name );
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unexpected error code %d returned for the attempt "
		"to initialize serial index %ld for Camera Link record '%s'.",
			cl_status, camera_link->serial_index,
			cl_record->name );
		break;
	}

	MX_DEBUG(-2,("%s: Camera Link record '%s' successfully opened.",
		fname, cl_record->name ));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_camera_link_serial_read( MX_RECORD *cl_record,
				char *buffer,
				size_t *buffer_size,
				double timeout_in_seconds )
{
	static const char fname[] = "mx_camera_link_serial_read()";

	MX_CAMERA_LINK *camera_link;
	MX_CAMERA_LINK_API_LIST *api_ptr;
	INT32 ( *serial_read_fn ) ( hSerRef, INT8 *, UINT32 *, UINT32 );
	INT32 cl_status;
	UINT32 timeout_in_ms;
	mx_status_type mx_status;

	mx_status = mx_camera_link_get_pointers( cl_record,
						&camera_link, &api_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, cl_record->name));

	serial_read_fn = api_ptr->serial_read;

	if ( serial_read_fn == NULL ) {
		MX_DEBUG(-2,("%s: Not implemented for record '%s'.",
			fname, cl_record->name ));

		return MX_SUCCESSFUL_RESULT;
	}

	/* Invoke the serial_read function. */

	timeout_in_ms = mx_round( 1000.0 * timeout_in_seconds );

	cl_status = (*serial_read_fn)( camera_link->serial_ref,
					buffer, buffer_size, timeout_in_ms );

	switch( cl_status ) {
	case CL_ERR_NO_ERR:	/* Success. */
		break;

	case CL_ERR_TIMEOUT:
		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %g seconds to read from "
		"Camera Link record '%s'.",
			timeout_in_seconds, cl_record->name );
		break;

	case CL_ERR_INVALID_REFERENCE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Serial reference %p requested for Camera Link record '%s' "
		"is invalid.", camera_link->serial_ref, cl_record->name );
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unexpected error code %d returned for the attempt "
		"to read from Camera Link record '%s'.",
			cl_status, cl_record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_camera_link_serial_write( MX_RECORD *cl_record,
				char *buffer,
				size_t *buffer_size,
				double timeout_in_seconds )
{
	static const char fname[] = "mx_camera_link_serial_write()";

	MX_CAMERA_LINK *camera_link;
	MX_CAMERA_LINK_API_LIST *api_ptr;
	INT32 ( *serial_write_fn ) ( hSerRef, INT8 *, UINT32 *, UINT32 );
	INT32 cl_status;
	UINT32 timeout_in_ms;
	mx_status_type mx_status;

	mx_status = mx_camera_link_get_pointers( cl_record,
						&camera_link, &api_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, cl_record->name));

	serial_write_fn = api_ptr->serial_write;

	if ( serial_write_fn == NULL ) {
		MX_DEBUG(-2,("%s: Not implemented for record '%s'.",
			fname, cl_record->name ));

		return MX_SUCCESSFUL_RESULT;
	}

	/* Invoke the serial_write function. */

	timeout_in_ms = mx_round( 1000.0 * timeout_in_seconds );

	cl_status = (*serial_write_fn)( camera_link->serial_ref,
					buffer, buffer_size, timeout_in_ms );

	switch( cl_status ) {
	case CL_ERR_NO_ERR:	/* Success. */
		break;

	case CL_ERR_TIMEOUT:
		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %g seconds to write to "
		"Camera Link record '%s'.",
			timeout_in_seconds, cl_record->name );
		break;

	case CL_ERR_INVALID_REFERENCE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Serial reference %p requested for Camera Link record '%s' "
		"is invalid.", camera_link->serial_ref, cl_record->name );
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unexpected error code %d returned for the attempt "
		"to write to Camera Link record '%s'.",
			cl_status, cl_record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_camera_link_set_baud_rate( MX_RECORD *cl_record, unsigned long baud_rate )
{
	static const char fname[] = "mx_camera_link_set_baud_rate()";

	MX_CAMERA_LINK *camera_link;
	MX_CAMERA_LINK_API_LIST *api_ptr;
	INT32 ( *set_baud_rate_fn ) ( hSerRef, UINT32 );
	INT32 cl_status;
	mx_status_type mx_status;

	mx_status = mx_camera_link_get_pointers( cl_record,
						&camera_link, &api_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, cl_record->name));

	set_baud_rate_fn = api_ptr->set_baud_rate;

	if ( set_baud_rate_fn == NULL ) {
		/* Skip this function. */

		MX_DEBUG(-2,("%s: Not implemented for record '%s'",
			fname, cl_record->name ));

		return MX_SUCCESSFUL_RESULT;
	}

	camera_link->baud_rate = baud_rate;

	cl_status = (*set_baud_rate_fn)( camera_link->serial_ref, baud_rate );

	switch( cl_status ) {
	case CL_ERR_NO_ERR:	/* Success. */
		break;

	case CL_ERR_INVALID_REFERENCE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Serial reference %p requested for Camera Link record '%s' "
		"is invalid.", camera_link->serial_ref, cl_record->name );
		break;
	case CL_ERR_BAUD_RATE_NOT_SUPPORTED:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Baud rate %lu is not supported for Camera Link record '%s'.",
			baud_rate, cl_record->name );
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unexpected error code %d returned for the attempt "
		"to initialize serial index %ld for Camera Link record '%s'.",
			cl_status, camera_link->serial_index,
			cl_record->name );
		break;
	}

	MX_DEBUG(-2,("%s: Camera Link record '%s' speed set to %lu.",
		fname, cl_record->name, camera_link->baud_rate ));

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_CAMERA_LINK */

