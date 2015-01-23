/*
 * Name:    d_epics_scaler_mce.c
 *
 * Purpose: MX multichannel encoder driver to record an EPICS-controlled
 *          motor's position as part of a synchronous group that also
 *          reads out channels from an EPICS Scaler record.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_EPICS_SCALER_MCE_DEBUG		TRUE

#define MXD_EPICS_SCALER_MCE_DEBUG_MOTOR_ARRAY	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_measurement.h"
#include "mx_callback.h"
#include "mx_mce.h"
#include "mx_mcs.h"
#include "mx_motor.h"
#include "mx_epics.h"
#include "d_epics_motor.h"
#include "d_epics_pmac_biocat.h"
#include "d_epics_scaler_mcs.h"
#include "d_epics_scaler_mce.h"

/* Initialize the MCE driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_epics_scaler_mce_record_function_list = {
	mxd_epics_scaler_mce_initialize_driver,
	mxd_epics_scaler_mce_create_record_structures,
	mxd_epics_scaler_mce_finish_record_initialization,
	NULL,
	NULL,
	mxd_epics_scaler_mce_open
};

MX_MCE_FUNCTION_LIST mxd_epics_scaler_mce_mce_function_list = {
	NULL,
	NULL,
	mxd_epics_scaler_mce_read,
	mxd_epics_scaler_mce_get_current_num_values,
	NULL,
	mxd_epics_scaler_mce_connect_mce_to_motor
};

/* Data structures */

MX_RECORD_FIELD_DEFAULTS mxd_epics_scaler_mce_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCE_STANDARD_FIELDS,
	MXD_EPICS_SCALER_MCE_STANDARD_FIELDS
};

