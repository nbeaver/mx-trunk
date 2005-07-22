/*
 * Name:    i_am9513.c
 *
 * Purpose: MX interface driver for Am9513 counter/timer chips.
 *
 *          Please note that the driver has only been fully implemented
 *          and tested for Am9513s using an 8-bit bus.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "mx_unistd.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_generic.h"
#include "mx_portio.h"

#include "i_am9513.h"

MX_RECORD_FUNCTION_LIST mxi_am9513_record_function_list = {
	mxi_am9513_initialize_type,
	mxi_am9513_create_record_structures,
	mxi_am9513_finish_record_initialization,
	mxi_am9513_delete_record,
	mxi_am9513_print_structure,
	mxi_am9513_read_parms_from_hardware,
	mxi_am9513_write_parms_to_hardware,
	mxi_am9513_open,
	mxi_am9513_close,
	NULL,
	mxi_am9513_resynchronize
};

MX_GENERIC_FUNCTION_LIST mxi_am9513_generic_function_list = {
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxi_am9513_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_AM9513_STANDARD_FIELDS
};

long mxi_am9513_num_record_fields
		= sizeof( mxi_am9513_record_field_defaults )
			/ sizeof( mxi_am9513_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_am9513_rfield_def_ptr
			= &mxi_am9513_record_field_defaults[0];

/* The structure mxi_am9513_debug_struct is used by the MX_AM9513_DUMP
 * macro and nothing else.
 */

MX_AM9513 mxi_am9513_debug_struct;

/* --- */

#define mxi_am9513_16bit_bus( am9513 ) \
		( (am9513)->master_mode_register & 0x2000 )

static mx_status_type
mxi_am9513_get_pointers( MX_RECORD *record,
			MX_AM9513 **am9513,
			const char *calling_fname )
{
	const char fname[] = "mxi_am9513_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( am9513 == (MX_AM9513 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AM9513 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*am9513 = (MX_AM9513 *) (record->record_type_struct);

	if ( *am9513 == (MX_AM9513 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AM9513 pointer for record '%s' is NULL.",
			record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=== Public functions ===*/

MX_EXPORT mx_uint8_type
mxi_am9513_inp8( MX_AM9513 *am9513, int port_number )
{
	const char fname[] = "mxi_am9513_inp8()";

	mx_uint8_type result;
	unsigned long port_address;

	if ( am9513 == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL am9513 pointer passed." );
		return (-1);
	}
	if ( port_number < 0 || port_number > 1 ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal port number %d for Am9513 record '%s'",
				port_number, am9513->record->name );
		return (-1);
	}

	port_address = am9513->base_address + port_number;

	result = mx_portio_inp8( am9513->portio_record, port_address );

	MX_AM9513_DELAY;

	return result;
}

MX_EXPORT mx_uint16_type
mxi_am9513_inp16( MX_AM9513 *am9513, int port_number )
{
	const char fname[] = "mxi_am9513_inp16()";

	mx_uint16_type result;
	unsigned long port_address;

	if ( am9513 == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL am9513 pointer passed." );
		return (-1);
	}
	if ( port_number < 0 || port_number > 1 ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal port number %d for Am9513 record '%s'",
				port_number, am9513->record->name );
		return (-1);
	}

	port_address = am9513->base_address + port_number;

	if ( mxi_am9513_16bit_bus( am9513 ) ) {
		result = mx_portio_inp16(am9513->portio_record, port_address);
	} else {
		result = mx_portio_inp8(am9513->portio_record, port_address);
		result &= 0xff;

		MX_AM9513_DELAY;

		result
		  += (mx_portio_inp8(am9513->portio_record, port_address) << 8);
	}

	MX_AM9513_DELAY;

	return result;
}

MX_EXPORT void
mxi_am9513_outp8( MX_AM9513 *am9513, int port_number,
				mx_uint8_type byte_value )
{
	const char fname[] = "mxi_am9513_outp8()";

	unsigned long port_address;

	if ( am9513 == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL am9513 pointer passed." );
		return;
	}
	if ( port_number < 0 || port_number > 1 ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal port number %d for Am9513 record '%s'",
				port_number, am9513->record->name );
		return;
	}

	port_address = am9513->base_address + port_number;

	mx_portio_outp8( am9513->portio_record, port_address, byte_value );

	MX_AM9513_DELAY;

	return;
}

