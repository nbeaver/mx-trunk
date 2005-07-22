/*
 * Name:    d_mardtb_shutter.c
 *
 * Purpose: MX relay driver for the shutter of a MarUSA Desktop Beamline
 *          goniostat controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MARDTB_SHUTTER_DEBUG 	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_record.h"
#include "mx_driver.h"
#include "mx_relay.h"
#include "mx_rs232.h"
#include "i_mardtb.h"
#include "d_mardtb_shutter.h"

/* Initialize the generic relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mardtb_shutter_record_function_list = {
	NULL,
	mxd_mardtb_shutter_create_record_structures
};

MX_RELAY_FUNCTION_LIST mxd_mardtb_shutter_relay_function_list = {
	mxd_mardtb_shutter_relay_command,
	mxd_mardtb_shutter_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_mardtb_shutter_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_MARDTB_SHUTTER_STANDARD_FIELDS
};

long mxd_mardtb_shutter_num_record_fields
	= sizeof( mxd_mardtb_shutter_rf_defaults )
		/ sizeof( mxd_mardtb_shutter_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mardtb_shutter_rfield_def_ptr
			= &mxd_mardtb_shutter_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_mardtb_get_pointers( MX_RELAY *relay,
			      MX_MARDTB_SHUTTER **mardtb_shutter,
			      MX_MARDTB **mardtb,
			      const char *fname )
{
	MX_MARDTB_SHUTTER *mardtb_shutter_ptr;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}

	mardtb_shutter_ptr = (MX_MARDTB_SHUTTER *)
				relay->record->record_type_struct;

	if ( mardtb_shutter_ptr == (MX_MARDTB_SHUTTER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MARDTB_SHUTTER pointer for record '%s' is NULL.",
			relay->record->name );
	}

	if ( mardtb_shutter != (MX_MARDTB_SHUTTER **) NULL ) {
		*mardtb_shutter = mardtb_shutter_ptr;
	}

	if ( mardtb != (MX_MARDTB **) NULL ) {
		*mardtb = (MX_MARDTB *)
			mardtb_shutter_ptr->mardtb_record->record_type_struct;

		if ( *mardtb == (MX_MARDTB *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MARDTB pointer for MARDTB record '%s' used by "
			"MARDTB relay record '%s' is NULL.",
				mardtb_shutter_ptr->mardtb_record->name,
				relay->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mardtb_shutter_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_mardtb_shutter_create_record_structures()";

        MX_RELAY *relay;
        MX_MARDTB_SHUTTER *mardtb_shutter;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_RELAY structure." );
        }

        mardtb_shutter = (MX_MARDTB_SHUTTER *)
					malloc( sizeof(MX_MARDTB_SHUTTER) );

        if ( mardtb_shutter == (MX_MARDTB_SHUTTER *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_MARDTB_SHUTTER structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = mardtb_shutter;
        record->class_specific_function_list =
				&mxd_mardtb_shutter_relay_function_list;

        relay->record = record;
	mardtb_shutter->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mardtb_shutter_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_mardtb_shutter_relay_command()";

	MX_MARDTB_SHUTTER *mardtb_shutter;
	MX_MARDTB *mardtb;
	char command[40];
	int move_in_progress;
	mx_status_type mx_status;

	mx_status = mxd_mardtb_get_pointers( relay,
					&mardtb_shutter, &mardtb, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_mardtb_check_for_move_in_progress( mardtb,
							&move_in_progress );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( move_in_progress ) {
		return mx_error( MXE_TRY_AGAIN, fname,
		"Mar DTB motor '%s' is currently in motion, "
		"so we cannot currently open or close Mar DTB shutter '%s'.",
			mardtb->currently_active_record->name,
			relay->record->name );
	}

	switch( relay->relay_command ) {
	case MXF_CLOSE_RELAY:
		strcpy( command, "shutter 0" );
		break;
	case MXF_OPEN_RELAY:
		strcpy( command, "shutter 1" );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal relay command value %d for MarDTB shutter '%s'.  "
		"The allowed values are 0 and 1",
			relay->relay_command, relay->record->name );
		break;
	}

	mx_status = mxi_mardtb_command( mardtb, command,
					NULL, 0,
					MXD_MARDTB_SHUTTER_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mardtb_shutter_get_relay_status( MX_RELAY *relay )
{
	static const char fname[] = "mxd_mardtb_shutter_get_relay_status()";

	MX_MARDTB_SHUTTER *mardtb_shutter;
	MX_MARDTB *mardtb;
	unsigned long parameter_value;
	unsigned long high_order_byte, shutter_mask, shutter_byte;
	mx_status_type mx_status;

	mx_status = mxd_mardtb_get_pointers( relay,
					&mardtb_shutter, &mardtb, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_mardtb_read_status_parameter( mardtb, 97,
							&parameter_value );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The shutter status is found in the highest order byte
	 * in the returned parameter value.
	 */

	high_order_byte = ( parameter_value >> 24 ) & 0xff;

	/* Only three bits are involved in the shutter status. */

	shutter_mask = 0xC2;

	shutter_byte = high_order_byte & shutter_mask;

	MX_DEBUG( 2,("%s: shutter_byte = %#lx", fname, shutter_byte));

	switch( shutter_byte ) {
	case 0x40:
		relay->relay_status = MXF_RELAY_IS_CLOSED;
		break;
	case 0x82:
		relay->relay_status = MXF_RELAY_IS_OPEN;
		break;
	default:
		relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The shutter status byte %#lx does not have one of the "
		"two legal values of 0x40 and 0x82 for MarDTB shutter '%s'.  "
		"The original MarDTB parameter 97 value was %#lx.",
			shutter_byte, relay->record->name, parameter_value );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