long mxd_epics_scaler_mce_num_record_fields
		= sizeof( mxd_epics_scaler_mce_record_field_defaults )
		/ sizeof( mxd_epics_scaler_mce_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_scaler_mce_rfield_def_ptr
			= &mxd_epics_scaler_mce_record_field_defaults[0];

static mx_status_type
mxd_epics_scaler_mce_get_pointers( MX_MCE *mce,
			MX_EPICS_SCALER_MCE **epics_scaler_mce,
			MX_MCS **mcs,
			MX_EPICS_SCALER_MCS **epics_scaler_mcs,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_scaler_mce_get_pointers()";

	MX_EPICS_SCALER_MCE *epics_scaler_mce_ptr;
	MX_RECORD *mcs_record;
	MX_MCS *mcs_ptr;

	if ( mce == (MX_MCE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mce->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MCE %p passed by '%s' was NULL.",
			mce, calling_fname );
	}

	epics_scaler_mce_ptr = (MX_EPICS_SCALER_MCE *)
					mce->record->record_type_struct;

	if ( epics_scaler_mce_ptr == (MX_EPICS_SCALER_MCE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_SCALER_MCE pointer for record '%s' is NULL.",
			mce->record->name );
	}

	if ( epics_scaler_mce != (MX_EPICS_SCALER_MCE **) NULL ) {
		*epics_scaler_mce = epics_scaler_mce_ptr;
	}

	mcs_record = epics_scaler_mce_ptr->mcs_record;

	if ( mcs_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mcs_record pointer for MCE '%s' is NULL.",
			mce->record->name );
	}

	mcs_ptr = (MX_MCS *) mcs_record->record_class_struct;

	if ( mcs_ptr == (MX_MCS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"MX_MCS pointer for MCS record '%s' is NULL.",
				epics_scaler_mce_ptr->mcs_record->name );
	}

	if ( mcs != (MX_MCS **) NULL ) {
		*mcs = mcs_ptr;
	}

	if ( epics_scaler_mcs != (MX_EPICS_SCALER_MCS **) NULL ) {
		*epics_scaler_mcs = (MX_EPICS_SCALER_MCS *)
				mcs_record->record_type_struct;

		if ( (*epics_scaler_mcs) == (MX_EPICS_SCALER_MCS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_EPICS_SCALER_MCS pointer for record '%s' is NULL.",
				mcs_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_epics_scaler_mce_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_values_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mce_initialize_driver( driver,
					&maximum_num_values_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mce_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_epics_scaler_mce_create_record_structures()";

	MX_MCE *mce;
	MX_EPICS_SCALER_MCE *epics_scaler_mce = NULL;

	/* Allocate memory for the necessary structures. */

	mce = (MX_MCE *) malloc( sizeof(MX_MCE) );

	if ( mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_MCE structure." );
	}

	epics_scaler_mce = (MX_EPICS_SCALER_MCE *)
				malloc( sizeof(MX_EPICS_SCALER_MCE) );

	if ( epics_scaler_mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_EPICS_SCALER_MCE structure.");
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mce;
	record->record_type_struct = epics_scaler_mce;
	record->class_specific_function_list
			= &mxd_epics_scaler_mce_mce_function_list;

	mce->record = record;
	epics_scaler_mce->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mce_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_epics_scaler_mce_finish_record_initialization()";

	MX_MCE *mce = NULL;
	MX_EPICS_SCALER_MCE *epics_scaler_mce = NULL;
	MX_MCS *mcs = NULL;
	MX_EPICS_SCALER_MCS *epics_scaler_mcs = NULL;
	MX_RECORD *list_head_record;
	MX_RECORD *current_record;
	MX_RECORD_FIELD *field;
	MX_EPICS_PV **pv_ptr;
	unsigned long i;
	const char *driver_name = NULL;
	mx_status_type mx_status;

	mce = (MX_MCE *) record->record_class_struct;

	mx_status = mxd_epics_scaler_mce_get_pointers( mce, &epics_scaler_mce,
					&mcs, &epics_scaler_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( record->network_type_name, "epics",
				MXU_NETWORK_TYPE_NAME_LENGTH );

	driver_name = mx_get_driver_name( mcs->record );

	if ( strcmp( driver_name, "epics_scaler_mcs" ) != 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"MX record '%s' is not an 'epics_scaler_mcs' record.  "
		"Instead, it is of type '%s'.  "
		"Only 'epics_scaler_mcs' records can be used with record '%s'.",
			mcs->record->name, driver_name, record->name );
	}

	/* The EPICS synchronous group will record absolute positions of
	 * the motors, so this is an absolute MCE.
	 */

	mce->encoder_type = MXT_MCE_ABSOLUTE_ENCODER;

	mce->current_num_values = mce->maximum_num_values;

	/* Initialize the number of motors to zero. */

	mce->num_motors = 0;

	mx_status = mx_mce_fixup_motor_record_array_field( mce );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We must find all of the motor records implemented using EPICS
	 * and then add them to the motor_record_array data structure.
	 * The records we are interested in are expected to have an MX field
	 * named 'epics_position_pv_ptr'.
	 *
	 * The first step is to find out how many of these motor records
	 * there are in the current database.
	 */

	list_head_record = record->list_head;

	if ( list_head_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The list head record for MX record '%s' is NULL.  "
		"It should not be possible for this to happen.",
			record->name );
	}

	current_record = list_head_record->next_record;

	while ( current_record != list_head_record ) {

		/* Is this record a motor record? */

		if ( current_record->mx_class == MXC_MOTOR ) {

			/* Look for records containing an MX field named
			 * 'epics_position_pv_ptr'.
			 */

			field = mx_get_record_field( current_record,
						"epics_position_pv_ptr" );

			if ( field != NULL ) {
				mce->num_motors++;
			}
		}

		current_record = current_record->next_record;
	}

#if MXD_EPICS_SCALER_MCE_DEBUG_MOTOR_ARRAY
	MX_DEBUG(-2,("%s: num_motors = %lu", fname, mce->num_motors));
#endif

	/* Allocate memory for the data structures we need. */

	mce->motor_record_array = (MX_RECORD **)
		calloc( mce->num_motors, sizeof(MX_RECORD *) );

	if ( mce->motor_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
		"for the motor_record_array field of record '%s'.",
			mce->num_motors, record->name );
	}

	epics_scaler_mce->epics_pv_array = (MX_EPICS_PV **)
		calloc( mce->num_motors, sizeof(MX_EPICS_PV *) );

	if ( epics_scaler_mce->epics_pv_array == (MX_EPICS_PV **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
		"of MX_EPICS_PV structures for record '%s'.",
			mce->num_motors, record->name );
	}

	/* Now walk through the record list a second time so that we
	 * can save a copy of the motor record and EPICS PV pointers
	 * for each EPICS-implemented motor record.
	 */

	current_record = list_head_record->next_record;

	i = 0;

	while ( current_record != list_head_record ) {

		/* Is this record a motor record? */

		if ( current_record->mx_class == MXC_MOTOR ) {

			/* Look for records containing an MX field named
			 * 'epics_position_pv_ptr'.
			 */

			field = mx_get_record_field( current_record,
						"epics_position_pv_ptr" );

			if ( field != NULL ) {
				mce->motor_record_array[i] = current_record;

				pv_ptr = mx_get_field_value_pointer( field );

				if ( pv_ptr == NULL ) {
					return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
					"The epics_position_pv_ptr for "
					"record '%s' is NULL.",
						current_record->name );
				}

				epics_scaler_mce->epics_pv_array[i] = *pv_ptr;

				i++;
			}
		}

		current_record = current_record->next_record;
	}

	/* Update the motor_record_array field to match the number
	 * of EPICS motors that we have found.
	 */

	mx_status = mx_mce_fixup_motor_record_array_field( mce );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mce_open( MX_RECORD *record )
{
#if MXD_EPICS_SCALER_MCE_DEBUG_MOTOR_ARRAY

	static const char fname[] = "mxd_epics_scaler_mce_open()";

	MX_MCE *mce = NULL;
	MX_EPICS_SCALER_MCE *epics_scaler_mce = NULL;
	MX_RECORD *current_record;
	MX_EPICS_PV *pv;
	unsigned long i;
	mx_status_type mx_status;

	mce = (MX_MCE *) record->record_class_struct;

	mx_status = mxd_epics_scaler_mce_get_pointers( mce, &epics_scaler_mce,
							NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < mce->num_motors; i++ ) {
		current_record = mce->motor_record_array[i];

		pv = epics_scaler_mce->epics_pv_array[i];

		MX_DEBUG(-2,("%s: (%lu) record '%s', pv = '%s'",
			fname, i, current_record->name, pv->pvname));
	}

#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mce_read( MX_MCE *mce )
{
	static const char fname[] = "mxd_epics_scaler_mce_read()";

	MX_EPICS_SCALER_MCE *epics_scaler_mce = NULL;
	MX_MCS *mcs = NULL;
	MX_EPICS_SCALER_MCS *epics_scaler_mcs = NULL;
	MX_RECORD *motor_record = NULL;
	MX_MOTOR *motor = NULL;
	unsigned long i;
	double raw_encoder_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_mce_get_pointers( mce, &epics_scaler_mce,
					&mcs, &epics_scaler_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_SCALER_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'",
			fname, mce->record->name));
#endif

	motor_record = mce->motor_record_array[0];

	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No motor has been assigned to MCE record '%s' yet.",
			mce->record->name );
	}

	motor = (MX_MOTOR *) motor_record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' is NULL.",
			motor_record->name );
	}

	if ( epics_scaler_mcs->motor_position_array == (double *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The motor_position_array pointer for 'epics_scaler_mcs' "
		"record '%s'.", mcs->record->name );
	}

	/*---*/

	for ( i = 0; i < mce->current_num_values; i++ ) {

		raw_encoder_value = motor->offset
		    + motor->scale * epics_scaler_mcs->motor_position_array[i];

		mce->value_array[i] =
			mce->offset + mce->scale * raw_encoder_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mce_get_current_num_values( MX_MCE *mce )
{
	static const char fname[] =
		"mxd_epics_scaler_mce_get_current_num_values()";

	MX_EPICS_SCALER_MCE *epics_scaler_mce = NULL;
	MX_MCS *mcs = NULL;
	MX_EPICS_SCALER_MCS *epics_scaler_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_mce_get_pointers( mce, &epics_scaler_mce,
					&mcs, &epics_scaler_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mce->current_num_values = (long) mcs->current_num_measurements;

#if MXD_EPICS_SCALER_MCE_DEBUG
	MX_DEBUG(-2,("%s: mce->current_num_values = %ld",
		fname, mce->current_num_values));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mce_connect_mce_to_motor( MX_MCE *mce,
					MX_RECORD *motor_record )
{
	static const char fname[] =
			"mxd_epics_scaler_mce_connect_mce_to_motor()";

	MX_EPICS_SCALER_MCE *epics_scaler_mce = NULL;
	MX_MCS *mcs = NULL;
	MX_EPICS_SCALER_MCS *epics_scaler_mcs = NULL;
	const char *driver_name = NULL;
	MX_EPICS_MOTOR *epics_motor = NULL;
	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat = NULL;
	mx_status_type mx_status;

	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mx_status = mxd_epics_scaler_mce_get_pointers( mce, &epics_scaler_mce,
					&mcs, &epics_scaler_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_SCALER_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	/* If the new motor record is the same as the already assigned
	 * motor record, then we do not need to change anything.
	 */

	if ( motor_record == epics_scaler_mcs->motor_record ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_epics_pv_disconnect(
			&(epics_scaler_mcs->motor_position_pv) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_scaler_mcs->motor_record = NULL;
	mce->selected_motor_name[0] = '\0';

	/* See if this is a supported motor record type. */

	if ( motor_record->mx_class != MXC_MOTOR ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record '%s' assigned to MCE '%s' is not a motor record.",
			motor_record->name, mce->record->name );
	}

	if ( motor_record->record_type_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The record_type_struct pointer for motor record '%s' is NULL.",
			motor_record->name );
	}

	driver_name = mx_get_driver_name( motor_record );

	if ( strcmp( driver_name, "epics_motor" ) == 0 ) {
		epics_motor = (MX_EPICS_MOTOR *)
				motor_record->record_type_struct;

		mx_epics_pvname_init( &(epics_scaler_mcs->motor_position_pv),
		    "%s.RBV", epics_motor->epics_record_name );
	} else
	if ( strcmp( driver_name, "epics_pmac_biocat" ) == 0 ) {
		epics_pmac_biocat = (MX_EPICS_PMAC_BIOCAT *)
					motor_record->record_type_struct;

		mx_epics_pvname_init( &(epics_scaler_mcs->motor_position_pv),
		    "%s%s%sActPos", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"MX driver '%s' for motor record '%s' is not supported "
		"by the '%s' driver for record '%s'.",
			driver_name, motor_record->name,
			mx_get_driver_name( mce->record ),
			mce->record->name );
	}

	/* Waiting until the first measuremnt of the MCS to connect 
	 * to the EPICS PV may disrupt the timing of the first measurement
	 * or so.  To stop that, we connect to the PV _now_.
	 */

	mx_status = mx_epics_pv_connect( &(epics_scaler_mcs->motor_position_pv),
						MXF_EPVC_WAIT_FOR_CONNECTION );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save the newly configured motor's information. */

	epics_scaler_mcs->motor_record = motor_record;

	strlcpy( mce->selected_motor_name, motor_record->name,
			sizeof(mce->selected_motor_name) );

	return mx_status;
}

