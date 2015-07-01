/*
 * Name:    d_soft_scaler.c
 *
 * Purpose: MX scaler driver to control a software emulated scaler.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004-2006, 2010-2011, 2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_amplifier.h"
#include "mx_digital_output.h"
#include "mx_motor.h"
#include "mx_scaler.h"
#include "d_soft_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_soft_scaler_record_function_list = {
	mxd_soft_scaler_initialize_driver,
	mxd_soft_scaler_create_record_structures,
	mxd_soft_scaler_finish_record_initialization,
	mxd_soft_scaler_delete_record,
	NULL,
	mxd_soft_scaler_open,
	mxd_soft_scaler_close
};

MX_SCALER_FUNCTION_LIST mxd_soft_scaler_scaler_function_list = {
	mxd_soft_scaler_clear,
	mxd_soft_scaler_overflow_set,
	mxd_soft_scaler_read,
	NULL,
	mxd_soft_scaler_is_busy,
	mxd_soft_scaler_start,
	mxd_soft_scaler_stop
};

/* Soft scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_soft_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_SOFT_SCALER_STANDARD_FIELDS
};

long mxd_soft_scaler_num_record_fields
		= sizeof( mxd_soft_scaler_record_field_defaults )
		  / sizeof( mxd_soft_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_scaler_rfield_def_ptr
			= &mxd_soft_scaler_record_field_defaults[0];

/* === */