MX_EXPORT void
mxi_am9513_outp16( MX_AM9513 *am9513, int port_number,
				mx_uint16_type word_value )
{
	const char fname[] = "mxi_am9513_outp16()";

	unsigned long port_address;

	if ( am9513 == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL am9513 pointer passed." );
		return;
	}
	if ( port_number < 0 || port_number > 1 ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal port number %d for Am9513 record '%s'",
				port_number, am9513->record->name );
		return;
	}

	port_address = am9513->base_address + port_number;

	if ( mxi_am9513_16bit_bus( am9513 ) ) {
		mx_portio_outp16( am9513->portio_record, port_address,
						word_value );
	} else {
		mx_portio_outp8( am9513->portio_record, port_address,
				(mx_uint8_type) (word_value & 0xff) );

		MX_AM9513_DELAY;

		mx_portio_outp8( am9513->portio_record, port_address,
				(mx_uint8_type) (( word_value >> 8 ) & 0xff) );
	}

	MX_AM9513_DELAY;

	return;
}

/* ====== */

MX_EXPORT mx_status_type
mxi_am9513_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxi_am9513_create_record_structures()";

	MX_GENERIC *generic;
	MX_AM9513 *am9513;
	int i;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	am9513 = (MX_AM9513 *) malloc( sizeof(MX_AM9513) );

	if ( am9513 == (MX_AM9513 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_AM9513 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = am9513;
	record->class_specific_function_list
				= &mxi_am9513_generic_function_list;

	generic->record = record;
	am9513->record = record;

	for ( i = 0; i < MX_AM9513_NUM_COUNTERS; i++ ) {
		am9513->counter_array[i] = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_delete_record( MX_RECORD *record )
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
mxi_am9513_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxi_am9513_print_structure()";

	MX_AM9513 *am9513;
	MX_RECORD *this_record;
	int i;
	mx_status_type mx_status;

	mx_status = mxi_am9513_get_pointers( record, &am9513, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "Record '%s':\n\n", record->name);
	fprintf(file, "  name                       = \"%s\"\n",
						record->name);

	fprintf(file, "  superclass                 = interface\n");
	fprintf(file, "  class                      = generic\n");
	fprintf(file, "  type                       = AM9513\n");
	fprintf(file, "  label                      = \"%s\"\n",
						record->label);

	fprintf(file, "  portio record              = \"%s\"\n",
						am9513->portio_record->name);

	fprintf(file, "  base address               = %#lx\n",
						am9513->base_address);

	fprintf(file, "  master mode register       = %#lx\n",
						am9513->master_mode_register);

	fprintf(file, "  counters in use            = (");

	for ( i = 0; i < MX_AM9513_NUM_COUNTERS; i++ ) {

		this_record = am9513->counter_array[i];

		if ( this_record == NULL ) {
			fprintf( file, "NULL" );
		} else {
			fprintf( file, "%s", this_record->name );
		}
		if ( i != (MX_AM9513_NUM_COUNTERS - 1) ) {
			fprintf( file, ", " );
		}
	}

	fprintf(file, ")\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_open( MX_RECORD *record )
{
	const char fname[] = "mxi_am9513_open()";

	MX_AM9513 *am9513;
	mx_uint16_type master_mode_register;
	int i;
	mx_status_type mx_status;

	mx_status = mxi_am9513_get_pointers( record, &am9513, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the debugging structure. */

	mxi_am9513_debug_struct.record = NULL;
	mxi_am9513_debug_struct.portio_record = am9513->portio_record;
	mxi_am9513_debug_struct.base_address = am9513->base_address;

	/* Check to see if we have permission to write to the Am9513's ports.*/

	mx_status = mx_portio_request_region( am9513->portio_record,
						am9513->base_address, 2 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reset the Am9513. */

	if ( mxi_am9513_16bit_bus( am9513 ) ) {
		mx_warning(
	"The Am9513 driver has never been tested with a 16-bit bus.");
		mx_warning(
	"It probably does not work with a 16-bit bus yet.");

		mxi_am9513_outp16( am9513, MX_AM9513_CMD_REGISTER, 0xffff );
	} else {
		mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, 0xff );
	}

	/* The AMD documentation for the Am9513 says that it is necessary
	 * to send a 'dummy' load counter command here in order to
	 * "insure proper operation of the part".
	 */

	if ( mxi_am9513_16bit_bus( am9513 ) ) {
		mxi_am9513_outp16( am9513, MX_AM9513_CMD_REGISTER, 0xff5f );
	} else {
		mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, 0x5f );
	}

	/* Set the master mode register.  The Am9513 will be in 8-bit bus
	 * mode at this point, so we need to send the master mode register
	 * in 8 bit chunks.
	 */

	master_mode_register = (mx_uint16_type)
				( am9513->master_mode_register & 0xffff );

	if ( mxi_am9513_16bit_bus( am9513 ) ) {
		mxi_am9513_outp16( am9513, MX_AM9513_CMD_REGISTER, 0xff17 );

		mxi_am9513_outp16( am9513, MX_AM9513_DATA_REGISTER,
			(mx_uint16_type)(master_mode_register & 0xffff) );

		mxi_am9513_outp16( am9513, MX_AM9513_DATA_REGISTER,
		(mx_uint16_type)(( master_mode_register >> 8 ) & 0xffff));
	} else {
		mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, 0x17 );

		mxi_am9513_outp8( am9513, MX_AM9513_DATA_REGISTER,
			(mx_uint8_type)(master_mode_register & 0xff) );

		mxi_am9513_outp8( am9513, MX_AM9513_DATA_REGISTER,
			(mx_uint8_type)(( master_mode_register >> 8 ) & 0xff));
	}

	/* Set default parameters for the counters. */

	for ( i = 0; i < MX_AM9513_NUM_COUNTERS; i++ ) {

		/* Mode register: Count up, count repetitively. */

		am9513->counter_mode_register[i] = 0x28;

		mx_status = mxi_am9513_set_counter_mode_register( am9513, i );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		am9513->load_register[i] = 0x0000;

		mx_status = mxi_am9513_load_counter( am9513, i );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		am9513->hold_register[i] = 0x0000;

		mx_status = mxi_am9513_set_hold_register( am9513, i );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	MX_AM9513_DEBUG( am9513, 1 );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_close( MX_RECORD *record )
{
	const char fname[] = "mxi_am9513_close()";

	MX_AM9513 *am9513;
	mx_status_type mx_status;

	mx_status = mxi_am9513_get_pointers( record, &am9513, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_portio_release_region( am9513->portio_record,
						am9513->base_address, 2 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_am9513_resynchronize( MX_RECORD *record )
{
	mx_status_type status;

	status = mxi_am9513_close( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxi_am9513_open( record );

	return status;
}

/* === Driver specific functions === */

MX_EXPORT mx_status_type
mxi_am9513_grab_counters( MX_RECORD *record,
		long num_counters,
		MX_INTERFACE *am9513_interface_array )
{
	const char fname[] = "mxi_am9513_grab_counters()";

	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	MX_RECORD **counter_array_element;
	long i, n;

	MX_DEBUG( 2, ("%s called.", fname));

	/* Let the interface records know that we are using these counters. */

	for ( i = 0; i < num_counters; i++ ) {

		this_record = am9513_interface_array[i].record;

		if ( this_record->mx_type != MXI_GEN_AM9513 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record '%s' is not an Am9513 interface record.",
				this_record->name );
		}

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		if ( this_am9513 == (MX_AM9513 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_AM9513 pointer for Am9513 interface record '%s' is NULL.",
				this_record->name );
		}

		n = am9513_interface_array[i].address - 1;

		counter_array_element = &( this_am9513->counter_array[n] );

		if ( *counter_array_element != NULL ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Error configuring record '%s':  "
	"Counter %ld in interface '%s' is already in use by record '%s'", 
				record->name, n+1, this_record->name,
				(*counter_array_element)->name );
		}

		/* Install the pointer to this record. */

		*counter_array_element = record;
	}

	MX_DEBUG( 2, ("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_release_counters( MX_RECORD *record,
		long num_counters,
		MX_INTERFACE *am9513_interface_array )
{
	const char fname[] = "mxi_am9513_release_counters()";

	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	MX_RECORD **counter_array_element;
	long i, n;

	MX_DEBUG( 2, ("%s called.", fname));

	/* Let the interface records know that we are using these counters. */

	for ( i = 0; i < num_counters; i++ ) {

		this_record = am9513_interface_array[i].record;

		if ( this_record->mx_type != MXI_GEN_AM9513 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record '%s' is not an Am9513 interface record.",
				this_record->name );
		}

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		if ( this_am9513 == (MX_AM9513 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_AM9513 pointer for Am9513 interface record '%s' is NULL.",
				this_record->name );
		}

		n = am9513_interface_array[i].address - 1;

		counter_array_element = &( this_am9513->counter_array[n] );

		if ( *counter_array_element == NULL ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error removing record '%s':  "
			"Attempting to release counter %ld in interface '%s' "
			"which is already marked as free.",
				record->name, n+1, this_record->name );
		}

		/* Delete the pointer to this record. */

		*counter_array_element = NULL;
	}

	MX_DEBUG( 2, ("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT char *
mxi_am9513_dump( MX_AM9513 *am9513, int do_inquire )
{
	static char returned_buffer[1000];
	char buffer[80];
	int i;

	sprintf( returned_buffer, "am9513 = %p, do_inquire = %d\n",
					am9513, do_inquire );

	if ( do_inquire ) {
		mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, 0x17 );

		am9513->master_mode_register = mxi_am9513_inp16( am9513,
						MX_AM9513_DATA_REGISTER );
	}

	sprintf( buffer, "Master mode reg = %04lx\n",
					am9513->master_mode_register );
	strcat( returned_buffer, buffer );

	for ( i = 0; i < 5; i++ ) {
		if ( do_inquire ) {
			(void) mxi_am9513_get_counter_mode_register(am9513, i);
		}

		sprintf( buffer, "Counter[%d] mode = %04x\n",
					i, am9513->counter_mode_register[i] );
		strcat( returned_buffer, buffer );
	}
	for ( i = 0; i < 5; i++ ) {
		if ( do_inquire ) {
			(void) mxi_am9513_get_load_register( am9513, i );
		}

		sprintf( buffer, "Counter[%d] load = %04x\n",
					i, am9513->load_register[i] );
		strcat( returned_buffer, buffer );
	}
	for ( i = 0; i < 5; i++ ) {
		if ( do_inquire ) {
			(void) mxi_am9513_get_hold_register( am9513, i );
		}

		sprintf( buffer, "Counter[%d] hold = %04x\n",
					i, am9513->hold_register[i] );
		strcat( returned_buffer, buffer );
	}

	if ( do_inquire ) {
		(void) mxi_am9513_get_status( am9513 );
	}

	sprintf( buffer, "Status          = %02x\n", am9513->status_register );
	strcat( returned_buffer, buffer );

	return &returned_buffer[0];
}

MX_EXPORT mx_status_type
mxi_am9513_get_counter_mode_register( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = counter + 1;

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	am9513->counter_mode_register[ counter ] = mxi_am9513_inp16( am9513,
						MX_AM9513_DATA_REGISTER );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_set_counter_mode_register( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = counter + 1;

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	mxi_am9513_outp16( am9513, MX_AM9513_DATA_REGISTER,
		(mx_uint16_type) am9513->counter_mode_register[ counter ] );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_get_load_register( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = counter + 9;

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	am9513->load_register[ counter ] = mxi_am9513_inp16( am9513,
						MX_AM9513_DATA_REGISTER );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_set_load_register( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = counter + 9;

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	mxi_am9513_outp16( am9513, MX_AM9513_DATA_REGISTER,
			(mx_uint16_type) am9513->load_register[ counter ] );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_load_counter_from_load_register( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = 0x40 + ( 1 << counter );

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_load_counter( MX_AM9513 *am9513, int counter )
{
	mx_status_type mx_status;

	mx_status = mxi_am9513_set_load_register( am9513, counter );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_am9513_load_counter_from_load_register(
							am9513, counter );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_am9513_get_hold_register( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = counter + 0x11;

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	am9513->hold_register[ counter ] = mxi_am9513_inp16( am9513,
						MX_AM9513_DATA_REGISTER );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_set_hold_register( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = counter + 0x11;

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	mxi_am9513_outp16( am9513, MX_AM9513_DATA_REGISTER,
			(mx_uint16_type) am9513->hold_register[ counter ] );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_save_counter_to_hold_register( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = 0xa0 + ( 1 << counter );

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_read_counter( MX_AM9513 *am9513, int counter )
{
	mx_status_type mx_status;

	mx_status = mxi_am9513_save_counter_to_hold_register( am9513, counter );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_am9513_get_hold_register( am9513, counter );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_am9513_set_tc( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = counter + 0xe9;

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_clear_tc( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = counter + 0xe1;

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_arm_counter( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = 0x20 + ( 1 << counter );

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_disarm_counter( MX_AM9513 *am9513, int counter )
{
	mx_uint8_type cmd;

	cmd = 0x80 + ( 1 << counter );

	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, cmd );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_am9513_get_status( MX_AM9513 *am9513 )
{
	mxi_am9513_outp8( am9513, MX_AM9513_CMD_REGISTER, 0x1f );

	am9513->status_register = mxi_am9513_inp8( am9513,
					MX_AM9513_DATA_REGISTER );

	return MX_SUCCESSFUL_RESULT;
}

