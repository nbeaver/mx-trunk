/*
 * Name:    d_panasonic_kx_dp702_ptz.c
 *
 * Purpose: MX driver for Panasonic KX-DP702 Pan/Tilt/Zoom camera units.
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

#define MXD_PANASONIC_KX_DP702_PTZ_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_ptz.h"
#include "i_panasonic_kx_dp702.h"
#include "d_panasonic_kx_dp702.h"

MX_RECORD_FUNCTION_LIST mxd_panasonic_kx_dp702_record_function_list = {
	NULL,
	mxd_panasonic_kx_dp702_create_record_structures
};

MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_panasonic_kx_dp702_ptz_function_list = {
	mxd_panasonic_kx_dp702_command,
	mxd_panasonic_kx_dp702_get_status,
	mxd_panasonic_kx_dp702_get_parameter,
	mxd_panasonic_kx_dp702_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_panasonic_kx_dp702_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PAN_TILT_ZOOM_STANDARD_FIELDS,
	MXD_PANASONIC_KX_DP702_PTZ_STANDARD_FIELDS
};

long mxd_panasonic_kx_dp702_num_record_fields
	= sizeof( mxd_panasonic_kx_dp702_rf_defaults )
		/ sizeof( mxd_panasonic_kx_dp702_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_panasonic_kx_dp702_rfield_def_ptr
			= &mxd_panasonic_kx_dp702_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_panasonic_kx_dp702_get_pointers( MX_PAN_TILT_ZOOM *ptz,
			MX_PANASONIC_KX_DP702_PTZ **kx_dp702_ptz,
			MX_PANASONIC_KX_DP702 **kx_dp702,
			const char *calling_fname )
{
	static const char fname[] = "mxd_panasonic_kx_dp702_get_pointers()";

	MX_PANASONIC_KX_DP702_PTZ *kx_dp702_ptz_ptr;
	MX_RECORD *kx_dp702_record;

	if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PAN_TILT_ZOOM pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( ptz->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_PTZ %p is NULL.", ptz );
	}

	kx_dp702_ptz_ptr = (MX_PANASONIC_KX_DP702_PTZ *)
					ptz->record->record_type_struct;

	if ( kx_dp702_ptz_ptr == (MX_PANASONIC_KX_DP702_PTZ *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_PANASONIC_KX_DP702_PTZ pointer for record '%s' is NULL.",
			ptz->record->name );
	}

	if ( kx_dp702_ptz != (MX_PANASONIC_KX_DP702_PTZ **) NULL ) {
		*kx_dp702_ptz = kx_dp702_ptz_ptr;
	}

	kx_dp702_record = kx_dp702_ptz_ptr->kx_dp702_record;

	if ( kx_dp702_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'kx_dp702_record' pointer for Panasonic PTZ '%s' is NULL.",
			ptz->record->name );
	}

	if ( kx_dp702 != (MX_PANASONIC_KX_DP702 **) NULL ) {
		*kx_dp702 = (MX_PANASONIC_KX_DP702 *)
				kx_dp702_record->record_type_struct;

		if ( *kx_dp702 == (MX_PANASONIC_KX_DP702 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PANASONIC_KX_DP702 pointer for VISCA "
			"record '%s' used by Panasonic PTZ '%s' is NULL.",
				kx_dp702_record->name,
				ptz->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_panasonic_kx_dp702_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_panasonic_kx_dp702_create_record_structures()";

        MX_PAN_TILT_ZOOM *ptz;
        MX_PANASONIC_KX_DP702_PTZ *kx_dp702_ptz;

        /* Allocate memory for the necessary structures. */

        ptz = (MX_PAN_TILT_ZOOM *) malloc(sizeof(MX_PAN_TILT_ZOOM));

        if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Unable to allocate memory for an MX_PAN_TILT_ZOOM structure." );
        }
	
	kx_dp702_ptz = (MX_PANASONIC_KX_DP702_PTZ *)
				malloc( sizeof(MX_PANASONIC_KX_DP702_PTZ) );

        if ( kx_dp702_ptz == (MX_PANASONIC_KX_DP702_PTZ *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
    "Unable to allocate memory for an MX_PANASONIC_KX_DP702_PTZ structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = ptz;
        record->record_type_struct = kx_dp702_ptz;
        record->class_specific_function_list =
				&mxd_panasonic_kx_dp702_ptz_function_list;

        ptz->record = record;
	kx_dp702_ptz->record = record;

	ptz->pan_speed = 1;
	ptz->tilt_speed = 1;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panasonic_kx_dp702_command( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_panasonic_kx_dp702_command()";

	MX_PANASONIC_KX_DP702_PTZ *kx_dp702_ptz;
	MX_PANASONIC_KX_DP702 *kx_dp702;
	unsigned char command[80];
	size_t command_length;
	mx_status_type mx_status;

	mx_status = mxd_panasonic_kx_dp702_get_pointers( ptz,
					&kx_dp702_ptz, &kx_dp702, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	command_length = 0;

	switch( ptz->command ) {
	case MXF_PTZ_PAN_LEFT:
		command[0] = 0x40;	command_length = 1;
		break;
	case MXF_PTZ_PAN_RIGHT:
		command[0] = 0x50;	command_length = 1;
		break;
	case MXF_PTZ_TILT_UP:
		command[0] = 0x45;	command_length = 1;
		break;
	case MXF_PTZ_TILT_DOWN:
		command[0] = 0x55;	command_length = 1;
		break;
	case MXF_PTZ_PAN_STOP:
	case MXF_PTZ_TILT_STOP:
	case MXF_PTZ_DRIVE_STOP:
		command[0] = 0x4a;	command_length = 1;
		break;
	case MXF_PTZ_DRIVE_HOME:
		command[0] = 0x39;	command_length = 1;
		break;
	case MXF_PTZ_ZOOM_IN:
		command[0] = 0x4c;	command_length = 1;
		break;
	case MXF_PTZ_ZOOM_OUT:
		command[0] = 0x4b;	command_length = 1;
		break;
	case MXF_PTZ_ZOOM_STOP:
		command[0] = 0x4d;	command_length = 1;
		break;
	case MXF_PTZ_FOCUS_MANUAL:
		command[0] = 0x08;	command_length = 1;
		break;
	case MXF_PTZ_FOCUS_AUTO:
		command[0] = 0x07;	command_length = 1;
		break;
	case MXF_PTZ_FOCUS_STOP:
		command[0] = 0x5d;	command_length = 1;
		break;
	case MXF_PTZ_FOCUS_FAR:
		command[0] = 0x5b;	command_length = 1;
		break;
	case MXF_PTZ_FOCUS_NEAR:
		command[0] = 0x5c;	command_length = 1;
		break;
	case MXF_PTZ_DRIVE_UPPER_LEFT:
	case MXF_PTZ_DRIVE_UPPER_RIGHT:
	case MXF_PTZ_DRIVE_LOWER_LEFT:
	case MXF_PTZ_DRIVE_LOWER_RIGHT:
	case MXF_PTZ_ZOOM_OFF:
	case MXF_PTZ_ZOOM_ON:
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The command %lu received for Panasonic PTZ '%s' "
			"is not a known command type.",
				ptz->command, kx_dp702_ptz->record->name );
	}

	/* Send the command. */

	mx_status = mxi_panasonic_kx_dp702_cmd( kx_dp702,
					kx_dp702_ptz->camera_number,
					command, command_length );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_panasonic_kx_dp702_get_status( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_panasonic_kx_dp702_get_status()";

	MX_PANASONIC_KX_DP702_PTZ *kx_dp702_ptz;
	MX_PANASONIC_KX_DP702 *kx_dp702;
	unsigned char response[20];
	mx_status_type mx_status;

	mx_status = mxd_panasonic_kx_dp702_get_pointers( ptz,
					&kx_dp702_ptz, &kx_dp702, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_panasonic_kx_dp702_raw_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						(unsigned char *) "\x90", 1,
						response, 3, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (response[0] != 0xa4) || (response[1] != 0x01) ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see the bytes 0xa4 and 0x01 at the start "
		"of the response to a 'read status of motion' "
		"command for Panasonic PTZ '%s'.  "
		"Instead saw %#x %#x %#x.",
			ptz->record->name, response[0],
			response[1], response[2] );
	}

	ptz->status = 0;

	if ( response[2] & 0x1 ) {
		ptz->status |= MXSF_PTZ_PAN_IS_BUSY;
	}
	if ( response[2] & 0x2 ) {
		ptz->status |= MXSF_PTZ_TILT_IS_BUSY;
	}
	if ( response[2] & 0x4 ) {
		ptz->status |= MXSF_PTZ_ZOOM_IS_BUSY;
	}
	if ( response[2] & 0x8 ) {
		ptz->status |= MXSF_PTZ_FOCUS_IS_BUSY;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_panasonic_kx_dp702_get_parameter( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] =
		"mxd_panasonic_kx_dp702_get_parameter()";

	MX_PANASONIC_KX_DP702_PTZ *kx_dp702_ptz;
	MX_PANASONIC_KX_DP702 *kx_dp702;
	unsigned char response[80];
	mx_status_type mx_status;

	mx_status = mxd_panasonic_kx_dp702_get_pointers( ptz,
					&kx_dp702_ptz, &kx_dp702, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PANASONIC_KX_DP702_PTZ_DEBUG
	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));
#endif

	switch( ptz->parameter_type ) {
	case MXF_PTZ_PAN_POSITION:
	case MXF_PTZ_TILT_POSITION:
	case MXF_PTZ_ZOOM_POSITION:
		mx_status = mxi_panasonic_kx_dp702_raw_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						(unsigned char *) "\x94", 1,
						response, 8, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (response[0] != 0xa0) || (response[1] != 0x06) ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the bytes 0xa0 and 0x06 at the start "
			"of the response to a 'read absolute coordination' "
			"command for Panasonic PTZ '%s'.  "
			"Instead saw %#x %#x %#x %#x %#x %#x %#x %#x.",
				ptz->record->name, response[0], response[1],
				response[2], response[3], response[4],
				response[5], response[6], response[7] );
		}

		ptz->pan_position = (int16_t) ( response[3] + 
			( (((unsigned long) response[2]) << 8) & 0xff00 ) );

		ptz->tilt_position = (int16_t) ( response[5] + 
			( (((unsigned long) response[4]) << 8) & 0xff00 ) );

		ptz->zoom_position = response[7] + 
			( (((unsigned long) response[6]) << 8) & 0xff00 );

		break;
	case MXF_PTZ_FOCUS_POSITION:
		/* The KX-DP702 does not seem to provide a way of reading
		 * the current focus setting.
		 */

		break;
	case MXF_PTZ_PAN_SPEED:
		mx_status = mxi_panasonic_kx_dp702_raw_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						(unsigned char *) "\x51", 1,
						response, 4, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (response[0] != 0xc1) || (response[1] != 0x02) ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the bytes 0xc1 and 0x02 at the start "
			"of the response to a 'read direct pan speed' "
			"command for Panasonic PTZ '%s'.  "
			"Instead saw %#x %#x %#x %#x.",
				ptz->record->name,
				response[0], response[1],
				response[2], response[3] );
		}

		ptz->pan_speed = response[3] +
			( (((unsigned long) response[2]) << 8) & 0xff00 );
		break;
	case MXF_PTZ_TILT_SPEED:
		mx_status = mxi_panasonic_kx_dp702_raw_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						(unsigned char *) "\x52", 1,
						response, 4, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (response[0] != 0xc2) || (response[1] != 0x02) ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the bytes 0xc2 and 0x02 at the start "
			"of the response to a 'read direct tilt speed' "
			"command for Panasonic PTZ '%s'.  "
			"Instead saw %#x %#x %#x %#x.",
				ptz->record->name,
				response[0], response[1],
				response[2], response[3] );
		}

		ptz->tilt_speed = response[3] +
			( (((unsigned long) response[2]) << 8) & 0xff00 );
		break;
	case MXF_PTZ_ZOOM_SPEED:
		mx_status = mxi_panasonic_kx_dp702_raw_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						(unsigned char *) "\x53", 1,
						response, 4, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (response[0] != 0xc3) || (response[1] != 0x02) ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the bytes 0xc3 and 0x02 at the start "
			"of the response to a 'read direct zoom speed' "
			"command for Panasonic PTZ '%s'.  "
			"Instead saw %#x %#x %#x %#x.",
				ptz->record->name,
				response[0], response[1],
				response[2], response[3] );
		}

		ptz->zoom_speed = response[3] +
			( (((unsigned long) response[2]) << 8) & 0xff00 );
		break;
	case MXF_PTZ_FOCUS_SPEED:
		mx_status = mxi_panasonic_kx_dp702_raw_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						(unsigned char *) "\x54", 1,
						response, 4, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (response[0] != 0xc4) || (response[1] != 0x02) ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the bytes 0xc4 and 0x02 at the start "
			"of the response to a 'read direct focus speed' "
			"command for Panasonic PTZ '%s'.  "
			"Instead saw %#x %#x %#x %#x.",
				ptz->record->name,
				response[0], response[1],
				response[2], response[3] );
		}

		ptz->focus_speed = response[3] +
			( (((unsigned long) response[2]) << 8) & 0xff00 );
		break;
	case MXF_PTZ_FOCUS_AUTO:
		mx_status = mxi_panasonic_kx_dp702_raw_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						(unsigned char *) "\x91", 1,
						response, 3, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (response[0] != 0xa5) || (response[1] != 0x01) ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the bytes 0xa5 and 0x01 at the start "
			"of the response to a 'read status of motion' "
			"command for Panasonic PTZ '%s'.  "
			"Instead saw %#x %#x %#x.",
				ptz->record->name, response[0],
				response[1], response[2] );
		}

		if ( response[2] & 0x4 ) {
			ptz->focus_auto = TRUE;
		} else {
			ptz->focus_auto = FALSE;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The parameter type %#lx requested for Panasonic PTZ '%s' "
		"is not a known command type.",
			ptz->command, kx_dp702_ptz->record->name );
	}