MX_EXPORT mx_status_type
mxd_soft_scaler_initialize_driver( MX_DRIVER *driver )
{
        static const char fname[] = "mxs_soft_scaler_initialize_driver()";

        MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
        long num_intensity_modifiers_varargs_cookie;
        mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

        mx_status = mx_find_record_field_defaults_index( driver,
                        			"num_intensity_modifiers",
						&referenced_field_index );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				&num_intensity_modifiers_varargs_cookie);

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        MX_DEBUG( 2,("%s: num_intensity_modifiers varargs cookie = %ld",
                        fname, num_intensity_modifiers_varargs_cookie));

	mx_status = mx_find_record_field_defaults( driver,
					"intensity_modifier_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_intensity_modifiers_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_soft_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_SOFT_SCALER *soft_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	soft_scaler = (MX_SOFT_SCALER *)
				malloc( sizeof(MX_SOFT_SCALER) );

	if ( soft_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SOFT_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = soft_scaler;
	record->class_specific_function_list
			= &mxd_soft_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_scaler_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_soft_scaler_finish_record_initialization()";

	MX_SOFT_SCALER *soft_scaler;
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	soft_scaler = (MX_SOFT_SCALER *)( record->record_type_struct );

	/* Allocate arrays for the motor positions and scaler values. */

	soft_scaler->motor_position = (double *)
		malloc( soft_scaler->num_datapoints * sizeof(double) );

	if ( soft_scaler->motor_position == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Couldn't allocate %lu doubles for motor position array.",
			soft_scaler->num_datapoints );
	}

	soft_scaler->scaler_value = (double *)
		malloc( soft_scaler->num_datapoints * sizeof(double) );

	if ( soft_scaler->scaler_value == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Couldn't allocate %lu doubles for scaler value array.",
			soft_scaler->num_datapoints );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_scaler_delete_record( MX_RECORD *record )
{
	MX_SOFT_SCALER *soft_scaler;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	soft_scaler = (MX_SOFT_SCALER *) record->record_type_struct;

	if ( soft_scaler != NULL ) {

		if ( soft_scaler->motor_position != NULL ) {
			free( soft_scaler->motor_position );
		}
		if ( soft_scaler->scaler_value != NULL ) {
			free( soft_scaler->scaler_value );
		}
		free( soft_scaler );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_scaler_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_scaler_open()";

	MX_SOFT_SCALER *soft_scaler;
	char datafile_name[ MXU_FILENAME_LENGTH + 1 ];
	FILE *datafile;
	char buffer[81];
	char *mxdir_ptr;
	long i;
	int saved_errno, num_items, filename_needs_expansion;
	double motor_position, scaler_value;

	soft_scaler = (MX_SOFT_SCALER *)( record->record_type_struct );

	/* Does the filename need to be expanded into a full name? */

	filename_needs_expansion = TRUE;

#ifdef OS_WIN32
	if ( strchr( soft_scaler->datafile_name, '\\' ) != 0 ) {
		filename_needs_expansion = FALSE;
	}
	if ( strchr( soft_scaler->datafile_name, ':' ) != 0 ) {
		filename_needs_expansion = FALSE;
	}
#endif /* OS_WIN32 */
	if ( strchr( soft_scaler->datafile_name, '/' ) != 0 ) {
		filename_needs_expansion = FALSE;
	}

	if ( filename_needs_expansion == FALSE ) {
		strlcpy( datafile_name, soft_scaler->datafile_name,
						MXU_FILENAME_LENGTH );
	} else {
		/* Does the MXDIR environment variable exist? */

		mxdir_ptr = getenv( "MXDIR" );

		if ( mxdir_ptr == NULL ) {

			/* If not, just use the unexpanded name. */

			strlcpy( datafile_name, soft_scaler->datafile_name,
						MXU_FILENAME_LENGTH );
		} else {
			/* Prepend the MXDIR string to the filename. */

			strlcpy( datafile_name, mxdir_ptr,
						MXU_FILENAME_LENGTH );

			strlcat( datafile_name, "/etc/", MXU_FILENAME_LENGTH );

			strlcat( datafile_name, soft_scaler->datafile_name,
						MXU_FILENAME_LENGTH );
		}
	}

	/* Open the scaler value datafile. */

	datafile = fopen( datafile_name, "r" );

	if ( datafile == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot open datafile '%s'.  Reason = '%s'",
			datafile_name, strerror( saved_errno ) );
	}

	/* Read in the datafile. */

	for ( i = 0; i < soft_scaler->num_datapoints; i++ ) {

		mx_fgets( buffer, sizeof buffer, datafile );

		if ( feof( datafile ) || ferror( datafile ) ) {
			fclose( datafile );

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Unable to read line %ld of soft scaler datafile '%s'",
				i+1, datafile_name );
		}

		num_items = sscanf( buffer, "%lg %lg",
					&motor_position, &scaler_value );

		if ( num_items != 2 ) {
			fclose( datafile );

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Line %ld of datafile '%s' is incorrectly formatted.  "
			"Contents = '%s'",
				i+1, datafile_name, buffer );
		}

		if ( (i > 0)
		  && (motor_position < soft_scaler->motor_position[i-1] ) ) {

			fclose( datafile );

			return mx_error( MXE_FILE_IO_ERROR, fname,
	"The motor position %g on line %ld in datafile '%s' is smaller than "
	"the previous line's position %g.  This is not permitted.",
			motor_position, i+1, datafile_name,
			soft_scaler->motor_position[i-1] );
		}

		soft_scaler->motor_position[i] = motor_position;
		soft_scaler->scaler_value[i] = scaler_value;
	}

	fclose( datafile );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_scaler_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_scaler_clear( MX_SCALER *scaler )
{
	scaler->raw_value = 0L;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_scaler_overflow_set( MX_SCALER *scaler )
{
	scaler->overflow_set = 0;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_soft_scaler_compute_intensity_modifier( MX_RECORD *modifier_record,
						double *modifier_value )
{
	static const char fname[] = "mxd_soft_scaler_compute_intensity_modifier()";

	unsigned long dout_value;
	double gain, filter_attenuation, single_thickness_attenuation;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for modifier '%s'",
				fname, modifier_record->name));

	*modifier_value = 1.0;

	switch( modifier_record->mx_superclass ) {
	default:
		break;

	case MXR_DEVICE:
		switch( modifier_record->mx_class ) {
		default:
			break;

		case MXC_AMPLIFIER:
			mx_status = mx_amplifier_get_gain( modifier_record,
								&gain );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			*modifier_value = ( gain / 1.0e8 );

			break;

		case MXC_DIGITAL_OUTPUT:
			mx_status = mx_digital_output_read( modifier_record,
							&dout_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Assume that each thicker filter is twice as
			 * thick as the previous one.
			 */

			single_thickness_attenuation = 0.5;

			filter_attenuation = 1.0;

			while ( dout_value != 0L ) {

				filter_attenuation
					*= single_thickness_attenuation;

				if ( dout_value & 1L ) {
					*modifier_value *= filter_attenuation;
				}

				dout_value >>= 1;
			}
			break;
		}
	}

	MX_DEBUG( 2,("%s: name = '%s', modifier_value = %g",
		fname, modifier_record->name, *modifier_value));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_soft_scaler_read()";

	MX_SOFT_SCALER *soft_scaler;
	double position, scaler_value, modifier_value;
	long i;
	mx_status_type mx_status;

	soft_scaler = (MX_SOFT_SCALER *)
				(scaler->record->record_type_struct);

	if ( soft_scaler == (MX_SOFT_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SOFT_SCALER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	/* Get the current motor position. */

	mx_status = mx_motor_get_position( soft_scaler->motor_record,
							&position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler_value = mxd_soft_scaler_get_value_from_position(
						soft_scaler, position );

	/* Apply any intensity modifiers. */

	for ( i = 0; i < soft_scaler->num_intensity_modifiers; i++ ) {

		mx_status = mxd_soft_scaler_compute_intensity_modifier(
				(soft_scaler->intensity_modifier_array)[0],
				&modifier_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		scaler_value *= modifier_value;
	}

	scaler->raw_value = mx_round( scaler_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT double
mxd_soft_scaler_get_value_from_position( MX_SOFT_SCALER *soft_scaler,
					double position )
{
	unsigned long i;
	double x1, x2, y1, y2, slope, intercept;
	double scaler_value;

	/* Use a linear search to find where in the motor position array
	 * that the given position appears.
	 */

	for ( i = 0; i < soft_scaler->num_datapoints; i++ ) {

		if ( position < soft_scaler->motor_position[i] )
			break;			/* Exit the for() loop. */
	}

	if ( i == 0 ) {

		scaler_value = soft_scaler->scaler_value[0];

	} else if ( i >= soft_scaler->num_datapoints ) {

		i = soft_scaler->num_datapoints - 1;

		scaler_value = soft_scaler->scaler_value[ i ];
	} else {
		x1 = soft_scaler->motor_position[ i-1 ];
		y1 = soft_scaler->scaler_value[ i-1 ];

		x2 = soft_scaler->motor_position[ i ];
		y2 = soft_scaler->scaler_value[ i ];

		/* Linear interpolation. */

		slope = ( y2 - y1 ) / ( x2 - x1 );

		intercept = y1 - slope * x1;

		scaler_value = slope * position + intercept;
	}

	return scaler_value;
}

MX_EXPORT mx_status_type
mxd_soft_scaler_is_busy( MX_SCALER *scaler )
{
	scaler->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_scaler_start( MX_SCALER *scaler )
{
	/* Wait for a second to simulate the scaler counting. */

	mx_msleep(1000);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_scaler_stop( MX_SCALER *scaler )
{
	scaler->raw_value = 0;

	return MX_SUCCESSFUL_RESULT;
}

