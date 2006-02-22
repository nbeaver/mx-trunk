/*
 * Name:    i_8255.c
 *
 * Purpose: MX interface driver for Intel 8255 digital I/O chips and their
 *          compatibles.  This driver only assumes the presence of 8255 mode 0.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

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
#include "i_8255.h"

MX_RECORD_FUNCTION_LIST mxi_8255_record_function_list = {
	mxi_8255_initialize_type,
	mxi_8255_create_record_structures,
	mxi_8255_finish_record_initialization,
	mxi_8255_delete_record,
	NULL,
	mxi_8255_read_parms_from_hardware,
	mxi_8255_write_parms_to_hardware,
	mxi_8255_open,
	mxi_8255_close,
	mxi_8255_finish_delayed_initialization
};

MX_GENERIC_FUNCTION_LIST mxi_8255_generic_function_list = {
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxi_8255_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_8255_STANDARD_FIELDS
};

long mxi_8255_num_record_fields
		= sizeof( mxi_8255_record_field_defaults )
			/ sizeof( mxi_8255_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_8255_rfield_def_ptr
			= &mxi_8255_record_field_defaults[0];

#define MXI_8255_DEBUG		FALSE

static mx_status_type
mxi_8255_get_pointers( MX_RECORD *record,
				MX_8255 **i8255,
				const char *calling_fname )
{
	const char fname[] = "mxi_8255_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( i8255 == (MX_8255 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_8255 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*i8255 = (MX_8255 *) (record->record_type_struct);

	if ( *i8255 == (MX_8255 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_8255 pointer for record '%s' is NULL.",
			record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=== Public functions ===*/

