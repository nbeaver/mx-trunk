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
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SONY_VISCA_PTZ_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_ptz.h"
#include "i_sony_visca.h"
#include "d_sony_visca_ptz.h"

MX_RECORD_FUNCTION_LIST mxd_sony_visca_ptz_record_function_list = {
	NULL,
	mxd_sony_visca_ptz_create_record_structures
};

MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_sony_visca_ptz_ptz_function_list = {
	mxd_sony_visca_ptz_command,
	mxd_sony_visca_ptz_get_status,
	mxd_sony_visca_ptz_get_parameter,
	mxd_sony_visca_ptz_set_parameter
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

static mx_status_type
mxd_sony_visca_ptz_pan_tilt_drive_string( MX_RECORD *ptz_record,
					unsigned char *destination,
					size_t max_command_length,
					unsigned char *suffix )
{
	static const char fname[] =
			"mxd_sony_visca_ptz_pan_tilt_drive_string()";

	unsigned long pan_speed, tilt_speed;
	mx_status_type mx_status;

	if ( ptz_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}
	if ( destination == (unsigned char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The destination pointer passed was NULL." );
	}
	if ( suffix == (unsigned char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The suffix pointer passed was NULL." );
	}
	if ( max_command_length < 9 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The command buffer of length %lu bytes is shorter "
		"than the minimum allowed length of 9 bytes.",
			(unsigned long) max_command_length );
	}

	mx_status = mx_ptz_get_pan_speed( ptz_record, &pan_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_ptz_get_tilt_speed( ptz_record, &tilt_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	destination[0] = 0x01;
	destination[1] = 0x06;
	destination[2] = 0x01;
	destination[3] = (unsigned char) pan_speed;
	destination[4] = (unsigned char) tilt_speed;
	destination[5] = suffix[0];
	destination[6] = suffix[1];
	destination[7] = 0xff;
	destination[8] = 0x0;

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
        } sony_visca_ptz = (MX_SONY_VISCA_PTZ *)
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

	ptz->pan_speed = 1;
	ptz->tilt_speed = 1;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sony_visca_ptz_command( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_sony_visca_ptz_command()";

	MX_SONY_VISCA_PTZ *sony_visca_ptz;
	MX_SONY_VISCA *sony_visca;
	unsigned char command[80];
	mx_status_type mx_status;

	mx_status = mxd_sony_visca_ptz_get_pointers( ptz,
					&sony_visca_ptz, &sony_visca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ptz->command ) {
	case MXF_PTZ_PAN_LEFT:
		mxd_sony_visca_ptz_pan_tilt_drive_string( ptz->record,
				command, sizeof(command), 
				(unsigned char *) "\x01\x03" );
		break;
	case MXF_PTZ_PAN_RIGHT:
		mxd_sony_visca_ptz_pan_tilt_drive_string( ptz->record,
				command, sizeof(command), 
				(unsigned char *) "\x02\x03" );
		break;
	case MXF_PTZ_TILT_UP:
		mxd_sony_visca_ptz_pan_tilt_drive_string( ptz->record,
				command, sizeof(command), 
				(unsigned char *) "\x03\x01" );
		break;
	case MXF_PTZ_TILT_DOWN:
		mxd_sony_visca_ptz_pan_tilt_drive_string( ptz->record,
				command, sizeof(command), 
				(unsigned char *) "\x03\x02" );
		break;
	case MXF_PTZ_DRIVE_UPPER_LEFT:
		mxd_sony_visca_ptz_pan_tilt_drive_string( ptz->record,
				command, sizeof(command), 
				(unsigned char *) "\x01\x01" );
		break;
	case MXF_PTZ_DRIVE_UPPER_RIGHT:
		mxd_sony_visca_ptz_pan_tilt_drive_string( ptz->record,
				command, sizeof(command), 
				(unsigned char *) "\x02\x01" );
		break;
	case MXF_PTZ_DRIVE_LOWER_LEFT:
		mxd_sony_visca_ptz_pan_tilt_drive_string( ptz->record,
				command, sizeof(command), 
				(unsigned char *) "\x01\x02" );
		break;
	case MXF_PTZ_DRIVE_LOWER_RIGHT:
		mxd_sony_visca_ptz_pan_tilt_drive_string( ptz->record,
				command, sizeof(command), 
				(unsigned char *) "\x02\x02" );
		break;
	case MXF_PTZ_PAN_STOP:
	case MXF_PTZ_TILT_STOP:
	case MXF_PTZ_DRIVE_STOP:
		mxd_sony_visca_ptz_pan_tilt_drive_string( ptz->record,
				command, sizeof(command),
				(unsigned char *) "\x03\x03" );
		break;
	case MXF_PTZ_DRIVE_HOME:
		mxi_sony_visca_copy( command,
				(unsigned char *) "\x01\x06\x04\xff",
				sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_IN:
		mxi_sony_visca_copy( command,
				(unsigned char *) "\x01\x04\x07\x02\xff",
				sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_OUT:
		mxi_sony_visca_copy( command,
				(unsigned char *) "\x01\x04\x07\x03\xff",
				sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_STOP:
		mxi_sony_visca_copy( command,
				(unsigned char *) "\x01\x04\x07\x00\xff",
				sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_OFF:
		mxi_sony_visca_copy( command,
				(unsigned char *) "\x01\x04\x06\x03\xff",
				sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_ON:
		mxi_sony_visca_copy( command,
				(unsigned char *) "\x01\x04\x06\x02\xff",
				sizeof( command ) );
		break;
	case MXF_PTZ_FOCUS_MANUAL:
		mxi_sony_visca_copy( command,
				(unsigned char *) "\x01\x04\x38\x03\xff",
				sizeof( command ) );
		break;
	case MXF_PTZ_FOCUS_AUTO:
		mxi_sony_visca_copy( command,
				(unsigned char *) "\x01\x04\x38\x02\xff",
				sizeof( command ) );
		break;
	case MXF_PTZ_FOCUS_STOP:
		mxi_sony_visca_copy( command,
				(unsigned char *) "\x01\x04\x08\x00\xff",
				sizeof( command ) );
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
				(unsigned char *) "\x01\x04\x08\x02\xff",
				sizeof( command ));
		break;

	case MXF_PTZ_FOCUS_NEAR:
		ptz->command = MXF_PTZ_FOCUS_MANUAL;

		mx_status = mxd_sony_visca_ptz_command( ptz );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Now send the focus far command. */

		mxi_sony_visca_copy( command,
				(unsigned char *) "\x01\x04\x08\x03\xff",
				sizeof( command ));
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
	unsigned char response[80];
	unsigned pq, rs;
	mx_status_type mx_status;

	mx_status = mxd_sony_visca_ptz_get_pointers( ptz,
					&sony_visca_ptz, &sony_visca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptz->status = 0;

	/* Get the Pan/Tilt status. */

	mx_status = mxi_sony_visca_cmd( sony_visca,
					sony_visca_ptz->camera_number,
					(unsigned char *) "\x09\x06\x10\xff",
					response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pq = response[2];
	rs = response[3];

	if ( ( pq & 0x30 ) == 0x30 ) {
		ptz->status |= MXSF_PTZ_CONTROLLER_DISABLED;
	}
	if ( ( pq & 0x30 ) == 0x20 ) {
		ptz->status |= MXSF_PTZ_HOME_SEARCH_SUCCEEDED;
	}
	if ( ( pq & 0x0c ) == 0x04 ) {
		ptz->status |= MXSF_PTZ_PAN_IS_BUSY;
		ptz->status |= MXSF_PTZ_TILT_IS_BUSY;
	}
	if ( ( pq & 0x0c ) == 0x0c ) {
		ptz->status |= MXSF_PTZ_PAN_DRIVE_FAULT;
		ptz->status |= MXSF_PTZ_TILT_DRIVE_FAULT;
	}
	if ( pq & 0x02 ) {
		ptz->status |= MXSF_PTZ_TILT_DRIVE_FAULT;
	}
	if ( pq & 0x01 ) {
		ptz->status |= MXSF_PTZ_TILT_FOLLOWING_ERROR;
	}
	if ( rs & 0x20 ) {
		ptz->status |= MXSF_PTZ_PAN_DRIVE_FAULT;
	}
	if ( rs & 0x10 ) {
		ptz->status |= MXSF_PTZ_PAN_FOLLOWING_ERROR;
	}
	if ( rs & 0x08 ) {
		ptz->status |= MXSF_PTZ_TILT_BOTTOM_LIMIT_HIT;
	}
	if ( rs & 0x04 ) {
		ptz->status |= MXSF_PTZ_TILT_TOP_LIMIT_HIT;
	}
	if ( rs & 0x02 ) {
		ptz->status |= MXSF_PTZ_PAN_RIGHT_LIMIT_HIT;
	}
	if ( rs & 0x01 ) {
		ptz->status |= MXSF_PTZ_PAN_LEFT_LIMIT_HIT;
	}

#if 0
	/* Get the lens control system status. */

	/* FIXME: The Sony EVI-D100 says that the following command
	 * is invalid despite the fact that it is listed in the manual.
	 */

	mx_status = mxi_sony_visca_cmd( sony_visca,
					sony_visca_ptz->camera_number,
					"\x09\x7e\x7e\x00\xff",
					response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_visca_ptz_get_parameter( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_sony_visca_ptz_get_parameter()";

	MX_SONY_VISCA_PTZ *sony_visca_ptz;
	MX_SONY_VISCA *sony_visca;
	unsigned char command[40];
	unsigned char response[40];
	unsigned long nibble3, nibble2, nibble1, nibble0, inquiry_value;
	mx_status_type mx_status;

	mx_status = mxd_sony_visca_ptz_get_pointers( ptz,
					&sony_visca_ptz, &sony_visca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_VISCA_PTZ_DEBUG
	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));
#endif

	switch( ptz->parameter_type ) {
	case MXF_PTZ_PAN_POSITION:
	case MXF_PTZ_TILT_POSITION:
		strlcpy( (char *) command, (char *) "\x09\x06\x12\xff",
				sizeof(command) );
		break;
	case MXF_PTZ_ZOOM_POSITION:
		strlcpy( (char *) command, (char *) "\x09\x04\x47\xff",
				sizeof(command) );
		break;
	case MXF_PTZ_FOCUS_POSITION:
		strlcpy( (char *) command, (char *) "\x09\x04\x48\xff",
				sizeof(command) );
		break;
	case MXF_PTZ_PAN_SPEED:
	case MXF_PTZ_TILT_SPEED:
		/* If neither the pan speed nor the tilt speed have been
		 * initialized, we initialize them to the maximum speed.
		 * Otherwise, we just preserve the speed values last set.
		 */

		if ( (ptz->pan_speed != 0) || (ptz->tilt_speed != 0) ) {
			return MX_SUCCESSFUL_RESULT;
		}

		strlcpy( (char *) command, (char *) "\x09\x06\x11\xff",
				sizeof(command) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The read data type %#lx requested for Hitachi PTZ '%s' "
		"is not a known command type.",
				ptz->command, sony_visca_ptz->record->name );
		break;
	}

	mx_status = mxi_sony_visca_cmd( sony_visca,
					sony_visca_ptz->camera_number,
				command, response, sizeof(response), NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	nibble3 = response[2];
	nibble2 = response[3];
	nibble1 = response[4];
	nibble0 = response[5];

	inquiry_value = (nibble3 << 12) + (nibble2 << 8)
				+ (nibble1 << 4) + nibble0;

	switch( ptz->parameter_type ) {
	case MXF_PTZ_PAN_POSITION:
	case MXF_PTZ_TILT_POSITION:
		ptz->pan_position = (int16_t) inquiry_value;

		nibble3 = response[6];
		nibble2 = response[7];
		nibble1 = response[8];
		nibble0 = response[9];

		inquiry_value = (nibble3 << 12) + (nibble2 << 8)
				+ (nibble1 << 4) + nibble0;

		ptz->tilt_position = (int16_t) inquiry_value;

#if MXD_SONY_VISCA_PTZ_DEBUG
		MX_DEBUG(-2,
	("%s: PTZ '%s' Pan position = %ld (%#lx), Tilt position = %ld (%#lx)",
			fname, ptz->record->name,
			ptz->pan_position, ptz->pan_position,
			ptz->tilt_position, ptz->tilt_position));
#endif
		break;
	case MXF_PTZ_ZOOM_POSITION:
		ptz->zoom_position = inquiry_value;

#if MXD_SONY_VISCA_PTZ_DEBUG
		MX_DEBUG(-2,("%s: PTZ '%s' Zoom position = %ld (%#lx)",
			fname, ptz->record->name,
			ptz->zoom_position, ptz->zoom_position));
#endif
		break;
	case MXF_PTZ_FOCUS_POSITION:
		ptz->focus_position = inquiry_value;

#if MXD_SONY_VISCA_PTZ_DEBUG
		MX_DEBUG(-2,("%s: PTZ '%s' Focus position = %ld (%#lx)",
			fname, ptz->record->name,
			ptz->focus_position, ptz->focus_position));
#endif
		break;
	case MXF_PTZ_PAN_SPEED:
	case MXF_PTZ_TILT_SPEED:
		ptz->pan_speed = response[2];
		ptz->tilt_speed = response[3];

#if MXD_SONY_VISCA_PTZ_DEBUG
		MX_DEBUG(-2,
	("%s: PTZ '%s' Pan speed = %ld (%#lx), Tilt speed = %ld (%#lx)",
			fname, ptz->record->name,
			ptz->pan_speed, ptz->pan_speed,
			ptz->tilt_speed, ptz->tilt_speed));
#endif
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sony_visca_ptz_set_parameter( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_sony_visca_ptz_set_parameter()";

	MX_SONY_VISCA_PTZ *sony_visca_ptz;
	MX_SONY_VISCA *sony_visca;
	unsigned char command[80];
	long pan_value, tilt_value;
	unsigned long pan_speed, tilt_speed;
	int saved_parameter_type;
	mx_status_type mx_status;

	mx_status = mxd_sony_visca_ptz_get_pointers( ptz,
					&sony_visca_ptz, &sony_visca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_VISCA_PTZ_DEBUG
	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));
#endif

	saved_parameter_type = ptz->parameter_type;

	switch( ptz->parameter_type ) {
	case MXF_PTZ_PAN_SPEED:
	case MXF_PTZ_TILT_SPEED:
		/* Need do nothing here except preserve the values for the
		 * next time that the moves are requested.
		 */
		break;
	case MXF_PTZ_PAN_DESTINATION:
	case MXF_PTZ_TILT_DESTINATION:
		mx_status = mx_ptz_get_pan_speed( ptz->record, &pan_speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_ptz_get_tilt_speed( ptz->record, &tilt_speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( saved_parameter_type == MXF_PTZ_PAN_DESTINATION ) {
			pan_value = ptz->pan_destination;

			mx_status = mx_ptz_get_tilt( ptz->record, &tilt_value );

#if MXD_SONY_VISCA_PTZ_DEBUG
			MX_DEBUG(-2,("%s: Moving pan to %ld (%#lx)",
				fname, pan_value, pan_value & 0xffff));
#endif
		} else {
			mx_status = mx_ptz_get_pan( ptz->record, &pan_value );

			tilt_value = ptz->tilt_destination;

#if MXD_SONY_VISCA_PTZ_DEBUG
			MX_DEBUG(-2,("%s: Moving tilt to %ld (%#lx)",
				fname, tilt_value, tilt_value & 0xffff));
#endif
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		command[0] = 0x01;
		command[1] = 0x06;
		command[2] = 0x02;
		command[3] = (unsigned char) ptz->pan_speed;
		command[4] = (unsigned char) ptz->tilt_speed;
		command[5] = (unsigned char) (( pan_value >> 12 ) & 0xf);
		command[6] = (unsigned char) (( pan_value >> 8 ) & 0xf);
		command[7] = (unsigned char) (( pan_value >> 4 ) & 0xf);
		command[8] = (unsigned char) (pan_value & 0xf);
		command[9] = (unsigned char) (( tilt_value >> 12 ) & 0xf);
		command[10] = (unsigned char) (( tilt_value >> 8 ) & 0xf);
		command[11] = (unsigned char) (( tilt_value >> 4 ) & 0xf);
		command[12] = (unsigned char) (tilt_value & 0xf);
		command[13] = 0xff;
		command[14] = 0x0;

		mx_status = mxi_sony_visca_cmd( sony_visca,
						sony_visca_ptz->camera_number,
						command, NULL, 0, NULL );
		break;
	case MXF_PTZ_ZOOM_DESTINATION:

#if MXD_SONY_VISCA_PTZ_DEBUG
		MX_DEBUG(-2,("%s: Moving zoom to %ld (%#lx)",
			fname, ptz->zoom_destination,
			ptz->zoom_destination));
#endif
		mx_status = mxi_sony_visca_value_string(
					command, sizeof(command), 3,
					(unsigned char *) "\x01\x04\x47",
					ptz->zoom_destination );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxi_sony_visca_cmd( sony_visca,
						sony_visca_ptz->camera_number,
						command, NULL, 0, NULL );
		break;
	case MXF_PTZ_FOCUS_DESTINATION:

#if MXD_SONY_VISCA_PTZ_DEBUG
		MX_DEBUG(-2,("%s: Moving focus to %ld (%#lx)",
			fname, ptz->focus_destination,
			ptz->focus_destination));
#endif
		mx_status = mxi_sony_visca_value_string(
					command, sizeof(command), 3,
					(unsigned char *) "\x01\x04\x48",
					ptz->focus_destination );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxi_sony_visca_cmd( sony_visca,
						sony_visca_ptz->camera_number,
						command, NULL, 0, NULL );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Parameter type %d received for Hitachi PTZ '%s' "
			"is not a known parameter type.",
			saved_parameter_type, sony_visca_ptz->record->name );
		break;
	}

	return mx_status;
}

