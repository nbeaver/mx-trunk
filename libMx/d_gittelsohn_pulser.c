/*
 * Name:    d_gittelsohn_pulser.c
 *
 * Purpose: MX pulse generator driver for Mark Gittelsohn's Arduino-based
 *          pulse generator.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_GITTELSOHN_PULSER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_rs232.h"
#include "mx_pulse_generator.h"
#include "d_gittelsohn_pulser.h"

/* Initialize the pulse generator driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_gittelsohn_pulser_record_function_list = {
	NULL,
	mxd_gittelsohn_pulser_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_gittelsohn_pulser_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST mxd_gittelsohn_pulser_pulser_function_list = {
	mxd_gittelsohn_pulser_is_busy,
	mxd_gittelsohn_pulser_start,
	mxd_gittelsohn_pulser_stop,
	mxd_gittelsohn_pulser_get_parameter,
	mxd_gittelsohn_pulser_set_parameter
};

/* MX digital output pulser data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_gittelsohn_pulser_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_GITTELSOHN_PULSER_STANDARD_FIELDS
};

long mxd_gittelsohn_pulser_num_record_fields
		= sizeof( mxd_gittelsohn_pulser_record_field_defaults )
		  / sizeof( mxd_gittelsohn_pulser_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_gittelsohn_pulser_rfield_def_ptr
			= &mxd_gittelsohn_pulser_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_gittelsohn_pulser_get_pointers( MX_PULSE_GENERATOR *pulser,
			MX_GITTELSOHN_PULSER **gittelsohn_pulser,
			const char *calling_fname )
{
	static const char fname[] = "mxd_gittelsohn_pulser_get_pointers()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser_ptr;

	if ( pulser == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PULSE_GENERATOR pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( pulser->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for timer pointer passed by '%s' is NULL.",
			calling_fname );
	}

	gittelsohn_pulser_ptr = (MX_GITTELSOHN_PULSER *)
					pulser->record->record_type_struct;

	if ( gittelsohn_pulser_ptr == (MX_GITTELSOHN_PULSER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_GITTELSOHN_PULSER pointer for pulse generator "
			"record '%s' passed by '%s' is NULL",
				pulser->record->name, calling_fname );
	}

	if ( gittelsohn_pulser != (MX_GITTELSOHN_PULSER **) NULL ) {
		*gittelsohn_pulser = gittelsohn_pulser_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_gittelsohn_pulser_command( MX_GITTELSOHN_PULSER *gittelsohn_pulser,
				char *command,
				char *response,
				size_t max_response_size )
{ 
	static const char fname[] = "mxd_gittelsohn_pulser_command()";

	MX_RECORD *rs232_record;
	unsigned long debug_flag, flags;
	char command_echo_buffer[200];
	mx_status_type mx_status;

	if ( gittelsohn_pulser == (MX_GITTELSOHN_PULSER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The gittelsohn_pulser pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed was NULL." );
	}

	rs232_record = gittelsohn_pulser->rs232_record;

	flags = gittelsohn_pulser->gittelsohn_pulser_flags;

	debug_flag = flags & MXF_GITTELSOHN_PULSER_DEBUG;

	/* Send the command to the Arduino. */

	mx_status = mx_rs232_putline( rs232_record, command, NULL, flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Arduino first sends back a line that just reports back
	 * the command string that was sent to the caller.  We just 
	 * throw this away.
	 */

	mx_status = mx_rs232_getline( rs232_record,
			command_echo_buffer, sizeof(command_echo_buffer),
			NULL, flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now read back the _real_ data from the Arduino. */

	mx_status = mx_rs232_getline( rs232_record,
			response, max_response_size, NULL, flags );

	return mx_status;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_gittelsohn_pulser_create_record_structures()";

	MX_PULSE_GENERATOR *pulser;
	MX_GITTELSOHN_PULSER *gittelsohn_pulser;

	/* Allocate memory for the necessary structures. */

	pulser = (MX_PULSE_GENERATOR *) malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PULSE_GENERATOR structure." );
	}

	gittelsohn_pulser = (MX_GITTELSOHN_PULSER *)
				malloc( sizeof(MX_GITTELSOHN_PULSER) );

	if ( gittelsohn_pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GITTELSOHN_PULSER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = pulser;
	record->record_type_struct = gittelsohn_pulser;
	record->class_specific_function_list
			= &mxd_gittelsohn_pulser_pulser_function_list;

	pulser->record = record;
	gittelsohn_pulser->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_gittelsohn_pulser_open()";

	MX_PULSE_GENERATOR *pulser;
	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	char response[255];
	unsigned long debug_flag, flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unwritten_output(
					gittelsohn_pulser->rs232_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input(
					gittelsohn_pulser->rs232_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

        flags = gittelsohn_pulser->gittelsohn_pulser_flags;

        debug_flag = flags & MXF_GITTELSOHN_PULSER_DEBUG;

	/* Print out the Gittelsohn pulser configuration. */

	mx_status = mx_rs232_putline( gittelsohn_pulser->rs232_record,
				"conf", NULL, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The first time we send a command to the Arduino, we must put in
	 * a 1 second delay before trying to read the response.  Otherwise,
	 * we time out.  Subsequent commands do not seem to have this problem.
	 */

	mx_msleep(1000);

	mx_status = mx_rs232_getline( gittelsohn_pulser->rs232_record,
				response, sizeof(response),
				NULL, debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_is_busy( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_is_busy()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GITTELSOHN_PULSER_DEBUG
	MX_DEBUG(-2,("%s: pulser '%s', busy = %d",
		fname, pulser->record->name, (int) pulser->busy));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_start( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_start()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	char response[200];
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GITTELSOHN_PULSER_DEBUG
	MX_DEBUG(-2,("%s: Pulse generator '%s' starting, "
		"pulse_width = %f, pulse_period = %f, num_pulses = %ld",
			fname, pulser->record->name,
			pulser->pulse_width,
			pulser->pulse_period,
			pulser->num_pulses));
#endif
	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
					"start", response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the pulse generator as busy. */

	pulser->busy = TRUE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_stop( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_stop()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	char response[200];
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GITTELSOHN_PULSER_DEBUG
	MX_DEBUG(-2,("%s: Stopping pulse generator '%s'.",
		fname, pulser->record->name ));
#endif
	mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
					"stop", response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulser->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_get_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_get_parameter()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	MX_PULSE_GENERATOR_FUNCTION_LIST *pulser_flist = NULL;
	char response[200];
	int argc, fn_status, saved_errno;
	char **argv;
	double on_ms, off_ms;
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulser->record, NULL,
						&pulser_flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GITTELSOHN_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
					"cycles", response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		fn_status = mx_string_split( response, " ", &argc, &argv );

		if ( fn_status < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FUNCTION_FAILED, fname,
			"The attempt to split the response '%s' "
			"to command '%s' for record '%s' "
			"using mx_string_split() failed with an errno value "
			"of %d.  Error message = '%s'",
				response, "cycles", pulser->record->name,
				saved_errno, strerror(saved_errno) );
		}

		if ( argc < 4 ) {
			mx_free(argv);

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The response '%s' to the 'cycles' command did not "
			"have at least 4 tokens in it for record '%s'.",
				response, pulser->record->name );
		} else {
			pulser->num_pulses = atol( argv[3] );

			mx_free(argv);

			return MX_SUCCESSFUL_RESULT;
		}
		break;

	case MXLV_PGN_PULSE_WIDTH:
		mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
					"onms", response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		fn_status = mx_string_split( response, " ", &argc, &argv );

		if ( fn_status < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FUNCTION_FAILED, fname,
			"The attempt to split the response '%s' "
			"to command '%s' for record '%s' "
			"using mx_string_split() failed with an errno value "
			"of %d.  Error message = '%s'",
				response, "onms", pulser->record->name,
				saved_errno, strerror(saved_errno) );
		}

		if ( argc < 4 ) {
			mx_free(argv);

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The response '%s' to the 'onms' command did not "
			"have at least 4 tokens in it for record '%s'.",
				response, pulser->record->name );
		} else {
			on_ms = atof( argv[3] );

			mx_free(argv);

			pulser->pulse_width = 0.001 * on_ms;

			return MX_SUCCESSFUL_RESULT;
		}
		break;

	case MXLV_PGN_PULSE_DELAY:
		/* The Gittelsohn Arduino pulser does not implement
		 * a pulse delay.
		 */

		pulser->pulse_delay = 0;
		break;

	case MXLV_PGN_MODE:
		/* The Gittelsohn Arduino pulser only does square waves. */

		pulser->mode = MXF_PGN_SQUARE_WAVE;
		break;

	case MXLV_PGN_PULSE_PERIOD:
		/* First, get the on time by getting the pulse width. */

		mx_status = pulser_flist->get_parameter( pulser );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/*--- Next, get the off time. ---*/

		mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
					"offms", response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		fn_status = mx_string_split( response, " ", &argc, &argv );

		if ( fn_status < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FUNCTION_FAILED, fname,
			"The attempt to split the response '%s' "
			"to command '%s' for record '%s' "
			"using mx_string_split() failed with an errno value "
			"of %d.  Error message = '%s'",
				response, "offms", pulser->record->name,
				saved_errno, strerror(saved_errno) );
		}

		if ( argc < 4 ) {
			mx_free(argv);

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The response '%s' to the 'offms' command did not "
			"have at least 4 tokens in it for record '%s'.",
				response, pulser->record->name );
		} else {
			off_ms = atof( argv[3] );

			mx_free(argv);

			/* The pulse period is the sum of 'on_ms' and 'off_ms'
			 * expressed in seconds.
			 */

			pulser->pulse_period = pulser->pulse_width
						+ 0.001 * off_ms;
		}
		break;

	default:
		return
		    mx_pulse_generator_default_get_parameter_handler( pulser );
	}

#if MXD_GITTELSOHN_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_gittelsohn_pulser_set_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_gittelsohn_pulser_set_parameter()";

	MX_GITTELSOHN_PULSER *gittelsohn_pulser = NULL;
	char command[200];
	char response[200];
	long on_ms, off_ms;
	mx_status_type mx_status;

	mx_status = mxd_gittelsohn_pulser_get_pointers( pulser,
						&gittelsohn_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GITTELSOHN_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		snprintf( command, sizeof(command),
			"cycles %ld", pulser->num_pulses );

		mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
					command, response, sizeof(response) );
		break;

	case MXLV_PGN_PULSE_WIDTH:
	case MXLV_PGN_PULSE_PERIOD:
		/* Unconditionally set 'on_ms' and 'off_ms' in terms of
		 * the values of 'pulse_width' and 'pulse_period' at the
		 * time of this call.
		 */

		on_ms = mx_round( 1000.0 * pulser->pulse_width );

		off_ms = mx_round( 1000.0 *
			( pulser->pulse_period - pulser->pulse_width ) );

		if ( off_ms < 0 ) {
			pulser->pulse_period = pulser->pulse_width;

			off_ms = 0;
		}

		snprintf( command, sizeof(command), "on_ms %ld", on_ms );

		mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
					command, response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command), "off_ms %ld", off_ms );

		mx_status = mxd_gittelsohn_pulser_command( gittelsohn_pulser,
					command, response, sizeof(response) );
		break;

	case MXLV_PGN_PULSE_DELAY:
		/* The Gittelsohn Arduino pulser does not implement
		 * a pulse delay.
		 */

		pulser->pulse_delay = 0;
		break;

	case MXLV_PGN_MODE:
		/* The Gittelsohn Arduino pulser only does square waves. */

		pulser->mode = MXF_PGN_SQUARE_WAVE;
		break;

	default:
		return
		    mx_pulse_generator_default_set_parameter_handler( pulser );
	}

#if MXD_GITTELSOHN_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

