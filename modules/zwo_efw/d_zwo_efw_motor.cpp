/*
 * Name:    d_zwo_efw_motor.c
 *
 * Purpose: MX driver for ZWO Electronic Filter Wheels treated as motors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ZWO_EFW_MOTOR_DEBUG			TRUE

#define MXD_ZWO_EFW_MOTOR_DEBUG_PARAMETERS	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* Vendor include file. */
#include "EFW_filter.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "i_zwo_efw.h"
#include "d_zwo_efw_motor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_zwo_efw_motor_record_function_list = {
	NULL,
	mxd_zwo_efw_motor_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_zwo_efw_motor_open,
	mxd_zwo_efw_motor_close,
};

MX_MOTOR_FUNCTION_LIST mxd_zwo_efw_motor_motor_function_list = {
	NULL,
	mxd_zwo_efw_motor_move_absolute,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_zwo_efw_motor_get_parameter,
	mxd_zwo_efw_motor_set_parameter,
	NULL,
	NULL,
	mxd_zwo_efw_motor_get_extended_status
};

/* ZWO_EFW_MOTOR motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_zwo_efw_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_ZWO_EFW_MOTOR_STANDARD_FIELDS
};

long mxd_zwo_efw_motor_num_record_fields
		= sizeof( mxd_zwo_efw_motor_record_field_defaults )
			/ sizeof( mxd_zwo_efw_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_zwo_efw_motor_rfield_def_ptr
			= &mxd_zwo_efw_motor_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_zwo_efw_motor_get_pointers( MX_MOTOR *motor,
			MX_ZWO_EFW_MOTOR **zwo_efw_motor,
			MX_ZWO_EFW **zwo_efw,
			const char *calling_fname )
{
	static const char fname[] = "mxd_zwo_efw_motor_get_pointers()";

	MX_ZWO_EFW_MOTOR *zwo_efw_motor_ptr;
	MX_RECORD *zwo_efw_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	zwo_efw_motor_ptr = (MX_ZWO_EFW_MOTOR *)
				motor->record->record_type_struct;

	if ( zwo_efw_motor_ptr == (MX_ZWO_EFW_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ZWO_EFW_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( zwo_efw_motor != (MX_ZWO_EFW_MOTOR **) NULL ) {
		*zwo_efw_motor = zwo_efw_motor_ptr;
	}

	if ( zwo_efw != (MX_ZWO_EFW **) NULL ) {
		zwo_efw_record = zwo_efw_motor_ptr->zwo_efw_record;

		if ( zwo_efw_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The zwo_efw_record pointer for record '%s' "
			"is NULL.", motor->record->name );
		}

		*zwo_efw = (MX_ZWO_EFW *) zwo_efw_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_zwo_efw_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_ZWO_EFW_MOTOR *zwo_efw_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	zwo_efw_motor = (MX_ZWO_EFW_MOTOR *) malloc( sizeof(MX_ZWO_EFW_MOTOR) );

	if ( zwo_efw_motor == (MX_ZWO_EFW_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ZWO_EFW_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = zwo_efw_motor;
	record->class_specific_function_list =
				&mxd_zwo_efw_motor_motor_function_list;

	motor->record = record;
	zwo_efw_motor->record = record;

	/* A ZWO Electronic Filter Wheel is treated as a stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* Set the (unused) acceleration type to the rate in counts/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_zwo_efw_motor_open()";

	MX_MOTOR *motor = NULL;
	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	MX_ZWO_EFW *zwo_efw = NULL;
	int filter_index, filter_id;
	EFW_ERROR_CODE efw_error_code;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
				&zwo_efw_motor, &zwo_efw, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the ID for this filter wheel. */

	/* FIXME? For some reason EFWGetID() was returning 0 instead of
	 * the integer that EFWGetProductIDs() saw.  So, for now we
	 * just copy the ID from the array that the 'zwo_efw' driver
	 * stored.
	 */

	filter_index = zwo_efw_motor->filter_index;

#if MXD_ZWO_EFW_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: filter '%s', index = %d",
		fname, record->name, filter_index ));
#endif

#if 0
	filter_id = zwo_efw->filter_wheel_id_array[ filter_index ];
#else 
	efw_error_code = EFWGetID( filter_index, &filter_id );

	switch( efw_error_code ) {
	case EFW_SUCCESS:
		break;
	case EFW_ERROR_INVALID_INDEX:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The filter index %lu requested for filter '%s' is not "
		"a valid filter index.  The filter index must be in the "
		"range from 0 to %ld.",
			zwo_efw_motor->filter_index, record->name,
			zwo_efw->maximum_num_filter_wheels - 1 );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The filter index %lu requested for filter '%s' has something "
		"wrong with it.  The EFW library returned an error code of %d.",
			zwo_efw_motor->filter_index,
			record->name, (int) efw_error_code );
		break;
	}
