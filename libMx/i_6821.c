/*
 * Name:    i_6821.c
 *
 * Purpose: MX interface driver for Motorola MC6821 digital I/O chips.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_6821_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_portio.h"
#include "mx_generic.h"
#include "i_6821.h"

MX_RECORD_FUNCTION_LIST mxi_6821_record_function_list = {
	NULL,
	mxi_6821_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_6821_open,
	mxi_6821_close
};

MX_GENERIC_FUNCTION_LIST mxi_6821_generic_function_list = {
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxi_6821_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_6821_STANDARD_FIELDS
};

long mxi_6821_num_record_fields
		= sizeof( mxi_6821_record_field_defaults )
			/ sizeof( mxi_6821_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_6821_rfield_def_ptr
			= &mxi_6821_record_field_defaults[0];

static mx_status_type
mxi_6821_get_pointers( MX_RECORD *record,
				MX_6821 **mc6821,
				const char *calling_fname )
{
	static const char fname[] = "mxi_6821_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mc6821 == (MX_6821 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_6821 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mc6821 = (MX_6821 *) (record->record_type_struct);

	if ( *mc6821 == (MX_6821 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_6821 pointer for record '%s' is NULL.",
			record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=== Public functions ===*/

MX_EXPORT mx_status_type
mxi_6821_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_6821_create_record_structures()";

	MX_GENERIC *generic;
	MX_6821 *mc6821;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_GENERIC structure." );
	}

	mc6821 = (MX_6821 *) malloc( sizeof(MX_6821) );

	if ( mc6821 == (MX_6821 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_6821 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = mc6821;
	record->class_specific_function_list = &mxi_6821_generic_function_list;

	generic->record = record;
	mc6821->record = record;

	mc6821->port_a_record = NULL;
	mc6821->port_b_record = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_6821_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_6821_open()";

	MX_6821 *mc6821;
	mx_status_type mx_status;

	mx_status = mxi_6821_get_pointers( record, &mc6821, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if we have permission to write to the 6821's ports. */

	mx_status = mx_portio_request_region( mc6821->portio_record,
						mc6821->base_address, 4 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Select the port A data direction register. */

	mx_portio_outp8( mc6821->portio_record,
			mc6821->base_address + MX_MC6821_PORT_A_CONTROL, 0x4 );

	/* Write the port A data direction byte. */

	mx_portio_outp8( mc6821->portio_record,
			mc6821->base_address + MX_MC6821_PORT_A_DATA,
			(uint8_t) mc6821->port_a_data_direction );

	/* Select the port A output register. */

	mx_portio_outp8( mc6821->portio_record,
			mc6821->base_address + MX_MC6821_PORT_A_CONTROL, 0x0 );

	/* Select the port B data direction register. */

	mx_portio_outp8( mc6821->portio_record,
			mc6821->base_address + MX_MC6821_PORT_B_CONTROL, 0x4 );

	/* Write the port B data direction byte. */

	mx_portio_outp8( mc6821->portio_record,
			mc6821->base_address + MX_MC6821_PORT_B_DATA,
			(uint8_t) mc6821->port_b_data_direction );

	/* Select the port B output register. */

	mx_portio_outp8( mc6821->portio_record,
			mc6821->base_address + MX_MC6821_PORT_B_CONTROL, 0x0 );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_6821_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_6821_close()";

	MX_6821 *mc6821;
	mx_status_type mx_status;

	mx_status = mxi_6821_get_pointers( record, &mc6821, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_portio_release_region( mc6821->portio_record,
						mc6821->base_address, 4 );

	return mx_status;
}