#if 0
	mx_status = mxi_panasonic_kx_dp702_cmd( kx_dp702,
					kx_dp702_ptz->camera_number,
				command, response, sizeof(response), NULL );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ptz->parameter_type ) {
	case MXF_PTZ_PAN_POSITION:
	case MXF_PTZ_TILT_POSITION:

#if MXD_PANASONIC_KX_DP702_PTZ_DEBUG
		MX_DEBUG(-2,
	("%s: PTZ '%s' Pan position = %ld (%#lx), Tilt position = %ld (%#lx)",
			fname, ptz->record->name,
			ptz->pan_position, ptz->pan_position,
			ptz->tilt_position, ptz->tilt_position));
#endif
		break;
	case MXF_PTZ_ZOOM_POSITION:

#if MXD_PANASONIC_KX_DP702_PTZ_DEBUG
		MX_DEBUG(-2,("%s: PTZ '%s' Zoom position = %ld (%#lx)",
			fname, ptz->record->name,
			ptz->zoom_position, ptz->zoom_position));
#endif
		break;
	case MXF_PTZ_FOCUS_POSITION:

#if MXD_PANASONIC_KX_DP702_PTZ_DEBUG
		MX_DEBUG(-2,("%s: PTZ '%s' Focus position = %ld (%#lx)",
			fname, ptz->record->name,
			ptz->focus_position, ptz->focus_position));
#endif
		break;
	case MXF_PTZ_PAN_SPEED:
	case MXF_PTZ_TILT_SPEED:

#if MXD_PANASONIC_KX_DP702_PTZ_DEBUG
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
mxd_panasonic_kx_dp702_set_parameter( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] =
		"mxd_panasonic_kx_dp702_set_parameter()";

	MX_PANASONIC_KX_DP702_PTZ *kx_dp702_ptz;
	MX_PANASONIC_KX_DP702 *kx_dp702;
	unsigned char command[80];
	int saved_parameter_type;
	long pan_value, tilt_value;
	unsigned long zoom_value;
	mx_status_type mx_status;

	mx_status = mxd_panasonic_kx_dp702_get_pointers( ptz,
					&kx_dp702_ptz, &kx_dp702, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PANASONIC_KX_DP702_PTZ_DEBUG
	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));
