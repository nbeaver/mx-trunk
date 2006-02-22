/*
 * Name:    d_auto_scaler.c
 *
 * Purpose: MX scaler driver to control autoscaling scaler records.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2002 Illinois Institute of Technology
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
#include "mx_autoscale.h"
#include "mx_scaler.h"
#include "d_auto_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_autoscale_scaler_record_function_list = {
	mxd_autoscale_scaler_initialize_type,
	mxd_autoscale_scaler_create_record_structures,
	mxd_autoscale_scaler_finish_record_initialization,
	mxd_autoscale_scaler_delete_record,
	NULL,
	mxd_autoscale_scaler_read_parms_from_hardware,
	mxd_autoscale_scaler_write_parms_to_hardware,
	mxd_autoscale_scaler_open,
	mxd_autoscale_scaler_close
};

MX_SCALER_FUNCTION_LIST mxd_autoscale_scaler_scaler_function_list = {
	mxd_autoscale_scaler_clear,
	mxd_autoscale_scaler_overflow_set,
	mxd_autoscale_scaler_read,
	NULL,
	mxd_autoscale_scaler_is_busy,
	mxd_autoscale_scaler_start,
	mxd_autoscale_scaler_stop,
	mxd_autoscale_scaler_get_parameter,
	mxd_autoscale_scaler_set_parameter
};

/* MX autoscale scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_autoscale_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_AUTOSCALE_SCALER_STANDARD_FIELDS
};

long mxd_autoscale_scaler_num_record_fields
		= sizeof( mxd_autoscale_scaler_record_field_defaults )
		  / sizeof( mxd_autoscale_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_autoscale_scaler_rfield_def_ptr
			= &mxd_autoscale_scaler_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_autoscale_scaler_get_pointers( MX_SCALER *scaler,
			MX_AUTOSCALE_SCALER **auto_scaler,
			MX_AUTOSCALE **autoscale,
			const char *calling_fname )
{
	const char fname[] = "mxd_autoscale_scaler_get_pointers()";

	MX_RECORD *record, *autoscale_record;
	MX_AUTOSCALE_SCALER *auto_scaler_ptr;

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The scaler pointer passed by '%s' was NULL",
			calling_fname );
	}

	record = scaler->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for scaler pointer passed by '%s' is NULL.",
			calling_fname );
	}

	auto_scaler_ptr = (MX_AUTOSCALE_SCALER *) record->record_type_struct;

	if ( auto_scaler_ptr == (MX_AUTOSCALE_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_AUTOSCALE_SCALER pointer for scaler record '%s' passed by '%s' is NULL",
			record->name, calling_fname );
	}

	if ( auto_scaler != (MX_AUTOSCALE_SCALER **) NULL ) {
		*auto_scaler = auto_scaler_ptr;
	}

	autoscale_record = auto_scaler_ptr->autoscale_record;

	if ( autoscale_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The autoscale record pointer for MX autoscale scaler '%s' is NULL.",
			scaler->record->name );
	}

	if ( autoscale != (MX_AUTOSCALE **) NULL ) {

		*autoscale = (MX_AUTOSCALE *)
				autoscale_record->record_class_struct;

		if ( *autoscale == (MX_AUTOSCALE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_AUTOSCALE pointer for autoscale record '%s' "
			"used by scaler '%s' is NULL.",
				autoscale_record->name, record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_autoscale_scaler_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_autoscale_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_AUTOSCALE_SCALER *auto_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	auto_scaler = (MX_AUTOSCALE_SCALER *)
				malloc( sizeof(MX_AUTOSCALE_SCALER) );

	if ( auto_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AUTOSCALE_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = auto_scaler;
	record->class_specific_function_list
			= &mxd_autoscale_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_finish_record_initialization( MX_RECORD *record )
{
	const char fname[]
		= "mxd_autoscale_scaler_finish_record_initialization()";

	MX_SCALER *scaler;
	MX_AUTOSCALE *autoscale;
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler = (MX_SCALER *) record->record_class_struct;

	mx_status = mxd_autoscale_scaler_get_pointers( scaler, NULL,
							&autoscale, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If autoscale->monitor_record points to the current record, then
	 * we have created a pointer loop which will result in infinite
	 * recursion if we try to use the current record.  If we tried to
	 * use the current record in this circumstance, the program would
	 * crash as soon as it ran out of stack space due to the infinite
	 * recursion.
	 */

	if ( record == autoscale->monitor_record ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The autoscale record '%s' used by the current record '%s' "
		"uses '%s' as its monitor record.  This condition would "
		"lead to infinite recursion and is not allowed.",
			autoscale->record->name, record->name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_delete_record( MX_RECORD *record )
{
	MX_AUTOSCALE_SCALER *auto_scaler;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	auto_scaler = (MX_AUTOSCALE_SCALER *) record->record_type_struct;

	if ( auto_scaler != NULL ) {
		free( auto_scaler );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_clear( MX_SCALER *scaler )
{
	const char fname[] = "mxd_autoscale_scaler_clear()";

	MX_AUTOSCALE *autoscale;
	mx_status_type mx_status;

	mx_status = mxd_autoscale_scaler_get_pointers( scaler, NULL,
							&autoscale, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_clear( autoscale->monitor_record );

	scaler->raw_value = 0L;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_overflow_set( MX_SCALER *scaler )
{
	const char fname[] = "mxd_autoscale_scaler_overflow_set()";

	MX_AUTOSCALE *autoscale;
	int overflow_set;
	mx_status_type mx_status;

	mx_status = mxd_autoscale_scaler_get_pointers( scaler, NULL,
							&autoscale, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_overflow_set( autoscale->monitor_record,
							&overflow_set );

	scaler->overflow_set = overflow_set;

	return mx_status;
}

/* Calling mx_autoscale_read_monitor() rather than mx_scaler_read() ensures
 * that the last value read is saved in autoscale->last_monitor_value.integer.
 */

MX_EXPORT mx_status_type
mxd_autoscale_scaler_read( MX_SCALER *scaler )
{
	const char fname[] = "mxd_autoscale_scaler_read()";

	MX_AUTOSCALE *autoscale;
	MX_AUTOSCALE_SCALER *auto_scaler;
	long scaler_value;
	double normalized_value, gain;
	mx_status_type mx_status;

	mx_status = mxd_autoscale_scaler_get_pointers( scaler, &auto_scaler,
							&autoscale, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_autoscale_read_monitor( autoscale->record,
							&scaler_value );

	if ((auto_scaler->autoscale_flags & MXF_AUTOSCALE_SCALER_NORMALIZE)== 0)
	{
		scaler->raw_value = scaler_value;
	} else {
		if ( autoscale->control_record->mx_superclass != MXR_DEVICE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
	"Normalization of autoscaled scaler readings is currently "
	"supported only for amplifier control records, but record '%s' is "
	"not an amplifier record.", autoscale->control_record->name );
		}

		switch( autoscale->control_record->mx_class ) {
		case MXC_AMPLIFIER:
			mx_status = mx_amplifier_get_gain(
					autoscale->control_record, &gain );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			normalized_value = mx_divide_safely(
				auto_scaler->factor * (double) scaler_value,
					gain );

			scaler->raw_value = mx_round( normalized_value );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
	"Normalization of autoscaled scaler readings is currently "
	"supported only for amplifier control records, but record '%s' is "
	"not an amplifier record.", autoscale->control_record->name );
		}
				
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_is_busy( MX_SCALER *scaler )
{
	const char fname[] = "mxd_autoscale_scaler_is_busy()";

	MX_AUTOSCALE *autoscale;
	int busy;
	mx_status_type mx_status;

	mx_status = mxd_autoscale_scaler_get_pointers( scaler, NULL,
							&autoscale, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_is_busy( autoscale->monitor_record, &busy );

	scaler->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_start( MX_SCALER *scaler )
{
	const char fname[] = "mxd_autoscale_scaler_start()";

	MX_AUTOSCALE *autoscale;
	mx_status_type mx_status;

	mx_status = mxd_autoscale_scaler_get_pointers( scaler, NULL,
							&autoscale, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_start( autoscale->monitor_record,
						scaler->raw_value );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_stop( MX_SCALER *scaler )
{
	const char fname[] = "mxd_autoscale_scaler_stop()";

	MX_AUTOSCALE *autoscale;
	long value;
	mx_status_type mx_status;

	mx_status = mxd_autoscale_scaler_get_pointers( scaler, NULL,
							&autoscale, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_stop( autoscale->monitor_record, &value );

	scaler->raw_value = value;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_get_parameter( MX_SCALER *scaler )
{
	const char fname[] = "mxd_autoscale_scaler_get_parameter()";

	MX_AUTOSCALE *autoscale;
	int mode;
	double dark_current;
	mx_status_type mx_status;

	mx_status = mxd_autoscale_scaler_get_pointers( scaler, NULL,
							&autoscale, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		mx_status = mx_scaler_get_mode(
					autoscale->monitor_record, &mode );
		scaler->mode = mode;
		break;
	case MXLV_SCL_DARK_CURRENT:
		mx_status = mx_scaler_get_dark_current(
				autoscale->monitor_record, &dark_current );

		scaler->dark_current = dark_current;
		break;
	default:
		mx_status = mx_scaler_default_get_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_autoscale_scaler_set_parameter( MX_SCALER *scaler )
{
	const char fname[] = "mxd_autoscale_scaler_set_parameter()";

	MX_AUTOSCALE *autoscale;
	mx_status_type mx_status;

	mx_status = mxd_autoscale_scaler_get_pointers( scaler, NULL,
							&autoscale, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		mx_status = mx_scaler_set_mode( autoscale->monitor_record,
							scaler->mode );
		break;
	case MXLV_SCL_DARK_CURRENT:
		mx_status = mx_scaler_set_dark_current(
						autoscale->monitor_record,
						scaler->dark_current );
		break;
	default:
		mx_status = mx_scaler_default_set_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

