/*
 * Name:    d_aps_adcmod2_amplifier.c
 *
 * Purpose: MX driver for the ADCMOD2 electrometer system designed by
 *          Steve Ross of the Advanced Photon Source.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_amplifier.h"

#include "i_aps_adcmod2.h"
#include "d_aps_adcmod2_amplifier.h"

/* Initialize the amplifier driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_aps_adcmod2_record_function_list = {
	NULL,
	mxd_aps_adcmod2_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_aps_adcmod2_open
};

MX_AMPLIFIER_FUNCTION_LIST mxd_aps_adcmod2_amplifier_function_list = {
	NULL,
	mxd_aps_adcmod2_set_gain,
	NULL,
	NULL,
	NULL,
	mxd_aps_adcmod2_set_time_constant,
	mxd_aps_adcmod2_get_parameter,
	mxd_aps_adcmod2_set_parameter
};

/* APS_ADCMOD2 amplifier data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_aps_adcmod2_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_APS_ADCMOD2_STANDARD_FIELDS
};

long mxd_aps_adcmod2_num_record_fields
		= sizeof( mxd_aps_adcmod2_record_field_defaults )
		  / sizeof( mxd_aps_adcmod2_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aps_adcmod2_rfield_def_ptr
			= &mxd_aps_adcmod2_record_field_defaults[0];

/* Private functions for the use of the driver. */

