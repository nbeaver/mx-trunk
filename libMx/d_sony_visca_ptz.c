/*
 * Name:    d_sony_visca_ptz.c
 *
 * Purpose: MX driver for Sony Pan/Tilt/Zoom camera supports that use the
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

#define MXD_SONY_VISCA_PTZ_DEBUG	TRUE

#include <stdio.h>

#include "mx_record.h"
#include "mx_ptz.h"
#include "i_sony_visca.h"
#include "d_sony_visca_ptz.h"

MX_RECORD_FUNCTION_LIST mxd_sony_visca_ptz_record_function_list = {
	NULL,
	mxd_sony_visca_ptz_create_record_structures
};

MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_sony_visca_ptz_ptz_function_list = {
	mxd_sony_visca_ptz_command,
	mxd_sony_visca_ptz_get_status
};

MX_RECORD_FIELD_DEFAULTS mxd_sony_visca_ptz_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PAN_TILT_ZOOM_STANDARD_FIELDS,
	MXD_SONY_VISCA_PTZ_STANDARD_FIELDS
};

long mxd_sony_visca_ptz_num_record_fields
	= sizeof( mxd_sony_visca_ptz_rf_defaults )
		/ sizeof( mxd_sony_visca_ptz_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sony_visca_ptz_rfield_def_ptr
			= &mxd_sony_visca_ptz_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_sony_visca_ptz_get_pointers( MX_PAN_TILT_ZOOM *ptz,
			MX_SONY_VISCA_PTZ **sony_visca_ptz,
			MX_SONY_VISCA **sony_visca,
			const char *calling_fname )
{
	static const char fname[] = "mxd_sony_visca_ptz_get_pointers()";

	MX_SONY_VISCA_PTZ *sony_visca_ptz_ptr;
	MX_RECORD *visca_record;

	if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PAN_TILT_ZOOM pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( ptz->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_PTZ %p is NULL.", ptz );
	}

	sony_visca_ptz_ptr = (MX_SONY_VISCA_PTZ *)
					ptz->record->record_type_struct;

	if ( sony_visca_ptz_ptr == (MX_SONY_VISCA_PTZ *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SONY_VISCA_PTZ pointer for record '%s' is NULL.",
			ptz->record->name );
	}

	if ( sony_visca_ptz != (MX_SONY_VISCA_PTZ **) NULL ) {
		*sony_visca_ptz = sony_visca_ptz_ptr;
	}

	visca_record = sony_visca_ptz_ptr->visca_record;

	if ( visca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'visca_record' pointer for Sony PTZ '%s' is NULL.",
			ptz->record->name );
	}

	if ( sony_visca != (MX_SONY_VISCA **) NULL ) {
		*sony_visca =
			(MX_SONY_VISCA *) visca_record->record_type_struct;

		if ( *sony_visca == (MX_SONY_VISCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SONY_VISCA pointer for VISCA record '%s' "
			"used by Sony PTZ '%s' is NULL.",
				visca_record->name,
				ptz->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_sony_visca_ptz_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_sony_visca_ptz_create_record_structures()";

        MX_PAN_TILT_ZOOM *ptz;
        MX_SONY_VISCA_PTZ *sony_visca_ptz;

        /* Allocate memory for the necessary structures. */

        ptz = (MX_PAN_TILT_ZOOM *) malloc(sizeof(MX_PAN_TILT_ZOOM));

        if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Unable to allocate memory for an MX_PAN_TILT_ZOOM structure." );
        }

        sony_visca_ptz = (MX_SONY_VISCA_PTZ *)
				malloc( sizeof(MX_SONY_VISCA_PTZ) );

        if ( sony_visca_ptz == (MX_SONY_VISCA_PTZ *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_SONY_VISCA_PTZ structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = ptz;
        record->record_type_struct = sony_visca_ptz;
        record->class_specific_function_list =
				&mxd_sony_visca_ptz_ptz_function_list;

        ptz->record = record;
	sony_visca_ptz->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sony_visca_ptz_command( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_sony_visca_ptz_command()";

	MX_SONY_VISCA_PTZ *sony_visca_ptz;
	MX_SONY_VISCA *sony_visca;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_sony_visca_ptz_get_pointers( ptz,
					&sony_visca_ptz, &sony_visca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ptz->command ) {
	case MXF_PTZ_DRIVE_UP:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x03\x01\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_DOWN:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x03\x02\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_LEFT:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x01\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_RIGHT:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x02\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_UPPER_LEFT:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x01\x01\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_UPPER_RIGHT:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x02\x01\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_LOWER_LEFT:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x01\x02\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_LOWER_RIGHT:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x02\x02\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_STOP:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x03\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_HOME:
		mxi_sony_visca_copy( command,
				"\x01\x06\x04\xff", sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_IN:
		mxi_sony_visca_copy( command,
				"\x01\x04\x07\x02\xff", sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_OUT:
		mxi_sony_visca_copy( command,
				"\x01\x04\x07\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_STOP:
		mxi_sony_visca_copy( command,
				"\x01\x04\x07\x00\xff", sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_OFF:
	case MXF_PTZ_ZOOM_ON:
		break;
	case MXF_PTZ_FOCUS_MANUAL:
		mxi_sony_visca_copy( command,
				"\x01\x04\x38\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_FOCUS_AUTO:
		mxi_sony_visca_copy( command,
				"\x01\x04\x38\x02\xff", sizeof( command ) );
		break;
	case MXF_PTZ_FOCUS_STOP:
		mxi_sony_visca_copy( command,
				"\x01\x04\x08\x00\xff", sizeof( command ) );
		break;
	
		/* For the next two cases, we have to switch to manual mode
		 * before sending the command.
		 */

	case MXF_PTZ_FOCUS_FAR:
		ptz->command = MXF_PTZ_FOCUS_MANUAL;

		mx_status = mxd_sony_visca_ptz_command( ptz );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Now send the focus far command. */

		mxi_sony_visca_copy( command,
				"\x01\x04\x08\x02\xff", sizeof( command ));
		break;

	case MXF_PTZ_FOCUS_NEAR:
		ptz->command = MXF_PTZ_FOCUS_MANUAL;

		mx_status = mxd_sony_visca_ptz_command( ptz );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Now send the focus far command. */

		mxi_sony_visca_copy( command,
				"\x01\x04\x08\x03\xff", sizeof( command ));
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The command %lu received for Sony PTZ '%s' "
			"is not a known command type.",
				ptz->command, sony_visca_ptz->record->name );
		break;
	}

	/* Send the command. */

	mx_status = mxi_sony_visca_cmd( sony_visca,
					sony_visca_ptz->camera_number,
					command, NULL, 0, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_visca_ptz_get_status( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_sony_visca_ptz_get_status()";

	MX_SONY_VISCA_PTZ *sony_visca_ptz;
	MX_SONY_VISCA *sony_visca;
	mx_status_type mx_status;

	mx_status = mxd_sony_visca_ptz_get_pointers( ptz,
					&sony_visca_ptz, &sony_visca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptz->status = 0;

	return mx_status;
}

