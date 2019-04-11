/*
 * Name:    d_numato_gpio_doutput.c
 *
 * Purpose: MX driver to control Keithley 2400 series digital outputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006, 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define NUMATO_GPIO_DOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "i_keithley.h"
#include "i_numato_gpio.h"
#include "d_numato_gpio_doutput.h"

/* Initialize the doutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_numato_gpio_doutput_record_function_list = {
	NULL,
	mxd_numato_gpio_doutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_numato_gpio_doutput_open,
	mxd_numato_gpio_doutput_close
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_numato_gpio_doutput_digital_output_function_list =
{
	NULL,
	mxd_numato_gpio_doutput_write
};

/* Keithley 2400 digital output data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_numato_gpio_doutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_NUMATO_GPIO_DOUTPUT_STANDARD_FIELDS
};

long mxd_numato_gpio_doutput_num_record_fields
		= sizeof( mxd_numato_gpio_doutput_rf_defaults )
		  / sizeof( mxd_numato_gpio_doutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_numato_gpio_doutput_rfield_def_ptr
			= &mxd_numato_gpio_doutput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_numato_gpio_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
				MX_NUMATO_GPIO_DOUTPUT **numato_gpio_doutput,
				MX_NUMATO_GPIO **numato_gpio,
				MX_INTERFACE **interface,
				const char *calling_fname )
{
	static const char fname[] = "mxd_numato_gpio_doutput_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_NUMATO_GPIO_DOUTPUT *numato_gpio_doutput_ptr;
	MX_NUMATO_GPIO *numato_gpio_ptr;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = doutput->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_DIGITAL_OUTPUT pointer passed was NULL." );
	}

	numato_gpio_doutput_ptr = (MX_NUMATO_GPIO_DOUTPUT *)
					record->record_type_struct;

	if ( numato_gpio_doutput_ptr == (MX_NUMATO_GPIO_DOUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NUMATO_GPIO_DOUTPUT pointer for doutput '%s' is NULL.",
			record->name );
	}

	if ( numato_gpio_doutput != (MX_NUMATO_GPIO_DOUTPUT **) NULL ) {
		*numato_gpio_doutput = numato_gpio_doutput_ptr;
	}

	controller_record = numato_gpio_doutput_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for doutput '%s' is NULL.",
				record->name );
	}

	numato_gpio_ptr = (MX_NUMATO_GPIO *)
					controller_record->record_type_struct;

	if ( numato_gpio_ptr == (MX_NUMATO_GPIO *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_NUMATO_GPIO pointer for controller record '%s' is NULL.",
				controller_record->name );
	}

	if ( numato_gpio != (MX_NUMATO_GPIO **) NULL ) {
		*numato_gpio = numato_gpio_ptr;
	}

	if ( interface != (MX_INTERFACE **) NULL ) {
		*interface = &(numato_gpio_ptr->port_interface);

		if ( *interface == (MX_INTERFACE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INTERFACE pointer for Keithley 2400 '%s' is NULL.",
				numato_gpio_ptr->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_numato_gpio_doutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_numato_gpio_doutput_create_record_structures()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_NUMATO_GPIO_DOUTPUT *numato_gpio_doutput;

	/* Allocate memory for the necessary structures. */

	doutput = (MX_DIGITAL_OUTPUT *) malloc( sizeof(MX_DIGITAL_OUTPUT) );

	if ( doutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
	}

	numato_gpio_doutput = (MX_NUMATO_GPIO_DOUTPUT *)
				malloc( sizeof(MX_NUMATO_GPIO_DOUTPUT) );

	if ( numato_gpio_doutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NUMATO_GPIO_DOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = doutput;
	record->record_type_struct = numato_gpio_doutput;
	record->class_specific_function_list
		= &mxd_numato_gpio_doutput_digital_output_function_list;

	doutput->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_numato_gpio_doutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_numato_gpio_doutput_open()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_NUMATO_GPIO_DOUTPUT *numato_gpio_doutput;
	MX_NUMATO_GPIO *numato_gpio;
	MX_INTERFACE *interface;
	long gpib_address;
	mx_status_type mx_status;

	doutput = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_numato_gpio_doutput_get_pointers( doutput,
		&numato_gpio_doutput, &numato_gpio, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( interface->record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' used by Keithley 2400 doutput '%s' is not "
		"an interface record.", interface->record->name, record->name );
	}

	switch( interface->record->mx_class ) {
	case MXI_RS232:
		break;
	case MXI_GPIB:
		gpib_address = interface->address;

		/* Check that the GPIB address is valid. */

		if ( gpib_address < 0 || gpib_address > 30 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"GPIB address %ld for record '%s' is out of allowed range 0-30.",
				gpib_address, record->name );
		}
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' is not an RS-232 or GPIB record.",
			interface->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_numato_gpio_doutput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_numato_gpio_doutput_close()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_NUMATO_GPIO_DOUTPUT *numato_gpio_doutput;
	MX_NUMATO_GPIO *numato_gpio;
	MX_INTERFACE *interface;
	mx_status_type mx_status;

	doutput = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_numato_gpio_doutput_get_pointers( doutput,
		&numato_gpio_doutput, &numato_gpio, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_numato_gpio_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_numato_gpio_doutput_write()";

	MX_NUMATO_GPIO_DOUTPUT *numato_gpio_doutput;
	MX_NUMATO_GPIO *numato_gpio;
	MX_INTERFACE *interface;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_numato_gpio_doutput_get_pointers( doutput,
		&numato_gpio_doutput, &numato_gpio, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "SOUR2:TTL %lu", doutput->value );

	mx_status = mxi_keithley_command( doutput->record, interface, command,
					NULL, 0, NUMATO_GPIO_DOUTPUT_DEBUG );

	return mx_status;
}