#endif

#if MXD_ZWO_EFW_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: filter '%s' ID = %d",
		fname, record->name, filter_id));
#endif

	zwo_efw_motor->filter_id = filter_id;

	/* Open the filter wheel */

	efw_error_code = EFWOpen( filter_id );

#if MXD_ZWO_EFW_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: EFWOpen( %d ) = %d",
		fname, filter_id, efw_error_code ));
#endif

	switch( efw_error_code ) {
	case EFW_SUCCESS:
		break;
	case EFW_ERROR_INVALID_ID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested filter ID %d for filter '%s' is not valid.",
			filter_id, record->name );
		break;
	case EFW_ERROR_GENERAL_ERROR:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested filter ID %d for filter '%s' would reach "
		"the maximum value.",
			filter_id, record->name );
		break;
	case EFW_ERROR_REMOVED:
		return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR, fname,
		"Filter '%s' ( ID %d ) has been removed.",
			record->name, filter_id );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"An unexpected error has occurred for filter '%s' ( ID %d ).  "
		"The EFW error code was %d.",
			record->name, filter_id, (int) efw_error_code );
		break;
	}

	/* Get the filter wheel's name. */

	efw_error_code = EFWGetProperty( filter_id,
					&(zwo_efw_motor->efw_info) );

#if MXD_ZWO_EFW_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: EFWGetProperty( %d ) = %d",
		fname, filter_id, efw_error_code ));
#endif

	switch( efw_error_code ) {
	case EFW_SUCCESS:
		break;
	case EFW_ERROR_INVALID_ID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested filter ID %d for filter '%s' is not valid.",
			filter_id, motor->record->name );
		break;
	case EFW_ERROR_CLOSED:
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Filter '%s' ( ID %d ) is not open.",
			motor->record->name, filter_id );
		break;
	case EFW_ERROR_MOVING:
		return mx_error( MXE_NOT_READY, fname,
		"Filter '%s' ( ID %d ) is currently moving.  "
		"Please wait until it is not moving.",
			motor->record->name, filter_id );
		break;
	case EFW_ERROR_REMOVED:
		return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR, fname,
		"Filter '%s' ( ID %d ) has been removed.",
			motor->record->name, filter_id );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"An unexpected error has occurred for filter '%s' ( ID %d ).  "
		"The EFW error code was %d.",
			motor->record->name, filter_id, (int) efw_error_code );
		break;
	}

#if MXD_ZWO_EFW_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: ID = %d, slotNum = %d, Name = '%s'",
		fname, zwo_efw_motor->efw_info.ID,
		zwo_efw_motor->efw_info.slotNum,
		zwo_efw_motor->efw_info.Name ));
#endif

	zwo_efw_motor->num_filters = zwo_efw_motor->efw_info.slotNum;

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_zwo_efw_motor_close()";

	MX_MOTOR *motor = NULL;
	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	MX_ZWO_EFW *zwo_efw = NULL;
	int filter_id;
	EFW_ERROR_CODE efw_error_code;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
				&zwo_efw_motor, &zwo_efw, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	filter_id = zwo_efw_motor->filter_id;

	efw_error_code = EFWClose( filter_id );

#if MXD_ZWO_EFW_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: EFWClose( %d ) = %d",
		fname, filter_id, (int) efw_error_code ));
#endif

	switch( efw_error_code ) {
	case EFW_SUCCESS:
		break;
	case EFW_ERROR_INVALID_ID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested filter ID %d for filter '%s' is not valid.",
			filter_id, motor->record->name );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"An unexpected error has occurred for filter '%s' ( ID %d ).  "
		"The EFW error code was %d.",
			motor->record->name, filter_id, (int) efw_error_code );
		break;
	}

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_zwo_efw_motor_move_absolute()";

	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	MX_ZWO_EFW *zwo_efw = NULL;
	int filter_id, efw_destination;
	EFW_ERROR_CODE efw_error_code;
	mx_status_type mx_status;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
				&zwo_efw_motor, &zwo_efw, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure that the reported position for the filter wheel
	 * is the position it is currently at.  We cannot directly
	 * detect the current position of the filter wheel while it
	 * is moving, so this is the best we can do.
	 */

	mx_status = mxd_zwo_efw_motor_get_extended_status( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now command the move. */

	filter_id = zwo_efw_motor->filter_id;

	efw_destination = motor->raw_destination.stepper;

	if ( (efw_destination < 0)
	  || (efw_destination >= (int) zwo_efw_motor->num_filters) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested filter number of %d for filter '%s' "
		"is outside the allowed range of 0 to %d.",
			efw_destination, motor->record->name,
			(int) zwo_efw_motor->num_filters - 1 );
	}

	efw_error_code = EFWSetPosition( filter_id, efw_destination );

