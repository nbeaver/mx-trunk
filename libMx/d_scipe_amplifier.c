/*
 * Name:    d_scipe_amplifier.c
 *
 * Purpose: MX driver for Stanford Research Systems SR570 current amplifiers
 *          controlled via a DND-CAT SCIPE server.
 *
 * Author:  William Lavender and Steven Weigand
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define SCIPE_AMPLIFIER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_amplifier.h"
#include "i_scipe.h"
#include "d_scipe_amplifier.h"

/* Initialize the amplifier driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_scipe_amplifier_record_function_list = {
	NULL,
	mxd_scipe_amplifier_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_scipe_amplifier_open,
	NULL,
	NULL,
	mxd_scipe_amplifier_open,
	mxd_scipe_amplifier_special_processing_setup
};

MX_AMPLIFIER_FUNCTION_LIST mxd_scipe_amplifier_amplifier_function_list = {
	mxd_scipe_amplifier_get_gain,
	mxd_scipe_amplifier_set_gain,
	mxd_scipe_amplifier_get_offset,
	mxd_scipe_amplifier_set_offset,
	NULL,
	mxd_scipe_amplifier_set_time_constant,
	mxd_scipe_amplifier_get_parameter,
	mxd_scipe_amplifier_set_parameter
};

/* SCIPE_AMPLIFIER amplifier data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_scipe_amplifier_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_SCIPE_AMPLIFIER_STANDARD_FIELDS
};

mx_length_type mxd_scipe_amplifier_num_record_fields
		= sizeof( mxd_scipe_amplifier_record_field_defaults )
		  / sizeof( mxd_scipe_amplifier_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_scipe_amplifier_rfield_def_ptr
			= &mxd_scipe_amplifier_record_field_defaults[0];

/* Private functions for the use of the driver. */

#if 0
static mx_status_type mxd_scipe_amplifier_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );
#endif

