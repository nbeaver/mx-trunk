/*
 * Name:    mx_camera_link.c
 *
 * Purpose: MX function library of generic Camera Link operations.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_camera_link.h"

MX_RECORD_FIELD_DEFAULTS mytest[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_CAMERA_LINK_STANDARD_FIELDS
};

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

