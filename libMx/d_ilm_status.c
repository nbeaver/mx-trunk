/*
 * Name:    d_ilm_status.c
 *
 * Purpose: Digital input driver for status values from the X command for
 *          Oxford Instruments ILM (Intelligent Level Meter) controllers.
 *          Channels 1, 2, and 3 report the nitrogen or helium channel
 *          status, while channel 4 returns the relay status.
 *
 * Author:  William Lavender / Henry Bellamy
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ILM_STATUS_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_digital_input.h"
#include "i_isobus.h"
#include "i_ilm.h"
#include "d_ilm_status.h"

/* Initialize the ILM digital input driver function tables. */

MX_RECORD_FUNCTION_LIST mxd_ilm_status_record_function_list = {
	NULL,
	mxd_ilm_status_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_ilm_status_open
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_ilm_status_digital_input_function_list = {
	mxd_ilm_status_read
};

MX_RECORD_FIELD_DEFAULTS mxd_ilm_status_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_ILM_STATUS_STANDARD_FIELDS
};

long mxd_ilm_status_num_record_fields
		= sizeof( mxd_ilm_status_record_field_defaults )
			/ sizeof( mxd_ilm_status_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ilm_status_rfield_def_ptr
		= &mxd_ilm_status_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_ilm_status_get_pointers( MX_DIGITAL_INPUT *dinput,
				MX_ILM_STATUS **ilm_status,
				MX_ILM **ilm,
				MX_ISOBUS **isobus,
				const char *calling_fname )
{
	static const char fname[] = "mxd_ilm_status_get_pointers()";

	MX_ILM_STATUS *ilm_status_ptr;
	MX_RECORD *ilm_record, *isobus_record;
	MX_ILM *ilm_ptr;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_INPUT pointer passed was NULL." );
	}

	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_DIGITAL_INPUT pointer passed was NULL." );
	}

	ilm_status_ptr = (MX_ILM_STATUS *)
				dinput->record->record_type_struct;

	if ( ilm_status_ptr == (MX_ILM_STATUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ILM_STATUS pointer for digital input '%s' is NULL",
			dinput->record->name );
	}

	if ( ilm_status != (MX_ILM_STATUS **) NULL ) {
		*ilm_status = ilm_status_ptr;
	}

	ilm_record = ilm_status_ptr->ilm_record;

	if ( ilm_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The ilm_record pointer for digital input '%s' is NULL.",
			dinput->record->name );
	}

	if ( ilm_record->mx_type != MXI_CTRL_ILM ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"ilm_record '%s' for ILM digital input '%s' "
		"is not an 'ilm' record.  Instead, it is of type '%s'.",
			ilm_record->name, dinput->record->name,
			mx_get_driver_name( ilm_record ) );
	}

	ilm_ptr = (MX_ILM *) ilm_record->record_type_struct;

	if ( ilm_ptr == (MX_ILM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ILM pointer for ILM record '%s' "
		"used by ILM digital input '%s' is NULL.",
			ilm_record->name,
			dinput->record->name );
	}

	if ( ilm != (MX_ILM **) NULL ) {
		*ilm = ilm_ptr;
	}

	if ( isobus != (MX_ISOBUS **) NULL ) {
		isobus_record = ilm_ptr->isobus_record;

		if ( isobus_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The isobus_record pointer for ILM "
			"record '%s' is NULL.", ilm_record->name );
		}

		*isobus = isobus_record->record_type_struct;

		if ( (*isobus) == (MX_ISOBUS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_ISOBUS pointer for ISOBUS record '%s' "
			"is NULL.", isobus_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_ilm_status_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_ilm_status_create_record_structures()";

	MX_DIGITAL_INPUT *dinput;
	MX_ILM_STATUS *ilm_status;

	/* Allocate memory for the necessary structures. */

	dinput = (MX_DIGITAL_INPUT *) malloc( sizeof(MX_DIGITAL_INPUT) );

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_DIGITAL_INPUT structure." );
	}

	ilm_status = (MX_ILM_STATUS *) malloc( sizeof(MX_ILM_STATUS) );

	if ( ilm_status == (MX_ILM_STATUS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_ILM_STATUS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = dinput;
	record->record_type_struct = ilm_status;
	record->class_specific_function_list =
		&mxd_ilm_status_digital_input_function_list;

	dinput->record = record;
	ilm_status->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ilm_status_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ilm_status_open()";

	MX_DIGITAL_INPUT *dinput;
	MX_ILM_STATUS *ilm_status = NULL;
	MX_ILM *ilm = NULL;
	MX_ISOBUS *isobus = NULL;
	char response[80];
	int channel_usage;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXD_ILM_STATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	dinput = record->record_class_struct;

	mx_status = mxd_ilm_status_get_pointers( dinput,
					&ilm_status, &ilm, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (ilm_status->channel < 1)
	  || (ilm_status->channel > 4) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal channel number %ld specified for ILM status "
		"record '%s'.  The allowed values are from 1 to 4.",
			ilm_status->channel,
			ilm_status->record->name );
	}

	/* "Channel 4" (the relay status) does not have a channel usage value.*/

	if ( ilm_status->channel == 4 )
		return MX_SUCCESSFUL_RESULT;

	/* Get the ILM status. */

	mx_status = mxi_isobus_command( isobus, ilm->isobus_address,
					"X", response, sizeof(response),
					ilm->maximum_retries,
					MXD_ILM_STATUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	channel_usage = (int) mx_hex_char_to_unsigned_long(
				response[ilm_status->channel] );

#if MXD_ILM_STATUS_DEBUG
	MX_DEBUG(-2,("%s: channel %ld, channel_usage = %d",
		fname, ilm_status->channel, channel_usage));
#endif

	switch( channel_usage ) {
	case 1:
	case 2:
	case 3:
		break;
	case 0:
		mx_warning( "Channel %ld for ILM status record '%s' "
		"is marked as 'not in use'.",
			ilm_status->channel, ilm_status->record->name );
		break;
	case 9:
		mx_warning( "Channel %ld for ILM status record '%s' "
		"has an error (usually means probe unplugged).",
			ilm_status->channel, ilm_status->record->name );
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognized channel usage value %x reported "
		"for channel %ld of ILM status record '%s'.",
			channel_usage, ilm_status->channel,
			ilm_status->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ilm_status_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_ilm_status_read()";

	MX_ILM_STATUS *ilm_status = NULL;
	MX_ILM *ilm = NULL;
	MX_ISOBUS *isobus = NULL;
	char response[80];
	int num_items;
	unsigned long status[4];
	mx_status_type mx_status;

	mx_status = mxd_ilm_status_get_pointers( dinput,
					&ilm_status, &ilm, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_isobus_command( isobus, ilm->isobus_address,
					"X", response, sizeof(response),
					ilm->maximum_retries,
					MXD_ILM_STATUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "X%*3cS%2lx%2lx%2lxR%2lx",
		&status[0], &status[1], &status[2], &status[3] );

	if ( num_items != 4 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable response '%s' was returned for ILM status "
		"channel '%s' from ILM controller '%s'.",
			response, ilm_status->record->name, ilm->record->name );
	}

	dinput->value = status[ ilm_status->channel - 1 ];

#if MXD_ILM_STATUS_DEBUG
	MX_DEBUG(-2,("%s: ILM channel '%s' status = %#02lx",
		fname, ilm_status->record->name, dinput->value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

