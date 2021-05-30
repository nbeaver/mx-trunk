/*
 * Name:    d_monte_carlo_mca.c
 *
 * Purpose: MX multichannel analyzer driver for software-emulated MCAs
 *          using simple Monte Carlo calculations.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012-2017, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MONTE_CARLO_MCA_DEBUG		FALSE

#define MXD_MONTE_CARLO_MCA_DEBUG_BUSY		FALSE

#define MXD_MONTE_CARLO_MCA_DEBUG_THREAD	FALSE

#define MXD_MONTE_CARLO_MCA_DEBUG_SOURCES	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_constants.h"
#include "mx_thread.h"
#include "mx_mutex.h"
#include "mx_mca.h"
#include "d_monte_carlo_mca.h"

/* Initialize the MCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_monte_carlo_mca_record_function_list = {
	mxd_monte_carlo_mca_initialize_driver,
	mxd_monte_carlo_mca_create_record_structures,
	mxd_monte_carlo_mca_finish_record_initialization,
	mxd_monte_carlo_mca_delete_record,
	mxd_monte_carlo_mca_print_structure,
	mxd_monte_carlo_mca_open
};

MX_MCA_FUNCTION_LIST mxd_monte_carlo_mca_mca_function_list = {
	NULL,
	mxd_monte_carlo_mca_trigger,
	mxd_monte_carlo_mca_stop,
	mxd_monte_carlo_mca_read,
	mxd_monte_carlo_mca_clear,
	mxd_monte_carlo_mca_busy,
	mxd_monte_carlo_mca_get_parameter,
	mx_mca_default_set_parameter_handler
};

MX_RECORD_FIELD_DEFAULTS mxd_monte_carlo_mca_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCA_STANDARD_FIELDS,
	MXD_MONTE_CARLO_MCA_STANDARD_FIELDS
};

long mxd_monte_carlo_mca_num_record_fields
		= sizeof( mxd_monte_carlo_mca_record_field_defaults )
		  / sizeof( mxd_monte_carlo_mca_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_monte_carlo_mca_rfield_def_ptr
			= &mxd_monte_carlo_mca_record_field_defaults[0];

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_monte_carlo_mca_get_pointers( MX_MCA *mca,
			MX_MONTE_CARLO_MCA **monte_carlo_mca,
			const char *calling_fname )
{
	static const char fname[] = "mxd_monte_carlo_mca_get_pointers()";

	MX_RECORD *mca_record;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( monte_carlo_mca == (MX_MONTE_CARLO_MCA **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MONTE_CARLO_MCA pointer passed by '%s' was NULL",
			calling_fname );
	}

	mca_record = mca->record;

	if ( mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_MCA pointer %p is NULL.", mca );
	}

	*monte_carlo_mca =
		(MX_MONTE_CARLO_MCA *) mca_record->record_type_struct;

	if ( *monte_carlo_mca == (MX_MONTE_CARLO_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MONTE_CARLO_MCA pointer for mca record '%s' "
		"passed by '%s' is NULL",
				mca_record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_monte_carlo_mca_process_uniform( MX_MCA *mca,
			MX_MONTE_CARLO_MCA *monte_carlo_mca,
			MX_MONTE_CARLO_MCA_SOURCE *monte_carlo_mca_source )
{
	unsigned long i, j, num_channels, num_tests_per_channel;
	unsigned long *private_array;

	double events_per_second, seconds_per_call;
	double events_per_call, events_per_call_per_channel, log_ecc;
	long floor_log_ecc;
	double max_random_number;
	double test_value;

	num_channels = mca->maximum_num_channels;

	private_array = monte_carlo_mca->private_array;

	events_per_second = monte_carlo_mca_source->u.uniform.events_per_second;

	seconds_per_call
		= 1.0e-6 * (double) monte_carlo_mca->sleep_microseconds;

	events_per_call = events_per_second * seconds_per_call;

	events_per_call_per_channel =
		mx_divide_safely( events_per_call, num_channels );

	/* If 'events_per_call_per_channel' ever is >= 1, then the expression
	 * below ( test_value <= events_per_call_per_channel ) will always
	 * evaluate to true.  To avoid this, we multiply the number of times
	 * that we evaluate the test below by 'num_tests_per_channel' and
	 * then divide 'events_per_call_per_channel' by the value of
	 * 'num_tests_per_channel'.
	 */

	if ( events_per_call_per_channel <= 1.0e-38 ) {
		/* Assume that we never get events for this case. */
		return MX_SUCCESSFUL_RESULT;
	} else
	if ( events_per_call_per_channel <= 0.2 ) {
		num_tests_per_channel = 1;
	} else {
		log_ecc = log10( events_per_call_per_channel );

		floor_log_ecc = mx_round( floor(log_ecc) );

		num_tests_per_channel = mx_round(
				pow(10.0, 2.0 + floor_log_ecc) );

		events_per_call_per_channel /= (double) num_tests_per_channel;
	}

	max_random_number = mx_get_random_max();

	for ( i = 0; i < num_tests_per_channel; i++ ) {

		/* Walk through the channels to see whether each one
		 * had an event.
		 */

		for ( j = 0; j < num_channels; j++ ) {
			test_value = (double) mx_random() / max_random_number;

			if ( test_value <= events_per_call_per_channel ) {
				private_array[j]++;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monte_carlo_mca_create_uniform( MX_MCA *mca,
			MX_MONTE_CARLO_MCA *monte_carlo_mca,
			MX_MONTE_CARLO_MCA_SOURCE *monte_carlo_mca_source,
			int argc, char **argv )
{
	static const char fname[] = "mxd_monte_carlo_mca_create_uniform()";

	monte_carlo_mca_source->type = MXT_MONTE_CARLO_MCA_SOURCE_UNIFORM;

	monte_carlo_mca_source->process = mxd_monte_carlo_mca_process_uniform;

	if ( argc < 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Not enough arguments were specified for the 'uniform' source "
		"type specified for MCA '%s'.", mca->record->name );
	}

	monte_carlo_mca_source->u.uniform.events_per_second = atof( argv[1] );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_monte_carlo_mca_process_polynomial( MX_MCA *mca,
			MX_MONTE_CARLO_MCA *monte_carlo_mca,
			MX_MONTE_CARLO_MCA_SOURCE *monte_carlo_mca_source )
{
	unsigned long i, j, num_channels, num_tests_per_channel;
	unsigned long *private_array;

	double events_per_second, seconds_per_call;
	double events_per_call, events_per_call_per_channel;
	double log_threshold;
	long floor_log_threshold;
	double a0, a1, a2, a3;
	double polynomial;
	double test_value, threshold;
	double max_random_number;

	num_channels = mca->maximum_num_channels;

	private_array = monte_carlo_mca->private_array;

	events_per_second =
		monte_carlo_mca_source->u.polynomial.events_per_second;

	a0 = monte_carlo_mca_source->u.polynomial.a0;

	a1 = monte_carlo_mca_source->u.polynomial.a1;

	a2 = monte_carlo_mca_source->u.polynomial.a2;

	a3 = monte_carlo_mca_source->u.polynomial.a3;

	seconds_per_call
		= 1.0e-6 * (double) monte_carlo_mca->sleep_microseconds;

	events_per_call = events_per_second * seconds_per_call;

	events_per_call_per_channel =
		mx_divide_safely( events_per_call, num_channels );

	max_random_number = mx_get_random_max();

	for ( i = 0; i < num_channels; i++ ) {

		polynomial = a0 + ( a1 * i ) + ( a2 * i * i )
				+ ( a3 * i * i * i );

		threshold = events_per_call_per_channel * polynomial;

		/* If 'threshold' is ever >= 1, then the expression
		 * below ( test_value <= threshold) will always
		 * evaluate to true.  To avoid this, we set the
		 * number of times that we evaluate the test below
		 * to 'num_tests_per_channel' and then divide the
		 * value of 'threshold' by the value of
		 * 'num_tests_per_channel'.
		 */

		if ( threshold <= 0.2 ) {
			num_tests_per_channel = 1;
		} else {
			log_threshold = log10( threshold );

			floor_log_threshold = mx_round( floor(log_threshold) );

			num_tests_per_channel = mx_round(
				pow(10.0, 2.0 + floor_log_threshold) );

			threshold /= (double) num_tests_per_channel;
		}

		for ( j = 0; j < num_tests_per_channel; j++ )  {

			test_value = (double) mx_random() / max_random_number;

			if ( test_value <= threshold ) {
				private_array[i]++;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monte_carlo_mca_create_polynomial( MX_MCA *mca,
			MX_MONTE_CARLO_MCA *monte_carlo_mca,
			MX_MONTE_CARLO_MCA_SOURCE *monte_carlo_mca_source,
			int argc, char **argv )
{
	static const char fname[] = "mxd_monte_carlo_mca_create_polynomial()";

	monte_carlo_mca_source->type = MXT_MONTE_CARLO_MCA_SOURCE_POLYNOMIAL;

	monte_carlo_mca_source->process =
				mxd_monte_carlo_mca_process_polynomial;

	if ( argc < 4 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Not enough arguments were specified for the 'peak' source "
		"type specified for MCA '%s'.", mca->record->name );
	}

	monte_carlo_mca_source->u.polynomial.events_per_second =
						atof( argv[1] );

	monte_carlo_mca_source->u.polynomial.a0 = atof( argv[2] );

	monte_carlo_mca_source->u.polynomial.a1 = atof( argv[3] );

	monte_carlo_mca_source->u.polynomial.a2 = atof( argv[4] );

	monte_carlo_mca_source->u.polynomial.a3 = atof( argv[5] );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_monte_carlo_mca_process_gaussian( MX_MCA *mca,
			MX_MONTE_CARLO_MCA *monte_carlo_mca,
			MX_MONTE_CARLO_MCA_SOURCE *monte_carlo_mca_source )
{
	unsigned long i, j, num_channels, num_tests_per_channel;
	unsigned long *private_array;

	double events_per_second, seconds_per_call;
	double events_per_call, events_per_call_per_channel;
	double log_threshold;
	long floor_log_threshold;
	double peak_mean, peak_sigma;
	double gaussian, exp_argument, exp_coefficient;
	double test_value, threshold;
	double max_random_number;

	num_channels = mca->maximum_num_channels;

	private_array = monte_carlo_mca->private_array;

	events_per_second =
		monte_carlo_mca_source->u.gaussian.events_per_second;

	peak_mean = monte_carlo_mca_source->u.gaussian.mean;

	peak_sigma = monte_carlo_mca_source->u.gaussian.standard_deviation;

	seconds_per_call
		= 1.0e-6 * (double) monte_carlo_mca->sleep_microseconds;

	events_per_call = events_per_second * seconds_per_call;

	events_per_call_per_channel =
		mx_divide_safely( events_per_call, num_channels );

	exp_coefficient = mx_divide_safely(1.0, peak_sigma * sqrt(2.0 * MX_PI));

	max_random_number = mx_get_random_max();

	for ( i = 0; i < num_channels; i++ ) {

		exp_argument = mx_divide_safely(
				(( i - peak_mean ) * ( i - peak_mean )),
				    2.0 * peak_sigma * peak_sigma );

		gaussian = exp_coefficient * exp( - exp_argument );

		threshold = events_per_call_per_channel * gaussian;

		/* If 'threshold' is ever >= 1, then the expression
		 * below ( test_value <= threshold) will always
		 * evaluate to true.  To avoid this, we set the
		 * number of times that we evaluate the test below
		 * to 'num_tests_per_channel' and then divide the
		 * value of 'threshold' by the value of
		 * 'num_tests_per_channel'.
		 */

		if ( threshold <= 0.2 ) {
			num_tests_per_channel = 1;
		} else {
			log_threshold = log10( threshold );

			floor_log_threshold = mx_round( floor(log_threshold) );

			num_tests_per_channel = mx_round(
				pow(10.0, 2.0 + floor_log_threshold) );

			threshold /= (double) num_tests_per_channel;
		}

		for ( j = 0; j < num_tests_per_channel; j++ )  {

			test_value = (double) mx_random() / max_random_number;

			if ( test_value <= threshold ) {
				private_array[i]++;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monte_carlo_mca_create_gaussian( MX_MCA *mca,
			MX_MONTE_CARLO_MCA *monte_carlo_mca,
			MX_MONTE_CARLO_MCA_SOURCE *monte_carlo_mca_source,
			int argc, char **argv )
{
	static const char fname[] = "mxd_monte_carlo_mca_create_gaussian()";

	monte_carlo_mca_source->type = MXT_MONTE_CARLO_MCA_SOURCE_GAUSSIAN;

	monte_carlo_mca_source->process = mxd_monte_carlo_mca_process_gaussian;

	if ( argc < 4 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Not enough arguments were specified for the 'peak' source "
		"type specified for MCA '%s'.", mca->record->name );
	}

	monte_carlo_mca_source->u.gaussian.events_per_second = atof( argv[1] );

	monte_carlo_mca_source->u.gaussian.mean = atof( argv[2] );

	monte_carlo_mca_source->u.gaussian.standard_deviation = atof( argv[3] );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_monte_carlo_mca_process_lorentzian( MX_MCA *mca,
			MX_MONTE_CARLO_MCA *monte_carlo_mca,
			MX_MONTE_CARLO_MCA_SOURCE *monte_carlo_mca_source )
{
	unsigned long i, j, num_channels, num_tests_per_channel;
	unsigned long *private_array;

	double events_per_second, seconds_per_call;
	double events_per_call, events_per_call_per_channel;
	double log_threshold;
	long floor_log_threshold;
	double peak_mean, peak_fwhm;
	double lorentzian, function, coefficient;
	double test_value, threshold;
	double max_random_number;

	num_channels = mca->maximum_num_channels;

	private_array = monte_carlo_mca->private_array;

	events_per_second =
		monte_carlo_mca_source->u.lorentzian.events_per_second;

	peak_mean = monte_carlo_mca_source->u.lorentzian.mean;

	peak_fwhm =
		monte_carlo_mca_source->u.lorentzian.full_width_half_maximum;

	seconds_per_call
		= 1.0e-6 * (double) monte_carlo_mca->sleep_microseconds;

	events_per_call = events_per_second * seconds_per_call;

	events_per_call_per_channel =
		mx_divide_safely( events_per_call, num_channels );

	coefficient = mx_divide_safely( 2.0, MX_PI * peak_fwhm );

	max_random_number = mx_get_random_max();

	for ( i = 0; i < num_channels; i++ ) {

		function = mx_divide_safely( peak_fwhm * peak_fwhm,
				(4.0 * ( i - peak_mean ) * ( i - peak_mean ))
					+ ( peak_fwhm * peak_fwhm ) );

		lorentzian = coefficient * function;

		threshold = events_per_call_per_channel * lorentzian;

		/* If 'threshold' is ever >= 1, then the expression
		 * below ( test_value <= threshold) will always
		 * evaluate to true.  To avoid this, we set the
		 * number of times that we evaluate the test below
		 * to 'num_tests_per_channel' and then divide the
		 * value of 'threshold' by the value of
		 * 'num_tests_per_channel'.
		 */

		if ( threshold <= 0.2 ) {
			num_tests_per_channel = 1;
		} else {
			log_threshold = log10( threshold );

			floor_log_threshold = mx_round( floor(log_threshold) );

			num_tests_per_channel = mx_round(
				pow(10.0, 2.0 + floor_log_threshold) );

			threshold /= (double) num_tests_per_channel;
		}

		for ( j = 0; j < num_tests_per_channel; j++ )  {

			test_value = (double) mx_random() / max_random_number;

			if ( test_value <= threshold ) {
				private_array[i]++;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_monte_carlo_mca_create_lorentzian( MX_MCA *mca,
			MX_MONTE_CARLO_MCA *monte_carlo_mca,
			MX_MONTE_CARLO_MCA_SOURCE *monte_carlo_mca_source,
			int argc, char **argv )
{
	static const char fname[] = "mxd_monte_carlo_mca_create_lorentzian()";

	monte_carlo_mca_source->type = MXT_MONTE_CARLO_MCA_SOURCE_LORENTZIAN;

	monte_carlo_mca_source->process = mxd_monte_carlo_mca_process_lorentzian;

	if ( argc < 4 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Not enough arguments were specified for the 'peak' source "
		"type specified for MCA '%s'.", mca->record->name );
	}

	monte_carlo_mca_source->u.lorentzian.events_per_second =
						atof( argv[1] );

	monte_carlo_mca_source->u.lorentzian.mean = atof( argv[2] );

	monte_carlo_mca_source->u.lorentzian.full_width_half_maximum =
								atof( argv[3] );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

/* Macros for locking and unlocking the mutex. */

#define MXD_LOCK( str ) \
	do {                                                               \
		unsigned long mx_status_code;                              \
                                                                           \
		mx_status_code = mx_mutex_lock( monte_carlo_mca->mutex );  \
                                                                           \
		if ( mx_status_code != MXE_SUCCESS ) {                     \
			return mx_error( mx_status_code, fname,            \
"The attempt to lock the mutex " str "for MCA '%s' failed.",               \
				mca->record->name );                       \
		}                                                          \
	} while (0);

#define MXD_UNLOCK( str ) \
	do {                                                               \
		unsigned long mx_status_code;                              \
                                                                           \
		mx_status_code = mx_mutex_unlock( monte_carlo_mca->mutex );\
                                                                           \
		if ( mx_status_code != MXE_SUCCESS ) {                     \
			return mx_error( mx_status_code, fname,            \
"The attempt to unlock the mutex " str "for MCA '%s' failed.",             \
				mca->record->name );                       \
		}                                                          \
	} while (0);

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_monte_carlo_mca_event_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxd_monte_carlo_mca_event_thread()";

	MX_MCA *mca;
	MX_MONTE_CARLO_MCA *monte_carlo_mca;
	MX_CLOCK_TICK current_time_in_clock_ticks;
	int result;
	unsigned long i;
	MX_MONTE_CARLO_MCA_SOURCE *source;
	mx_status_type mx_status;

	if ( args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed was NULL." );
	}

	mca = (MX_MCA *) args;

	mx_status = mxd_monte_carlo_mca_get_pointers( mca,
						&monte_carlo_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MONTE_CARLO_MCA_DEBUG_THREAD
	MX_DEBUG(-2,("%s invoked for MCA '%s'.", fname, mca->record->name));
#endif

	/* Enter an infinite loop that generates events when the value of
	 * monte_carlo_mca->finish_time_in_clock_ticks is greater than
	 * the current time measured in clock ticks.
	 */

	while (1) {
		MXD_LOCK( "to compare times " );

		current_time_in_clock_ticks = mx_current_clock_tick();

		result = mx_compare_clock_ticks( current_time_in_clock_ticks,
				monte_carlo_mca->finish_time_in_clock_ticks );

		MXD_UNLOCK( "to compare times " );

		if ( result >= 0 ) {
			/* The current time is after the MCA measurement
			 * finish time, so we do not generate events.
			 */

			mx_usleep( monte_carlo_mca->sleep_microseconds );

			/* Go back to the top of the while() loop. */
			continue;
		}

		/* If we get here. we attempt to generate events. */

#if MXD_MONTE_CARLO_MCA_DEBUG_THREAD
		MX_DEBUG(-2,("%s attempting to generate an event.", fname));
#endif

		for ( i = 0; i < monte_carlo_mca->num_sources; i++ ) {
			source = &(monte_carlo_mca->source_array[i]);

			mx_status = source->process( mca,
						monte_carlo_mca, source );
		}

		mca->new_data_available = TRUE;

		mx_usleep( monte_carlo_mca->sleep_microseconds );
	}

	MXW_NOT_REACHED( return MX_SUCCESSFUL_RESULT; )
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_monte_carlo_mca_initialize_driver( MX_DRIVER *driver )
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
mxd_monte_carlo_mca_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_monte_carlo_mca_create_record_structures()";

	MX_MCA *mca;
	MX_MONTE_CARLO_MCA *monte_carlo_mca = NULL;

	/* Allocate memory for the necessary structures. */

	mca = (MX_MCA *) malloc( sizeof(MX_MCA) );

	if ( mca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA structure." );
	}

	monte_carlo_mca =
		(MX_MONTE_CARLO_MCA *) malloc( sizeof(MX_MONTE_CARLO_MCA) );

	if ( monte_carlo_mca == (MX_MONTE_CARLO_MCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MONTE_CARLO_MCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mca;
	record->record_type_struct = monte_carlo_mca;
	record->class_specific_function_list =
				&mxd_monte_carlo_mca_mca_function_list;

	mca->record = record;
	monte_carlo_mca->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monte_carlo_mca_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_monte_carlo_mca_finish_record_initialization()";

	MX_MCA *mca = NULL;
	MX_MONTE_CARLO_MCA *monte_carlo_mca = NULL;
	MX_CLOCK_TICK current_tick;
	mx_status_type mx_status;

	mca = (MX_MCA *) record->record_class_struct;

	mx_status = mxd_monte_carlo_mca_get_pointers( mca,
						&monte_carlo_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca->current_num_channels = mca->maximum_num_channels;

	mca->busy = FALSE;

	mca->current_num_rois = mca->maximum_num_rois;

	current_tick = mx_current_clock_tick();

	monte_carlo_mca->start_time_in_clock_ticks = current_tick;
	monte_carlo_mca->finish_time_in_clock_ticks = current_tick;

	mx_status = mx_mca_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_monte_carlo_mca_delete_record( MX_RECORD *record )
{
	MX_MCA *mca;
	MX_MONTE_CARLO_MCA *monte_carlo_mca = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	monte_carlo_mca = (MX_MONTE_CARLO_MCA *) record->record_type_struct;

	if ( monte_carlo_mca != (MX_MONTE_CARLO_MCA *) NULL ) {

		/* Free monte_carlo_mca structures here. */

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
mxd_monte_carlo_mca_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_monte_carlo_mca_print_structure()";

	MX_MCA *mca;
	MX_MONTE_CARLO_MCA *monte_carlo_mca = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_monte_carlo_mca_get_pointers( mca,
						&monte_carlo_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MCA parameters for record '%s':\n", record->name);

	fprintf(file, "  MCA type              = MONTE_CARLO_MCA.\n\n");
	fprintf(file, "  maximum # of channels = %ld\n",
					mca->maximum_num_channels);
	fprintf(file, "  maximum # of ROIs     = %ld\n",
					mca->maximum_num_rois);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monte_carlo_mca_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_monte_carlo_mca_open()";

	MX_MCA *mca = NULL;
	MX_MONTE_CARLO_MCA *monte_carlo_mca = NULL;
	long i;
	char *source_dup;
	int argc; char **argv;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_monte_carlo_mca_get_pointers( mca,
						&monte_carlo_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	monte_carlo_mca->sleep_microseconds =
		mx_round( 1000000.0 * monte_carlo_mca->time_step_size );

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

	/* Seed the random number generator. */

	mx_seed_random( time(NULL) );

	/* Create an array of event sources. */

	monte_carlo_mca->source_array = ( MX_MONTE_CARLO_MCA_SOURCE *)
		calloc( monte_carlo_mca->num_sources,
			sizeof(MX_MONTE_CARLO_MCA_SOURCE) );

	if ( monte_carlo_mca->source_array
			== (MX_MONTE_CARLO_MCA_SOURCE *) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array of "
		"MX_MONTE_CARLO_MCA_SOURCE structures for MCA '%s'.",
			monte_carlo_mca->num_sources, record->name );
	}

	/* Parse source_string_array to find all of the event sources. */

	for ( i = 0; i < monte_carlo_mca->num_sources; i++ ) {

		source_dup = strdup( monte_carlo_mca->source_string_array[i] );

		mx_string_split( source_dup, " ", &argc, &argv );

#if 0
		{
			int j;
			for ( j = 0; j < argc; j++ ) {
				MX_DEBUG(-2,("%s: [%ld][%d] = '%s'",
					fname, i, j, argv[j] ));
			}
		}
#endif
		/* What kind of event source is this? */

		if ( strcmp( argv[0], "uniform" ) == 0 ) {
			mx_status = mxd_monte_carlo_mca_create_uniform(
					mca, monte_carlo_mca,
					&(monte_carlo_mca->source_array[i]),
					argc, argv );
		} else
		if ( strcmp( argv[0], "polynomial" ) == 0 ) {
			mx_status = mxd_monte_carlo_mca_create_polynomial(
					mca, monte_carlo_mca,
					&(monte_carlo_mca->source_array[i]),
					argc, argv );
		} else
		if ( strcmp( argv[0], "gaussian" ) == 0 ) {
			mx_status = mxd_monte_carlo_mca_create_gaussian(
					mca, monte_carlo_mca,
					&(monte_carlo_mca->source_array[i]),
					argc, argv );
		} else
		if ( strcmp( argv[0], "lorentzian" ) == 0 ) {
			mx_status = mxd_monte_carlo_mca_create_lorentzian(
					mca, monte_carlo_mca,
					&(monte_carlo_mca->source_array[i]),
					argc, argv );
		} else {
			mx_warning( "Unrecognized event type '%s' seen.  "
				"Skipping...", argv[0] );
		}

		mx_free( argv );
		mx_free( source_dup );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Initialize the finish time to the current time, so that the
	 * event thread will not immediately start counting.
	 */

	monte_carlo_mca->finish_time_in_clock_ticks = mx_current_clock_tick();

	/* Create the mutex that will be used to control access to these:
	 *     monte_carlo_mca->finish_time_in_clock_ticks
	 *     monte_carlo_mca->private_array
	 */

	mx_status = mx_mutex_create( &(monte_carlo_mca->mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create the private array itself. */

	monte_carlo_mca->private_array = (unsigned long *)
		calloc( mca->maximum_num_channels, sizeof(unsigned long) );

	if ( monte_carlo_mca->private_array == (unsigned long *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
		"of unsigned longs for the private array of record '%s'.",
			mca->maximum_num_channels, record->name );
	}

	/* Start the event thread that will generate events
	 * for the private array.
	 */

	mx_status = mx_thread_create( &(monte_carlo_mca->event_thread),
				mxd_monte_carlo_mca_event_thread, mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_monte_carlo_mca_trigger( MX_MCA *mca )
{
	static const char fname[] = "mxd_monte_carlo_mca_trigger()";

	MX_MONTE_CARLO_MCA *monte_carlo_mca = NULL;
	MX_CLOCK_TICK measurement_time_in_clock_ticks;
	double measurement_time;
	mx_status_type mx_status;

	mx_status = mxd_monte_carlo_mca_get_pointers( mca,
						&monte_carlo_mca, fname );

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
		"Preset count operation is not supported by the "
		"'monte_carlo_mca' driver used for MCA '%s'.",
			mca->record->name );
		break;
	}

	MXD_LOCK( "to set the finish time " );

	monte_carlo_mca->start_time_in_clock_ticks = mx_current_clock_tick();

	measurement_time_in_clock_ticks = mx_convert_seconds_to_clock_ticks(
							measurement_time );

	monte_carlo_mca->finish_time_in_clock_ticks = mx_add_clock_ticks(
				monte_carlo_mca->start_time_in_clock_ticks,
				measurement_time_in_clock_ticks );

	MXD_UNLOCK( "to set the finish time " );

#if MXD_MONTE_CARLO_MCA_DEBUG
	MX_DEBUG(-2,("%s: counting for %g seconds, (%lu,%lu) in clock ticks.",
		fname, mca->preset_live_time,
		measurement_time_in_clock_ticks.high_order,
		(unsigned long) measurement_time_in_clock_ticks.low_order));

	MX_DEBUG(-2,("%s: starting time = (%lu,%lu), finish time = (%lu,%lu)",
		fname, monte_carlo_mca->start_time_in_clock_ticks.high_order,
	(unsigned long) monte_carlo_mca->start_time_in_clock_ticks.low_order,
		monte_carlo_mca->finish_time_in_clock_ticks.high_order,
	(unsigned long) monte_carlo_mca->finish_time_in_clock_ticks.low_order));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monte_carlo_mca_stop( MX_MCA *mca )
{
	static const char fname[] = "mxd_monte_carlo_mca_stop()";

	MX_MONTE_CARLO_MCA *monte_carlo_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_monte_carlo_mca_get_pointers( mca,
						&monte_carlo_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MONTE_CARLO_MCA_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif
	MXD_LOCK( "to stop counting " );

	monte_carlo_mca->finish_time_in_clock_ticks = mx_current_clock_tick();

	MXD_UNLOCK( "to stop counting " );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monte_carlo_mca_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_monte_carlo_mca_read()";

	MX_MONTE_CARLO_MCA *monte_carlo_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_monte_carlo_mca_get_pointers( mca,
						&monte_carlo_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MXD_LOCK( "to read the spectrum " );

	memcpy( mca->channel_array, monte_carlo_mca->private_array,
		mca->maximum_num_channels * sizeof(unsigned long) );

	MXD_UNLOCK( "to read the spectrum " );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monte_carlo_mca_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_monte_carlo_mca_clear()";

	MX_MONTE_CARLO_MCA *monte_carlo_mca = NULL;
	unsigned long i;
	mx_status_type mx_status;

	mx_status = mxd_monte_carlo_mca_get_pointers( mca,
						&monte_carlo_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MONTE_CARLO_MCA_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	MXD_LOCK( "to clear the spectrum " );

	memset( monte_carlo_mca->private_array, 0,
		mca->maximum_num_channels * sizeof(unsigned long) );

	MXD_UNLOCK( "to clear the spectrum " );

	for ( i = 0; i < mca->maximum_num_channels; i++ ) {
		mca->channel_array[i] = 0L;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monte_carlo_mca_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_monte_carlo_mca_busy()";

	MX_MONTE_CARLO_MCA *monte_carlo_mca = NULL;
	MX_CLOCK_TICK current_time_in_clock_ticks;
	int result;
	mx_status_type mx_status;

	mx_status = mxd_monte_carlo_mca_get_pointers( mca,
						&monte_carlo_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MXD_LOCK( "to check for busy " );

	current_time_in_clock_ticks = mx_current_clock_tick();

	result = mx_compare_clock_ticks( current_time_in_clock_ticks,
				monte_carlo_mca->finish_time_in_clock_ticks );

	MXD_UNLOCK( "to check for busy " );

	if ( result >= 0 ) {
		mca->busy = FALSE;
	} else {
		mca->busy = TRUE;
	}

#if MXD_MONTE_CARLO_MCA_DEBUG_BUSY
	MX_DEBUG(-2,("%s: current time = (%lu,%lu), busy = %d",
		fname, current_time_in_clock_ticks.high_order,
		(unsigned long) current_time_in_clock_ticks.low_order,
		mca->busy ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_monte_carlo_mca_get_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_monte_carlo_mca_get_parameter()";

	MX_MONTE_CARLO_MCA *monte_carlo_mca = NULL;
	MX_CLOCK_TICK current_tick, difference_in_ticks;
	int comparison;
	unsigned long i, j, channel_value, integral;
	mx_status_type mx_status;

	mx_status = mxd_monte_carlo_mca_get_pointers( mca,
						&monte_carlo_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MONTE_CARLO_MCA_DEBUG
	MX_DEBUG(-2,("%s invoked for parameter type %ld.",
			fname, mca->parameter_type));
#endif

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
	case MXLV_MCA_CHANNEL_NUMBER:

		/* None of these cases require any action since the 
		 * simulated value is already in the location it needs
		 * to be in.
		 */

		break;

	case MXLV_MCA_REAL_TIME:
	case MXLV_MCA_LIVE_TIME:
		/* Has the finish time already arrived? */

		current_tick = mx_current_clock_tick();

		comparison = mx_compare_clock_ticks( current_tick,
				monte_carlo_mca->finish_time_in_clock_ticks );

		if ( comparison < 0 ) {
			difference_in_ticks = mx_subtract_clock_ticks(
				current_tick,
				monte_carlo_mca->start_time_in_clock_ticks );
		} else {
			difference_in_ticks = mx_subtract_clock_ticks(
				monte_carlo_mca->finish_time_in_clock_ticks,
				monte_carlo_mca->start_time_in_clock_ticks );
		}

		mca->real_time = mx_convert_clock_ticks_to_seconds(
							difference_in_ticks );

		mca->live_time = mca->real_time;

#if MXD_MONTE_CARLO_MCA_DEBUG
		MX_DEBUG(-2,("%s: start = (%lu,%lu), finish = (%lu,%lu)",
			fname,
			monte_carlo_mca->start_time_in_clock_ticks.high_order,
			monte_carlo_mca->start_time_in_clock_ticks.low_order,
			monte_carlo_mca->finish_time_in_clock_ticks.high_order,
			monte_carlo_mca->finish_time_in_clock_ticks.low_order));

		MX_DEBUG(-2,("%s: current_tick = (%lu,%lu), comparison = %d",
			fname,
			current_tick.high_order,
			current_tick.low_order,
			comparison));

		MX_DEBUG(-2,("%s: difference = (%lu,%lu), real_time = %g",
			fname,
			difference_in_ticks.high_order,
			difference_in_ticks.low_order,
			mca->real_time));
#endif
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
			 * would result in the integral exceeding ULONG_MAX,
			 * assign ULONG_MAX to the integral and break out
			 * of the loop.
			 */

			if ( integral > ( ULONG_MAX - channel_value ) ) {
				integral = ULONG_MAX;
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

