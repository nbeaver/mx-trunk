/*
 * Name:    d_network_mce.c
 *
 * Purpose: MX MCE driver to support MX network multichannel encoders.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2004, 2006-2007, 2010, 2013-2016, 2022
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_MCE_DEBUG_MOTOR_ARRAY	FALSE

#define MXD_NETWORK_MCE_DEBUG_READ_MEASUREMENT	FALSE

#define MXD_NETWORK_MCE_DEBUG_PARAMETERS	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_mce.h"
#include "mx_net.h"
#include "mx_motor.h"
#include "d_network_motor.h"
#include "d_network_mce.h"

/* Initialize the MCE driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_network_mce_record_function_list = {
	mxd_network_mce_initialize_driver,
	mxd_network_mce_create_record_structures,
	mxd_network_mce_finish_record_initialization,
	mxd_network_mce_delete_record
};

MX_MCE_FUNCTION_LIST mxd_network_mce_mce_function_list = {
	NULL,
	NULL,
	mxd_network_mce_read,
	mxd_network_mce_get_current_num_values,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_network_mce_read_measurement,
	mxd_network_mce_get_motor_record_array,
	mxd_network_mce_connect_mce_to_motor,
	mxd_network_mce_get_parameter,
	mxd_network_mce_set_parameter
};

/* MCS encoder data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_network_mce_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCE_STANDARD_FIELDS,
	MXD_NETWORK_MCE_STANDARD_FIELDS
};

long mxd_network_mce_num_record_fields
		= sizeof( mxd_network_mce_record_field_defaults )
		  / sizeof( mxd_network_mce_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_mce_rfield_def_ptr
			= &mxd_network_mce_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_network_mce_get_pointers( MX_MCE *mce,
			MX_NETWORK_MCE **network_mce,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_mce_get_pointers()";

	if ( mce == (MX_MCE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( network_mce == (MX_NETWORK_MCE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_MCE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*network_mce = (MX_NETWORK_MCE *) (mce->record->record_type_struct);

	if ( *network_mce == (MX_NETWORK_MCE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_MCE pointer for record '%s' is NULL.",
			mce->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_network_mce_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_values_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mce_initialize_driver( driver,
					&maximum_num_values_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mce_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_network_mce_create_record_structures()";

	MX_MCE *mce = NULL;
	MX_NETWORK_MCE *network_mce = NULL;

	/* Allocate memory for the necessary structures. */

	mce = (MX_MCE *) malloc( sizeof(MX_MCE) );

	if ( mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MCE structure." );
	}

	network_mce = (MX_NETWORK_MCE *) malloc( sizeof(MX_NETWORK_MCE) );

	if ( network_mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_NETWORK_MCE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mce;
	record->record_type_struct = network_mce;
	record->class_specific_function_list
			= &mxd_network_mce_mce_function_list;

	mce->record = record;
	network_mce->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mce_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_network_mce_finish_record_initialization()";

	MX_MCE *mce = NULL;
	MX_NETWORK_MCE *network_mce = NULL;
	mx_status_type mx_status;

	network_mce = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	mce = (MX_MCE *) record->record_class_struct;

	mx_status = mxd_network_mce_get_pointers( mce, &network_mce, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mce->window_is_available = FALSE;
	mce->use_window = FALSE;

	memset( mce->window,
		0, MXU_MTR_NUM_WINDOW_PARAMETERS * sizeof(double) );

	/*----*/

	strlcpy(record->network_type_name, "mx", MXU_NETWORK_TYPE_NAME_LENGTH);

	mce->encoder_type = MXT_MCE_UNKNOWN_ENCODER_TYPE;

	mce->current_num_values = mce->maximum_num_values;

	network_mce->selected_motor_record = NULL;

	strlcpy( mce->selected_motor_name, "",
			sizeof(mce->selected_motor_name) );

	mce->num_motors = 0;

	mce->motor_record_array = NULL;

	mx_status = mx_mce_fixup_motor_record_array_field( mce );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_mce->current_num_values_nf),
		network_mce->server_record,
		"%s.current_num_values", network_mce->remote_record_name );

	mx_network_field_init( &(network_mce->encoder_type_nf),
		network_mce->server_record,
		"%s.encoder_type", network_mce->remote_record_name );

	mx_network_field_init( &(network_mce->measurement_index_nf),
		network_mce->server_record,
		"%s.measurement_index", network_mce->remote_record_name );

	mx_network_field_init( &(network_mce->measurement_window_offset_nf),
		network_mce->server_record,
		"%s.measurement_window_offset",
					network_mce->remote_record_name );

	mx_network_field_init( &(network_mce->motor_record_array_nf),
		network_mce->server_record,
		"%s.motor_record_array", network_mce->remote_record_name );

	mx_network_field_init( &(network_mce->num_motors_nf),
		network_mce->server_record,
		"%s.num_motors", network_mce->remote_record_name );

	mx_network_field_init( &(network_mce->selected_motor_name_nf),
		network_mce->server_record,
		"%s.selected_motor_name", network_mce->remote_record_name );

	mx_network_field_init( &(network_mce->value_nf),
		network_mce->server_record,
		"%s.value", network_mce->remote_record_name );

	mx_network_field_init( &(network_mce->value_array_nf),
		network_mce->server_record,
		"%s.value_array", network_mce->remote_record_name );

	mx_network_field_init( &(network_mce->use_window_nf),
		network_mce->server_record,
		"%s.use_window", network_mce->remote_record_name );

	mx_network_field_init( &(network_mce->window_nf),
		network_mce->server_record,
		"%s.window", network_mce->remote_record_name );

	mx_network_field_init( &(network_mce->window_is_available_nf),
		network_mce->server_record,
		"%s.window_is_available", network_mce->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mce_delete_record( MX_RECORD *record )
{
	MX_MCE *mce = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mce = (MX_MCE *) record->record_class_struct;

	if ( mce != (MX_MCE *) NULL ) {
		if ( mce->motor_record_array != (MX_RECORD **) NULL ) {
			mx_free( mce->motor_record_array );
		}
		mx_free( record->record_class_struct );
	}

	if ( record->record_type_struct != NULL ) {
		mx_free( record->record_type_struct );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mce_read( MX_MCE *mce )
{
	static const char fname[] = "mxd_network_mce_read()";

	MX_NETWORK_MCE *network_mce = NULL;
	MX_MOTOR *selected_motor = NULL;
	double motor_scale, scaled_encoder_value;
	long dimension_array[1];
	long i, num_values;
	mx_status_type mx_status;

	network_mce = NULL;

	mx_status = mxd_network_mce_get_pointers( mce, &network_mce, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCE '%s'",
			fname, mce->record->name));

	if ( network_mce->selected_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No motor has been selected yet for network MCE '%s'.",
			mce->record->name );
	}

	selected_motor = (MX_MOTOR *)
		network_mce->selected_motor_record->record_class_struct;

	if ( selected_motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for motor record '%s' selected "
		"by network MCE '%s' is NULL.",
			network_mce->selected_motor_record->name,
			mce->record->name );
	}

	motor_scale = selected_motor->scale;

	/* First get the current number of encoder values. */

	mx_status = mx_get( &(network_mce->current_num_values_nf),
					MXFT_LONG, &num_values );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_values > mce->maximum_num_values ) {
		mce->current_num_values = mce->maximum_num_values;
	} else {
		mce->current_num_values = num_values;
	}

	/* Now transfer the MCE contents. */