static mx_status_type
mxd_scipe_amplifier_get_pointers( MX_AMPLIFIER *amplifier,
			MX_SCIPE_AMPLIFIER **scipe_amplifier,
			MX_SCIPE_SERVER **scipe_server,
			const char *calling_fname )
{
	const char fname[] = "mxd_scipe_amplifier_get_pointers()";

	MX_RECORD *scipe_server_record;

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AMPLIFIER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scipe_amplifier == (MX_SCIPE_AMPLIFIER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCIPE_AMPLIFIER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scipe_server == (MX_SCIPE_SERVER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCIPE_SERVER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( amplifier->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_AMPLIFIER pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*scipe_amplifier = (MX_SCIPE_AMPLIFIER *)
				amplifier->record->record_type_struct;

	if ( *scipe_amplifier == (MX_SCIPE_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCIPE_AMPLIFIER pointer for record '%s' is NULL.",
			amplifier->record->name );
	}

	scipe_server_record = (*scipe_amplifier)->scipe_server_record;

	if ( scipe_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'scipe_server_record' pointer for record '%s' is NULL.",
			amplifier->record->name );
	}

	*scipe_server = (MX_SCIPE_SERVER *)
				scipe_server_record->record_type_struct;

	if ( *scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SCIPE_SERVER pointer for the SCIPE server '%s' used by '%s' is NULL.",
			scipe_server_record->name, amplifier->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static void
mxd_scipe_amplifier_round_gain_or_offset( double requested_value,
			int *rounded_mantissa,
			int *rounded_exponent )
{
	static const char fname[] =
			"mxd_scipe_amplifier_round_gain_or_offset()";

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
mxd_scipe_amplifier_compute_filter_3db_point( int integer_setting,
					double *filter_frequency )
{
	static const char fname[] =
			"mxd_scipe_amplifier_compute_filter_3db_point()";

	double mantissa, exponent;

	MX_DEBUG( 2,("%s: integer_setting = %d", fname, integer_setting));

	if ( ( integer_setting < 0 ) || ( integer_setting > 15 ) ) {
		mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
		"The reported filter 3dB point setting %d "
		"is outside the allowed range of 0-15.",
			integer_setting );

		*filter_frequency = 0.0;
		return;
	}

	if ( ( integer_setting % 2 ) == 0 ) {
		mantissa = 3.0;
	} else {
		mantissa = 1.0;
	}

	/* Please note that following formula relies on the truncation of 
	 * values that occurs during integer division.
	 */

	exponent = (double) ( -2 + (  ( integer_setting + 1 ) / 2 ) );

	*filter_frequency = mantissa * pow( 10.0, exponent );

	return;
}

static void
mxd_scipe_amplifier_round_filter_3db_point( double requested_value,
			int *rounded_mantissa,
			int *rounded_exponent )
{
	static const char fname[] =
			"mxd_scipe_amplifier_round_filter_3db_point()";

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
mxd_scipe_amplifier_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_scipe_amplifier_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_SCIPE_AMPLIFIER *scipe_amplifier;

	/* Allocate memory for the necessary structures. */

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AMPLIFIER structure." );
	}

	scipe_amplifier = (MX_SCIPE_AMPLIFIER *)
				malloc( sizeof(MX_SCIPE_AMPLIFIER) );

	if ( scipe_amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCIPE_AMPLIFIER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = amplifier;
	record->record_type_struct = scipe_amplifier;
	record->class_specific_function_list
			= &mxd_scipe_amplifier_amplifier_function_list;

	amplifier->record = record;

	/* The gain range for an SCIPE_AMPLIFIER is from 1e3 to 1e12. */

	amplifier->gain_range[0] = 1.0e3;
	amplifier->gain_range[1] = 1.0e12;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_amplifier_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_scipe_amplifier_open()";

	MX_AMPLIFIER *amplifier;
	MX_SCIPE_AMPLIFIER *scipe_amplifier;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	amplifier = (MX_AMPLIFIER *) (record->record_class_struct);

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_AMPLIFIER pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_scipe_amplifier_get_pointers(amplifier,
					&scipe_amplifier, &scipe_server, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out whether or not the SCIPE server knows about this
	 * amplifier.
	 */

	sprintf( command, "%s_gain desc",
				scipe_amplifier->scipe_amplifier_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_amplifier_get_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_scipe_amplifier_get_gain()";

	MX_SCIPE_AMPLIFIER *scipe_amplifier;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int num_items, scipe_response_code;
	int sensitivity_setting, remainder_value, exponent;
	double sensitivity;
	mx_status_type mx_status;

	mx_status = mxd_scipe_amplifier_get_pointers(amplifier,
					&scipe_amplifier, &scipe_server, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the sensitivity setting (SENS) from the remote SCIPE server. */

	sprintf( command, "%s_gain position",
			scipe_amplifier->scipe_amplifier_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, amplifier->record->name, response );
	}

	num_items = sscanf( result_ptr, "%d", &sensitivity_setting );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unrecognizable response received from "
	"SCIPE server '%s' for SCIPE amplifier '%s'.  "
	"Original command = '%s', server response = '%s'",
			scipe_server->record->name,
			amplifier->record->name,
			command, response );
	}

	if ( ( sensitivity_setting < 0 ) || ( sensitivity_setting > 27 ) ) {
		return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
	"The reported sensitivity setting %d for SCIPE amplifier '%s' "
	"is outside the allowed range of values from 0 to 27.",
			sensitivity_setting, amplifier->record->name );
	}

	/* The division by 3 below relies on the truncation produced by
	 * integer division.
	 */

	exponent = (int) ( sensitivity_setting / 3 );

	remainder_value = sensitivity_setting % 3;

	sensitivity = pow( 10.0, exponent ) * 1e-12;

	if ( remainder_value == 1 ) {
		sensitivity *= 2.0;
	} else if ( remainder_value == 2 ) {
		sensitivity *= 5.0;
	}

	MX_DEBUG( 2,("%s: exponent = %d, remainder_value = %d sensitivity = %g",
		fname, exponent, remainder_value, sensitivity));


	/* The amplifier gain is the reciprocal of the sensitivity. */

	amplifier->gain = mx_divide_safely( 1.0, sensitivity );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_amplifier_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_scipe_amplifier_set_gain()";

	MX_SCIPE_AMPLIFIER *scipe_amplifier;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	double requested_gain;
	int mantissa, exponent, sensitivity_setting;
	mx_status_type mx_status;

	mx_status = mxd_scipe_amplifier_get_pointers(amplifier,
					&scipe_amplifier, &scipe_server, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	requested_gain = amplifier->gain;

	MX_DEBUG( 2,("%s: requested_gain = %g, requested sens = %g",
		fname, requested_gain, mx_divide_safely(1.0, requested_gain) ));

	/* Attempting to set the gain to zero or a negative value is illegal. */

	if ( amplifier->gain <= 0.0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested gain of %g for amplifier '%s' is illegal.  "
	"The amplifier gain must be a positive number.",
			requested_gain, amplifier->record->name );
	}

	/* Force the gain to the nearest allowed value and store the
	 * rounded value in amplifier->gain.
	 */

	mxd_scipe_amplifier_round_gain_or_offset( amplifier->gain,
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
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested gain of %g for amplifier '%s' is outside "
	"the allowed range is 1.0e3 to 1.0e12. ",
			requested_gain,
			amplifier->record->name );
	}

	/* Set the sensitivity calibration mode to calibrated (SUCM0). */

	sprintf( command, "%s_calibration_mode movenow %d",
			scipe_amplifier->scipe_amplifier_name, 0 );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, amplifier->record->name, response );
	}

	/* Set the new gain. */

	sprintf( command, "%s_gain movenow %d",
		scipe_amplifier->scipe_amplifier_name, sensitivity_setting );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, amplifier->record->name, response );
	}

	/* Update the input offset current level to match the new gain range. */

	mx_status = mxd_scipe_amplifier_set_offset( amplifier );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_scipe_amplifier_get_offset( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_scipe_amplifier_get_offset()";

	MX_SCIPE_AMPLIFIER *scipe_amplifier;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int num_items, scipe_response_code, offset_toggle;
	int offset_setting, remainder_value, exponent;
	double input_offset_current;
	mx_status_type mx_status;

	mx_status = mxd_scipe_amplifier_get_pointers(amplifier,
					&scipe_amplifier, &scipe_server, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the input offset current setting (IOLV) from the remote
	 * SCIPE server.
	 */

	sprintf( command, "%s_calibrated_offset_current position",
			scipe_amplifier->scipe_amplifier_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, amplifier->record->name, response );
	}

	num_items = sscanf( result_ptr, "%d", &offset_setting );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unrecognizable response received from "
	"SCIPE server '%s' for SCIPE amplifier '%s'.  "
	"Original command = '%s', server response = '%s'",
			scipe_server->record->name,
			amplifier->record->name,
			command, response );
	}

	/* Read the input offset current toggle (IOON) from the remote
	 * SCIPE server.
	 */

	sprintf( command, "%s_toggle_offset_current position",
			scipe_amplifier->scipe_amplifier_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, amplifier->record->name, response );
	}

	num_items = sscanf( result_ptr, "%d", &offset_toggle );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unrecognizable response received from "
	"SCIPE server '%s' for SCIPE amplifier '%s'.  "
	"Original command = '%s', server response = '%s'",
			scipe_server->record->name,
			amplifier->record->name,
			command, response );
	}

	if ( ( offset_setting < 0 ) || ( offset_setting > 29 ) ) {
		return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
	"The reported input offset current setting %d for SCIPE amplifier '%s' "
	"is outside the allowed range of values from 0 to 29.",
			offset_setting, amplifier->record->name );
	}

	/* The division by 3 below relies on the truncation produced by
	 * integer division.
	 */

	if ( offset_toggle != 0 ) {
		
		exponent = (int) ( offset_setting / 3 );

		remainder_value = offset_setting % 3;

		input_offset_current = pow( 10.0, exponent )  * 1e-12;

		if ( remainder_value == 1 ) {
			input_offset_current *= 2.0;
		} else if ( remainder_value == 2 ) {
			input_offset_current *= 5.0;
		}

	} else {

		input_offset_current = 0.0;

	}


	/* What we really want is the output voltage offset.  For that,
	 * we need to know the current gain setting.
	 */

	mx_status = mxd_scipe_amplifier_get_gain( amplifier );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	amplifier->offset = amplifier->gain * input_offset_current;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_amplifier_set_offset( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_scipe_amplifier_set_offset()";

	MX_SCIPE_AMPLIFIER *scipe_amplifier;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	double offset_current, abs_offset_current, requested_offset;
	double max_offset, min_offset, max_offset_current, min_offset_current;
	int mantissa, exponent, sign_offset_current, offset_current_setting;
	mx_status_type mx_status;

	mx_status = mxd_scipe_amplifier_get_pointers(amplifier,
					&scipe_amplifier, &scipe_server, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	requested_offset = amplifier->offset;

	max_offset = 6.0 ;
	min_offset = 1.0e-9 ;

	max_offset_current = mx_divide_safely( max_offset,
                                               amplifier->gain) ;
	min_offset_current = mx_divide_safely( min_offset,
                                               amplifier->gain) ;

	/* Compute the input offset current in amps. */

	offset_current = mx_divide_safely( amplifier->offset,
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
		 * input offset current, turn off the input offset instead
		 * using IOON.
		 */

		sprintf( command, "%s_toggle_offset_current movenow 0",
				scipe_amplifier->scipe_amplifier_name );

		mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( scipe_response_code != MXF_SCIPE_OK ) {
			return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, amplifier->record->name, response );
		}

		return mx_status;
	}

	/* Force the input offset current to the nearest allowed value
	 * and save the rounded value in amplifier->offset.
	 */

	mxd_scipe_amplifier_round_gain_or_offset( abs_offset_current,
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
	  || ( abs_offset_current > max_offset_current ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested amplifier offset of %g volts for amplifier '%s' is "
	"outside the allowed range of %g volts to %g volts.",
			requested_offset, amplifier->record->name,
			-max_offset, max_offset );
	}

	/* Set the input offset calibration mode to callibrated (IOUC0). */

	sprintf( command, "%s_offset_current_calibration_mode movenow %d",
			scipe_amplifier->scipe_amplifier_name, 0 );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, amplifier->record->name, response );
	}

	/* Set the magnitude of the input offset current using IOLV. */

	sprintf( command, "%s_calibrated_offset_current movenow %d",
			scipe_amplifier->scipe_amplifier_name,
			offset_current_setting );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, amplifier->record->name, response );
	}

	/* Set the sign of the input offset current using IOSN. */

	if ( sign_offset_current == 1 ) {
		sprintf( command, "%s_offset_current_sign movenow 1",
				scipe_amplifier->scipe_amplifier_name );
	} else {
		sprintf( command, "%s_offset_current_sign movenow 0",
				scipe_amplifier->scipe_amplifier_name );
	}

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, amplifier->record->name, response );
	}

	/* Turn on the input offset current using IOON. */

	sprintf( command, "%s_toggle_offset_current movenow 1",
			scipe_amplifier->scipe_amplifier_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, amplifier->record->name, response );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_scipe_amplifier_set_time_constant( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_scipe_amplifier_set_time_constant()";

	MX_SCIPE_AMPLIFIER *scipe_amplifier;
	MX_SCIPE_SERVER *scipe_server;
	mx_status_type mx_status;

	mx_status = mxd_scipe_amplifier_get_pointers(amplifier,
					&scipe_amplifier, &scipe_server, fname);

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
mxd_scipe_amplifier_get_parameter( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_scipe_amplifier_get_parameter()";

	MX_SCIPE_AMPLIFIER *scipe_amplifier;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int num_items, scipe_response_code;
	int bias_voltage_setting, lfrq_setting, hfrq_setting;
	mx_status_type mx_status;

	mx_status = mxd_scipe_amplifier_get_pointers(amplifier,
					&scipe_amplifier, &scipe_server, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for amplifier '%s' for parameter type '%s' (%d).",
		fname, amplifier->record->name,
		mx_get_field_label_string( amplifier->record,
			amplifier->parameter_type ),
		amplifier->parameter_type));

	/* Format the necessary SCIPE command. */

	switch( amplifier->parameter_type ) {
	case MXLV_SCIPE_AMPLIFIER_BIAS_VOLTAGE:
		sprintf( command, "%s_bias_voltage_level position",
				scipe_amplifier->scipe_amplifier_name );
		break;
	case MXLV_SCIPE_AMPLIFIER_FILTER_TYPE:
		sprintf( command, "%s_filter_type position",
				scipe_amplifier->scipe_amplifier_name );
		break;
	case MXLV_SCIPE_AMPLIFIER_LOWPASS_FILTER_3DB_POINT:
		sprintf( command, "%s_lowpass_filter_3dB position",
				scipe_amplifier->scipe_amplifier_name );
		break;
	case MXLV_SCIPE_AMPLIFIER_HIGHPASS_FILTER_3DB_POINT:
		sprintf( command, "%s_highpass_filter_3dB position",
				scipe_amplifier->scipe_amplifier_name );
		break;
	case MXLV_SCIPE_AMPLIFIER_RESET_FILTER:
		sprintf( command, "%s_reset_filter position",
				scipe_amplifier->scipe_amplifier_name );
		break;
	case MXLV_SCIPE_AMPLIFIER_GAIN_MODE:
		sprintf( command, "%s_gain_mode position",
				scipe_amplifier->scipe_amplifier_name );
		break;
	case MXLV_SCIPE_AMPLIFIER_INVERT_SIGNAL:
		sprintf( command, "%s_invert_signal position",
				scipe_amplifier->scipe_amplifier_name );
		break;
	case MXLV_SCIPE_AMPLIFIER_BLANK_OUTPUT:
		sprintf( command, "%s_blank position",
				scipe_amplifier->scipe_amplifier_name );
		break;
	default:
		return mx_amplifier_default_get_parameter_handler( amplifier );
		break;
	}

	/* Send the SCIPE command. */

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for amplifier '%s'.  "
	"Response = '%s'", command, amplifier->record->name, response );
	}

	/* Parse the response. */

	num_items = 0;     /* Suppress a GCC uninitialized variable warning. */

	switch( amplifier->parameter_type ) {
	case MXLV_SCIPE_AMPLIFIER_BIAS_VOLTAGE:
		num_items = sscanf( result_ptr, "%d", &bias_voltage_setting );

		scipe_amplifier->bias_voltage =
				0.001 * (double) bias_voltage_setting;
		break;
	case MXLV_SCIPE_AMPLIFIER_FILTER_TYPE:
		num_items = sscanf( result_ptr, "%d",
					&(scipe_amplifier->filter_type) );
		break;
	case MXLV_SCIPE_AMPLIFIER_LOWPASS_FILTER_3DB_POINT:
		num_items = sscanf( result_ptr, "%d", &lfrq_setting );

		mxd_scipe_amplifier_compute_filter_3db_point( lfrq_setting,
				&(scipe_amplifier->lowpass_filter_3db_point) );
		break;
	case MXLV_SCIPE_AMPLIFIER_HIGHPASS_FILTER_3DB_POINT:
		num_items = sscanf( result_ptr, "%d", &hfrq_setting );

		mxd_scipe_amplifier_compute_filter_3db_point( hfrq_setting,
				&(scipe_amplifier->highpass_filter_3db_point) );
		break;
	case MXLV_SCIPE_AMPLIFIER_RESET_FILTER:
		num_items = sscanf( result_ptr, "%d",
				&(scipe_amplifier->reset_filter) );
		break;
	case MXLV_SCIPE_AMPLIFIER_GAIN_MODE:
		num_items = sscanf( result_ptr, "%d",
				&(scipe_amplifier->gain_mode) );
		break;
	case MXLV_SCIPE_AMPLIFIER_INVERT_SIGNAL:
		num_items = sscanf( result_ptr, "%d",
				&(scipe_amplifier->invert_signal) );
		break;
	case MXLV_SCIPE_AMPLIFIER_BLANK_OUTPUT:
		num_items = sscanf( result_ptr, "%d",
				&(scipe_amplifier->blank_output) );
	}

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot find an integer value in the response from SCIPE server '%s' "
"for SCIPE amplifier '%s'.  Original command = '%s', server response = '%s'",
			scipe_server->record->name,
			amplifier->record->name,
			command, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_amplifier_set_parameter( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_scipe_amplifier_set_parameter()";

	MX_SCIPE_AMPLIFIER *scipe_amplifier;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	int bias_voltage_setting, three_db_point_setting;
	int mantissa, exponent;
	mx_status_type mx_status;

	mx_status = mxd_scipe_amplifier_get_pointers(amplifier,
					&scipe_amplifier, &scipe_server, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for amplifier '%s' for parameter type '%s' (%d).",
		fname, amplifier->record->name,
		mx_get_field_label_string( amplifier->record,
			amplifier->parameter_type ),
		amplifier->parameter_type));

	switch( amplifier->parameter_type ) {
	case MXLV_SCIPE_AMPLIFIER_BIAS_VOLTAGE:
		bias_voltage_setting = mx_round( 1000.0 *
				scipe_amplifier->bias_voltage );

		if ( abs( bias_voltage_setting ) > 5000 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested bias voltage of %g volts for amplifier '%s' is outside "
	"the allowed range of -5.0 volts to +5.0 volts.",
				scipe_amplifier->bias_voltage,
				amplifier->record->name );
		}

		scipe_amplifier->bias_voltage =
			0.001 * (double) bias_voltage_setting;

		/* Set the bias voltage level using the BSLV command. */

		sprintf( command, "%s_bias_voltage_level movenow %d",
				scipe_amplifier->scipe_amplifier_name,
				bias_voltage_setting );


		mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( scipe_response_code != MXF_SCIPE_OK ) {
			return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
		"Unexpected response to '%s' command for amplifier '%s'.  "
		"Response = '%s'", command, amplifier->record->name, response );
		}

		/* If the level is zero, turn off the bias voltage via
		 * the BSON command.  Otherwise, turn it on.
		 */

		if ( bias_voltage_setting == 0 ) {
			sprintf( command, "%s_toggle_bias_voltage movenow 0",
					scipe_amplifier->scipe_amplifier_name );
		} else {
			sprintf( command, "%s_toggle_bias_voltage movenow 1",
					scipe_amplifier->scipe_amplifier_name );
		}

		/* This command will be executed at the end of the routine. */
		break;

	case MXLV_SCIPE_AMPLIFIER_FILTER_TYPE:
		if ( ( scipe_amplifier->filter_type < 0 )
		  || ( scipe_amplifier->filter_type > 5 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested filter type %d for amplifier '%s' is outside "
	"the allowed range of 0 to 5.",
				scipe_amplifier->filter_type,
				amplifier->record->name );
		}

		/* Set the filter type using the FLTT command. */

		sprintf( command, "%s_filter_type movenow %d",
				scipe_amplifier->scipe_amplifier_name,
				scipe_amplifier->filter_type );
		break;

	case MXLV_SCIPE_AMPLIFIER_LOWPASS_FILTER_3DB_POINT:

		if ( scipe_amplifier->lowpass_filter_3db_point < 0.02 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested lowpass filter 3dB point of %g Hz "
			"for amplifier '%s' is outside the allowed range "
			"of 0.03 Hz to 1.0e6 Hz.",
				scipe_amplifier->lowpass_filter_3db_point,
				amplifier->record->name );
		}

		/* Force the 3dB point to the nearest setting. */

		mxd_scipe_amplifier_round_filter_3db_point(
				scipe_amplifier->lowpass_filter_3db_point,
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
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested lowpass filter 3dB point of %g Hz "
	"for amplifier '%s' is outside the allowed range "
	"of 0.03 Hz to 1.0e6 Hz.",
				scipe_amplifier->lowpass_filter_3db_point,
				amplifier->record->name );
		}

		scipe_amplifier->lowpass_filter_3db_point =
			( (double) mantissa ) * pow( 10.0, (double) exponent );

		/* Set the filter 3db point using the LFRQ command. */

		sprintf( command, "%s_lowpass_filter_3dB movenow %d",
				scipe_amplifier->scipe_amplifier_name,
				three_db_point_setting );
		break;

	case MXLV_SCIPE_AMPLIFIER_HIGHPASS_FILTER_3DB_POINT:

		if ( scipe_amplifier->highpass_filter_3db_point < 0.02 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested highpass filter 3dB point of %g Hz "
			"for amplifier '%s' is outside the alhighed range "
			"of 0.03 Hz to 1.0e6 Hz.",
				scipe_amplifier->highpass_filter_3db_point,
				amplifier->record->name );
		}

		/* Force the 3dB point to the nearest setting. */

		mxd_scipe_amplifier_round_filter_3db_point(
				scipe_amplifier->highpass_filter_3db_point,
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
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested highpass filter 3dB point of %g Hz "
	"for amplifier '%s' is outside the allowed range "
	"of 0.03 Hz to 1.0e4 Hz.",
				scipe_amplifier->highpass_filter_3db_point,
				amplifier->record->name );
		}

		scipe_amplifier->highpass_filter_3db_point =
			( (double) mantissa ) * pow( 10.0, (double) exponent );

		/* Set the filter 3db point using the HFRQ command. */

		sprintf( command, "%s_highpass_filter_3dB movenow %d",
				scipe_amplifier->scipe_amplifier_name,
				three_db_point_setting );
		break;

	case MXLV_SCIPE_AMPLIFIER_RESET_FILTER:
		/* Reset the filters using the ROLD command.
		 * A value of zero is ignored.
		 */

		if ( scipe_amplifier->reset_filter == 0 ) {
			return MX_SUCCESSFUL_RESULT;
		}

		sprintf( command, "%s_reset_filter movenow 1",
				scipe_amplifier->scipe_amplifier_name );

		scipe_amplifier->reset_filter = 0;
		break;

	case MXLV_SCIPE_AMPLIFIER_GAIN_MODE:
		if ( ( scipe_amplifier->gain_mode < 0 )
		  || ( scipe_amplifier->gain_mode > 2 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested gain mode %d for amplifier '%s' is outside "
	"the allowed range of 0 to 2.",
				scipe_amplifier->gain_mode,
				amplifier->record->name );
		}

		/* Set the gain mode using the GNMD command. */

		sprintf( command, "%s_gain_mode movenow %d",
				scipe_amplifier->scipe_amplifier_name,
				scipe_amplifier->gain_mode );
		break;

	case MXLV_SCIPE_AMPLIFIER_INVERT_SIGNAL:
		if ( ( scipe_amplifier->invert_signal < 0 )
		  || ( scipe_amplifier->invert_signal > 1 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested invert signal value %d for amplifier '%s' is "
	"not one of the allowed values, namely, 0 or 1.",
				scipe_amplifier->invert_signal,
				amplifier->record->name );
		}

		/* Change the signal inversion using the INVT command. */

		sprintf( command, "%s_invert_signal movenow %d",
				scipe_amplifier->scipe_amplifier_name,
				scipe_amplifier->invert_signal );
		break;

	case MXLV_SCIPE_AMPLIFIER_BLANK_OUTPUT:
		if ( ( scipe_amplifier->blank_output < 0 )
		  || ( scipe_amplifier->blank_output > 1 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested blank output value %d for amplifier '%s' is "
	"not one of the allowed values, namely, 0 or 1.",
				scipe_amplifier->blank_output,
				amplifier->record->name );
		}

		/* Change the blank output via the BLNK command. */

		sprintf( command, "%s_blank movenow %d",
				scipe_amplifier->scipe_amplifier_name,
				scipe_amplifier->blank_output );
		break;

	default:
		mx_status = mx_amplifier_default_set_parameter_handler(
				amplifier );

		return mx_status;
		break;
	}

	/* Send the SCIPE command. */

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_AMPLIFIER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
		"Unexpected response to '%s' command for amplifier '%s'.  "
		"Response = '%s'", command, amplifier->record->name, response );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_scipe_amplifier_special_processing_setup( MX_RECORD *record )
{
#if 0
	static const char fname[] = "mxd_scipe_amplifier_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_SCIPE_AMPLIFIER_BIAS_VOLTAGE:
		case MXLV_SCIPE_AMPLIFIER_FILTER_TYPE:
		case MXLV_SCIPE_AMPLIFIER_LOWPASS_FILTER_3DB_POINT:
		case MXLV_SCIPE_AMPLIFIER_HIGHPASS_FILTER_3DB_POINT:
		case MXLV_SCIPE_AMPLIFIER_RESET_FILTER:
		case MXLV_SCIPE_AMPLIFIER_GAIN_MODE:
		case MXLV_SCIPE_AMPLIFIER_INVERT_SIGNAL:
		case MXLV_SCIPE_AMPLIFIER_BLANK_OUTPUT:
			record_field->process_function
					= mxd_scipe_amplifier_process_function;
			break;
		default:
			break;
		}
	}
