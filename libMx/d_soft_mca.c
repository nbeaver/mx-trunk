/*
 * Name:    d_soft_mca.c
 *
 * Purpose: MX multichannel analyzer driver for software-emulated MCAs
 *          using simple Monte Carlo calculations.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2005-2006, 2008, 2010-2012
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_MCA_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_constants.h"
#include "mx_mca.h"
#include "d_soft_mca.h"

/* Initialize the MCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_soft_mca_record_function_list = {
	mxd_soft_mca_initialize_driver,
	mxd_soft_mca_create_record_structures,
	mxd_soft_mca_finish_record_initialization,
	mxd_soft_mca_delete_record,
	mxd_soft_mca_print_structure,
	mxd_soft_mca_open
};

MX_MCA_FUNCTION_LIST mxd_soft_mca_mca_function_list = {
	mxd_soft_mca_start,
	mxd_soft_mca_stop,
	mxd_soft_mca_read,
	mxd_soft_mca_clear,
	mxd_soft_mca_busy,
	mxd_soft_mca_get_parameter,
	mx_mca_default_set_parameter_handler
};

MX_RECORD_FIELD_DEFAULTS mxd_soft_mca_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCA_STANDARD_FIELDS,
	MXD_SOFT_MCA_STANDARD_FIELDS
};

long mxd_soft_mca_num_record_fields
		= sizeof( mxd_soft_mca_record_field_defaults )
		  / sizeof( mxd_soft_mca_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_mca_rfield_def_ptr
			= &mxd_soft_mca_record_field_defaults[0];

/*--------*/

