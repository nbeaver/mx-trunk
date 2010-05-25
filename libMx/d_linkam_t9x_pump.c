/*
 * Name:    d_linkam_t9x_pump.c
 *
 * Purpose: MX output driver for the pump controller part of the Linkam T9x
 *          series of cooling controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_LINKAM_T9X_PUMP_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "i_linkam_t9x.h"
#include "d_linkam_t9x_pump.h"

/* Initialize the LINKAM_T9X_PUMP driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_linkam_t9x_pump_record_function_list = {
	NULL,
	mxd_linkam_t9x_pump_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_linkam_t9x_pump_analog_output_function_list = {
	mxd_linkam_t9x_pump_read,
	mxd_linkam_t9x_pump_write
};

MX_RECORD_FIELD_DEFAULTS mxd_linkam_t9x_pump_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_LINKAM_T9X_PUMP_STANDARD_FIELDS
};

long mxd_linkam_t9x_pump_num_record_fields
		= sizeof( mxd_linkam_t9x_pump_record_field_defaults )
			/ sizeof( mxd_linkam_t9x_pump_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_linkam_t9x_pump_rfield_def_ptr
			= &mxd_linkam_t9x_pump_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_linkam_t9x_pump_get_pointers( MX_ANALOG_OUTPUT *dac,
			MX_LINKAM_T9X_PUMP **linkam_t9x_pump,
			MX_LINKAM_T9X **linkam_t9x,
			const char *calling_fname )
{
	static const char fname[] = "mxd_linkam_t9x_pump_get_pointers()";

	MX_RECORD *linkam_t9x_record;

	if ( dac == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( linkam_t9x_pump == (MX_LINKAM_T9X_PUMP **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LINKAM_T9X_PUMP pointer passed was NULL." );
	}
	if ( linkam_t9x == (MX_LINKAM_T9X **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LINKAM_T9X pointer passed was NULL." );
	}

	if ( dac->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_RECORD pointer for MX_ANALOG_OUTPUT pointer %p is NULL.",
			dac );
	}

	*linkam_t9x_pump = dac->record->record_type_struct;

	if ( (*linkam_t9x_pump) == (MX_LINKAM_T9X_PUMP *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LINKAM_T9X_PUMP pointer for record '%s' is NULL.",
			dac->record->name );
	}

	linkam_t9x_record = (*linkam_t9x_pump)->linkam_t9x_record;

	if ( linkam_t9x_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The linkam_t9x_record pointer for record '%s' is NULL.",
				dac->record->name );
	}

	*linkam_t9x = (MX_LINKAM_T9X *) linkam_t9x_record->record_type_struct;

	if ( (*linkam_t9x) == (MX_LINKAM_T9X *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LINKAM_T9X pointer for record '%s' is NULL.",
			dac->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_linkam_t9x_pump_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_linkam_t9x_pump_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_LINKAM_T9X_PUMP *linkam_t9x_pump;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc( sizeof(MX_ANALOG_OUTPUT) );

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        linkam_t9x_pump = (MX_LINKAM_T9X_PUMP *)
				malloc( sizeof(MX_LINKAM_T9X_PUMP) );

        if ( linkam_t9x_pump == (MX_LINKAM_T9X_PUMP *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_LINKAM_T9X_PUMP structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = linkam_t9x_pump;
        record->class_specific_function_list
			= &mxd_linkam_t9x_pump_analog_output_function_list;

        analog_output->record = record;
	linkam_t9x_pump->record = record;

	/* Raw analog output values are stored as longs. */

	analog_output->subclass = MXT_AOU_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_pump_read( MX_ANALOG_OUTPUT *dac )
{
	static const char fname[] = "mxd_linkam_t9x_pump_read()";

	MX_LINKAM_T9X_PUMP *linkam_t9x_pump;
	MX_LINKAM_T9X *linkam_t9x;
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_pump_get_pointers( dac,
			&linkam_t9x_pump, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_linkam_t9x_get_status( linkam_t9x, 
						MXD_LINKAM_T9X_PUMP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dac->raw_value.long_value = linkam_t9x->pump_byte - 0x80;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_pump_write( MX_ANALOG_OUTPUT *dac )
{
	static const char fname[] = "mxd_linkam_t9x_pump_write()";

	MX_LINKAM_T9X_PUMP *linkam_t9x_pump;
	MX_LINKAM_T9X *linkam_t9x;
	long raw_value;
	int speed_char;
	char command[10];
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_pump_get_pointers( dac,
			&linkam_t9x_pump, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_value = dac->raw_value.long_value;

	if ( raw_value < 0 ) {

		/* If the raw output value is less than 0, put the pump
		 * into 'automatic' mode.
		 */

		mx_status = mxi_linkam_t9x_command( linkam_t9x, "Pa0",
					NULL, 0, MXD_LINKAM_T9X_PUMP_DEBUG );

		return mx_status;
	}

	/* Otherwise, put the pump in 'manual' mode. */

	mx_status = mxi_linkam_t9x_command( linkam_t9x, "Pm0",
					NULL, 0, MXD_LINKAM_T9X_PUMP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the pump speed. */

	if ( raw_value <= 9 ) {
		speed_char = '0' + raw_value;
	} else
	if ( raw_value <= 30 ) {
		speed_char = '0' + raw_value - 10;
	} else {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested pump speed %ld for Linkam T9x pump '%s' "
		"is larger than the maximum allowed value of 30.",
			raw_value, dac->record->name );
	}

	snprintf( command, sizeof(command), "P%c", speed_char );

	mx_status = mxi_linkam_t9x_command( linkam_t9x, command,
					NULL, 0, MXD_LINKAM_T9X_PUMP_DEBUG );

	return mx_status;
}