#if 0
	dimension_array[0] = mce->current_num_values;
#else
	/* !!! FIXME FIXME FIXME !!! - Transmitting the entire remote buffer
	 * if we only want a subset of the values is VERY inefficient.
	 */

	{
		MX_NETWORK_SERVER *network_server;

		network_server = (MX_NETWORK_SERVER *)
			network_mce->server_record->record_class_struct;

		if ( network_server->data_format == MX_NETWORK_DATAFMT_XDR ) {

			/* XDR gets VERY upset if you do not read all
			 * of the values that the remote server sent.
			 */

			dimension_array[0] = (long) mce->maximum_num_values;
		} else {
			dimension_array[0] = (long) mce->current_num_values;
		}
	}
#endif

	mx_status = mx_get_array( &(network_mce->value_array_nf),
				MXFT_DOUBLE, 1, dimension_array,
				mce->value_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < mce->current_num_values; i++ ) {

		scaled_encoder_value =
			mce->offset + mce->scale * mce->value_array[i];

		mce->value_array[i] = motor_scale * scaled_encoder_value;
	}

	/* Read the measurement window offset.  If the remote MCE sends
	 * all measurements to the client, the measurement window offset
	 * will be zero.  If the remote MCE skips over some measurements
	 * at the the start of the data buffer, then reading the value
	 * of measurement_window_offset will tell you how many measurements
	 * were skipped over.
	 * 
	 * Typically, measurement offsets appear in connection with MCEs
	 * that use a window to return a subset of the values that the
	 * original hardware acquires.
	 *
	 * An example of the use of this is a Newport XPS motor controller
	 * that returns encoder quadrature signals only within a position
	 * window.
	 */

	mx_status = mx_get( &(network_mce->measurement_window_offset_nf),
			MXFT_LONG, &(mce->measurement_window_offset) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mce_get_current_num_values( MX_MCE *mce )
{
	static const char fname[] = "mxd_network_mce_get_current_num_values()";

	MX_NETWORK_MCE *network_mce = NULL;
	long num_values;
	mx_status_type mx_status;

	mx_status = mxd_network_mce_get_pointers( mce, &network_mce, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCE '%s'",
			fname, mce->record->name));

	mx_status = mx_get( &(network_mce->current_num_values_nf),
					MXFT_LONG, &num_values );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: num_values = %ld", fname, num_values));

	MX_DEBUG( 2,("%s: mce->maximum_num_values = %ld",
				fname, mce->maximum_num_values));

	if ( num_values > mce->maximum_num_values ) {
		mce->current_num_values = mce->maximum_num_values;
	} else {
		mce->current_num_values = num_values;
	}

	MX_DEBUG( 2,("%s: mce->current_num_values = %ld",
			fname, mce->current_num_values ));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mce_read_measurement( MX_MCE *mce )
{
	static const char fname[] = "mxd_network_mce_read_measurement()";

	MX_NETWORK_MCE *network_mce = NULL;
	mx_status_type mx_status;

	mx_status = mxd_network_mce_get_pointers( mce, &network_mce, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_mce->measurement_index_nf),
				MXFT_ULONG, &(mce->measurement_index) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_mce->value_nf),
				MXFT_DOUBLE, &(mce->value) );

