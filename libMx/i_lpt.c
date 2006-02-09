/*
 * Name:    i_lpt.c
 *
 * Purpose: MX interface driver for using a PC compatible LPT printer port
 *          for digital I/O.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_LPT_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_portio.h"

#include "i_lpt.h"

MX_RECORD_FUNCTION_LIST mxi_lpt_record_function_list = {
	NULL,
	mxi_lpt_create_record_structures,
	mxi_lpt_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_lpt_open,
	mxi_lpt_close
};

MX_RECORD_FIELD_DEFAULTS mxi_lpt_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_LPT_STANDARD_FIELDS
};

mx_length_type mxi_lpt_num_record_fields
		= sizeof( mxi_lpt_record_field_defaults )
			/ sizeof( mxi_lpt_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_lpt_rfield_def_ptr
			= &mxi_lpt_record_field_defaults[0];

static mx_status_type
mxi_lpt_get_pointers( MX_RECORD *record,
			MX_LPT **lpt,
			const char *calling_fname )
{
	static const char fname[] = "mxi_lpt_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( lpt == (MX_LPT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_LPT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*lpt = (MX_LPT *) (record->record_type_struct);

	if ( *lpt == (MX_LPT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LPT pointer for record '%s' is NULL.",
			record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=== Public functions ===*/

MX_EXPORT mx_status_type
mxi_lpt_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_lpt_create_record_structures()";

	MX_LPT *lpt;

	/* Allocate memory for the necessary structures. */

	lpt = (MX_LPT *) malloc( sizeof(MX_LPT) );

	if ( lpt == (MX_LPT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_LPT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = lpt;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	lpt->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_lpt_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_lpt_finish_record_initialization()";

	MX_LPT *lpt;
	size_t i, length;
	mx_status_type mx_status;

	mx_status = mxi_lpt_get_pointers( record, &lpt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Force the LPT mode name to upper case. */

	length = strlen( lpt->lpt_mode_name );

	for ( i = 0; i < length; i++ ) {
		if ( islower( (int) (lpt->lpt_mode_name[i]) ) ) {
			lpt->lpt_mode_name[i] = 
				toupper( (int) (lpt->lpt_mode_name[i]) );
		}
	}

	MX_DEBUG( 2,("%s: LPT '%s' mode name = '%s'",
		fname, record->name, lpt->lpt_mode_name));

	/* Figure out which LPT mode this is. */

	if ( strcmp( lpt->lpt_mode_name, "SPP" ) == 0 ) {
		lpt->lpt_mode = MX_LPT_SPP_MODE;
	} else
	if ( strcmp( lpt->lpt_mode_name, "EPP" ) == 0 ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support for EPP mode for record '%s' is not yet implemented.  "
		"For now, use SPP mode instead.",
			record->name );
	} else
	if ( strcmp( lpt->lpt_mode_name, "ECP" ) == 0 ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support for EPP mode for record '%s' is not yet implemented.  "
		"For now, use SPP mode instead.",
			record->name );
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"LPT mode '%s' for record '%s' is not recognized as a legal LPT mode.",
			lpt->lpt_mode_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_lpt_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_lpt_open()";

	MX_LPT *lpt;
	mx_status_type mx_status;

	mx_status = mxi_lpt_get_pointers( record, &lpt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if we have permission to write to the LPT's ports. */

	mx_status = mx_portio_request_region( lpt->portio_record,
						lpt->base_address, 3 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Record the current state of all of the ports. */

	mx_status = mxi_lpt_read_port( lpt, MX_LPT_DATA_PORT, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_lpt_read_port( lpt, MX_LPT_STATUS_PORT, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_lpt_read_port( lpt, MX_LPT_CONTROL_PORT, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_lpt_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_lpt_close()";

	MX_LPT *lpt;
	mx_status_type mx_status;

	mx_status = mxi_lpt_get_pointers( record, &lpt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_portio_release_region( lpt->portio_record,
						lpt->base_address, 3 );

	return mx_status;
}

/* === Driver specific functions === */

MX_EXPORT mx_status_type
mxi_lpt_read_port( MX_LPT *lpt, int port_number, uint8_t *value )
{
	static const char fname[] = "mxi_lpt_read_port()";

	unsigned long port_address, invert_bits;
	uint8_t value_read;

	if ( ( port_number < 0 ) || ( port_number > MX_LPT_CONTROL_PORT ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal port number %d.", port_number );
	}

	/* Read the value from the LPT port. */

	port_address = lpt->base_address + port_number;

	value_read = mx_portio_inp8( lpt->portio_record, port_address );

	/* The value read from the port is handled differently depending
	 * on which port it is.
	 */

	invert_bits = lpt->lpt_flags & MXF_LPT_INVERT_INVERTED_BITS;

	switch( port_number ) {
	case MX_LPT_DATA_PORT:
		lpt->data_port_value = value_read;
		break;
	case MX_LPT_STATUS_PORT:
		lpt->status_port_value = value_read;

		if ( invert_bits ) {
			value_read ^= 0x80;
		}

		value_read >>= 3;

		value_read &= 0x1F;

		break;
	case MX_LPT_CONTROL_PORT:
		lpt->control_port_value = value_read;

		if ( invert_bits ) {
			value_read ^= 0x0B;
		}

		value_read &= 0x0F;

		break;
	} 

	if ( value != NULL ) {
		*value = value_read;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_lpt_write_port( MX_LPT *lpt, int port_number, uint8_t value )
{
	static const char fname[] = "mxi_lpt_write_port()";

	unsigned long port_address, invert_bits;

	if ( ( port_number < 0 ) || ( port_number > MX_LPT_CONTROL_PORT ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal port number %d.", port_number );
	}

	/* The value to be written is handled differently depending
	 * on which port it is.
	 */

	invert_bits = lpt->lpt_flags & MXF_LPT_INVERT_INVERTED_BITS;

	switch( port_number ) {
	case MX_LPT_DATA_PORT:
		lpt->data_port_value = value;
		break;
	case MX_LPT_STATUS_PORT:

		value &= 0x1F;

		value <<= 3;

		if ( invert_bits ) {
			value ^= 0x80;
		}

		lpt->status_port_value = value;
		break;
	case MX_LPT_CONTROL_PORT:

		value &= 0x0F;

		if ( invert_bits ) {
			value ^= 0x0B;
		}

		lpt->control_port_value = value;
		break;
	} 

	/* Write the value to the LPT port. */

	port_address = lpt->base_address + port_number;

	mx_portio_outp8( lpt->portio_record, port_address, value );

	return MX_SUCCESSFUL_RESULT;
}

