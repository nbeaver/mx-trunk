/*
 * Name:    d_p6000a.c
 *
 * Purpose: MX driver to read measurements from a Newport Electronics
 *          P6000A analog input.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define P6000A_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_rs232.h"
#include "d_p6000a.h"

#if 1
#include "mx_key.h"
#endif

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_p6000a_record_function_list = {
	NULL,
	mxd_p6000a_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_p6000a_open,
	mxd_p6000a_close
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_p6000a_analog_input_function_list =
{
	mxd_p6000a_read
};

/* P6000A analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_p6000a_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_P6000A_STANDARD_FIELDS
};

mx_length_type mxd_p6000a_num_record_fields
		= sizeof( mxd_p6000a_rf_defaults )
		  / sizeof( mxd_p6000a_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_p6000a_rfield_def_ptr
			= &mxd_p6000a_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_p6000a_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_P6000A **p6000a,
				const char *calling_fname )
{
	static const char fname[] = "mxd_p6000a_get_pointers()";

	MX_RECORD *record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( p6000a == (MX_P6000A **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_P6000A pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = ainput->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_ANALOG_INPUT pointer passed was NULL." );
	}

	*p6000a = (MX_P6000A *) record->record_type_struct;

	if ( (*p6000a) == (MX_P6000A *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_P6000A pointer for ainput '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_p6000a_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_p6000a_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_P6000A *p6000a;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	p6000a = (MX_P6000A *) malloc( sizeof(MX_P6000A) );

	if ( p6000a == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_P6000A structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = p6000a;
	record->class_specific_function_list
			= &mxd_p6000a_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_p6000a_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_p6000a_open()";

	MX_ANALOG_INPUT *ainput;
	MX_P6000A *p6000a;
	mx_status_type mx_status;

	ainput = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_p6000a_get_pointers( ainput, &p6000a, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_verify_configuration( p6000a->rs232_record,
					MXF_232_DONT_CARE, 7, 'E', 1, 'N',
					0x0d, 0x0d );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_warning(
	"The RS232 record '%s' for P6000A '%s' _must_ be configured for "
	"no flow control since the MX driver must directly control the "
	"state of the RTS line.", p6000a->rs232_record->name, record->name );

		return mx_status;
	}

	/* Turn off RTS. */

	mx_status = mx_rs232_set_signal_bit( p6000a->rs232_record,
					MXF_232_REQUEST_TO_SEND, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait a little bit for characters to arrive from the P6000A. */

	mx_msleep(100);

	/* Discard any characters that have arrived. */

	mx_status = mx_rs232_discard_unread_input( p6000a->rs232_record,
							P6000A_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_p6000a_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_p6000a_close()";

	MX_ANALOG_INPUT *ainput;
	MX_P6000A *p6000a;
	mx_status_type mx_status;

	ainput = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_p6000a_get_pointers( ainput, &p6000a, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn on RTS before exiting. */

	mx_status = mx_rs232_set_signal_bit( p6000a->rs232_record,
					MXF_232_REQUEST_TO_SEND, 1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_p6000a_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_p6000a_read()";

	MX_P6000A *p6000a;
	char message_buffer[80];
	char *value_ptr;
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_p6000a_get_pointers( ainput, &p6000a, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( p6000a->rs232_record,
							P6000A_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We request a measurement by turning on RTS. */

	mx_status = mx_rs232_set_signal_bit( p6000a->rs232_record,
					MXF_232_REQUEST_TO_SEND, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now wait for the next measurement to show up in the incoming
	 * RS232 buffer.
	 */

	mx_status = mx_rs232_getline( p6000a->rs232_record, message_buffer,
					sizeof(message_buffer)-1, NULL,
					P6000A_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off RTS. */

	mx_status = mx_rs232_set_signal_bit( p6000a->rs232_record,
					MXF_232_REQUEST_TO_SEND, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any trailing input. */

	mx_status = mx_rs232_discard_unread_input( p6000a->rs232_record,
							P6000A_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now look at the contents of the buffer returned above
	 * by mx_rs232_getline().
	 */

	value_ptr = message_buffer + 1;

	/* Null terminate the value received. */

	value_ptr[7] = '\0';

	/* Finish by parsing the value string. */

	num_items = sscanf(value_ptr, "%lg", &(ainput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cannot find the measurement value in the string '%s' "
		"sent by P6000A '%s'.",
			message_buffer, ainput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