static mx_status_type
mxd_soft_mca_get_pointers( MX_MCA *mca,
			MX_SOFT_MCA **soft_mca,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_mca_get_pointers()";

	MX_RECORD *mca_record;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( soft_mca == (MX_SOFT_MCA **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SOFT_MCA pointer passed by '%s' was NULL",
			calling_fname );
	}

	mca_record = mca->record;

	if ( mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_MCA pointer %p is NULL.", mca );
	}

	*soft_mca = (MX_SOFT_MCA *) mca_record->record_type_struct;

	if ( *soft_mca == (MX_SOFT_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SOFT_MCA pointer for mca record '%s' passed by '%s' is NULL",
				mca_record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_soft_mca_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_channels_varargs_cookie;
	long maximum_num_rois_varargs_cookie;
	long num_soft_rois_varargs_cookie;

	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	long num_sources_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mca_initialize_driver( driver,
				&maximum_num_channels_varargs_cookie,
				&maximum_num_rois_varargs_cookie,
				&num_soft_rois_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct a varargs cookie for 'num_sources'. */

	mx_status = mx_find_record_field_defaults_index( driver,
						"num_sources",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
					&num_sources_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the 'source_string_array' field. */

	mx_status = mx_find_record_field_defaults( driver,
					"source_string_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_sources_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxd_soft_mca_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_mca_create_record_structures()";

	MX_MCA *mca;
	MX_SOFT_MCA *soft_mca = NULL;

	/* Allocate memory for the necessary structures. */

	mca = (MX_MCA *) malloc( sizeof(MX_MCA) );

	if ( mca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA structure." );
	}

	soft_mca = (MX_SOFT_MCA *) malloc( sizeof(MX_SOFT_MCA) );

	if ( soft_mca == (MX_SOFT_MCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SOFT_MCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mca;
	record->record_type_struct = soft_mca;
	record->class_specific_function_list = &mxd_soft_mca_mca_function_list;

	mca->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mca_finish_record_initialization( MX_RECORD *record )
{
#if 0
	static const char fname[] =
		"mxd_soft_mca_finish_record_initialization()";
#endif

	MX_MCA *mca;
	MX_SOFT_MCA *soft_mca = NULL;
	mx_status_type mx_status;

	mca = (MX_MCA *) record->record_class_struct;

	mca->current_num_channels = mca->maximum_num_channels;

	mca->busy = FALSE;

	mca->current_num_rois = mca->maximum_num_rois;

	soft_mca = (MX_SOFT_MCA *) record->record_type_struct;

	/* Do soft_mca stuff here. */

	mx_status = mx_mca_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mca_delete_record( MX_RECORD *record )
{
	MX_MCA *mca;
	MX_SOFT_MCA *soft_mca = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	soft_mca = (MX_SOFT_MCA *) record->record_type_struct;

	if ( soft_mca != (MX_SOFT_MCA *) NULL ) {

		/* Free soft_mca structures here. */

		mx_free( record->record_type_struct );

		record->record_type_struct = NULL;
	}

	mca = (MX_MCA *) record->record_class_struct;

	if ( mca != (MX_MCA *) NULL ) {

		mx_free( record->record_class_struct );

		record->record_class_struct = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mca_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_mca_print_structure()";

	MX_MCA *mca;
	MX_SOFT_MCA *soft_mca = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mca = (MX_MCA *) (record->record_class_struct);

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MCA pointer for record '%s' is NULL.", record->name);
	}

	soft_mca = (MX_SOFT_MCA *) (record->record_type_struct);

	if ( soft_mca == (MX_SOFT_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SOFT_MCA pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "MCA parameters for record '%s':\n", record->name);

	fprintf(file, "  MCA type              = SOFT_MCA.\n\n");
	fprintf(file, "  maximum # of channels = %ld\n",
					mca->maximum_num_channels);
	fprintf(file, "  maximum # of ROIs     = %ld\n",
					mca->maximum_num_rois);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mca_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_mca_open()";

	MX_MCA *mca = NULL;
	MX_SOFT_MCA *soft_mca = NULL;
	long i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_soft_mca_get_pointers( mca, &soft_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	soft_mca->finish_time_in_clock_ticks = mx_current_clock_tick();

	/* Set some reasonable defaults. */

	mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;
	mca->parameter_type = 0;

	for ( i = 0; i < mca->maximum_num_rois; i++ ) {
		mca->roi_array[i][0] = 0;
		mca->roi_array[i][1] = 0;
		mca->roi_integral_array[i] = 0;
	}

	mca->roi[0] = 0;
	mca->roi[1] = mca->maximum_num_channels - 1;
	mca->roi_integral = 0;

	mca->channel_number = 0;
	mca->channel_value = 0;

	mca->real_time = 0.0;
	mca->live_time = 0.0;

	mca->preset_real_time = 0.0;
	mca->preset_live_time = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mca_start( MX_MCA *mca )
{
	static const char fname[] = "mxd_soft_mca_start()";

	MX_SOFT_MCA *soft_mca = NULL;
	MX_CLOCK_TICK start_time_in_clock_ticks;
	MX_CLOCK_TICK measurement_time_in_clock_ticks;
	double measurement_time;
	mx_status_type mx_status;

	mx_status = mxd_soft_mca_get_pointers( mca, &soft_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	measurement_time = 0;

	switch( mca->preset_type ) {
	case MXF_MCA_PRESET_LIVE_TIME:
		measurement_time = mca->preset_live_time;
		break;
	case MXF_MCA_PRESET_REAL_TIME:
		measurement_time = mca->preset_real_time;
		break;
	case MXF_MCA_PRESET_COUNT:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Preset count operation is not supported by the 'soft mca' "
		"driver used for MCA '%s'.", mca->record->name );
		break;
	}

	start_time_in_clock_ticks = mx_current_clock_tick();

	measurement_time_in_clock_ticks = mx_convert_seconds_to_clock_ticks(
							measurement_time );

	soft_mca->finish_time_in_clock_ticks = mx_add_clock_ticks(
				start_time_in_clock_ticks,
				measurement_time_in_clock_ticks );

#if MXD_SOFT_MCA_DEBUG
	MX_DEBUG(-2,("%s: counting for %g seconds, (%lu,%lu) in clock ticks.",
		fname, mca->preset_live_time,
		measurement_time_in_clock_ticks.high_order,
		(unsigned long) measurement_time_in_clock_ticks.low_order));

	MX_DEBUG(-2,("%s: starting time = (%lu,%lu), finish time = (%lu,%lu)",
		fname, start_time_in_clock_ticks.high_order,
		(unsigned long) start_time_in_clock_ticks.low_order,
		soft_mca->finish_time_in_clock_ticks.high_order,
	(unsigned long) soft_mca->finish_time_in_clock_ticks.low_order));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mca_stop( MX_MCA *mca )
{
	static const char fname[] = "mxd_soft_mca_stop()";

	MX_SOFT_MCA *soft_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mca_get_pointers( mca, &soft_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCA_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	soft_mca->finish_time_in_clock_ticks = mx_current_clock_tick();

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mca_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_soft_mca_read()";

	MX_SOFT_MCA *soft_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mca_get_pointers( mca, &soft_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mca_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_soft_mca_clear()";

	MX_SOFT_MCA *soft_mca = NULL;
	unsigned long i;
	mx_status_type mx_status;

	mx_status = mxd_soft_mca_get_pointers( mca, &soft_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCA_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	for ( i = 0; i < mca->maximum_num_channels; i++ ) {
		mca->channel_array[i] = 0L;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mca_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_soft_mca_busy()";

	MX_SOFT_MCA *soft_mca = NULL;
	MX_CLOCK_TICK current_time_in_clock_ticks;
	int result;
	mx_status_type mx_status;

	mx_status = mxd_soft_mca_get_pointers( mca, &soft_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	current_time_in_clock_ticks = mx_current_clock_tick();

	result = mx_compare_clock_ticks( current_time_in_clock_ticks,
				soft_mca->finish_time_in_clock_ticks );

	if ( result >= 0 ) {
		mca->busy = FALSE;
	} else {
		mca->busy = TRUE;
	}

#if MXD_SOFT_MCA_DEBUG
	MX_DEBUG(-2,("%s: current time = (%lu,%lu), busy = %d",
		fname, current_time_in_clock_ticks.high_order,
		(unsigned long) current_time_in_clock_ticks.low_order,
		mca->busy ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mca_get_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_soft_mca_get_parameter()";

	MX_SOFT_MCA *soft_mca = NULL;
	unsigned long i, j, channel_value, integral;
	mx_status_type mx_status;

	mx_status = mxd_soft_mca_get_pointers( mca, &soft_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCA_DEBUG
	MX_DEBUG(-2,("%s invoked for parameter type %ld.",
			fname, mca->parameter_type));
#endif

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
	case MXLV_MCA_CHANNEL_NUMBER:
	case MXLV_MCA_REAL_TIME:
	case MXLV_MCA_LIVE_TIME:

		/* None of these cases require any action since the 
		 * simulated value is already in the location it needs
		 * to be in.
		 */

		break;

	case MXLV_MCA_ROI:
		i = mca->roi_number;

		mca->roi[0] = mca->roi_array[i][0];
		mca->roi[1] = mca->roi_array[i][1];
		break;

	case MXLV_MCA_ROI_INTEGRAL:

		i = mca->roi_number;

		integral = 0L;

		for ( j = mca->roi_array[i][0]; j <= mca->roi_array[i][1]; j++ )
		{

			channel_value = mca->channel_array[j];

			/* If adding the value in the current channel
			 * would result in the integral exceeding MX_ULONG_MAX,
			 * assign MX_ULONG_MAX to the integral and break out
			 * of the loop.
			 */

			if ( integral > ( MX_ULONG_MAX - channel_value ) ) {
				integral = MX_ULONG_MAX;
				break;
			}

			integral += channel_value;
		}

		mca->roi_integral = integral;

		mca->roi_integral_array[i] = integral;
		break;

	case MXLV_MCA_CHANNEL_VALUE:

		if ( mca->channel_number >= mca->maximum_num_channels ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"mca->channel_number (%lu) is greater than or equal to "
		"mca->maximum_num_channels (%ld).  This should not be "
		"able to happen, so if you see this message, please "
		"report the program bug to Bill Lavender.",
			mca->channel_number, mca->maximum_num_channels );
		}

		/* FIXME: Read the channel value from the private array. */

		mca->channel_value = 0;
		break;

	default:
		return mx_mca_default_get_parameter_handler( mca );
	}

	return MX_SUCCESSFUL_RESULT;
}

