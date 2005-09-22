/*
 * Name:    d_soft_ptz.c
 *
 * Purpose: MX sofware-emulated Pan/Tilt/Zoom driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_PTZ_DEBUG	TRUE

#include <stdio.h>

#include "mx_record.h"
#include "mx_ptz.h"
#include "d_soft_ptz.h"

MX_RECORD_FUNCTION_LIST mxd_soft_ptz_record_function_list = {
	NULL,
	mxd_soft_ptz_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_soft_ptz_open
};

MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_soft_ptz_ptz_function_list = {
	mxd_soft_ptz_command,
	mxd_soft_ptz_get_status,
	mxd_soft_ptz_get_parameter,
	mxd_soft_ptz_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_soft_ptz_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PAN_TILT_ZOOM_STANDARD_FIELDS,
	MXD_SOFT_PTZ_STANDARD_FIELDS
};

long mxd_soft_ptz_num_record_fields
	= sizeof( mxd_soft_ptz_rf_defaults )
		/ sizeof( mxd_soft_ptz_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_ptz_rfield_def_ptr
			= &mxd_soft_ptz_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_soft_ptz_get_pointers( MX_PAN_TILT_ZOOM *ptz,
			MX_SOFT_PTZ **soft_ptz,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_ptz_get_pointers()";

	if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PAN_TILT_ZOOM pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( soft_ptz ==  (MX_SOFT_PTZ **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOFT_PTZ pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( ptz->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_PTZ %p is NULL.", ptz );
	}

	*soft_ptz = (MX_SOFT_PTZ *) ptz->record->record_type_struct;

	if ( (*soft_ptz) == (MX_SOFT_PTZ *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SOFT_PTZ pointer for record '%s' is NULL.",
			ptz->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_soft_ptz_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_soft_ptz_create_record_structures()";

        MX_PAN_TILT_ZOOM *ptz;
        MX_SOFT_PTZ *soft_ptz;

        /* Allocate memory for the necessary structures. */

        ptz = (MX_PAN_TILT_ZOOM *) malloc(sizeof(MX_PAN_TILT_ZOOM));

        if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Unable to allocate memory for an MX_PAN_TILT_ZOOM structure." );
        }

        soft_ptz = (MX_SOFT_PTZ *)
				malloc( sizeof(MX_SOFT_PTZ) );

        if ( soft_ptz == (MX_SOFT_PTZ *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_SOFT_PTZ structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = ptz;
        record->record_type_struct = soft_ptz;
        record->class_specific_function_list =
				&mxd_soft_ptz_ptz_function_list;

        ptz->record = record;
	soft_ptz->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_ptz_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_ptz_open()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_SOFT_PTZ *soft_ptz;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ptz = (MX_PAN_TILT_ZOOM *) record->record_class_struct;

	mx_status = mxd_soft_ptz_get_pointers( ptz, &soft_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for PTZ '%s'.", fname, record->name));

	return MX_SUCCESSFUL_RESULT;;
}

MX_EXPORT mx_status_type
mxd_soft_ptz_command( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_soft_ptz_command()";

	MX_SOFT_PTZ *soft_ptz;
	mx_status_type mx_status;

	mx_status = mxd_soft_ptz_get_pointers( ptz, &soft_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));

	switch( ptz->command ) {
	case MXF_PTZ_ZOOM_IN:
		mx_info("PTZ '%s': zoom in.", ptz->record->name);
		break;
	case MXF_PTZ_ZOOM_OUT:
		mx_info("PTZ '%s': zoom out", ptz->record->name);
		break;
	case MXF_PTZ_ZOOM_STOP:
		mx_info("PTZ '%s': zoom stop", ptz->record->name);
		break;
	case MXF_PTZ_ZOOM_OFF:
		mx_info("PTZ '%s': zoom off", ptz->record->name);
		break;
	case MXF_PTZ_ZOOM_ON:
		mx_info("PTZ '%s': zoom on", ptz->record->name);
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The command %#lx received for soft PTZ '%s' "
			"is not a known command type.",
				ptz->command, soft_ptz->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;;
}

MX_EXPORT mx_status_type
mxd_soft_ptz_get_status( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_soft_ptz_get_status()";

	MX_SOFT_PTZ *soft_ptz;
	mx_status_type mx_status;

	mx_status = mxd_soft_ptz_get_pointers( ptz, &soft_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: invoked for PTZ '%s'", fname, ptz->record->name));

	ptz->status = 0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_ptz_get_parameter( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_soft_ptz_get_parameter()";

	MX_SOFT_PTZ *soft_ptz;
	mx_status_type mx_status;

	mx_status = mxd_soft_ptz_get_pointers( ptz, &soft_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));

	switch( ptz->parameter_type ) {
	case MXF_PTZ_ZOOM_POSITION:
		mx_info("PTZ '%s': zoom is at %lu",
			ptz->record->name, ptz->zoom_position);

		ptz->zoom_position = ptz->zoom_destination;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Parameter type %d received for soft PTZ '%s' "
			"is not a known parameter type.",
			ptz->parameter_type, soft_ptz->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_ptz_set_parameter( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_soft_ptz_set_parameter()";

	MX_SOFT_PTZ *soft_ptz;
	mx_status_type mx_status;

	mx_status = mxd_soft_ptz_get_pointers( ptz, &soft_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));

	switch( ptz->parameter_type ) {
	case MXF_PTZ_ZOOM_DESTINATION:
		mx_info("PTZ '%s': zoom to %lu",
			ptz->record->name, ptz->zoom_destination);

		ptz->zoom_position = ptz->zoom_destination;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Parameter type %d received for soft PTZ '%s' "
			"is not a known parameter type.",
			ptz->parameter_type, soft_ptz->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

