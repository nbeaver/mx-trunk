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
	mxd_sony_visca_ptz_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_sony_visca_ptz_open
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
mxd_sony_visca_ptz_open( MX_RECORD *record )
{
	mx_status_type mx_status;

	/* All we need do here is initialize the pan and tilt speeds to the
	 * maximum possible value.  For this driver, getting the pan speed
	 * has the side effect of initializing the tilt speed as well.
	 */

	mx_status = mx_ptz_get_pan_speed( record, NULL );

	return mx_status;
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
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x01\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_PAN_RIGHT:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x02\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_TILT_UP:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x03\x01\xff", sizeof( command ) );
		break;
	case MXF_PTZ_TILT_DOWN:
		mxi_sony_visca_copy( command,
				"\x01\x06\x01\x03\x02\xff", sizeof( command ) );
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
		mxi_sony_visca_copy( command,
				"\x01\x04\x06\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_ON:
		mxi_sony_visca_copy( command,
				"\x01\x04\x06\x02\xff", sizeof( command ) );
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
		strlcpy( command, "\x09\x06\x12\xff", sizeof(command) );
		break;
	case MXF_PTZ_ZOOM_POSITION:
		strlcpy( command, "\x09\x04\x47\xff", sizeof(command) );
		break;
	case MXF_PTZ_FOCUS_POSITION:
		strlcpy( command, "\x09\x04\x48\xff", sizeof(command) );
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

		strlcpy( command, "\x09\x06\x11\xff", sizeof(command) );
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

	MX_DEBUG(-2,
	("%s: nibble3 = %#lx, nibble2 = %#lx, nibble1 = %#lx, nibble0 = %#lx",
	 	fname, nibble3, nibble2, nibble1, nibble0));

	inquiry_value = (nibble3 << 12) + (nibble2 << 8)
				+ (nibble1 << 4) + nibble0;

#if MXD_SONY_VISCA_PTZ_DEBUG
	MX_DEBUG(-2,("%s: PTZ '%s' inquiry_value = %#lx",
		fname, ptz->record->name, inquiry_value));
#endif

	switch( ptz->parameter_type ) {
	case MXF_PTZ_PAN_POSITION:
	case MXF_PTZ_TILT_POSITION:
		ptz->pan_position = inquiry_value;

		nibble3 = response[6];
		nibble2 = response[7];
		nibble1 = response[8];
		nibble0 = response[9];

		MX_DEBUG(-2,
("%s: TILT - nibble3 = %#lx, nibble2 = %#lx, nibble1 = %#lx, nibble0 = %#lx",
	 	fname, nibble3, nibble2, nibble1, nibble0));

		ptz->tilt_position = (nibble3 << 12) + (nibble2 << 8)
				+ (nibble1 << 4) + nibble0;
#if MXD_SONY_VISCA_PTZ_DEBUG
	MX_DEBUG(-2,("%s: PTZ '%s' tilt = %#lx",
		fname, ptz->record->name, ptz->tilt_position));
#endif

		break;
	case MXF_PTZ_ZOOM_POSITION:
		ptz->zoom_position = inquiry_value;
		break;
	case MXF_PTZ_FOCUS_POSITION:
		ptz->focus_position = inquiry_value;
		break;
	case MXF_PTZ_PAN_SPEED:
	case MXF_PTZ_TILT_SPEED:
		ptz->pan_speed = response[2];
		ptz->tilt_speed = response[3];
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

		if ( ptz->parameter_type == MXF_PTZ_PAN_DESTINATION ) {
			pan_value = ptz->pan_destination;

			mx_status = mx_ptz_get_tilt( ptz->record, &tilt_value );
		} else {
			mx_status = mx_ptz_get_pan( ptz->record, &pan_value );

			tilt_value = ptz->tilt_destination;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		command[0] = 0x01;
		command[1] = 0x06;
		command[2] = 0x02;
		command[3] = ptz->pan_speed;
		command[4] = ptz->tilt_speed;
		command[5] = ( pan_value >> 12 ) & 0xf;
		command[6] = ( pan_value >> 8 ) & 0xf;
		command[7] = ( pan_value >> 4 ) & 0xf;
		command[8] = pan_value & 0xf;
		command[9] = ( tilt_value >> 12 ) & 0xf;
		command[10] = ( tilt_value >> 8 ) & 0xf;
		command[11] = ( tilt_value >> 4 ) & 0xf;
		command[12] = tilt_value & 0xf;
		command[13] = 0xff;
		command[14] = 0x0;

		mx_status = mxi_sony_visca_cmd( sony_visca,
						sony_visca_ptz->camera_number,
						command, NULL, 0, NULL );
		break;
	case MXF_PTZ_ZOOM_DESTINATION:
		if ( ptz->zoom_destination >= 0x400 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested zoom value of %lu is outside "
			"the allowed range (0-1023) for Hitachi PTZ '%s'",
				ptz->zoom_destination, ptz->record->name );
		}

		mx_status = mxi_sony_visca_value_command(
						command, sizeof(command),
						3, "\x01\x04\x47",
						ptz->zoom_destination );
		break;
	case MXF_PTZ_FOCUS_DESTINATION:
		mx_status = mxi_sony_visca_value_command(
						command, sizeof(command),
						3, "\x01\x04\x48",
						ptz->zoom_destination );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Parameter type %d received for Hitachi PTZ '%s' "
			"is not a known parameter type.",
			ptz->parameter_type, sony_visca_ptz->record->name );
		break;
	}

	return mx_status;
}

