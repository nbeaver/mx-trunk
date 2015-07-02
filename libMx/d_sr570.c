/*
 * Name:    d_sr570.c
 *
 * Purpose: MX driver for the Stanford Research Systems Model SR570
 *          current amplifier.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006, 2008, 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define SR570_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_amplifier.h"
#include "d_sr570.h"

/* Initialize the amplifier driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_sr570_record_function_list = {
	NULL,
	mxd_sr570_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_sr570_open,
	NULL,
	NULL,
	mxd_sr570_open,
	mxd_sr570_special_processing_setup
};

MX_AMPLIFIER_FUNCTION_LIST mxd_sr570_amplifier_function_list = {
	NULL,
	mxd_sr570_set_gain,
	NULL,
	mxd_sr570_set_offset,
	NULL,
	mxd_sr570_set_time_constant,
	mxd_sr570_get_parameter,
	mxd_sr570_set_parameter
};

/* SR570 amplifier data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_sr570_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_SR570_STANDARD_FIELDS
};

long mxd_sr570_num_record_fields
		= sizeof( mxd_sr570_record_field_defaults )
		  / sizeof( mxd_sr570_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sr570_rfield_def_ptr
			= &mxd_sr570_record_field_defaults[0];

/* Private functions for the use of the driver. */

static mx_status_type mxd_sr570_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

