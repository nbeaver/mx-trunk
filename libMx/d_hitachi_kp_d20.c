/*
 * Name:    d_hitachi_kp_d20.c
 *
 * Purpose: MX Pan/Tilt/Zoom driver for Hitachi KP-D20A/B cameras.
 *          VISCA protocol for communication.
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

#define MXD_HITACHI_KP_D20_DEBUG	TRUE

#include <stdio.h>

#include "mx_record.h"
#include "mx_ptz.h"
#include "d_hitachi_kp_d20.h"

MX_RECORD_FUNCTION_LIST mxd_hitachi_kp_d20_record_function_list = {
	NULL,
	mxd_hitachi_kp_d20_create_record_structures
};

MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_hitachi_kp_d20_ptz_function_list = {
	mxd_hitachi_kp_d20_command,
	mxd_hitachi_kp_d20_get_status,
	mxd_hitachi_kp_d20_get_parameter,
	mxd_hitachi_kp_d20_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_hitachi_kp_d20_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PAN_TILT_ZOOM_STANDARD_FIELDS,
	MXD_HITACHI_KP_D20_STANDARD_FIELDS
};

long mxd_hitachi_kp_d20_num_record_fields
	= sizeof( mxd_hitachi_kp_d20_rf_defaults )
		/ sizeof( mxd_hitachi_kp_d20_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_hitachi_kp_d20_rfield_def_ptr
			= &mxd_hitachi_kp_d20_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_hitachi_kp_d20_get_pointers( MX_PAN_TILT_ZOOM *ptz,
			MX_HITACHI_KP_D20 **hitachi_kp_d20,
			const char *calling_fname )
{
	static const char fname[] = "mxd_hitachi_kp_d20_get_pointers()";

	if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PAN_TILT_ZOOM pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( hitachi_kp_d20 ==  (MX_HITACHI_KP_D20 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HITACHI_KP_D20 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( ptz->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_PTZ %p is NULL.", ptz );
	}

	*hitachi_kp_d20 = (MX_HITACHI_KP_D20 *) ptz->record->record_type_struct;

	if ( (*hitachi_kp_d20) == (MX_HITACHI_KP_D20 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HITACHI_KP_D20 pointer for record '%s' is NULL.",
			ptz->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_hitachi_kp_d20_cmd( MX_HITACHI_KP_D20 *hitachi_kp_d20, char *command )
{
	static const char fname[] = "mxd_hitachi_kp_d20_cmd()";

	if ( hitachi_kp_d20 == (MX_HITACHI_KP_D20 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HITACHI_KP_D20 pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'command' pointer passed was NULL." );
	}

#if MXD_HITACHI_KP_D20_DEBUG
	MX_DEBUG(-2,("%s: sending command '%s' to Hitachi KP-D20 '%s'.",
		fname, command, hitachi_kp_d20->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_hitachi_kp_d20_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_hitachi_kp_d20_create_record_structures()";

        MX_PAN_TILT_ZOOM *ptz;
        MX_HITACHI_KP_D20 *hitachi_kp_d20;

        /* Allocate memory for the necessary structures. */

        ptz = (MX_PAN_TILT_ZOOM *) malloc(sizeof(MX_PAN_TILT_ZOOM));

        if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Unable to allocate memory for an MX_PAN_TILT_ZOOM structure." );
        }

        hitachi_kp_d20 = (MX_HITACHI_KP_D20 *)
				malloc( sizeof(MX_HITACHI_KP_D20) );

        if ( hitachi_kp_d20 == (MX_HITACHI_KP_D20 *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_HITACHI_KP_D20 structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = ptz;
        record->record_type_struct = hitachi_kp_d20;
        record->class_specific_function_list =
				&mxd_hitachi_kp_d20_ptz_function_list;

        ptz->record = record;
	hitachi_kp_d20->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_hitachi_kp_d20_command( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_hitachi_kp_d20_command()";

	MX_HITACHI_KP_D20 *hitachi_kp_d20;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_hitachi_kp_d20_get_pointers( ptz,
						&hitachi_kp_d20, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ptz->command ) {
	case MXF_PTZ_ZOOM_IN:
		strlcpy( command, "371000", sizeof(command) );
		break;
	case MXF_PTZ_ZOOM_OUT:
		strlcpy( command, "381000", sizeof(command) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The command %lu received for Hitachi PTZ '%s' "
			"is not a known command type.",
				ptz->command, hitachi_kp_d20->record->name );
		break;
	}

	/* Send the command. */

	mx_status = mxd_hitachi_kp_d20_cmd( hitachi_kp_d20, command );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_hitachi_kp_d20_get_status( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_hitachi_kp_d20_status()";

	MX_HITACHI_KP_D20 *hitachi_kp_d20;
	mx_status_type mx_status;

	mx_status = mxd_hitachi_kp_d20_get_pointers( ptz,
						&hitachi_kp_d20, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptz->status = 0;

	return mx_status;
}

