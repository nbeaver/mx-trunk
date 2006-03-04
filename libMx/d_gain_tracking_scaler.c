/*
 * Name:    d_gain_tracking_scaler.c
 *
 * Purpose: MX scaler driver to control MX gain tracking scalers.
 *          Gain tracking scalers are pseudoscalers that rescale
 *          their reported number of counts to go up and down when
 *          an associated amplifier changes its gain.
 *
 *          For example, suppose that the real scaler was reading
 *          1745 counts, the amplifier was set to 1e8 gain and the
 *          gain tracking scale factor was 1e10.  Then, the gain
 *          tracking scaler would report a value of 174500 counts.
 *          If the amplifier gain was changed to 1e9, then the gain
 *          tracking scaler would report a value of 17450 counts.
 *
 *          Gain tracking scalers are intended to be used in 
 *          combination with autoscaling scalers, so that when the
 *          autoscaling scaler changes the gain of the amplifier,
 *          the values reported by gain tracking scalers will change
 *          to match.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_amplifier.h"
#include "mx_scaler.h"
#include "d_gain_tracking_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_gain_tracking_scaler_record_function_list = {
	mxd_gain_tracking_scaler_initialize_type,
	mxd_gain_tracking_scaler_create_record_structures,
	mx_scaler_finish_record_initialization,
	NULL,
	NULL,
	mxd_gain_tracking_scaler_read_parms_from_hardware,
	mxd_gain_tracking_scaler_write_parms_to_hardware,
	mxd_gain_tracking_scaler_open,
	mxd_gain_tracking_scaler_close
};

MX_SCALER_FUNCTION_LIST mxd_gain_tracking_scaler_scaler_function_list = {
	mxd_gain_tracking_scaler_clear,
	mxd_gain_tracking_scaler_overflow_set,
	mxd_gain_tracking_scaler_read,
	NULL,
	mxd_gain_tracking_scaler_is_busy,
	mxd_gain_tracking_scaler_start,
	mxd_gain_tracking_scaler_stop,
	mxd_gain_tracking_scaler_get_parameter,
	mxd_gain_tracking_scaler_set_parameter
};

/* MX gain_tracking scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_gain_tracking_scaler_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_GAIN_TRACKING_SCALER_STANDARD_FIELDS
};

long mxd_gain_tracking_scaler_num_record_fields
		= sizeof( mxd_gain_tracking_scaler_rf_defaults )
		  / sizeof( mxd_gain_tracking_scaler_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_gain_tracking_scaler_rfield_def_ptr
			= &mxd_gain_tracking_scaler_rf_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_gain_tracking_scaler_get_pointers( MX_SCALER *scaler,
			MX_GAIN_TRACKING_SCALER **gain_tracking_scaler,
			const char *calling_fname )
{
	const char fname[] = "mxd_gain_tracking_scaler_get_pointers()";

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( gain_tracking_scaler == (MX_GAIN_TRACKING_SCALER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_GAIN_TRACKING_SCALER pointer passed was NULL." );
	}

	if ( scaler->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for scaler pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*gain_tracking_scaler = (MX_GAIN_TRACKING_SCALER *)
					scaler->record->record_type_struct;

	if ( *gain_tracking_scaler == (MX_GAIN_TRACKING_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_GAIN_TRACKING_SCALER pointer for scaler "
			"record '%s' passed by '%s' is NULL",
				scaler->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_create_record_structures( MX_RECORD *record )
{
	const char fname[]
		= "mxd_gain_tracking_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_GAIN_TRACKING_SCALER *gain_tracking_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_SCALER structure." );
	}

	gain_tracking_scaler = (MX_GAIN_TRACKING_SCALER *)
				malloc( sizeof(MX_GAIN_TRACKING_SCALER) );

	if ( gain_tracking_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for an MX_GAIN_TRACKING_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = gain_tracking_scaler;
	record->class_specific_function_list
			= &mxd_gain_tracking_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_delete_record( MX_RECORD *record )
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
mxd_gain_tracking_scaler_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_open( MX_RECORD *record )
{
	const char fname[] = "mxd_gain_tracking_scaler_open()";

	MX_SCALER *scaler;
	MX_GAIN_TRACKING_SCALER *gain_tracking_scaler;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	scaler = (MX_SCALER *) record->record_class_struct;

	mx_status = mxd_gain_tracking_scaler_get_pointers(
				scaler, &gain_tracking_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_open_hardware(gain_tracking_scaler->real_scaler_record);

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_close( MX_RECORD *record )
{
	const char fname[] = "mxd_gain_tracking_scaler_close()";

	MX_SCALER *scaler;
	MX_GAIN_TRACKING_SCALER *gain_tracking_scaler;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	scaler = (MX_SCALER *) record->record_class_struct;

	mx_status = mxd_gain_tracking_scaler_get_pointers(
				scaler, &gain_tracking_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_close_hardware(gain_tracking_scaler->real_scaler_record);

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_clear( MX_SCALER *scaler )
{
	const char fname[] = "mxd_gain_tracking_scaler_clear()";

	MX_GAIN_TRACKING_SCALER *gain_tracking_scaler;
	mx_status_type mx_status;

	mx_status = mxd_gain_tracking_scaler_get_pointers(
				scaler, &gain_tracking_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_clear( gain_tracking_scaler->real_scaler_record );

	scaler->raw_value = 0L;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_overflow_set( MX_SCALER *scaler )
{
	const char fname[] = "mxd_gain_tracking_scaler_overflow_set()";

	MX_GAIN_TRACKING_SCALER *gain_tracking_scaler;
	mx_bool_type overflow_set;
	mx_status_type mx_status;

	mx_status = mxd_gain_tracking_scaler_get_pointers(
				scaler, &gain_tracking_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_overflow_set(
		gain_tracking_scaler->real_scaler_record, &overflow_set );

	scaler->overflow_set = overflow_set;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_read( MX_SCALER *scaler )
{
	const char fname[] = "mxd_gain_tracking_scaler_read()";

	MX_GAIN_TRACKING_SCALER *gain_tracking_scaler;
	long real_scaler_value;
	double numerator, amplifier_gain, modified_scaler_value;
	mx_status_type mx_status;

	mx_status = mxd_gain_tracking_scaler_get_pointers(
				scaler, &gain_tracking_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_read(
		gain_tracking_scaler->real_scaler_record, &real_scaler_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_get_gain(
				gain_tracking_scaler->amplifier_record,
					&amplifier_gain );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	numerator = ((double) real_scaler_value)
				* gain_tracking_scaler->gain_tracking_scale;

	modified_scaler_value = mx_divide_safely( numerator, amplifier_gain );

	scaler->raw_value = mx_round( modified_scaler_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_is_busy( MX_SCALER *scaler )
{
	const char fname[] = "mxd_gain_tracking_scaler_is_busy()";

	MX_GAIN_TRACKING_SCALER *gain_tracking_scaler;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_gain_tracking_scaler_get_pointers(
				scaler, &gain_tracking_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_is_busy(
			gain_tracking_scaler->real_scaler_record, &busy );

	scaler->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_start( MX_SCALER *scaler )
{
	const char fname[] = "mxd_gain_tracking_scaler_start()";

	MX_GAIN_TRACKING_SCALER *gain_tracking_scaler;
	long real_scaler_value;
	double numerator, amplifier_gain, modified_scaler_value;
	mx_status_type mx_status;

	mx_status = mxd_gain_tracking_scaler_get_pointers(
				scaler, &gain_tracking_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_get_gain(
				gain_tracking_scaler->amplifier_record,
					&amplifier_gain );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	numerator = ((double) scaler->raw_value) * amplifier_gain;

	modified_scaler_value = mx_divide_safely( numerator,
				gain_tracking_scaler->gain_tracking_scale );

	real_scaler_value = mx_round( modified_scaler_value );

	mx_status = mx_scaler_start(
			gain_tracking_scaler->real_scaler_record,
			real_scaler_value );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_stop( MX_SCALER *scaler )
{
	const char fname[] = "mxd_gain_tracking_scaler_stop()";

	MX_GAIN_TRACKING_SCALER *gain_tracking_scaler;
	long real_scaler_value;
	double numerator, amplifier_gain, modified_scaler_value;
	mx_status_type mx_status;

	mx_status = mxd_gain_tracking_scaler_get_pointers(
				scaler, &gain_tracking_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_stop(
		gain_tracking_scaler->real_scaler_record, &real_scaler_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_get_gain(
				gain_tracking_scaler->amplifier_record,
					&amplifier_gain );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	numerator = ((double) real_scaler_value)
				* gain_tracking_scaler->gain_tracking_scale;

	modified_scaler_value = mx_divide_safely( numerator, amplifier_gain );

	scaler->raw_value = mx_round( modified_scaler_value );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_get_parameter( MX_SCALER *scaler )
{
	const char fname[] = "mxd_gain_tracking_scaler_get_parameter()";

	MX_GAIN_TRACKING_SCALER *gain_tracking_scaler;
	long mode;
	double dark_current;
	mx_status_type mx_status;

	mx_status = mxd_gain_tracking_scaler_get_pointers(
				scaler, &gain_tracking_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		mx_status = mx_scaler_get_mode(
				gain_tracking_scaler->real_scaler_record,
				&mode );
		scaler->mode = mode;
		break;
	case MXLV_SCL_DARK_CURRENT:
		mx_status = mx_scaler_get_dark_current(
				gain_tracking_scaler->real_scaler_record,
				&dark_current );
		scaler->dark_current = dark_current;
		break;
	default:
		mx_status = mx_scaler_default_get_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gain_tracking_scaler_set_parameter( MX_SCALER *scaler )
{
	const char fname[] = "mxd_gain_tracking_scaler_set_parameter()";

	MX_GAIN_TRACKING_SCALER *gain_tracking_scaler;
	mx_status_type mx_status;

	mx_status = mxd_gain_tracking_scaler_get_pointers(
				scaler, &gain_tracking_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		mx_status = mx_scaler_set_mode(
				gain_tracking_scaler->real_scaler_record,
				scaler->mode );
		break;
	case MXLV_SCL_DARK_CURRENT:
		mx_status = mx_scaler_set_dark_current(
				gain_tracking_scaler->real_scaler_record,
				scaler->dark_current );
		break;
	default:
		mx_status = mx_scaler_default_set_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