#endif
	return MX_SUCCESSFUL_RESULT;
}

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

#if 0
static mx_status_type
mxd_scipe_amplifier_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	const char fname[] = "mxd_scipe_amplifier_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_SCIPE_AMPLIFIER *scipe_amplifier;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	scipe_amplifier = (MX_SCIPE_AMPLIFIER *) (record->record_type_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_SCIPE_AMPLIFIER_BIAS_VOLTAGE:
		case MXLV_SCIPE_AMPLIFIER_FILTER_TYPE:
		case MXLV_SCIPE_AMPLIFIER_LOWPASS_FILTER_3DB_POINT:
		case MXLV_SCIPE_AMPLIFIER_HIGHPASS_FILTER_3DB_POINT:
		case MXLV_SCIPE_AMPLIFIER_RESET_FILTER:
		case MXLV_SCIPE_AMPLIFIER_GAIN_MODE:
		case MXLV_SCIPE_AMPLIFIER_INVERT_SIGNAL:
		case MXLV_SCIPE_AMPLIFIER_BLANK_OUTPUT:
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
		case MXLV_SCIPE_AMPLIFIER_BIAS_VOLTAGE:
		case MXLV_SCIPE_AMPLIFIER_FILTER_TYPE:
		case MXLV_SCIPE_AMPLIFIER_LOWPASS_FILTER_3DB_POINT:
		case MXLV_SCIPE_AMPLIFIER_HIGHPASS_FILTER_3DB_POINT:
		case MXLV_SCIPE_AMPLIFIER_RESET_FILTER:
		case MXLV_SCIPE_AMPLIFIER_GAIN_MODE:
		case MXLV_SCIPE_AMPLIFIER_INVERT_SIGNAL:
		case MXLV_SCIPE_AMPLIFIER_BLANK_OUTPUT:
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
		break;
	}

	return mx_status;
}
#endif