#if MXD_NETWORK_MCE_DEBUG_READ_MEASUREMENT
	MX_DEBUG(-2,("%s: MCE '%s', index = %ld, value = %f",
		fname, mce->record->name, mce->measurement_index, mce->value));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mce_get_motor_record_array( MX_MCE *mce )
{
	static const char fname[] = "mxd_network_mce_get_motor_record_array()";

	MX_NETWORK_MCE *network_mce = NULL;
	mx_bool_type name_matches;
	long i, num_remote_motors;
	char **motor_name_array;
	long dimension_array[2];
	size_t size_array[2];
	MX_RECORD *list_head_record = NULL;
	MX_RECORD *current_record = NULL;
	MX_NETWORK_MOTOR *network_motor = NULL;
	char *remote_motor_name;
	char *non_mx_motor_name;
	mx_status_type mx_status;

	mx_status = mxd_network_mce_get_pointers( mce, &network_mce, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_MCE_DEBUG_MOTOR_ARRAY
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	/* The first time this function is invoked, we ask the remote
	 * server for the list of motors.  On the second and subsequent calls,
	 * we just return back the values that we received the first time.
	 *
	 * This is done so that client programs only suffer the speed penalty
	 * of looking up all of the record names on the _first_ quick scan
	 * they run and not on subsequent ones.
	 *
	 */

	if ( ( mce->num_motors > 0 ) && ( mce->motor_record_array != NULL ) )
	{
		/* We have already received the list of motors on a previous
		 * call, so just return the already cached values.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

#if MXD_NETWORK_MCE_DEBUG_MOTOR_ARRAY
	MX_DEBUG(-2,("%s: Finding motors for MCE '%s'",
			fname, mce->record->name ));
#endif

	mce->num_motors = 0;

	if ( mce->motor_record_array != NULL ) {
		mx_free( mce->motor_record_array );
	}

	mx_status = mx_mce_fixup_motor_record_array_field( mce );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out how many remote motors are supported by this MCE. */

	mx_status = mx_get( &(network_mce->num_motors_nf),
				MXFT_LONG, &num_remote_motors );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate the motor_record_array to store the local motor 
	 * record pointers.
	 */

	mce->motor_record_array = (MX_RECORD **)
		malloc( num_remote_motors * sizeof(MX_RECORD *) );

	if ( mce->motor_record_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld element "
			"array of MX_RECORD pointers for record '%s'.",
			num_remote_motors, mce->record->name );
	}

	/* Allocate an array to contain the remote motor names. */

	dimension_array[0] = num_remote_motors;
	dimension_array[1] = MXU_RECORD_NAME_LENGTH;

	size_array[0] = sizeof(char);
	size_array[1] = sizeof(char *);

	motor_name_array = (char **)
	    mx_allocate_array( MXFT_STRING, 2, dimension_array, size_array );

	if ( motor_name_array == NULL ) {
		mx_free( mce->motor_record_array );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld element "
			"array of %ld character strings for record '%s'.",
				dimension_array[0], dimension_array[1],
				mce->record->name );
	}

	/* Get the remote motor names. */

	mx_status = mx_get_array( &(network_mce->motor_record_array_nf),
				MXFT_STRING, 2, dimension_array,
				motor_name_array );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( mce->motor_record_array );
		mx_free_array( motor_name_array );

		return mx_status;
	}