#if MXD_ZWO_EFW_MOTOR_DEBUG
	MX_DEBUG(-2,
	("%s: filter '%s', efw_error_code = %d, efw_destination = %d",
		fname, motor->record->name, efw_error_code, efw_destination));
#endif

	switch( efw_error_code ) {
	case EFW_SUCCESS:
		break;
	case EFW_ERROR_INVALID_ID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested filter ID %d for filter '%s' is not valid.",
			filter_id, motor->record->name );
		break;
	case EFW_ERROR_CLOSED:
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Filter '%s' ( ID %d ) is not open.",
			motor->record->name, filter_id );
		break;
	case EFW_ERROR_INVALID_VALUE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested filter wheel destination %d for filter '%s' "
		"is not valid.", efw_destination, motor->record->name );
		break;
	case EFW_ERROR_MOVING:
		return mx_error( MXE_NOT_READY, fname,
		"Filter '%s' ( ID %d ) is currently moving.  "
		"Please wait until it is not moving.",
			motor->record->name, filter_id );
		break;
	case EFW_ERROR_ERROR_STATE:
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"Filter '%s' ( ID %d ) is in an error state.",
			motor->record->name, filter_id );
		break;
	case EFW_ERROR_REMOVED:
		return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR, fname,
		"Filter '%s' ( ID %d ) has been removed.",
			motor->record->name, filter_id );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"An unexpected error has occurred for filter '%s' ( ID %d ).  "
		"The EFW error code was %d.",
			motor->record->name, filter_id, (int) efw_error_code );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_zwo_efw_motor_get_parameter()";

	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	MX_ZWO_EFW *zwo_efw = NULL;
	mx_status_type mx_status;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
				&zwo_efw_motor, &zwo_efw, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ZWO_EFW_MOTOR_DEBUG_PARAMETERS
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	default:
		return mx_motor_default_get_parameter_handler( motor );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_zwo_efw_motor_set_parameter()";

	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	MX_ZWO_EFW *zwo_efw = NULL;
	mx_status_type mx_status;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
				&zwo_efw_motor, &zwo_efw, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ZWO_EFW_MOTOR_DEBUG_PARAMETERS
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_zwo_efw_motor_get_extended_status()";

	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	MX_ZWO_EFW *zwo_efw = NULL;
	int filter_id, efw_position;
	EFW_ERROR_CODE efw_error_code;
	mx_status_type mx_status;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
				&zwo_efw_motor, &zwo_efw, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	filter_id = zwo_efw_motor->filter_id;

	efw_error_code = EFWGetPosition( filter_id, &efw_position );

#if MXD_ZWO_EFW_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: filter '%s', efw_error_code = %d, efw_position = %d",
		fname, motor->record->name, efw_error_code, efw_position));
#endif

	switch( efw_error_code ) {
	case EFW_SUCCESS:
		break;
	case EFW_ERROR_INVALID_ID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested filter ID %d for filter '%s' is not valid.",
			filter_id, motor->record->name );
		break;
	case EFW_ERROR_CLOSED:
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Filter '%s' ( ID %d ) is not open.",
			motor->record->name, filter_id );
		break;
	case EFW_ERROR_ERROR_STATE:
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"Filter '%s' ( ID %d ) is in an error state.",
			motor->record->name, filter_id );
		break;
	case EFW_ERROR_REMOVED:
		return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR, fname,
		"Filter '%s' ( ID %d ) has been removed.",
			motor->record->name, filter_id );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"An unexpected error has occurred for filter '%s' ( ID %d ).  "
		"The EFW error code was %d.",
			motor->record->name, filter_id, (int) efw_error_code );
		break;
	}

	if ( efw_position == -1 ) {
		/* Note that if the filter wheel is moving, we cannot
		 * detect the current position of the filter wheel,
		 * so we leave it at the position that it had before
		 * the current move.
		 */

		motor->status = MXSF_MTR_IS_BUSY;
	} else {
		motor->status = 0;

		motor->raw_position.stepper = efw_position;
	}

	return MX_SUCCESSFUL_RESULT;
}

