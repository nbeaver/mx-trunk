/*
 * Name:    d_pmac_mce.c
 *
 * Purpose: MX multichannel encoder driver to record the position of
 *          a PMAC motor in a pair of multichannel scaler channels.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PMAC_MCE_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_mce.h"
#include "mx_mcs.h"
#include "mx_motor.h"
#include "i_pmac.h"
#include "d_pmac.h"
#include "d_pmac_mce.h"

/* Initialize the MCE driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_pmac_mce_record_function_list = {
	mxd_pmac_mce_initialize_type,
	mxd_pmac_mce_create_record_structures,
	mxd_pmac_mce_finish_record_initialization,
	mxd_pmac_mce_delete_record
};

MX_MCE_FUNCTION_LIST mxd_pmac_mce_mce_function_list = {
	NULL,
	NULL,
	mxd_pmac_mce_read,
	mxd_pmac_mce_get_current_num_values,
	NULL,
	mxd_pmac_mce_connect_mce_to_motor
};

/* MCS encoder data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_pmac_mce_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCE_STANDARD_FIELDS,
	MXD_PMAC_MCE_STANDARD_FIELDS
};

long mxd_pmac_mce_num_record_fields
		= sizeof( mxd_pmac_mce_record_field_defaults )
		  / sizeof( mxd_pmac_mce_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmac_mce_rfield_def_ptr
			= &mxd_pmac_mce_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_pmac_mce_get_pointers( MX_MCE *mce,
			MX_PMAC_MCE **pmac_mce,
			MX_MCS **mcs,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmac_mce_get_pointers()";

	MX_PMAC_MCE *pmac_mce_ptr;

	if ( mce == (MX_MCE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pmac_mce_ptr = (MX_PMAC_MCE *) mce->record->record_type_struct;

	if ( pmac_mce_ptr == (MX_PMAC_MCE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PMAC_MCE pointer for record '%s' is NULL.",
			mce->record->name );
	}

	if ( pmac_mce != (MX_PMAC_MCE **) NULL ) {
		*pmac_mce = pmac_mce_ptr;
	}

	if ( mcs != (MX_MCS **) NULL ) {
		*mcs = (MX_MCS *) pmac_mce_ptr->mcs_record->record_class_struct;

		if ( *mcs == (MX_MCS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"MX_MCS pointer for MCS record '%s' is NULL.",
				pmac_mce_ptr->mcs_record->name );
		}
		if ( (*mcs)->data_array == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"data_array pointer for MCS record '%s' is NULL.",
				pmac_mce_ptr->mcs_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_pmac_mce_initialize_type( long record_type )
{
	long num_record_fields;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long maximum_num_values_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mce_initialize_type( record_type,
					&num_record_fields,
					&record_field_defaults,
					&maximum_num_values_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_mce_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmac_mce_create_record_structures()";

	MX_MCE *mce;
	MX_PMAC_MCE *pmac_mce;

	/* Allocate memory for the necessary structures. */

	mce = (MX_MCE *) malloc( sizeof(MX_MCE) );

	if ( mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCE structure." );
	}

	pmac_mce = (MX_PMAC_MCE *) malloc( sizeof(MX_PMAC_MCE) );

	if ( pmac_mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PMAC_MCE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mce;
	record->record_type_struct = pmac_mce;
	record->class_specific_function_list
			= &mxd_pmac_mce_mce_function_list;

	mce->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_mce_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_pmac_mce_finish_record_initialization()";

	MX_RECORD *list_head_record, *current_record, *real_motor_record;
	MX_MCE *mce;
	MX_PMAC_MCE *pmac_mce;
	MX_MCS *mcs;
	MX_MOTOR *motor;
	MX_PMAC_MOTOR *pmac_motor;
	int is_a_motor;
	mx_status_type mx_status;

	mce = (MX_MCE *) record->record_class_struct;

	mx_status = mxd_pmac_mce_get_pointers( mce,
					&pmac_mce, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate memory for the motor record array.  Since
	 * this field is marked with the flag MXFF_VARARGS, we
	 * cannot safely allocate this array until the 
	 * finish_record_initialization function is executed.
	 */

	mce->num_motors = 0;

	mce->motor_record_array = (MX_RECORD **)
		malloc( MX_PMAC_MAX_MOTORS * sizeof(MX_RECORD *) );

	if ( mce->motor_record_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate a %d element array of MX_RECORD pointers for "
		"the mce->motor_record_array.", MX_PMAC_MAX_MOTORS );
	}

	mce->encoder_type = MXT_MCE_DELTA_ENCODER;

	mce->current_num_values = mce->maximum_num_values;

	/* Check to see if the scaler channel numbers listed are in
	 * the legal range.
	 */

	if ( pmac_mce->up_channel >= mcs->maximum_num_scalers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The up channel %lu for MCS encoder '%s' is outside the allowed range 0-%ld "
"for MCS '%s'.", pmac_mce->up_channel, record->name,
		mcs->maximum_num_scalers - 1L,
		pmac_mce->mcs_record->name );
	}

	if ( pmac_mce->down_channel >= mcs->maximum_num_scalers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The down channel %lu for MCS encoder '%s' is outside the allowed range 0-%ld "
"for MCS '%s'.", pmac_mce->down_channel, record->name,
		mcs->maximum_num_scalers - 1L,
		pmac_mce->mcs_record->name );
	}

	/* Find all the records that can be handled by this MCE. */

	list_head_record = record->list_head;

	current_record = list_head_record->next_record;

	while ( current_record != list_head_record ) {

	    /* We are only interested in motor records. */

	    is_a_motor = mx_verify_driver_type( current_record,
			    		MXR_DEVICE, MXC_MOTOR, MXT_ANY );

	    if ( is_a_motor ) {

	        /* If the current motor is a pseudomotor, try to find the
	         * real motor record.  Note that for pseudomotors that 
		 * cannot quick scan, the real_motor_record pointer will
		 * be set to NULL.
	         */

	        motor = (MX_MOTOR *) current_record->record_class_struct;

		if ( motor->motor_flags & MXF_MTR_CANNOT_QUICK_SCAN ) {
		    real_motor_record = NULL;

		} else if ( motor->motor_flags & MXF_MTR_IS_PSEUDOMOTOR ) {
		    real_motor_record = motor->real_motor_record;

	        } else {
		    real_motor_record = current_record;
		}

		/* Only go further if real_motor_record is not NULL. */

		if ( real_motor_record != NULL ) {

		    /* Is the real motor record a PMAC motor record? */

		    if ( real_motor_record->mx_type == MXT_MTR_PMAC ) {

			/* Is this PMAC motor handled by the same PMAC
			 * controller that this PMAC MCE uses?
			 */

			pmac_motor = (MX_PMAC_MOTOR *)
					real_motor_record->record_type_struct;

			if ( (pmac_motor->pmac_record == pmac_mce->pmac_record)
			  && (pmac_motor->card_number == pmac_mce->card_number))
			{
				/* Yes, this PMAC motor is handled by the
				 * same controller as the MCE, so add the
				 * current record to the list.
				 */

				mce->motor_record_array[ mce->num_motors ]
					= current_record;

				(mce->num_motors)++;
			}
		    }
		}
	    }
	    current_record = current_record->next_record;
	}

	pmac_mce->selected_motor_record = NULL;

	strcpy( mce->selected_motor_name, "" );

