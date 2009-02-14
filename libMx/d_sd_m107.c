/*
 * Name:    d_sd_m107.c
 *
 * Purpose: MX output driver for the Systron-Donner M107 DC voltage source.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SD_M107_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_gpib.h"
#include "mx_analog_output.h"
#include "d_sd_m107.h"

/* Initialize the SD_M107 driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_sd_m107_record_function_list = {
	NULL,
	mxd_sd_m107_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_sd_m107_analog_output_function_list = {
	NULL,
	mxd_sd_m107_write
};

MX_RECORD_FIELD_DEFAULTS mxd_sd_m107_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_SD_M107_STANDARD_FIELDS
};

long mxd_sd_m107_num_record_fields
		= sizeof( mxd_sd_m107_record_field_defaults )
			/ sizeof( mxd_sd_m107_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sd_m107_rfield_def_ptr
			= &mxd_sd_m107_record_field_defaults[0];

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_sd_m107_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_sd_m107_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_SD_M107 *sd_m107;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        sd_m107 = (MX_SD_M107 *) malloc( sizeof(MX_SD_M107) );

        if ( sd_m107 == (MX_SD_M107 *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SD_M107 structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = sd_m107;
        record->class_specific_function_list
                                = &mxd_sd_m107_analog_output_function_list;

        analog_output->record = record;

	/* Raw analog output values are stored as longs. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sd_m107_write( MX_ANALOG_OUTPUT *dac )
{
	static const char fname[] = "mxd_sd_m107_write()";

	MX_SD_M107 *sd_m107;
	double voltage, abs_voltage;
	unsigned long normalized_voltage;
	int polarity, range;
	unsigned long digit0, digit1, digit2, digit3, digit4, digit5;
	char command[80];
	mx_status_type mx_status;

	sd_m107 = (MX_SD_M107 *) dac->record->record_type_struct;

	if ( sd_m107 == (MX_SD_M107 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SD_M107 pointer is NULL.");
	}

	/* Figure out what voltage range the request is in. */

	voltage = dac->raw_value.double_value;

	if ( voltage < 0.0 ) {
		polarity = 2;	/* negative polarity */
	} else {
		polarity = 1;   /* positive polarity */
	}

	abs_voltage = fabs(voltage);

	if ( abs_voltage >= 1000.0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested voltage of %g volts for analog output '%s' "
		"is outside the allowed range of -999.999 volts "
		"to +999.999 volts.",
			voltage, dac->record->name );
	} else
	if ( abs_voltage >= 100.0 ) {
		range = 3;

		normalized_voltage = mx_round( 1000.0 * abs_voltage );
	} else
	if ( abs_voltage >= 10.0 ) {
		range = 2;

		normalized_voltage = mx_round( 10000.0 * abs_voltage );
	} else
	if ( abs_voltage >= 1.0 ) {
		range = 1;

		normalized_voltage = mx_round( 100000.0 * abs_voltage );
	} else {
		range = 0;

		normalized_voltage = mx_round( 1000000.0 * abs_voltage );
	}

#if MXD_SD_M107_DEBUG
	MX_DEBUG(-2,
	("m107: voltage = %f, polarity = %d, range = %d, normalized = %lu",
		voltage, polarity, range, normalized_voltage));
#endif

	/* Construct a voltage command for the M107. */

	digit0 = normalized_voltage % 10; 
	normalized_voltage /= 10;

	digit1 = normalized_voltage % 10; 
	normalized_voltage /= 10;

	digit2 = normalized_voltage % 10; 
	normalized_voltage /= 10;

	digit3 = normalized_voltage % 10; 
	normalized_voltage /= 10;

	digit4 = normalized_voltage % 10; 
	normalized_voltage /= 10;

	digit5 = normalized_voltage % 10; 
	normalized_voltage /= 10;

	snprintf( command, sizeof(command),
	    "A%luB%luC%luD%luE%luF%luR%dP%dS",
	    digit5, digit4, digit3, digit2, digit1, digit0, range, polarity );

#if MXD_SD_M107_DEBUG
	MX_DEBUG(-2,("m107: command = '%s'", command));
#endif
		
	mx_status = mx_gpib_putline( sd_m107->gpib_record,
				sd_m107->address,
				command, NULL, MXD_SD_M107_DEBUG );

	return mx_status;
}