static mx_status_type
mxd_sr570_get_pointers( MX_AMPLIFIER *amplifier,
				MX_SR570 **sr570,
				const char *calling_fname )
{
	static const char fname[] = "mxd_sr570_get_pointers()";

	MX_RECORD *record;

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AMPLIFIER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( sr570 == (MX_SR570 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SR570 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = amplifier->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_AMPLIFIER structure "
		"passed by '%s' is NULL.", calling_fname );
	}

	*sr570 = (MX_SR570 *) record->record_type_struct;

	if ( *sr570 == (MX_SR570 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SR570 pointer for record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static void
mxd_sr570_round_gain_or_offset( double requested_value,
			int *rounded_mantissa,
			int *rounded_exponent )
{
	static const char fname[] = "mxd_sr570_round_gain_or_offset()";

	double mantissa, exponent, fractional_exponent;

	MX_DEBUG( 2,("%s: requested_value = %g", fname, requested_value));

	exponent = log10( requested_value );

	/* Round to the next most negative exponent. */

	if ( exponent >= 0.0 ) {
		*rounded_exponent = (int) exponent;
	} else {
		*rounded_exponent = -1 + (int) exponent;
	}

	fractional_exponent = exponent - (double) *rounded_exponent;

	MX_DEBUG( 2,("%s: exponent = %g", fname, exponent ));

	MX_DEBUG( 2,
	("%s: first rounded_exponent = %d, fractional_exponent = %g",
		fname, *rounded_exponent, fractional_exponent));

#define MXD_CROSSOVER_POINT_BETWEEN_5_AND_10	( 0.849485 )

	/* Please note that 0.849485 = log  ( sqrt(5 * 10) )
	 *                                10
	 * is the base 10 logarithm of the geometrical mean of 5 and 10.
	 */

	if ( fractional_exponent >= MXD_CROSSOVER_POINT_BETWEEN_5_AND_10 ) {
		(*rounded_exponent) ++;
	}

	MX_DEBUG( 2,("%s: final rounded_exponent = %d",
				fname, *rounded_exponent));

	mantissa = requested_value / pow( 10.0, (double) *rounded_exponent );

	/* The allowed values of the mantissa are 1, 2, and 5.  Use the
	 * geometrical mean to round to one of those values.
	 */

	if ( mantissa <= sqrt(2.0) ) {
		*rounded_mantissa = 1;
	} else
	if ( mantissa <= sqrt(10.0) ) {
		*rounded_mantissa = 2;
	} else {
		*rounded_mantissa = 5;
	}

	MX_DEBUG( 2,
("%s: requested_value = %g, rounded_mantissa = %d, rounded_exponent = %d",
		fname, requested_value, *rounded_mantissa, *rounded_exponent));

	return;
}

static void
mxd_sr570_round_filter_3db_point( double requested_value,
			int *rounded_mantissa,
			int *rounded_exponent )
{
	static const char fname[] = "mxd_sr570_round_filter_3db_point()";

	double mantissa, exponent, fractional_exponent;

	MX_DEBUG( 2,("%s: requested_value = %g", fname, requested_value));

	exponent = log10( requested_value );

	/* Round to the next most negative exponent. */

	if ( exponent >= 0.0 ) {
		*rounded_exponent = (int) exponent;
	} else {
		*rounded_exponent = -1 + (int) exponent;
	}

	fractional_exponent = exponent - (double) *rounded_exponent;

	MX_DEBUG( 2,("%s: exponent = %g", fname, exponent));

	MX_DEBUG( 2,
	("%s: first rounded_exponent = %d, fractional_exponent = %g",
		fname, *rounded_exponent, fractional_exponent));

#define MXD_CROSSOVER_POINT_BETWEEN_3_AND_10	0.738561

	/* Please note that 0.738561 = log  ( sqrt(3 * 10) )
	 *                                10
	 * is the base 10 logarithm of the geometric mean of 3 and 10.
	 */

	if ( fractional_exponent >= MXD_CROSSOVER_POINT_BETWEEN_3_AND_10 ) {
		(*rounded_exponent) ++;
	}

	MX_DEBUG( 2,("%s: final rounded_exponent = %d",
				fname, *rounded_exponent));

	mantissa = requested_value / pow( 10.0, (double) *rounded_exponent );

	/* The allowed values of the mantissa are 1, and 3.  Use the
	 * geometrical mean to round to one of those values.
	 */

	if ( mantissa <= sqrt(3.0) ) {
		*rounded_mantissa = 1;
	} else {
		*rounded_mantissa = 3;
	}

	MX_DEBUG( 2,
("%s: requested_value = %g, rounded_mantissa = %d, rounded_exponent = %d",
		fname, requested_value, *rounded_mantissa, *rounded_exponent));

	return;
}

/* === */

MX_EXPORT mx_status_type
mxd_sr570_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_sr570_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_SR570 *sr570 = NULL;

	/* Allocate memory for the necessary structures. */

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AMPLIFIER structure." );
	}

	sr570 = (MX_SR570 *) malloc( sizeof(MX_SR570) );

	if ( sr570 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SR570 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = amplifier;
	record->record_type_struct = sr570;
	record->class_specific_function_list
			= &mxd_sr570_amplifier_function_list;

	amplifier->record = record;

	/* The gain range for an SR570 is from 1e3 to 1e12. */

	amplifier->gain_range[0] = 1.0e3;
	amplifier->gain_range[1] = 1.0e12;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sr570_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sr570_open()";

	MX_AMPLIFIER *amplifier;
	MX_SR570 *sr570 = NULL;
	mx_status_type mx_status;

	amplifier = (MX_AMPLIFIER *) (record->record_class_struct);

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_AMPLIFIER pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_sr570_get_pointers(amplifier, &sr570, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear the RS-232 port buffers. */

	mx_status = mx_rs232_discard_unread_input( sr570->rs232_record,
							SR570_DEBUG );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;
	default:
		return mx_status;
	}

	mx_status = mx_rs232_discard_unwritten_output( sr570->rs232_record,
							SR570_DEBUG );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;
	default:
		return mx_status;
	}

	/* Reset the SR570 so that it is in a known state. */

	mx_status = mx_rs232_putline( sr570->rs232_record,
					"*RST", NULL, SR570_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the old settings to the default values. */

	sr570->old_gain = 1.0e6;
	sr570->old_offset = 0.0;
	sr570->old_bias_voltage = 0.0;
	sr570->old_filter_type = 5;
	sr570->old_lowpass_filter_3db_point = 1.0e6;
	sr570->old_highpass_filter_3db_point = 0.03;
	sr570->old_gain_mode = 0;
	sr570->old_invert_signal = 0;
	sr570->old_blank_output = 0;

	/* Set the sensitivity to calibrated mode. */

	mx_status = mx_rs232_putline( sr570->rs232_record,
					"SUCM 0", NULL, SR570_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the input offset to calibrated mode. */

	mx_status = mx_rs232_putline( sr570->rs232_record,
					"IOUC 0", NULL, SR570_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the gain and offset.
	 *
	 * Setting the gain also sets the offset as a side effect,
	 * so we do not need to explicitly set the offset here.
	 */

	mx_status = mxd_sr570_set_gain( amplifier );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** Set all the remaining SR570 specific parameters. ***/

	mx_status = mx_amplifier_set_parameter( record,
					MXLV_SR570_BIAS_VOLTAGE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_set_parameter( record,
					MXLV_SR570_FILTER_TYPE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_set_parameter( record,
					MXLV_SR570_LOWPASS_FILTER_3DB_POINT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_set_parameter( record,
					MXLV_SR570_HIGHPASS_FILTER_3DB_POINT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_set_parameter( record,
					MXLV_SR570_RESET_FILTER );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_set_parameter( record,
					MXLV_SR570_GAIN_MODE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_set_parameter( record,
					MXLV_SR570_INVERT_SIGNAL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_set_parameter( record,
					MXLV_SR570_BLANK_OUTPUT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sr570_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_sr570_set_gain()";

	MX_SR570 *sr570 = NULL;
	char command[20];
	double requested_gain;
	int mantissa, exponent, sensitivity_setting;
	mx_status_type mx_status;

	mx_status = mxd_sr570_get_pointers( amplifier, &sr570, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	requested_gain = amplifier->gain;

	MX_DEBUG( 2,("%s: requested_gain = %g, requested sens = %g",
		fname, requested_gain, mx_divide_safely(1.0, requested_gain) ));

	/* Attempting to set the gain to zero or a negative value is illegal. */

	if ( amplifier->gain <= 0.0 ) {
		amplifier->gain = sr570->old_gain;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested gain of %g for amplifier '%s' is illegal.  "
	"The amplifier gain must be a positive number.",
			requested_gain, amplifier->record->name );
	}

	/* Force the gain to the nearest allowed value and store the
	 * rounded value in amplifier->gain.
	 */

	mxd_sr570_round_gain_or_offset( amplifier->gain,
						&mantissa, &exponent );

	amplifier->gain = ((double) mantissa) * pow( 10.0, (double) exponent );

	MX_DEBUG( 2,("%s: rounded gain = %g, rounded sens = %g",
	fname, amplifier->gain, mx_divide_safely(1.0, amplifier->gain) ));

	/* Compute the new gain setting depending on the mantissa and
	 * exponent obtained above.
	 */

	if ( mantissa == 1 ) {
		sensitivity_setting = 0;
	} else if ( mantissa == 2 ) {
		sensitivity_setting = -1;
	} else {
		sensitivity_setting = -2;
	}

	sensitivity_setting += 3 * ( 12 - exponent );

	MX_DEBUG( 2,("%s: sensitivity_setting = %d",
		fname, sensitivity_setting));

	/* Is this a legal sensitivity setting? */

	if ( ( sensitivity_setting < 0 )
	  || ( sensitivity_setting > 27 ) )
	{
		amplifier->gain = sr570->old_gain;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested gain of %g for amplifier '%s' is outside "
	"the allowed range is 1.0e3 to 1.0e12. ",
			requested_gain,
			amplifier->record->name );
	}

	/* Set the new gain. */

	snprintf( command, sizeof(command),
			"SENS %d", sensitivity_setting );

	mx_status = mx_rs232_putline( sr570->rs232_record,
					command, NULL, SR570_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the input offset current level to match the new gain range. */

	mx_status = mxd_sr570_set_offset( amplifier );

	if ( mx_status.code != MXE_SUCCESS ) {
		amplifier->gain = sr570->old_gain;
	} else {
		sr570->old_gain = amplifier->gain;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sr570_set_offset( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_sr570_set_offset()";

	MX_SR570 *sr570 = NULL;
	char command[40];
	double offset_current, abs_offset_current, requested_offset;
	double max_offset_current, min_offset_current;
	double max_offset_voltage;
	int mantissa, exponent, sign_offset_current, offset_current_setting;
	mx_status_type mx_status;

	mx_status = mxd_sr570_get_pointers( amplifier, &sr570, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	requested_offset = amplifier->offset;

	/* Specify the minimum and maximum offset currents in amps. */

	max_offset_current = 5.0e-03;
	min_offset_current = 1.0e-12;

	max_offset_voltage = max_offset_current * amplifier->gain;

	/* Compute the input offset current in amps. */

	offset_current = - mx_divide_safely( amplifier->offset,
						amplifier->gain );

	abs_offset_current = fabs( offset_current );

	if ( offset_current < 0.0 ) {
		sign_offset_current = -1;
	} else {
		sign_offset_current = 1;
	}

	/* An offset current near 0 is treated as a special case. */

	if ( abs_offset_current < min_offset_current ) {

		/* Since we are closer to zero than the smallest allowed
		 * input offset current, turn off the input offset instead.
		 */

		mx_status = mx_rs232_putline( sr570->rs232_record,
						"IOON 0", NULL, SR570_DEBUG );

		if ( mx_status.code != MXE_SUCCESS ) {
			amplifier->offset = sr570->old_offset;
		} else {
			amplifier->offset = 0.0;
		}
		return mx_status;
	}

	/* Force the input offset current to the nearest allowed value
	 * and save the rounded value in amplifier->offset.
	 */

	mxd_sr570_round_gain_or_offset( abs_offset_current,
						&mantissa, &exponent );

	MX_DEBUG( 2,("%s: amplifier->offset = %g", fname, amplifier->offset));

	amplifier->offset = sign_offset_current * amplifier->gain
			* ((double) mantissa) * pow( 10.0, (double) exponent );

	MX_DEBUG( 2,("%s: amplifier->gain = %g, mantissa = %d, exponent = %d",
		fname, amplifier->gain, mantissa, exponent ));

	MX_DEBUG( 2,("%s: new amplifier->offset = %g",
			fname, amplifier->offset));

	/* Compute the new input offset current setting depending on
	 * the mantissa and exponent obtained above.
	 */

	if ( mantissa == 1 ) {
		offset_current_setting = 0;
	} else
	if ( mantissa == 2 ) {
		offset_current_setting = 1;
	} else {
		offset_current_setting = 2;
	}

	offset_current_setting += 3 * ( 12 + exponent );

	MX_DEBUG( 2,("%s: offset_current_setting = %d",
		fname, offset_current_setting));

	/* Is this a legal offset current setting? */

	if ( ( offset_current_setting < 0 )
	  || ( offset_current_setting > 29 ) )
	{
		amplifier->offset = sr570->old_offset;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested amplifier offset of %g volts for amplifier '%s' is "
	"outside the allowed range of %g volts to %g volts.",
			requested_offset, amplifier->record->name,
			-max_offset_voltage, max_offset_voltage );
	}

	/* Set the magnitude of the input offset current. */

	snprintf( command, sizeof(command),
			"IOLV %d", offset_current_setting );

	mx_status = mx_rs232_putline( sr570->rs232_record,
					command, NULL, SR570_DEBUG );

	if ( mx_status.code != MXE_SUCCESS ) {
		amplifier->offset = sr570->old_offset;
		return mx_status;
	}

	/* Set the sign of the input offset current. */

	if ( sign_offset_current == 1 ) {
		strlcpy( command, "IOSN 1", sizeof(command) );
	} else {
		strlcpy( command, "IOSN 0", sizeof(command) );
	}

	mx_status = mx_rs232_putline( sr570->rs232_record,
					command, NULL, SR570_DEBUG );

	if ( mx_status.code != MXE_SUCCESS ) {
		amplifier->offset = sr570->old_offset;
		return mx_status;
	}

	/* Turn on the input offset current. */

	mx_status = mx_rs232_putline( sr570->rs232_record,
					"IOON 1", NULL, SR570_DEBUG );

	if ( mx_status.code != MXE_SUCCESS ) {
		amplifier->offset = sr570->old_offset;
	} else {
		sr570->old_offset = amplifier->offset;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sr570_set_time_constant( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_sr570_set_time_constant()";

	MX_SR570 *sr570 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sr570_get_pointers( amplifier, &sr570, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	amplifier->time_constant = 0.0;

	return mx_error( MXE_UNSUPPORTED, fname,
	"The '%s' driver for amplifier '%s' does not support "
	"filter rise time constants.  You must control the filter via the "
	"'filter_type', 'lowpass_filter_3db_point', and "
	"'highpass_filter_3db_point' parameters instead.",
		mx_get_driver_name( amplifier->record ),
		amplifier->record->name );
}

MX_EXPORT mx_status_type
mxd_sr570_get_parameter( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_sr570_get_parameter()";

	MX_SR570 *sr570 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sr570_get_pointers( amplifier, &sr570, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for amplifier '%s' for parameter type '%s' (%ld).",
		fname, amplifier->record->name,
		mx_get_field_label_string( amplifier->record,
			amplifier->parameter_type ),
		amplifier->parameter_type));

	switch( amplifier->parameter_type ) {
	case MXLV_SR570_BIAS_VOLTAGE:
	case MXLV_SR570_FILTER_TYPE:
	case MXLV_SR570_LOWPASS_FILTER_3DB_POINT:
	case MXLV_SR570_HIGHPASS_FILTER_3DB_POINT:
	case MXLV_SR570_RESET_FILTER:
	case MXLV_SR570_GAIN_MODE:
	case MXLV_SR570_INVERT_SIGNAL:
	case MXLV_SR570_BLANK_OUTPUT:
		/* Just return the values in memory for all of these
		 * parameter.
		 */
		break;

	default:
		return mx_amplifier_default_get_parameter_handler( amplifier );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sr570_set_parameter( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_sr570_set_parameter()";

	MX_SR570 *sr570 = NULL;
	char command[80];
	int bias_voltage_setting, three_db_point_setting;
	int mantissa, exponent;
	long saved_integer;
	double saved_double;
	mx_status_type mx_status;

	mx_status = mxd_sr570_get_pointers( amplifier, &sr570, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for amplifier '%s' for parameter type '%s' (%ld).",
		fname, amplifier->record->name,
		mx_get_field_label_string( amplifier->record,
			amplifier->parameter_type ),
		amplifier->parameter_type));

	switch( amplifier->parameter_type ) {
	case MXLV_SR570_BIAS_VOLTAGE:
		bias_voltage_setting = 
			(int) mx_round( 1000.0 * sr570->bias_voltage );

		if ( abs( bias_voltage_setting ) > 5000 ) {
			saved_double = sr570->bias_voltage;

			sr570->bias_voltage = sr570->old_bias_voltage;

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested bias voltage of %g volts for amplifier '%s' is outside "
	"the allowed range of -5.0 volts to +5.0 volts.",
				saved_double,
				amplifier->record->name );
		}

		sr570->bias_voltage = 0.001 * (double) bias_voltage_setting;

		/* Set the bias voltage level. */

		snprintf( command, sizeof(command),
			"BSLV %d", bias_voltage_setting );

		mx_status = mx_rs232_putline( sr570->rs232_record,
						command, NULL, SR570_DEBUG );

		if ( mx_status.code != MXE_SUCCESS ) {
			sr570->bias_voltage = sr570->old_bias_voltage;
			return mx_status;
		}

		/* If the level is zero, turn off the bias voltage.
		 * Otherwise, turn it on.
		 */

		if ( bias_voltage_setting == 0 ) {
			strlcpy( command, "BSON 0", sizeof(command) );
		} else {
			strlcpy( command, "BSON 1", sizeof(command) );
		}

		mx_status = mx_rs232_putline( sr570->rs232_record,
						command, NULL, SR570_DEBUG );

		if ( mx_status.code != MXE_SUCCESS ) {
			sr570->bias_voltage = 0.0;
		} else {
			sr570->old_bias_voltage = sr570->bias_voltage;
		}
		break;

	case MXLV_SR570_FILTER_TYPE:
		if ( ( sr570->filter_type < 0 ) || ( sr570->filter_type > 5 ) )
		{
			saved_integer = sr570->filter_type;

			sr570->filter_type = sr570->old_filter_type;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested filter type %ld for amplifier '%s' is outside "
	"the allowed range of 0 to 5.",
				saved_integer,
				amplifier->record->name );
		}

		/* Set the filter type. */

		snprintf( command, sizeof(command),
			"FLTT %ld", sr570->filter_type );

		mx_status = mx_rs232_putline( sr570->rs232_record,
						command, NULL, SR570_DEBUG );

		if ( mx_status.code != MXE_SUCCESS ) {
			sr570->filter_type = sr570->old_filter_type;
		} else {
			sr570->old_filter_type = sr570->filter_type;
		}
		break;

	case MXLV_SR570_LOWPASS_FILTER_3DB_POINT:

		if ( sr570->lowpass_filter_3db_point < 0.02 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested lowpass filter 3dB point of %g Hz "
			"for amplifier '%s' is outside the allowed range "
			"of 0.03 Hz to 1.0e6 Hz.",
				sr570->lowpass_filter_3db_point,
				amplifier->record->name );
		}

		/* Force the 3dB point to the nearest setting. */

		mxd_sr570_round_filter_3db_point(
				sr570->lowpass_filter_3db_point,
				&mantissa, &exponent );

		if ( mantissa == 3 ) {
			three_db_point_setting = 0;
		} else {
			three_db_point_setting = -1;
		}

		three_db_point_setting += 2 * ( 2 + exponent );

		if ( ( three_db_point_setting < 0 )
		  || ( three_db_point_setting > 15 ) )
		{
			saved_double = sr570->lowpass_filter_3db_point;

			sr570->lowpass_filter_3db_point
				= sr570->old_lowpass_filter_3db_point;

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested lowpass filter 3dB point of %g Hz "
	"for amplifier '%s' is outside the allowed range "
	"of 0.03 Hz to 1.0e6 Hz.",
				saved_double,
				amplifier->record->name );
		}

		sr570->lowpass_filter_3db_point = ( (double) mantissa )
					* pow( 10.0, (double) exponent );

		/* Set the filter 3db point. */

		snprintf( command, sizeof(command),
			"LFRQ %d", three_db_point_setting );

		mx_status = mx_rs232_putline( sr570->rs232_record,
						command, NULL, SR570_DEBUG );

		if ( mx_status.code != MXE_SUCCESS ) {
			sr570->lowpass_filter_3db_point
				= sr570->old_lowpass_filter_3db_point;
		} else {
			sr570->old_lowpass_filter_3db_point
				= sr570->lowpass_filter_3db_point;
		}
		break;

	case MXLV_SR570_HIGHPASS_FILTER_3DB_POINT:

		if ( sr570->highpass_filter_3db_point < 0.02 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested highpass filter 3dB point of %g Hz "
			"for amplifier '%s' is outside the alhighed range "
			"of 0.03 Hz to 1.0e6 Hz.",
				sr570->highpass_filter_3db_point,
				amplifier->record->name );
		}

		/* Force the 3dB point to the nearest setting. */

		mxd_sr570_round_filter_3db_point(
				sr570->highpass_filter_3db_point,
				&mantissa, &exponent );

		if ( mantissa == 3 ) {
			three_db_point_setting = 0;
		} else {
			three_db_point_setting = -1;
		}

		three_db_point_setting += 2 * ( 2 + exponent );

		if ( ( three_db_point_setting < 0 )
		  || ( three_db_point_setting > 11 ) )
		{
			saved_double = sr570->highpass_filter_3db_point;

			sr570->highpass_filter_3db_point
				= sr570->old_highpass_filter_3db_point;

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested highpass filter 3dB point of %g Hz "
	"for amplifier '%s' is outside the allowed range "
	"of 0.03 Hz to 1.0e4 Hz.",
				saved_double,
				amplifier->record->name );
		}

		sr570->highpass_filter_3db_point = ( (double) mantissa )
					* pow( 10.0, (double) exponent );

		/* Set the filter 3db point. */

		snprintf( command, sizeof(command),
			"HFRQ %d", three_db_point_setting );

		mx_status = mx_rs232_putline( sr570->rs232_record,
						command, NULL, SR570_DEBUG );

		if ( mx_status.code != MXE_SUCCESS ) {
			sr570->highpass_filter_3db_point
				= sr570->old_highpass_filter_3db_point;
		} else {
			sr570->old_highpass_filter_3db_point
				= sr570->highpass_filter_3db_point;
		}
		break;

	case MXLV_SR570_RESET_FILTER:
		/* A value of zero is ignored. */

		if ( sr570->reset_filter != 0 ) {
			mx_status = mx_rs232_putline( sr570->rs232_record,
						"ROLD", NULL, SR570_DEBUG );
		}

		sr570->reset_filter = 0;
		break;

	case MXLV_SR570_GAIN_MODE:
		if ( ( sr570->gain_mode < 0 ) || ( sr570->gain_mode > 2 ) )
		{
			saved_integer = sr570->gain_mode;

			sr570->gain_mode = sr570->old_gain_mode;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested gain mode %ld for amplifier '%s' is outside "
	"the allowed range of 0 to 2.",
				saved_integer,
				amplifier->record->name );
		}

		/* Set the gain mode. */

		snprintf( command, sizeof(command),
			"GNMD %ld", sr570->gain_mode );

		mx_status = mx_rs232_putline( sr570->rs232_record,
						command, NULL, SR570_DEBUG );

		if ( mx_status.code != MXE_SUCCESS ) {
			sr570->gain_mode = sr570->old_gain_mode;
		} else {
			sr570->old_gain_mode = sr570->gain_mode;
		}
		break;

	case MXLV_SR570_INVERT_SIGNAL:
		if ( ( sr570->invert_signal < 0 )
		  || ( sr570->invert_signal > 1 ) )
		{
			saved_integer = sr570->invert_signal;

			sr570->invert_signal = sr570->old_invert_signal;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested invert signal value %ld for amplifier '%s' is "
	"not one of the allowed values, namely, 0 or 1.",
				saved_integer,
				amplifier->record->name );
		}

		snprintf( command, sizeof(command),
			"INVT %ld", sr570->invert_signal );

		mx_status = mx_rs232_putline( sr570->rs232_record,
						command, NULL, SR570_DEBUG );

		if ( mx_status.code != MXE_SUCCESS ) {
			sr570->invert_signal = sr570->old_invert_signal;
		} else {
			sr570->old_invert_signal = sr570->invert_signal;
		}
		break;

	case MXLV_SR570_BLANK_OUTPUT:
		if ( ( sr570->blank_output < 0 )
		  || ( sr570->blank_output > 1 ) )
		{
			saved_integer = sr570->blank_output;

			sr570->blank_output = sr570->old_blank_output;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested blank output value %ld for amplifier '%s' is "
	"not one of the allowed values, namely, 0 or 1.",
				saved_integer,
				amplifier->record->name );
		}

		snprintf( command, sizeof(command),
			"BLNK %ld", sr570->blank_output );

		mx_status = mx_rs232_putline( sr570->rs232_record,
						command, NULL, SR570_DEBUG );

		if ( mx_status.code != MXE_SUCCESS ) {
			sr570->blank_output = sr570->old_blank_output;
		} else {
			sr570->old_blank_output = sr570->blank_output;
		}
		break;

	default:
		mx_status = mx_amplifier_default_set_parameter_handler(
				amplifier );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sr570_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxd_sr570_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_SR570_BIAS_VOLTAGE:
		case MXLV_SR570_FILTER_TYPE:
		case MXLV_SR570_LOWPASS_FILTER_3DB_POINT:
		case MXLV_SR570_HIGHPASS_FILTER_3DB_POINT:
		case MXLV_SR570_RESET_FILTER:
		case MXLV_SR570_GAIN_MODE:
		case MXLV_SR570_INVERT_SIGNAL:
		case MXLV_SR570_BLANK_OUTPUT:
			record_field->process_function
					= mxd_sr570_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxd_sr570_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxd_sr570_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_SR570_BIAS_VOLTAGE:
		case MXLV_SR570_FILTER_TYPE:
		case MXLV_SR570_LOWPASS_FILTER_3DB_POINT:
		case MXLV_SR570_HIGHPASS_FILTER_3DB_POINT:
		case MXLV_SR570_RESET_FILTER:
		case MXLV_SR570_GAIN_MODE:
		case MXLV_SR570_INVERT_SIGNAL:
		case MXLV_SR570_BLANK_OUTPUT:
			/* Nothing to do since the necessary values are
			 * already stored in the data structures.
			 */

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_SR570_BIAS_VOLTAGE:
		case MXLV_SR570_FILTER_TYPE:
		case MXLV_SR570_LOWPASS_FILTER_3DB_POINT:
		case MXLV_SR570_HIGHPASS_FILTER_3DB_POINT:
		case MXLV_SR570_RESET_FILTER:
		case MXLV_SR570_GAIN_MODE:
		case MXLV_SR570_INVERT_SIGNAL:
		case MXLV_SR570_BLANK_OUTPUT:
			mx_status = mx_amplifier_set_parameter( record,
						record_field->label_value );

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