#endif

	saved_parameter_type = ptz->parameter_type;

	switch( ptz->parameter_type ) {
	case MXF_PTZ_PAN_SPEED:
		command[0] = 0x41;
		command[1] = (unsigned char) (( ptz->pan_speed >> 8 ) & 0xff);
		command[2] = (unsigned char) (ptz->pan_speed & 0xff);

		mx_status = mxi_panasonic_kx_dp702_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						command, 3 );
		break;
	case MXF_PTZ_TILT_SPEED:
		command[0] = 0x42;
		command[1] = (unsigned char) (( ptz->tilt_speed >> 8 ) & 0xff);
		command[2] = (unsigned char) (ptz->tilt_speed & 0xff);

		mx_status = mxi_panasonic_kx_dp702_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						command, 3 );
		break;
	case MXF_PTZ_ZOOM_SPEED:
		command[0] = 0x43;
		command[1] = (unsigned char) (( ptz->zoom_speed >> 8 ) & 0xff);
		command[2] = (unsigned char) (ptz->zoom_speed & 0xff);

		mx_status = mxi_panasonic_kx_dp702_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						command, 3 );
		break;
	case MXF_PTZ_FOCUS_SPEED:
		command[0] = 0x44;
		command[1] = (unsigned char) (( ptz->focus_speed >> 8 ) & 0xff);
		command[2] = (unsigned char) (ptz->focus_speed & 0xff);

		mx_status = mxi_panasonic_kx_dp702_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						command, 3 );
		break;
	case MXF_PTZ_PAN_DESTINATION:
	case MXF_PTZ_TILT_DESTINATION:
	case MXF_PTZ_ZOOM_DESTINATION:
		if ( saved_parameter_type == MXF_PTZ_PAN_DESTINATION ) {
			pan_value = ptz->pan_destination;
		} else {
			mx_status = mx_ptz_get_pan( ptz->record, &pan_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		} 
		if ( saved_parameter_type == MXF_PTZ_TILT_DESTINATION ) {
			tilt_value = ptz->tilt_destination;
		} else {
			mx_status = mx_ptz_get_tilt( ptz->record, &tilt_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		if ( saved_parameter_type == MXF_PTZ_ZOOM_DESTINATION ) {
			zoom_value = ptz->zoom_destination;
		} else {
			mx_status = mx_ptz_get_zoom( ptz->record, &zoom_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		pan_value  &= 0xffff;
		tilt_value &= 0xffff;
		zoom_value &= 0xffff;

#if MXD_PANASONIC_KX_DP702_PTZ_DEBUG
		MX_DEBUG(-2,
("%s: PTZ move to pan = %ld (%#lx), tilt = %ld (%#lx), zoom = %ld (%#lx)",
			fname, pan_value, pan_value, tilt_value, tilt_value,
			zoom_value, zoom_value ));
#endif
		command[0] = 0x3e;
		command[1] = (unsigned char) (( pan_value >> 8 ) & 0xff);
		command[2] = (unsigned char) (pan_value & 0xff);
		command[3] = (unsigned char) (( tilt_value >> 8 ) & 0xff);
		command[4] = (unsigned char) (tilt_value & 0xff);
		command[5] = (unsigned char) (( zoom_value >> 8 ) & 0xff);
		command[6] = (unsigned char) (zoom_value & 0xff);

		mx_status = mxi_panasonic_kx_dp702_cmd( kx_dp702,
						kx_dp702_ptz->camera_number,
						command, 7 );
		break;
	case MXF_PTZ_FOCUS_DESTINATION:

#if MXD_PANASONIC_KX_DP702_PTZ_DEBUG
		MX_DEBUG(-2,("%s: Moving focus to %ld (%#lx)",
			fname, ptz->focus_destination,
			ptz->focus_destination));
#endif
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Parameter type %d received for Panasonic PTZ '%s' "
			"is not a known parameter type.",
			ptz->parameter_type, kx_dp702_ptz->record->name );
	}

	return mx_status;
}