#if MXD_NETWORK_MCE_DEBUG_MOTOR_ARRAY
	for ( i = 0; i < num_remote_motors; i++ ) {
		MX_DEBUG(-2,("%s: motor_name_array[%ld] = '%s'",
			fname, i, motor_name_array[i] ));
	}
#endif

	/* We now must look for the listed motors in our local database. */

	list_head_record = mce->record->list_head;

	current_record = list_head_record->next_record;

	while ( current_record != list_head_record ) {

	    /* Is this a motor record? */

	    if ( current_record->mx_class == MXC_MOTOR ) {

		/* Is this motor an MX network_motor record? */

		if ( current_record->mx_type == MXT_MTR_NETWORK ) {

			network_motor = (MX_NETWORK_MOTOR *)
					current_record->record_type_struct;

			/* The remote motor must be on the same server as
			 * the remote MCE.
			 */

			if ( network_mce->server_record
				== network_motor->server_record )
			{
				/* See if the remote record name matches
				 * one of the names in 'motor_name_array'.
				 */

				remote_motor_name
					= network_motor->remote_record_name;

				name_matches = FALSE;

				for ( i = 0; i < num_remote_motors; i++ ) {

					if ( strcmp( remote_motor_name,
						motor_name_array[i] ) == 0 )
					{
						name_matches = TRUE;
						break;
					}
				}

				if ( name_matches ) {
				    mce->motor_record_array[ mce->num_motors ]
					    = current_record;

				    (mce->num_motors)++;
				}
			}
		} else {
			/* If this motor record is _not_ a network_motor
			 * record, see if it uses another network protocol
			 * by checking to see if the network_type_name
			 * field has a non-empty name in it.
			 */

			if ( current_record->network_type_name[0] != '\0' ) {

				/* Non-MX network protocols are typically
				 * opaque to us, so we just _hope_ that the
				 * client database is using the same name
				 * for the motor as is used in the MX server
				 * that contains the MCE record.
				 */

				non_mx_motor_name = current_record->name;

				name_matches = FALSE;

				for ( i = 0; i < num_remote_motors; i++ ) {

					if ( strcmp( non_mx_motor_name,
						motor_name_array[i] ) == 0 )
					{
						name_matches = TRUE;
						break;
					}
				}

				if ( name_matches ) {
				    mce->motor_record_array[ mce->num_motors ]
						= current_record;

				    (mce->num_motors)++;
				}
			}
		}
	    }

	    current_record = current_record->next_record;
	}

	mx_free_array( motor_name_array );

	mx_status = mx_mce_fixup_motor_record_array_field( mce );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_MCE_DEBUG_MOTOR_ARRAY
	MX_DEBUG(-2,("%s: Found the following local matches:", fname));

	for ( i = 0; i < mce->num_motors; i++ ) {
		MX_DEBUG(-2,("%s: mce->motor_record_array[%ld] = '%s'",
			fname, i, mce->motor_record_array[i]->name ));
	}

	MX_DEBUG(-2,("%s: Finished finding motors for MCE '%s'",
			fname, mce->record->name ));
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mce_connect_mce_to_motor( MX_MCE *mce, MX_RECORD *motor_record )
{
	static const char fname[] = "mxd_network_mce_connect_mce_to_motor()";

	MX_NETWORK_MCE *network_mce = NULL;
	long dimension_array[1];
	mx_status_type mx_status;

	network_mce = NULL;

	mx_status = mxd_network_mce_get_pointers( mce, &network_mce, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The motor_record pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s invoked for MCE '%s'",
			fname, mce->record->name));

	/* The specified motor record must be one that uses a recognized
	 * network protocol.  We check for this by looking at the
	 * network_type_name field in the MX_RECORD object.  If this string
	 * has a non-zero length, then it is a supported motor type for this.
	 */

	if ( motor_record->network_type_name[0] == '\0' ) {

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The '%s' driver used by record '%s' selected for "
		"network MCE '%s' is not supported for MX network MCEs.",
			mx_get_driver_name( motor_record ),
    			motor_record->name,
			mce->record->name );
	}

	network_mce->selected_motor_record = motor_record;

	/* Send the motor name to the remote MX server. */

	dimension_array[0] = MXU_RECORD_NAME_LENGTH;

	mx_status = mx_put_array( &(network_mce->selected_motor_name_nf),
				MXFT_STRING, 1, dimension_array,
				mce->selected_motor_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the encoder type has not yet been set, this is a good time
	 * to find out what type of encoder the remote MCE is.
	 */

	if ( mce->encoder_type == MXT_MCE_UNKNOWN_ENCODER_TYPE ) {

		mx_status = mx_get( &(network_mce->encoder_type_nf),
				MXFT_LONG, &(mce->encoder_type) );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mce_get_parameter( MX_MCE *mce )
{
	static const char fname[] = "mxd_network_mce_get_parameter()";

	MX_NETWORK_MCE *network_mce = NULL;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_network_mce_get_pointers( mce, &network_mce, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_MCE_DEBUG_PARAMETERS
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, mce->record->name,
		mx_get_field_label_string( mce->record, mce->parameter_type ),
		mce->parameter_type ));
#endif

	switch( mce->parameter_type ) {
	case MXLV_MCE_USE_WINDOW:
		mx_status = mx_get( &(network_mce->use_window_nf),
				MXFT_BOOL, &(mce->use_window) );
		break;

	case MXLV_MCE_WINDOW:
		dimension[0] = 2;	/* window start, window end */

		mx_status = mx_get_array( &(network_mce->window_nf),
				MXFT_DOUBLE, 1, dimension, mce->window );
		break;

	case MXLV_MCE_WINDOW_IS_AVAILABLE:
		mx_status = mx_get( &(network_mce->window_is_available_nf),
				MXFT_BOOL, &(mce->window_is_available) );
		break;

	case MXLV_MCE_MEASUREMENT_WINDOW_OFFSET:
		mx_status = mx_get(
				&(network_mce->measurement_window_offset_nf),
				MXFT_LONG, &(mce->measurement_window_offset) );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by "
		"the driver for MCE '%s'.",
			mce->parameter_type, mce->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mce_set_parameter( MX_MCE *mce )
{
	static const char fname[] = "mxd_network_mce_set_parameter()";

	MX_NETWORK_MCE *network_mce = NULL;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_network_mce_get_pointers( mce, &network_mce, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_MCE_DEBUG_PARAMETERS
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, mce->record->name,
		mx_get_field_label_string( mce->record, mce->parameter_type ),
		mce->parameter_type ));
#endif

	switch( mce->parameter_type ) {
	case MXLV_MCE_USE_WINDOW:
		mx_status = mx_put( &(network_mce->use_window_nf),
				MXFT_BOOL, &(mce->use_window) );
		break;

	case MXLV_MCE_WINDOW:
		dimension[0] = 2;	/* window start, window end */

		mx_status = mx_put_array( &(network_mce->window_nf),
				MXFT_DOUBLE, 1, dimension, mce->window );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by "
		"the driver for MCE '%s'.",
			mce->parameter_type, mce->record->name );
		break;
	}

	return mx_status;
}