MX_EXPORT mx_status_type
mxi_8255_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_8255_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxi_8255_create_record_structures()";

	MX_GENERIC *generic;
	MX_8255 *i8255;
	int i;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	i8255 = (MX_8255 *) malloc( sizeof(MX_8255) );

	if ( i8255 == (MX_8255 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_8255 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = i8255;
	record->class_specific_function_list
				= &mxi_8255_generic_function_list;

	generic->record = record;
	i8255->record = record;

	for ( i = 0; i < MX_8255_NUM_PORTS; i++ ) {
		i8255->port_array[i] = NULL;
		i8255->port_value[i] = 0;
	}

	i8255->delayed_initialization_in_progress = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_8255_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_8255_delete_record( MX_RECORD *record )
{
        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }
        if ( record->record_type_struct != NULL ) {
                free( record->record_type_struct );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_8255_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_8255_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_8255_open( MX_RECORD *record )
{
	const char fname[] = "mxi_8255_open()";

	MX_8255 *i8255;
	int i;
	mx_status_type mx_status;

	mx_status = mxi_8255_get_pointers( record, &i8255, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if we have permission to write to the 8255's ports. */

	mx_status = mx_portio_request_region( i8255->portio_record,
						i8255->base_address, 4 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the 8255 to a known state, namely, mode 0 with all
	 * ports set for input.
	 */

	i8255->port_value[ MX_8255_PORT_A ] = 0;
	i8255->port_value[ MX_8255_PORT_B ] = 0;
	i8255->port_value[ MX_8255_PORT_C ] = 0;

	i8255->port_value[ MX_8255_PORT_D ] = MXF_8255_ACTIVE

		| MXF_8255_PORT_A_INPUT | MXF_8255_PORT_B_INPUT
		| MXF_8255_PORT_CL_INPUT | MXF_8255_PORT_CH_INPUT;

	for ( i = 0; i < MX_8255_MAX_RECORDS; i++ ) {
		i8255->port_array[i] = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_8255_close( MX_RECORD *record )
{
	const char fname[] = "mxi_8255_close()";

	MX_8255 *i8255;
	mx_status_type mx_status;

	mx_status = mxi_8255_get_pointers( record, &i8255, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_portio_release_region( i8255->portio_record,
						i8255->base_address, 4 );

	return mx_status;
}

/* We must delay initialization of the configuration port to the last
 * moment because each time we write to that port, all of the outputs
 * are reset to zero.  Thus, we need to write to the configuration
 * port only once.
 */

MX_EXPORT mx_status_type
mxi_8255_finish_delayed_initialization( MX_RECORD *record )
{
	const char fname[] = "mxi_8255_finish_delayed_initialization()";

	MX_8255 *i8255;
	mx_status_type mx_status;

	mx_status = mxi_8255_get_pointers( record, &i8255, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i8255->delayed_initialization_in_progress = FALSE;
	
	/* Write to the configuration port.  This will automatically
	 * update the output ports.
	 */

	mx_status = mxi_8255_write_port( i8255, MX_8255_PORT_D,
				i8255->port_value[ MX_8255_PORT_D ] );

	return mx_status;
}

/* === Driver specific functions === */

MX_EXPORT mx_status_type
mxi_8255_read_port( MX_8255 *i8255, int port_number, uint8_t *value )
{
	const char fname[] = "mxi_8255_read_port()";

	unsigned long port_address;
	int port_to_read;
	uint8_t value_read;

	port_address = 0;

	switch( port_number ) {
	case MX_8255_PORT_A:
	case MX_8255_PORT_B:
	case MX_8255_PORT_C:
	case MX_8255_PORT_D:
		port_to_read = port_number;
		break;
	case MX_8255_PORT_CL:
	case MX_8255_PORT_CH:
		port_to_read = MX_8255_PORT_C;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal port number %d.", port_number );
	}

	/* Read the value from the 8255 chip. */

	if ( port_number == MX_8255_PORT_D ) {
		/* We cannot read Port D from the hardware, so just return
		 * the last value we wrote.
		 */

		value_read = i8255->port_value[MX_8255_PORT_D];
	} else {
		port_address = i8255->base_address + port_to_read;

		value_read = mx_portio_inp8(
					i8255->portio_record, port_address );

		/* Save a copy of the value read. */

		i8255->port_value[ port_to_read ] = value_read;
	}

	/* Transform the value if necessary. */

	switch ( port_number ) {
	case MX_8255_PORT_CL:
		*value = ( value_read & 0x0f );
		break;

	case MX_8255_PORT_CH:
		*value = (( value_read & 0xf0 ) >> 4);
		break;
	default:
		*value = value_read;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_8255_write_port( MX_8255 *i8255, int port_number, uint8_t value )
{
	const char fname[] = "mxi_8255_write_port()";

	unsigned long port_address;
	int port_to_write;
	uint8_t value_to_write, old_port_c_value;

	switch( port_number ) {
	case MX_8255_PORT_A:
	case MX_8255_PORT_B:
	case MX_8255_PORT_C:
	case MX_8255_PORT_D:
		port_to_write = port_number;
		break;
	case MX_8255_PORT_CL:
	case MX_8255_PORT_CH:
		port_to_write = MX_8255_PORT_C;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal port number %d.", port_number );
	}

	port_address = i8255->base_address + port_to_write;

	/* Transform the value if necessary. */

	old_port_c_value = 0;

	switch( port_number ) {
	case MX_8255_PORT_CL:
		old_port_c_value = i8255->port_value[ MX_8255_PORT_C ];

		value_to_write = old_port_c_value & 0xf0;

		value_to_write |= ( value & 0x0f );
		break;
	case MX_8255_PORT_CH:
		old_port_c_value = i8255->port_value[ MX_8255_PORT_C ];

		value_to_write = old_port_c_value & 0x0f;

		value_to_write |= ( ( value << 4 ) & 0xf0 );
		break;
	default:
		value_to_write = ( value & 0xff );
		break;
	}

	/* Save a copy of the value we are sending to the 8255 chip. */

	i8255->port_value[ port_to_write ] = value_to_write;

	/* Now send the value to the 8255 chip.  We do not have a way
	 * of telling whether the write succeeded or not.
	 */

	if ( i8255->delayed_initialization_in_progress == FALSE ) {

		mx_portio_outp8( i8255->portio_record, port_address,
						value_to_write );

		if ( port_number == MX_8255_PORT_D ) {
			mxi_8255_update_outputs( i8255 );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mxi_8255_update_outputs( MX_8255 *i8255 )
{
	unsigned long port_address;
	uint8_t port_d_value, mask;

	port_d_value = i8255->port_value[ MX_8255_PORT_D ];

	/* Update port A if it is an output. */

	if ( (port_d_value & MXF_8255_PORT_A_INPUT) == 0 ) {

		port_address = i8255->base_address + MX_8255_PORT_A;

		mx_portio_outp8( i8255->portio_record, port_address,
					i8255->port_value[ MX_8255_PORT_A ] );
	}

	/* Update port B if it is an output. */

	if ( (port_d_value & MXF_8255_PORT_B_INPUT) == 0 ) {

		port_address = i8255->base_address + MX_8255_PORT_B;

		mx_portio_outp8( i8255->portio_record, port_address,
					i8255->port_value[ MX_8255_PORT_B ] );
	}

	/* Update port C if either half is an output. */

	mask = MXF_8255_PORT_CL_INPUT | MXF_8255_PORT_CH_INPUT;

	if ( mask & ~port_d_value ) {

		port_address = i8255->base_address + MX_8255_PORT_C;

		mx_portio_outp8( i8255->portio_record, port_address,
					i8255->port_value[ MX_8255_PORT_C ] );
	}

	return;
}

