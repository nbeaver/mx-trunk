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
#include "mx_types.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "mx_ptz.h"
#include "d_hitachi_kp_d20.h"

MX_RECORD_FUNCTION_LIST mxd_hitachi_kp_d20_record_function_list = {
	NULL,
	mxd_hitachi_kp_d20_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_hitachi_kp_d20_open
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

#define MXD_HITACHI_KP_D20_COMMAND_TEXT_LENGTH	14

#define MXD_HITACHI_KP_D20_COMMAND_LENGTH \
		( MXD_HITACHI_KP_D20_COMMAND_TEXT_LENGTH + 4 )

static mx_status_type
mxd_hitachi_kp_d20_cmd( MX_HITACHI_KP_D20 *hitachi_kp_d20, char *command )
{
	static const char fname[] = "mxd_hitachi_kp_d20_cmd()";

	MX_RECORD *rs232_record;
	unsigned long i, checksum_data_length, wait_ms, max_attempts;
	mx_uint16_type checksum, xor_8bit_checksum;
	unsigned char low_nibble, high_nibble;
	unsigned char low_nibble_ascii, high_nibble_ascii;
	char *checksum_address;
	char c;
	unsigned char local_command[40];
	mx_status_type mx_status;

	if ( hitachi_kp_d20 == (MX_HITACHI_KP_D20 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HITACHI_KP_D20 pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'command' pointer passed was NULL." );
	}

	rs232_record = hitachi_kp_d20->rs232_record;

#if MXD_HITACHI_KP_D20_DEBUG
	MX_DEBUG(-2,("%s: sending command '%s' to Hitachi KP-D20 '%s'.",
		fname, command, hitachi_kp_d20->record->name ));
#endif

	/* Format the string to be sent to the controller. */

	snprintf( local_command, sizeof(local_command),
		"%c00FF01%s%c", MX_STX, command, MX_ETX );

	/* Compute the command checksum. */

	checksum_data_length = MXD_HITACHI_KP_D20_COMMAND_TEXT_LENGTH + 2;

	checksum = 0;

	for ( i = 0; i < checksum_data_length; i++ ) {
		checksum += local_command[i];
		MX_DEBUG(-2,("%s: checksum = %#x", fname, checksum));
	}

	xor_8bit_checksum = checksum ^ 0xff;

	xor_8bit_checksum &= 0xff;

	MX_DEBUG(-2,("%s: xor_8bit_checksum = %#x", fname, xor_8bit_checksum));

	checksum_address = local_command
				+ MXD_HITACHI_KP_D20_COMMAND_TEXT_LENGTH + 2;

	low_nibble = xor_8bit_checksum & 0xf;

	high_nibble = (xor_8bit_checksum >> 4) & 0xf;

	MX_DEBUG(-2,("%s: high_nibble = %x, low_nibble = %x",
			fname, high_nibble, low_nibble));

	if ( low_nibble < 10 ) {
		low_nibble_ascii = 0x30 + low_nibble;
	} else {
		low_nibble_ascii = 0x37 + low_nibble;
	}

	if ( high_nibble < 10 ) {
		high_nibble_ascii = 0x30 + high_nibble;
	} else {
		high_nibble_ascii = 0x37 + high_nibble;
	}

	MX_DEBUG(-2,("%s: high_nibble_ascii = %c, low_nibble_ascii = %c",
			fname, high_nibble_ascii, low_nibble_ascii));

	checksum_address[0] = high_nibble_ascii;
	checksum_address[1] = low_nibble_ascii;
	checksum_address[2] = '\0';

#if MXD_HITACHI_KP_D20_DEBUG
	fprintf( stderr, "%s: Bytes to send = ", fname );

	for ( i = 0; i < MXD_HITACHI_KP_D20_COMMAND_LENGTH; i++ ) {
		fprintf( stderr, "%#x ", local_command[i] );
	}

	fprintf( stderr, "\n" );
#endif

	wait_ms = 100;
	max_attempts = 4;

	/* Attempt to handshake with the Hitachi camera. */

	for ( i = 0; i < max_attempts; i++ ) {

		/* Send an ENQ byte. */

		mx_status = mx_rs232_putchar( rs232_record, MX_ENQ,
					MXD_HITACHI_KP_D20_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Wait up to 6 seconds for an ACK byte back from the camera. */

		mx_status = mx_rs232_getchar_with_timeout( rs232_record, &c,
					MXD_HITACHI_KP_D20_DEBUG, 6.0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( c == MX_ACK ) {
			/* The camera says that it successfully 
			 * received the ENQ byte, so jump out
			 * of this for() loop.
			 */

			MX_DEBUG(-2,("%s: ACK seen for ENQ.", fname));

			break;
		} else
		if ( c == MX_NAK ) {
			/* The camera says that it did not successfully
			 * receive the ENQ byte, so go back and try again.
			 */

			mx_warning( "Received a NAK byte from "
					"Hitachi camera '%s'.",
					hitachi_kp_d20->record->name );
		} else {
			mx_warning( "Received an unexpected byte %#x from "
					"Hitachi camera '%s'.",
					c, hitachi_kp_d20->record->name );

			/* Got a bad byte from the camera.
			 * Go back and try again.
			 */
		}

		mx_msleep(1000);
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out waiting for an ACK character from "
		"Hitachi camera '%s'.", hitachi_kp_d20->record->name );
	}

	/* Attempt to send the command to the Hitachi camera. */

	for ( i = 0; i < max_attempts; i++ ) {

		/* Send the command. */

		mx_status = mx_rs232_write( rs232_record, local_command,
					MXD_HITACHI_KP_D20_COMMAND_LENGTH,
					NULL, MXD_HITACHI_KP_D20_DEBUG );
						

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Wait up to 6 seconds for an ACK byte back from the camera. */

		mx_status = mx_rs232_getchar_with_timeout( rs232_record, &c,
					MXD_HITACHI_KP_D20_DEBUG, 6.0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( c == MX_ACK ) {
			/* The camera says that it successfully 
			 * received the command, so jump out
			 * of this for() loop.
			 */

			MX_DEBUG(-2,("%s: ACK seen for command.", fname));

			break;
		} else
		if ( c == MX_NAK ) {
			/* The camera says that it did not successfully
			 * receive the command, so go back and try again.
			 */

			mx_warning( "Received a NAK byte from "
					"Hitachi camera '%s'.",
					hitachi_kp_d20->record->name );
		} else {
			mx_warning( "Received an unexpected byte %#x from "
					"Hitachi camera '%s'.",
					c, hitachi_kp_d20->record->name );

			/* Got a bad byte from the camera.
			 * Go back and try again.
			 */
		}

		mx_msleep(1000);
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out waiting for an ACK character from "
		"Hitachi camera '%s'.", hitachi_kp_d20->record->name );
	}

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
mxd_hitachi_kp_d20_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_hitach_kp_d20_open()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_HITACHI_KP_D20 *hitachi_kp_d20;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ptz = (MX_PAN_TILT_ZOOM *) record->record_class_struct;

	mx_status = mxd_hitachi_kp_d20_get_pointers( ptz,
						&hitachi_kp_d20, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mx_rs232_discard_unread_input( hitachi_kp_d20->rs232_record,
						MXD_HITACHI_KP_D20_DEBUG );

	return mx_status;
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

	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));

	switch( ptz->command ) {
	case MXF_PTZ_ZOOM_IN:
		strlcpy( command, "3800FF00", sizeof(command) );
		break;
	case MXF_PTZ_ZOOM_OUT:
		strlcpy( command, "38000000", sizeof(command) );
		break;
	case MXF_PTZ_ZOOM_STOP:
		/* Ignore this command since it is irrelevant to 
		 * a digital zoom camera which zooms immediately.
		 */
		break;
	case MXF_PTZ_ZOOM_OFF:
		strlcpy( command, "37010000", sizeof(command) );
		break;
	case MXF_PTZ_ZOOM_ON:
		strlcpy( command, "37000000", sizeof(command) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The command %#lx received for Hitachi PTZ '%s' "
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
	static const char fname[] = "mxd_hitachi_kp_d20_get_status()";

	MX_HITACHI_KP_D20 *hitachi_kp_d20;
	mx_status_type mx_status;

	mx_status = mxd_hitachi_kp_d20_get_pointers( ptz,
						&hitachi_kp_d20, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: invoked for PTZ '%s'", fname, ptz->record->name));

	ptz->status = 0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_hitachi_kp_d20_get_parameter( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_hitachi_kp_d20_get_parameter()";

	MX_HITACHI_KP_D20 *hitachi_kp_d20;
	mx_status_type mx_status;

	mx_status = mxd_hitachi_kp_d20_get_pointers( ptz,
						&hitachi_kp_d20, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_hitachi_kp_d20_set_parameter( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_hitachi_kp_d20_set_parameter()";

	MX_HITACHI_KP_D20 *hitachi_kp_d20;
	char command[80];
	unsigned long ulong_value;
	mx_status_type mx_status;

	mx_status = mxd_hitachi_kp_d20_get_pointers( ptz,
						&hitachi_kp_d20, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));

	switch( ptz->parameter_type ) {
	case MXF_PTZ_ZOOM_TO:
		ulong_value = ptz->parameter_value[0];

		if ( ulong_value >= 256 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested zoom value of %lu is outside "
			"the allowed range (0-255) for Hitachi PTZ '%s'",
				ulong_value, ptz->record->name );
		}

		snprintf( command, sizeof(command), "3800%02lX00",
				ulong_value );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Parameter type %d received for Hitachi PTZ '%s' "
			"is not a known parameter type.",
			ptz->parameter_type, hitachi_kp_d20->record->name );
		break;
	}

	/* Send the command. */

	mx_status = mxd_hitachi_kp_d20_cmd( hitachi_kp_d20, command );

	return mx_status;
}