static mx_status_type
mxd_aps_adcmod2_get_pointers( MX_AMPLIFIER *amplifier,
			MX_APS_ADCMOD2_AMPLIFIER **aps_adcmod2_amplifier,
			MX_APS_ADCMOD2 **aps_adcmod2,
			const char *calling_fname )
{
	static const char fname[] = "mxd_aps_adcmod2_get_pointers()";

	MX_APS_ADCMOD2_AMPLIFIER *aps_adcmod2_amplifier_ptr;
	MX_RECORD *record, *aps_adcmod2_record;

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AMPLIFIER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = amplifier->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_AMPLIFIER structure "
		"passed by '%s' is NULL.", calling_fname );
	}

	aps_adcmod2_amplifier_ptr = (MX_APS_ADCMOD2_AMPLIFIER *)
						record->record_type_struct;

	if ( aps_adcmod2_amplifier_ptr == (MX_APS_ADCMOD2_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_APS_ADCMOD2_AMPLIFIER pointer for record '%s' is NULL.",
			record->name );
	}

	if ( aps_adcmod2_amplifier != (MX_APS_ADCMOD2_AMPLIFIER **) NULL ) {
		*aps_adcmod2_amplifier = aps_adcmod2_amplifier_ptr;
	}

	if ( aps_adcmod2 != (MX_APS_ADCMOD2 **) NULL ) {
		aps_adcmod2_record =
			aps_adcmod2_amplifier_ptr->aps_adcmod2_record;

		if ( aps_adcmod2_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The aps_adcmod2_record pointer for "
			"amplifier '%s' is NULL.", record->name );
		}

		*aps_adcmod2 = (MX_APS_ADCMOD2 *)
				aps_adcmod2_record->record_type_struct;

		if ( *aps_adcmod2 == (MX_APS_ADCMOD2 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_APS_ADCMOD2 pointer for record '%s' used by "
			"amplifier '%s' is NULL.", aps_adcmod2_record->name,
				record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_aps_adcmod2_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_aps_adcmod2_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_APS_ADCMOD2_AMPLIFIER *aps_adcmod2_amplifier;

	/* Allocate memory for the necessary structures. */

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AMPLIFIER structure." );
	}

	aps_adcmod2_amplifier = (MX_APS_ADCMOD2_AMPLIFIER *)
				malloc( sizeof(MX_APS_ADCMOD2_AMPLIFIER) );

	if ( aps_adcmod2_amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_APS_ADCMOD2_AMPLIFIER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = amplifier;
	record->record_type_struct = aps_adcmod2_amplifier;
	record->class_specific_function_list
			= &mxd_aps_adcmod2_amplifier_function_list;

	amplifier->record = record;

	/* The gain range for an APS_ADCMOD2 is from 1.76e11 to 1e12. */

	amplifier->gain_range[0] = 1.76e11;
	amplifier->gain_range[1] = 1.00e12;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_adcmod2_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_aps_adcmod2_open()";

	MX_AMPLIFIER *amplifier;
	MX_APS_ADCMOD2_AMPLIFIER *aps_adcmod2_amplifier;
	MX_APS_ADCMOD2 *aps_adcmod2;
	int i, num_times_to_loop;
	unsigned long delay_microseconds;
	mx_status_type mx_status;

	amplifier = (MX_AMPLIFIER *) (record->record_class_struct);

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_AMPLIFIER pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_aps_adcmod2_get_pointers( amplifier,
						&aps_adcmod2_amplifier,
						&aps_adcmod2, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_times_to_loop = 2;

	delay_microseconds = 1000;

	/* Set the initial gain. */

	for ( i = 0; i < num_times_to_loop; i++ ) {
		mx_status = mxd_aps_adcmod2_set_gain( amplifier );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_usleep( delay_microseconds );
	}

	/* Set the number of data points to 1. */

	mx_status = mxi_aps_adcmod2_command( aps_adcmod2, 0, 0x6, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_usleep( delay_microseconds );

	/* Tell the ADCMOD2 to ignore the pulse period
	 * by setting it to 0xffff.
	 */

	mx_status = mxi_aps_adcmod2_command( aps_adcmod2, 0, 0x7, 0xffff );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_usleep( delay_microseconds );

	/* Set the initial conversion time. */

	mx_status = mxd_aps_adcmod2_set_time_constant( amplifier );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_usleep( delay_microseconds );

	/* Start the ADCMOD2 in case it has not yet been started. */

	mx_status = mxi_aps_adcmod2_command( aps_adcmod2, 0, 0x4, 0x1 );

	mx_usleep( delay_microseconds );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_adcmod2_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_aps_adcmod2_set_gain()";

	MX_APS_ADCMOD2_AMPLIFIER *aps_adcmod2_amplifier;
	MX_APS_ADCMOD2 *aps_adcmod2;
	double gain;
	uint16_t gain_range;
	mx_status_type mx_status;

	/* The following magic ADU per picoamp values are from Steve Ross. */

	static const double adu_per_picoamp[8] =
		{ 1.0, 17.6, 8.8, 5.87, 4.40, 3.52, 2.93, 2.51 };

	mx_status = mxd_aps_adcmod2_get_pointers( amplifier,
						&aps_adcmod2_amplifier,
						&aps_adcmod2, fname);
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out which gain range to use. */

	gain = 1.0e-12 * amplifier->gain;

	if ( gain < 0.9999 * adu_per_picoamp[0] ) {
		gain_range = 0;

		mx_warning(
		"The gain of %g ADU/amp requested for amplifier '%s' "
		"is lower than the minimum allowed gain of %g ADU/amp.",
			amplifier->gain, amplifier->record->name,
			1.0e12 * adu_per_picoamp[0] );
	} else
	if ( gain > 1.0001 * adu_per_picoamp[1] ) {
		gain_range = 1;

		mx_warning(
		"The gain of %g ADU/amp requested for amplifier '%s' "
		"is higher than the maximum allowed gain of %g ADU/amp.",
			amplifier->gain, amplifier->record->name,
			1.0e12 * adu_per_picoamp[1] );
	} else
	if ( gain < sqrt( adu_per_picoamp[0] * adu_per_picoamp[7] ) ) {
		gain_range = 0;
	} else
	if ( gain < sqrt( adu_per_picoamp[6] * adu_per_picoamp[7] ) ) {
		gain_range = 7;
	} else
	if ( gain < sqrt( adu_per_picoamp[5] * adu_per_picoamp[6] ) ) {
		gain_range = 6;
	} else
	if ( gain < sqrt( adu_per_picoamp[4] * adu_per_picoamp[5] ) ) {
		gain_range = 5;
	} else
	if ( gain < sqrt( adu_per_picoamp[3] * adu_per_picoamp[4] ) ) {
		gain_range = 4;
	} else
	if ( gain < sqrt( adu_per_picoamp[2] * adu_per_picoamp[3] ) ) {
		gain_range = 3;
	} else
	if ( gain < sqrt( adu_per_picoamp[1] * adu_per_picoamp[2] ) ) {
		gain_range = 2;
	} else {
		gain_range = 1;
	}

	amplifier->gain = 1.0e12 * adu_per_picoamp[ gain_range ];

	/* Send the 'range' command. */

	mx_status = mxi_aps_adcmod2_command( aps_adcmod2,
					aps_adcmod2_amplifier->amplifier_number,
					0x1, gain_range );

	return mx_status;
}

#define ADCMOD2_SECONDS_FROM_HEX( h )	( (1.6e-6 * (double)(h)) + 0.6e-6 )

#define ADCMOD2_HEX_FROM_SECONDS( s )	( ((double)(s) - 0.6e-6) / 1.6e-6 )

#define ADCMOD2_MIN_SECONDS		ADCMOD2_SECONDS_FROM_HEX( 0x0180 )
#define ADCMOD2_MAX_SECONDS		ADCMOD2_SECONDS_FROM_HEX( 0x2000 )

MX_EXPORT mx_status_type
mxd_aps_adcmod2_set_time_constant( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_aps_adcmod2_set_time_constant()";

	MX_APS_ADCMOD2_AMPLIFIER *aps_adcmod2_amplifier;
	MX_APS_ADCMOD2 *aps_adcmod2;
	double time_constant;
	uint16_t conversion_time;
	mx_status_type mx_status;

	mx_status = mxd_aps_adcmod2_get_pointers( amplifier,
						&aps_adcmod2_amplifier,
						&aps_adcmod2, fname);
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	time_constant = amplifier->time_constant;

	if ( time_constant < ADCMOD2_MIN_SECONDS ) {
		amplifier->time_constant = ADCMOD2_MIN_SECONDS;

		mx_warning( "The time constant ( %g seconds ) requested for "
			"amplifier '%s' is too small.  The time constant will "
			"be set to the minimum allowed value of %g seconds.",
				time_constant, amplifier->record->name,
				amplifier->time_constant );

	} else if ( time_constant > ADCMOD2_MAX_SECONDS ) {
		amplifier->time_constant = ADCMOD2_MAX_SECONDS;

		mx_warning( "The time constant ( %g seconds ) requested for "
			"amplifier '%s' is too large.  The time constant will "
			"be set to the maximum allowed value of %g seconds.",
				time_constant, amplifier->record->name,
				amplifier->time_constant );
	}

	conversion_time = (uint16_t)
	      mx_round( ADCMOD2_HEX_FROM_SECONDS( amplifier->time_constant ) );

	/* Send the 'conv' command. */

	mx_status = mxi_aps_adcmod2_command( aps_adcmod2,
					aps_adcmod2_amplifier->amplifier_number,
					0x5, conversion_time );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_adcmod2_get_parameter( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_aps_adcmod2_get_parameter()";

	MX_APS_ADCMOD2_AMPLIFIER *aps_adcmod2_amplifier;
	MX_APS_ADCMOD2 *aps_adcmod2;
	mx_status_type mx_status;

	mx_status = mxd_aps_adcmod2_get_pointers( amplifier,
						&aps_adcmod2_amplifier,
						&aps_adcmod2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,
	("%s invoked for amplifier '%s' for parameter type '%s' (%ld).",
		fname, amplifier->record->name,
		mx_get_field_label_string( amplifier->record,
			amplifier->parameter_type ),
		amplifier->parameter_type));

	switch( amplifier->parameter_type ) {
	default:
		mx_status = mx_amplifier_default_get_parameter_handler(
				amplifier );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_adcmod2_set_parameter( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_aps_adcmod2_set_parameter()";

	MX_APS_ADCMOD2_AMPLIFIER *aps_adcmod2_amplifier;
	MX_APS_ADCMOD2 *aps_adcmod2;
	mx_status_type mx_status;

	mx_status = mxd_aps_adcmod2_get_pointers( amplifier,
						&aps_adcmod2_amplifier,
						&aps_adcmod2, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,
	("%s invoked for amplifier '%s' for parameter type '%s' (%ld).",
		fname, amplifier->record->name,
		mx_get_field_label_string( amplifier->record,
			amplifier->parameter_type ),
		amplifier->parameter_type));

	switch( amplifier->parameter_type ) {
	default:
		mx_status = mx_amplifier_default_set_parameter_handler(
				amplifier );
		break;
	}

	return mx_status;
}