#if 0
	{
		int i;

		for ( i = 0; i < mce->num_motors; i++ ) {
			MX_DEBUG(-2,("%s: motor_record_array[%d] = '%s'",
				fname, i, mce->motor_record_array[i]->name ));
		}
	}
#endif

	/* Set the array size in the 'motor_record_array' field. */

	mx_status = mx_mce_fixup_motor_record_array_field( mce );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_mce_delete_record( MX_RECORD *record )
{
	MX_MCE *mce;

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
mxd_pmac_mce_read( MX_MCE *mce )
{
	static const char fname[] = "mxd_pmac_mce_read()";

	MX_PMAC_MCE *pmac_mce;
	MX_MCS *mcs;
	MX_MOTOR *selected_motor;
	double motor_scale, raw_encoder_value, scaled_encoder_value;
	long i;
	long up_channel, down_channel;
	long *up_value_array, *down_value_array;
	mx_status_type mx_status;

	mx_status = mxd_pmac_mce_get_pointers( mce,
					&pmac_mce, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCE '%s'",
			fname, mce->record->name));

        if ( pmac_mce->selected_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No motor has been selected yet for PMAC MCE '%s'.",
			mce->record->name );
        }

        selected_motor = (MX_MOTOR *)
                pmac_mce->selected_motor_record->record_class_struct;

        if ( selected_motor == (MX_MOTOR *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "The MX_MOTOR pointer for motor record '%s' selected "
                "by PMAC MCE '%s' is NULL.",
                        pmac_mce->selected_motor_record->name,
                        mce->record->name );
        }

        motor_scale = selected_motor->scale;

	up_channel = (long) pmac_mce->up_channel;
	down_channel = (long) pmac_mce->down_channel;

	mx_status = mx_mcs_read_scaler( pmac_mce->mcs_record,
						up_channel, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_read_scaler( pmac_mce->mcs_record,
						down_channel, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	up_value_array = mcs->data_array[ up_channel ];
	down_value_array = mcs->data_array[ down_channel ];

	if ( mcs->current_num_measurements <= mce->maximum_num_values ) {
		mce->current_num_values = (long) mcs->current_num_measurements;
	} else {
		mce->current_num_values = mce->maximum_num_values;

		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"\007*** MX database configuration error ***\007\n"
	"MCS record '%s' currently contains %ld measurements, "
	"but MCE record '%s' is only configured for a maximum of %ld "
	"motor positions.  This means that the recorded motor positions "
	"for all measurements after measurement %ld will be INCORRECT!!!  "
	"This can only be fixed by modifying MX records '%s' and '%s' in "
	"your MX database configuration file.",
			mcs->record->name, (long) mcs->current_num_measurements,
			mce->record->name, mce->maximum_num_values,
			mce->maximum_num_values,
			mcs->record->name, mce->record->name );
	}

	for ( i = 0; i < mce->current_num_values; i++ ) {

		raw_encoder_value = (double)
			( up_value_array[i] - down_value_array[i] );

		scaled_encoder_value =
			mce->offset + mce->scale * raw_encoder_value;

		mce->value_array[i] = motor_scale * scaled_encoder_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_mce_get_current_num_values( MX_MCE *mce )
{
	static const char fname[] = "mxd_pmac_mce_get_current_num_values()";

	MX_PMAC_MCE *pmac_mce;
	MX_MCS *mcs;
	mx_status_type mx_status;

	mx_status = mxd_pmac_mce_get_pointers( mce,
					&pmac_mce, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCE '%s'", fname, mce->record->name));

	mce->current_num_values = (long) mcs->current_num_measurements;

	MX_DEBUG( 2,("%s: mce->current_num_values = %ld",
		fname, mce->current_num_values));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_mce_connect_mce_to_motor( MX_MCE *mce, MX_RECORD *motor_record )
{
	static const char fname[] = "mxd_pmac_mce_connect_mce_to_motor()";

	MX_PMAC_MCE *pmac_mce;
	MX_RECORD *pmac_interface_record;
	MX_PMAC *pmac;
	MX_PMAC_MOTOR *pmac_motor;
	char command[80];
	mx_status_type mx_status;

	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mx_status = mxd_pmac_mce_get_pointers( mce, &pmac_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCE '%s'", fname, mce->record->name));

	pmac_interface_record = pmac_mce->pmac_record;

	if ( pmac_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The pmac_interface_record pointer for PMAC MCE record '%s' is NULL.",
			pmac_interface_record->name );
	}

	pmac = (MX_PMAC *) pmac_interface_record->record_type_struct;

	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PMAC pointer for PMAC controller '%s' is NULL.",
			pmac_interface_record->name );
	}

	/*---*/

	pmac_motor = (MX_PMAC_MOTOR *) motor_record->record_type_struct;

	if ( pmac_motor == (MX_PMAC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PMAC_MOTOR pointer for motor '%s' is NULL.",
			motor_record->name );
	}

	pmac_mce->selected_motor_record = motor_record;

	/* Change the configured axis number for the MCE by assigning it
	 * to the PMAC variable M3300.
	 */

	if ( pmac->num_cards > 1 ) {
		sprintf( command, "@%xM3300=%d", pmac_motor->card_number,
						pmac_motor->motor_number );
	} else {
		sprintf( command, "M3300=%d", pmac_motor->motor_number );
	}

	mx_status = mxi_pmac_command( pmac, command,
					NULL, 0, MXD_PMAC_MCE_DEBUG );

	/* Run the PLCC program to change the axis. */

	if ( pmac->num_cards > 1 ) {
		sprintf( command, "@%xENA PLC %d", pmac_motor->card_number,
						pmac_mce->plc_program_number );
	} else {
		sprintf( command, "ENA PLC %d", pmac_mce->plc_program_number );
	}

	mx_status = mxi_pmac_command( pmac, command,
					NULL, 0, MXD_PMAC_MCE_DEBUG );

	return mx_status;
}

