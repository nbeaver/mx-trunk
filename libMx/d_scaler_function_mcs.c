/*
 * Name:    d_scaler_function_mcs.c 
 *
 * Purpose: MX multichannel scaler driver for MX scaler function
 *          pseudo multichannel scalers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mxconfig.h"

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_mcs.h"
#include "mx_scaler.h"
#include "mx_variable.h"
#include "d_scaler_function_mcs.h"
#include "d_scaler_function.h"
#include "d_mcs_scaler.h"

/* Initialize the mcs driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_scaler_function_mcs_record_function_list = {
	mxd_scaler_function_mcs_initialize_type,
	mxd_scaler_function_mcs_create_record_structures,
	mxd_scaler_function_mcs_finish_record_initialization,
	mxd_scaler_function_mcs_delete_record,
	mxd_scaler_function_mcs_print_structure,
	NULL,
	NULL,
	mxd_scaler_function_mcs_open
};

MX_MCS_FUNCTION_LIST mxd_scaler_function_mcs_mcs_function_list = {
	mxd_scaler_function_mcs_start,
	mxd_scaler_function_mcs_stop,
	mxd_scaler_function_mcs_clear,
	mxd_scaler_function_mcs_busy,
	mxd_scaler_function_mcs_read_all,
	mxd_scaler_function_mcs_read_scaler,
	NULL,
	NULL,
	NULL,
	mxd_scaler_function_mcs_set_parameter
};

/* EPICS mcs data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_scaler_function_mcs_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCS_STANDARD_FIELDS,
	MXD_SCALER_FUNCTION_MCS_STANDARD_FIELDS
};

mx_length_type mxd_scaler_function_mcs_num_record_fields
		= sizeof( mxd_scaler_function_mcs_rf_defaults )
		/ sizeof( mxd_scaler_function_mcs_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_scaler_function_mcs_rfield_def_ptr
			= &mxd_scaler_function_mcs_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_scaler_function_mcs_get_pointers( MX_MCS *mcs,
			MX_SCALER_FUNCTION_MCS **scaler_function_mcs,
			MX_SCALER_FUNCTION **scaler_function,
			const char *calling_fname )
{
	static const char fname[] = "mxd_scaler_function_mcs_get_pointers()";

	MX_SCALER_FUNCTION_MCS *scaler_function_mcs_ptr;

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mcs->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MCS pointer passed by '%s' is NULL.",
			calling_fname );
	}

	scaler_function_mcs_ptr = (MX_SCALER_FUNCTION_MCS *)
					mcs->record->record_type_struct;

	if ( scaler_function_mcs_ptr == (MX_SCALER_FUNCTION_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCALER_FUNCTION_MCS pointer for record '%s' is NULL.",
			mcs->record->name );
	}

	if ( scaler_function_mcs_ptr->scaler_function_record
		== (MX_RECORD *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The scaler_function_record pointer for record '%s' is NULL.",
			mcs->record->name );
	}

	if ( scaler_function_mcs != (MX_SCALER_FUNCTION_MCS **) NULL ) {
		*scaler_function_mcs = scaler_function_mcs_ptr;
	}

	if ( scaler_function != (MX_SCALER_FUNCTION **) NULL ) {
		*scaler_function = (MX_SCALER_FUNCTION *)
	    scaler_function_mcs_ptr->scaler_function_record->record_type_struct;

		if ( (*scaler_function) == (MX_SCALER_FUNCTION *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SCALER_FUNCTION pointer for record '%s' used by record '%s' is NULL.",
			scaler_function_mcs_ptr->scaler_function_record->name,
			mcs->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	mx_length_type num_record_fields;
	mx_length_type maximum_num_scalers_varargs_cookie;
	mx_length_type maximum_num_measurements_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mcs_initialize_type( record_type,
				&num_record_fields,
				&record_field_defaults,
				&maximum_num_scalers_varargs_cookie,
				&maximum_num_measurements_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_scaler_function_mcs_create_record_structures()";

	MX_MCS *mcs;
	MX_SCALER_FUNCTION_MCS *scaler_function_mcs;

	/* Allocate memory for the necessary structures. */

	mcs = (MX_MCS *) malloc( sizeof(MX_MCS) );

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS structure." );
	}

	scaler_function_mcs = (MX_SCALER_FUNCTION_MCS *)
				malloc( sizeof(MX_SCALER_FUNCTION_MCS) );

	if ( scaler_function_mcs == (MX_SCALER_FUNCTION_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER_FUNCTION_MCS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mcs;
	record->record_type_struct = scaler_function_mcs;
	record->class_specific_function_list
				= &mxd_scaler_function_mcs_mcs_function_list;

	mcs->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_mcs_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_delete_record( MX_RECORD *record )
{
	MX_SCALER_FUNCTION_MCS *scaler_function_mcs;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	scaler_function_mcs = (MX_SCALER_FUNCTION_MCS *)
					record->record_type_struct;

	if ( scaler_function_mcs != NULL ) {
		if ( scaler_function_mcs->mcs_record_array != NULL ) {
			mx_free( scaler_function_mcs->mcs_record_array );
		}
		if ( scaler_function_mcs->scaler_mcs_record_array != NULL ) {
			mx_free( scaler_function_mcs->scaler_mcs_record_array );
		}
		if ( scaler_function_mcs->scaler_index_array != NULL ) {
			mx_free( scaler_function_mcs->scaler_index_array );
		}

		scaler_function_mcs->num_scalers = 0;

		mx_free( record->record_type_struct );
	}

	if ( record->record_class_struct != NULL ) {
		mx_free( record->record_class_struct );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_scaler_function_mcs_print_structure()";

	MX_MCS *mcs;
	MX_SCALER_FUNCTION_MCS *scaler_function_mcs;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mcs = (MX_MCS *) (record->record_class_struct);

	mx_status = mxd_scaler_function_mcs_get_pointers( mcs,
					&scaler_function_mcs, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MCS parameters for record '%s':\n", record->name);

	fprintf(file, "  MCS type                  = SCALER_FUNCTION_MCS.\n\n");
	fprintf(file, "  scaler function record    = %s\n",
			scaler_function_mcs->scaler_function_record->name);

	fprintf(file, "  maximum # scalers         = %lu\n",
				(unsigned long) mcs->maximum_num_scalers);
	fprintf(file, "  maximum # of measurements = %lu\n",
				(unsigned long) mcs->maximum_num_measurements);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_scaler_function_mcs_open()";

	MX_MCS *mcs;
	MX_SCALER_FUNCTION_MCS *scaler_function_mcs;
	MX_SCALER_FUNCTION *scaler_function;
	MX_RECORD *scaler_record;
	MX_MCS_SCALER *mcs_scaler;
	MX_RECORD *scaler_mcs_record;
	MX_MCS *scaler_mcs;
	int i, j, valid_type, not_yet_recorded;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	MX_DEBUG( 2,("%s invoked for record '%s'", fname, record->name ));

	mcs = (MX_MCS *) (record->record_class_struct);

	mx_status = mxd_scaler_function_mcs_get_pointers( mcs,
				&scaler_function_mcs, &scaler_function, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	valid_type = mx_verify_driver_type(
			scaler_function_mcs->scaler_function_record,
			MXR_DEVICE, MXC_SCALER, MXT_SCL_SCALER_FUNCTION );

	if ( valid_type == FALSE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"The scaler function record '%s' specified by scaler function MCS record '%s' "
"is not actually an MX 'scaler_function' record.",
			scaler_function_mcs->scaler_function_record->name,
			record->name );
	}

	/* Allocate arrays to save the mapping from scaler to MCS channel. */

	scaler_function_mcs->num_scalers = scaler_function->num_scalers;

	scaler_function_mcs->scaler_mcs_record_array = (MX_RECORD **)
	    malloc( scaler_function_mcs->num_scalers * sizeof(MX_RECORD *) );

	if ( scaler_function_mcs->scaler_mcs_record_array
			== (MX_RECORD **) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate an %ld element MX_RECORD pointer array.",
			scaler_function_mcs->num_scalers );
	}

	scaler_function_mcs->scaler_index_array = (int *)
	    malloc( scaler_function_mcs->num_scalers * sizeof(int) );

	if ( scaler_function_mcs->scaler_index_array == (int *) NULL ) {
		mx_free( scaler_function_mcs->scaler_mcs_record_array );
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate an %ld element integer array.",
			scaler_function_mcs->num_scalers );
	}

	/* Now save the mapping from scaler to MCS channel. */

	for ( i = 0; i < scaler_function_mcs->num_scalers; i++ ) {
		scaler_record = scaler_function->scaler_record_array[i];

		valid_type = mx_verify_driver_type( scaler_record,
				MXR_DEVICE, MXC_SCALER, MXT_SCL_MCS );

		if ( valid_type == FALSE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
	"Scaler function record '%s' used by scaler function MCS '%s' "
	"makes use of scaler record '%s' which is not an MCS scaler.  "
	"The use of non-MCS scalers by scaler function MCS '%s' "
	"is not supported.", scaler_function->record->name, record->name,
				scaler_record->name, record->name );
		}

		mcs_scaler = (MX_MCS_SCALER *)
					scaler_record->record_type_struct;

		scaler_mcs_record = mcs_scaler->mcs_record;

		scaler_function_mcs->scaler_mcs_record_array[i]
				= scaler_mcs_record;

		scaler_mcs = (MX_MCS *) scaler_mcs_record->record_class_struct;

		scaler_function_mcs->scaler_index_array[i] =
						mcs_scaler->scaler_number;
	}

	/* We also need a list of the MCSs used without duplicates so that
	 * we do not send multiple starts, stops, clears, etc. to the 
	 * same MCS.
	 */

	scaler_function_mcs->mcs_record_array = (MX_RECORD **)
	    malloc( scaler_function_mcs->num_scalers * sizeof(MX_RECORD *) );

	if ( scaler_function_mcs->mcs_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate an %ld element MX_RECORD pointer array.",
			scaler_function_mcs->num_scalers );
	}

	scaler_function_mcs->num_mcs = 0;

	for ( i = 0; i < scaler_function_mcs->num_scalers; i++ ) {
		not_yet_recorded = TRUE;

		for ( j = 0; j < scaler_function_mcs->num_mcs; j++ ) {
			if ( scaler_function_mcs->scaler_mcs_record_array[i]
				== scaler_function_mcs->mcs_record_array[j] )
			{
				not_yet_recorded = FALSE;

				break;	/* This MCS is already recorded. */
			}
		}
		if ( not_yet_recorded ) {
			/* The MCS was not already recorded, so record it. */

			scaler_function_mcs->mcs_record_array
				[ scaler_function_mcs->num_mcs ]
			    = scaler_function_mcs->scaler_mcs_record_array[i];

			(scaler_function_mcs->num_mcs)++;
		}
	}

	/* Zero out the rest of the array. */

	for ( j = scaler_function_mcs->num_mcs;
		j < scaler_function_mcs->num_scalers; j++ )
	{
		scaler_function_mcs->mcs_record_array[j] = NULL;
	}

#if 0
	for ( i = 0; i < scaler_function_mcs->num_scalers; i++ ) {
		MX_DEBUG(-2,("%s: scaler '%s', MCS '%s', scaler number %d",
			fname, scaler_function->scaler_record_array[i]->name,
			scaler_function_mcs->scaler_mcs_record_array[i]->name,
			scaler_function_mcs->scaler_index_array[i]));
	}

	for ( j = 0; j < scaler_function_mcs->num_mcs; j++ ) {
		MX_DEBUG(-2,("%s: MCS %d = '%s'", fname, j,
			scaler_function_mcs->mcs_record_array[j]->name));
	}
#endif

	/* If not already found by some previous initialization step
	 * such as mx_mcs_finish_record_initialization(), then look for
	 * the timer record ourselves.
	 */

	if ( mcs->timer_record == (MX_RECORD *) NULL ) {

		strcpy( mcs->timer_name, "" );

		if ( scaler_function_mcs->num_mcs == 1 ) {

			/* No timer name was specified, but the records
			 * used by the scaler function record all depend
			 * on the same MCS, so we can find the timer from
			 * that MCS record.
			 */

			scaler_mcs = (MX_MCS *)
		scaler_function_mcs->mcs_record_array[0]->record_class_struct;

			if ( scaler_mcs == (MX_MCS *) NULL ) {
				return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
			fname, "The MX_MCS pointer for MCS '%s' is NULL.",
				scaler_function_mcs->mcs_record_array[0]->name);
			}

			mcs->timer_record = scaler_mcs->timer_record;

			strlcpy( mcs->timer_name,
				mcs->timer_record->name,
				MXU_RECORD_NAME_LENGTH );
		} else {
			/* No timer name was specified and the records used
			 * by the scaler function record depend on more than
			 * one unique MCS.
			 */

			mx_warning(
	"Cannot figure out which MCS timer record to use for MCS '%s' "
	"since there are multiple possible candidates.  You should explicitly "
	"specify the MCS timer name for MCS '%s' in the database.  "
	"If you do not, this may make it impossible to use some of your "
	"MCS scan records.", record->name, record->name );
		}
	}

	MX_DEBUG( 2,("%s completed for record '%s'", fname, record->name ));

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_start( MX_MCS *mcs )
{
	static const char fname[] = "mxd_scaler_function_mcs_start()";

	MX_SCALER_FUNCTION_MCS *scaler_function_mcs;
	MX_RECORD *mcs_record;
	long i;
	mx_status_type mx_status;

	mx_status = mxd_scaler_function_mcs_get_pointers( mcs,
					&scaler_function_mcs, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < scaler_function_mcs->num_mcs; i++ ) {
		mcs_record = scaler_function_mcs->mcs_record_array[i];

		mx_status = mx_mcs_start( mcs_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_stop( MX_MCS *mcs )
{
	static const char fname[] = "mxd_scaler_function_mcs_stop()";

	MX_SCALER_FUNCTION_MCS *scaler_function_mcs;
	MX_RECORD *mcs_record;
	long i;
	mx_status_type mx_status;

	mx_status = mxd_scaler_function_mcs_get_pointers( mcs,
					&scaler_function_mcs, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < scaler_function_mcs->num_mcs; i++ ) {
		mcs_record = scaler_function_mcs->mcs_record_array[i];

		mx_status = mx_mcs_stop( mcs_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_clear( MX_MCS *mcs )
{
	static const char fname[] = "mxd_scaler_function_mcs_clear()";

	MX_SCALER_FUNCTION_MCS *scaler_function_mcs;
	MX_RECORD *mcs_record;
	long i;
	mx_status_type mx_status;

	mx_status = mxd_scaler_function_mcs_get_pointers( mcs,
					&scaler_function_mcs, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < scaler_function_mcs->num_mcs; i++ ) {
		mcs_record = scaler_function_mcs->mcs_record_array[i];

		mx_status = mx_mcs_clear( mcs_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_busy( MX_MCS *mcs )
{
	static const char fname[] = "mxd_scaler_function_mcs_busy()";

	MX_SCALER_FUNCTION_MCS *scaler_function_mcs;
	MX_RECORD *mcs_record;
	long i;
	int busy;
	mx_status_type mx_status;

	mx_status = mxd_scaler_function_mcs_get_pointers( mcs,
					&scaler_function_mcs, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs->busy = FALSE;

	for ( i = 0; i < scaler_function_mcs->num_mcs; i++ ) {
		mcs_record = scaler_function_mcs->mcs_record_array[i];

		mx_status = mx_mcs_is_busy( mcs_record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy ) {
			mcs->busy = TRUE;

			break;	/* Exit the for() loop. */
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_read_all( MX_MCS *mcs )
{
	static const char fname[] = "mxd_scaler_function_mcs_read_all()";

	long i;
	mx_status_type mx_status;

	if ( mcs == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCS pointer passed was NULL." );
	}

	for ( i = 0; i < mcs->current_num_scalers; i++ ) {

		mcs->scaler_index = i;

		mx_status = mxd_scaler_function_mcs_read_scaler( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_read_scaler( MX_MCS *mcs )
{
	static const char fname[] = "mxd_scaler_function_mcs_read_scaler()";

	MX_SCALER_FUNCTION_MCS *scaler_function_mcs;
	MX_SCALER_FUNCTION *scaler_function;
	MX_RECORD *child_record;
	MX_RECORD *scaler_mcs_record;
	MX_MCS *scaler_mcs;
	long i, j;
	int32_t int32_value;
	int32_t *data_ptr;
	int scaler_index;

	mx_length_type num_scalers;
	MX_RECORD **scaler_record_array;
	double *real_scaler_scale;
	double *real_scaler_offset;

	mx_length_type num_variables;
	MX_RECORD **variable_record_array;
	double *real_variable_scale;
	double *real_variable_offset;

	double sum, variable_sum, scaled_value, double_value;

	mx_status_type mx_status;

	mx_status = mxd_scaler_function_mcs_get_pointers( mcs,
				&scaler_function_mcs, &scaler_function, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Setup pointers needed by this routine. */

	num_scalers = scaler_function->num_scalers;
	scaler_record_array = scaler_function->scaler_record_array;
	real_scaler_scale = scaler_function->real_scaler_scale;
	real_scaler_offset = scaler_function->real_scaler_offset;

	num_variables = scaler_function->num_variables;
	variable_record_array = scaler_function->variable_record_array;
	real_variable_scale = scaler_function->real_variable_scale;
	real_variable_offset = scaler_function->real_variable_offset;

	/* Compute the total variable sum first since it will be the
	 * same for all measurements.
	 */

	variable_sum = 0.0;

        for ( i = 0; i < num_variables; i++ ) {

                child_record = variable_record_array[i];

                mx_status = mx_get_double_variable( child_record,
                                                        &double_value );

                if ( mx_status.code != MXE_SUCCESS )
                        return mx_status;

                scaled_value = real_variable_offset[i] + real_variable_scale[i]
                                                        * double_value;

                variable_sum += scaled_value;

                MX_DEBUG( 2,
		("%s: variable = '%s', variable sum = %g, scaled_value = %g",
			fname, child_record->name, variable_sum, scaled_value));
        }

	/* Read all of the MCS scaler channels. */

	for ( i = 0; i < scaler_function_mcs->num_scalers; i++ ) {

		scaler_mcs_record =
			scaler_function_mcs->scaler_mcs_record_array[i];

		scaler_index = scaler_function_mcs->scaler_index_array[i];

		mx_status = mx_mcs_read_scaler( scaler_mcs_record, scaler_index,
							NULL, NULL );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Compute all the values for the scaler function MCS. */

	data_ptr = mcs->data_array[ mcs->scaler_index ];

	for ( j = 0; j < mcs->current_num_measurements; j++ ) {
		sum = variable_sum;

		for ( i = 0; i < scaler_function_mcs->num_scalers; i++ ) {
			scaler_mcs =
	scaler_function_mcs->scaler_mcs_record_array[i]->record_class_struct;

			scaler_index =
				scaler_function_mcs->scaler_index_array[i];

			int32_value = scaler_mcs->data_array[ scaler_index ][j];

			scaled_value = real_scaler_offset[i] +
			    real_scaler_scale[i] * (double) int32_value;

			sum += scaled_value;
		}

		data_ptr[j] = mx_round( sum );
	}

#if 0
	{
		fprintf(stderr,"%s: MCS '%s' -> ", fname, mcs->record->name);
		for ( i = 0; i < mcs->current_num_measurements; i++ ) {
			fprintf(stderr,"%ld ", (long) data_ptr[i]);
		}
		fprintf(stderr,"\n");
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scaler_function_mcs_set_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_scaler_function_mcs_set_parameter()";

	MX_SCALER_FUNCTION_MCS *scaler_function_mcs;
	MX_SCALER_FUNCTION *scaler_function;
	int i;
	mx_status_type mx_status;

	mx_status = mxd_scaler_function_mcs_get_pointers( mcs,
				&scaler_function_mcs, &scaler_function, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 1,("%s invoked for MCS '%s', type = %d",
		fname, mcs->record->name, mcs->parameter_type));

	if ( mcs->parameter_type == MXLV_MCS_MODE ) {

		for ( i = 0; i < scaler_function_mcs->num_mcs; i++ ) {
			mx_status = mx_mcs_set_mode(
				scaler_function_mcs->mcs_record_array[i],
				mcs->mode );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

	} else if ( mcs->parameter_type == MXLV_MCS_MEASUREMENT_TIME ) {

		for ( i = 0; i < scaler_function_mcs->num_mcs; i++ ) {
			mx_status = mx_mcs_set_measurement_time(
				scaler_function_mcs->mcs_record_array[i],
				mcs->measurement_time );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

	} else if ( mcs->parameter_type == MXLV_MCS_MEASUREMENT_COUNTS ) {

		for ( i = 0; i < scaler_function_mcs->num_mcs; i++ ) {
			mx_status = mx_mcs_set_measurement_counts(
				scaler_function_mcs->mcs_record_array[i],
				mcs->measurement_counts );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

	} else if ( mcs->parameter_type == MXLV_MCS_CURRENT_NUM_MEASUREMENTS ){

		for ( i = 0; i < scaler_function_mcs->num_mcs; i++ ) {
			mx_status = mx_mcs_set_num_measurements(
				scaler_function_mcs->mcs_record_array[i],
				mcs->current_num_measurements );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

	} else {
		mx_status = mx_mcs_default_set_parameter_handler( mcs );
	}
	MX_DEBUG( 1,("%s complete.", fname));

	return mx_status;
}

