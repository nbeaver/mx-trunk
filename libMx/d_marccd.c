/*
 * Name:    d_marccd.c
 *
 * Purpose: MX driver for MarCCD remote control.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008-2010, 2013-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MARCCD_DEBUG			TRUE

#define MXD_MARCCD_DEBUG_INPUT_IS_AVAILABLE	FALSE

#define MXD_MARCCD_DEBUG_SHOW_RESPONSE		TRUE

#define MXD_MARCCD_DEBUG_SHOW_STATE_RESPONSE	FALSE

#define MXD_MARCCD_DEBUG_SHOW_STATE_CHANGES	TRUE

#define MXD_MARCCD_DEBUG_EXTENDED_STATUS	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_thread.h"
#include "mx_unistd.h"
#include "mx_io.h"
#include "mx_driver.h"
#include "mx_pulse_generator.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_marccd.h"

/*---*/

#define MX_CR_LF	"\015\012"

MX_RECORD_FUNCTION_LIST mxd_marccd_record_function_list = {
	mxd_marccd_initialize_driver,
	mxd_marccd_create_record_structures,
	mxd_marccd_finish_record_initialization,
	NULL,
	NULL,
	mxd_marccd_open,
	mxd_marccd_close
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_marccd_ad_function_list = {
	mxd_marccd_arm,
	mxd_marccd_trigger,
	NULL,
	mxd_marccd_stop,
	mxd_marccd_stop,
	NULL,
	NULL,
	NULL,
	mxd_marccd_get_extended_status,
	mxd_marccd_readout_frame,
	mxd_marccd_correct_frame,
	mxd_marccd_transfer_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_marccd_get_parameter,
	mxd_marccd_set_parameter,
	mx_area_detector_default_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_marccd_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_MARCCD_STANDARD_FIELDS
};

long mxd_marccd_num_record_fields
		= sizeof( mxd_marccd_record_field_defaults )
			/ sizeof( mxd_marccd_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_marccd_rfield_def_ptr
			= &mxd_marccd_record_field_defaults[0];

/*-------------------------------------------------------------------------*/

static int mxd_marccd_input_is_available( MX_MARCCD * );

static mx_status_type mxd_marccd_handle_response( MX_AREA_DETECTOR *,
						MX_MARCCD *, unsigned long );

static mx_status_type mxd_marccd_handle_v0_state_value( MX_AREA_DETECTOR *,
						MX_MARCCD *, int );

static mx_status_type mxd_marccd_handle_v1_state_value( MX_AREA_DETECTOR *,
						MX_MARCCD *, int );

static const char *mxd_marccd_get_command_name( int );

static const char *mxd_marccd_get_v0_state_name( int );

#if MXD_MARCCD_DEBUG_SHOW_STATE_CHANGES
#    define MX_MARCCD_SHOW_STATE_CHANGE(x) \
		mxd_marccd_show_state_change( marccd, (x) )

     static void mxd_marccd_show_state_change( MX_MARCCD *, const char * );
#else
#    define MX_MARCCD_SHOW_STATE_CHANGE(x)
#endif


/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_marccd_get_pointers( MX_AREA_DETECTOR *ad,
			MX_MARCCD **marccd,
			const char *calling_fname )
{
	static const char fname[] = "mxd_marccd_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( marccd == (MX_MARCCD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MARCCD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*marccd = (MX_MARCCD *) ad->record->record_type_struct;

	if ( *marccd == (MX_MARCCD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MARCCD pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

/* Once started, mxd_marccd_monitor_thread() is the only thread that is
 * allowed to receive messages from MarCCD.
 */

static mx_status_type
mxd_marccd_monitor_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxd_marccd_monitor_thread()";

	MX_RECORD *marccd_record;
	MX_AREA_DETECTOR *ad;
	MX_MARCCD *marccd;
	int input_available;
	unsigned long sleep_us;
	mx_status_type mx_status;

	if ( args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	marccd_record = (MX_RECORD *) args;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, marccd_record->name ));
#endif
	ad = (MX_AREA_DETECTOR *) marccd_record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR pointer for record '%s' is NULL.",
			marccd_record->name );
	}

	marccd = (MX_MARCCD *) marccd_record->record_type_struct;

	if ( marccd == (MX_MARCCD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MARCCD pointer for record '%s' is NULL.",
			marccd_record->name );
	}

	sleep_us = 1000;

	while (1) {
		/* Check for a new message from MarCCD. */

		input_available = mxd_marccd_input_is_available(marccd);

		if ( input_available ) {
			mx_status = mxd_marccd_handle_response( ad,
				marccd, MXD_MARCCD_DEBUG_SHOW_RESPONSE );
		}

		mx_status = mxd_marccd_get_extended_status( ad );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/*---*/

		mx_usleep( sleep_us );

		mx_status = mx_thread_check_for_stop_request( thread );
	}
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_marccd_find_file_descriptors( MX_MARCCD *marccd )
{
	static const char fname[] = "mxd_marccd_find_file_descriptors()";

	char *out_fd_string;
	char *in_fd_string;
	char *endptr;

	/* If this function is called, it means that we are running on
	 * the same computer as MarCCD and that the current process was
	 * actually created by MarCCD via the "Remote" control mode.
	 *
	 * If so, then the MarCCD process will have created two pipes for us
	 * to use for communicating with MarCCD.  MarCCD will have stored
	 * the numerical values of the associated file descriptors into
	 * two environment variable:
	 *
	 *   IN_FD  - Used to read responses from MarCCD.
	 *   OUT_FD - Used to write commands to MarCCD.
	 *
	 * Thus, we must find these two environment variables and treat them as
	 * file descriptors.
	 */

	in_fd_string = getenv( "IN_FD" );

	if ( in_fd_string == (char *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
			"MarCCD environment variable 'IN_FD' was not found.  "
			"That probably means that this process was not created "
			"by MarCCD." );
	}

	out_fd_string = getenv( "OUT_FD" );

	if ( out_fd_string == (char *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
			"MarCCD environment variable 'OUT_FD' was not found.  "
			"That probably means that this process was not created "
			"by MarCCD." );
	}

	marccd->fd_from_marccd = (int) strtoul( in_fd_string, &endptr, 10 );

	if ( *endptr != '\0' ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The MarCCD created environment variable 'IN_FD' "
		"does not contain an integer.  Instead it is '%s'",
			in_fd_string );
	}

	marccd->fd_to_marccd = (int) strtoul( out_fd_string, &endptr, 10 );

	if ( *endptr != '\0' ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The MarCCD created environment variable 'OUT_FD' "
		"does not contain an integer.  Instead it is '%s'",
			out_fd_string );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_marccd_wait_for_idle( MX_MARCCD *marccd, double timeout )
{
	static const char fname[] = "mxd_marccd_wait_for_idle()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_CLOCK_TICK timeout_in_ticks, current_tick, finish_tick;
	int comparison;
	mx_status_type mx_status;

	if ( marccd == (MX_MARCCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MARCCD pointer passed was NULL." );
	}
	if ( marccd->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_MARCCD pointer %p is NULL.",
			marccd );
	}

	MX_DEBUG(-2,("%s invoked for '%s'.", fname, marccd->record->name));

	ad = (MX_AREA_DETECTOR *) marccd->record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR pointer for record '%s' is NULL.",
			marccd->record->name );
	}

	timeout_in_ticks = mx_convert_seconds_to_clock_ticks( timeout );

	current_tick = mx_current_clock_tick();

	finish_tick = mx_add_clock_ticks( current_tick, timeout_in_ticks );

	while(1) {
		mx_status = mxd_marccd_get_extended_status( ad );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (ad->status & MXSF_AD_IS_BUSY) == 0 ) {
			break;		/* Exit the while() loop. */
		}

		current_tick = mx_current_clock_tick();

		comparison = mx_compare_clock_ticks(current_tick, finish_tick);

		if ( comparison >= 0 ) {
			return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %f seconds for MarCCD "
			"detector '%s' to become idle.",
				timeout, marccd->record->name );
		}

		mx_msleep(1);
	}

	MX_DEBUG(-2,("%s complete for '%s'.", fname, marccd->record->name));

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_marccd_writefile( MX_AREA_DETECTOR *ad,
			MX_MARCCD *marccd,
			mx_bool_type write_corrected_frame )
{
	static const char fname[] = "mxd_marccd_writefile()";

	MX_FILE_MONITOR *file_monitor;
	char command[(2*MXU_FILENAME_LENGTH)+20];
	char filename[MXU_FILENAME_LENGTH+1];
	double timeout_in_seconds;
	MX_CLOCK_TICK timeout_in_ticks, current_tick, finish_tick;
	int comparison;
	size_t dirname_length;
	FILE *file;
	int saved_errno;
	mx_bool_type file_has_changed;
	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}
	if ( marccd == (MX_MARCCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MARCCD pointer passed was NULL." );
	}

	mx_status = mx_area_detector_construct_next_datafile_name(
						ad->record, TRUE, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strlen(ad->datafile_name) == 0 ) {
		return mx_error( MXE_NOT_READY, fname,
		"The MarCCD datafile name pattern has not "
		"been initialized for MarCCD detector '%s'.  "
		"You can fix this by assigning a value to the MX server "
		"variable '%s.datafile_pattern'.",
			ad->record->name, ad->record->name );
	}

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s: next datafile name = '%s'",
		fname, ad->datafile_name));
#endif
	/* Wait for the area detector to become idle. */

	timeout_in_seconds = 5.0;

	mx_status = mxd_marccd_wait_for_idle( marccd, timeout_in_seconds );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the name of the datafile. */

	dirname_length = strlen( ad->datafile_directory );
	
	if ( dirname_length > 0 ) {
		snprintf( filename, sizeof(filename), "%s/%s",
				ad->datafile_directory, ad->datafile_name );
	} else {
		strlcpy( filename, ad->datafile_name, sizeof(filename) );
	}

	/* We need to know when MarCCD has created the file.  The plan is
	 * to create an empty file with this name and then set up a file
	 * monitor on this file so that we can detect when the file
	 * is changed.
	 */

	file = fopen( filename, "wb" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"An error occurred when area detector '%s' tried "
			"to create a zero length file called file '%s'.  "
			"Errno = %d, error message = '%s'",
				marccd->record->name, filename,
				saved_errno, strerror(saved_errno) );
	}

	fclose( file );

	/* Setup a file monitor so that we can tell when the file is
	 * modified by MarCCD.
	 */

	mx_status = mx_create_file_monitor( &file_monitor, W_OK, filename );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the writefile command. */

	if ( write_corrected_frame ) {
		snprintf( command, sizeof(command),
				"writefile,%s,1", filename );
	} else {
		snprintf( command, sizeof(command),
				"writefile,%s,0", filename );
	}

	/* Send the writefile command. */

	mx_status = mxd_marccd_command( marccd, command,
					MXF_MARCCD_CMD_WRITEFILE,
					MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_delete_file_monitor( file_monitor );
		return mx_status;
	}

	/* Call the get_extended_status method to push the status handling
	 * system into forward motion.
	 */

	MX_DEBUG(-2,("%s: invoking mxd_marccd_get_extended_status()", fname));

	mx_status = mxd_marccd_get_extended_status( ad );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_delete_file_monitor( file_monitor );
		return mx_status;
	}

	/* Wait until the file is created. */

	timeout_in_seconds = 10.0;

	timeout_in_ticks =
		mx_convert_seconds_to_clock_ticks( timeout_in_seconds );

	current_tick = mx_current_clock_tick();

	finish_tick = mx_add_clock_ticks( current_tick, timeout_in_ticks );

	while (1) {
		file_has_changed = mx_file_has_changed( file_monitor );

		if ( file_has_changed ) {
			break;		/* Exit the while() loop. */
		}

		current_tick = mx_current_clock_tick();

		comparison = mx_compare_clock_ticks(current_tick, finish_tick);

		if ( comparison >= 0 ) {
			(void) mx_delete_file_monitor( file_monitor );

			return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %f seconds for "
			"MarCCD detector '%s' to create image file '%s'.",
				timeout_in_seconds,
				marccd->record->name,
				filename );
		}

		mx_msleep(1);
	}

	/* If we get here, the file has been created. */

	(void) mx_delete_file_monitor( file_monitor );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_marccd_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_marccd_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_MARCCD *marccd;
	mx_status_type mx_status;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	marccd = (MX_MARCCD *) malloc( sizeof(MX_MARCCD) );

	if ( marccd == (MX_MARCCD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_MARCCD structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = marccd;
	record->class_specific_function_list = &mxd_marccd_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	marccd->record = record;

	marccd->fd_from_marccd = -1;
	marccd->fd_to_marccd = -1;

	marccd->old_command = MXF_MARCCD_CMD_NONE;
	marccd->old_state = MXF_MARCCD_STATE_UNKNOWN;

	marccd->current_command = MXF_MARCCD_CMD_NONE;
	marccd->current_state = MXF_MARCCD_STATE_UNKNOWN;

	marccd->next_state = MXF_MARCCD_STATE_UNKNOWN;

	/* Create a mutex for access to the extended_status method. */

	mx_status = mx_mutex_create( &(marccd->extended_status_mutex) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_finish_record_initialization( MX_RECORD *record )
{
	return mx_area_detector_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_marccd_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_marccd_open()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_MARCCD *marccd = NULL;
	unsigned long mask;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	if ( marccd->pulse_generator_record->mx_class != MXC_PULSE_GENERATOR ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The pulse_generator_record '%s' used by area detector '%s' "
		"is not actually a pulse generator.", 
			marccd->pulse_generator_record->name,
			record->name );
	}

	marccd->finish_time = mx_set_clock_tick_to_maximum();

	marccd->detector_started = FALSE;

	marccd->old_status = ULONG_MAX;
	marccd->old_total_num_frames = ULONG_MAX;

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	ad->bytes_per_pixel = 2;
	ad->bits_per_pixel = 16;

	ad->image_format = MXT_IMAGE_FORMAT_GREY16;

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						MXU_IMAGE_FORMAT_NAME_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;

	/*------------------------------------------------------------*/

	/* Find the MarCCD file descriptors in the local environment. */

	mx_status = mxd_marccd_find_file_descriptors( marccd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s: fd_from_marccd = %d, fd_to_marccd = %d",
			fname, marccd->fd_from_marccd, marccd->fd_to_marccd));
#endif

	/* We default to assuming that we are using the oldest version
	 * of MarCCD that supports remote mode.  As we detect the
	 * presence of newer features, we will bump up the version
	 * number.  It is unfortunate that MarCCD does not provide
	 * an explicit 'is_version' message.
	 */

	marccd->marccd_version = MX_MARCCD_VER_REMOTE_MODE_0;

	/* Start a thread to monitor messages sent by MarCCD. */

	mx_status = mx_thread_create(
			&(marccd->marccd_monitor_thread),
			mxd_marccd_monitor_thread,
			record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure automatic saving or readout of image frames. */

	mask = MXF_AD_SAVE_FRAME_AFTER_ACQUISITION
		| MXF_AD_READOUT_FRAME_AFTER_ACQUISITION;

	if ( ad->area_detector_flags & mask ) {
		mx_status =
		  mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_marccd_close()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_MARCCD *marccd = NULL;
	int saved_errno, status_from_marccd, status_to_marccd;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_marccd_get_pointers(ad, &marccd, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.", fname, record->name));

	/* Tell MarCCD that it is time to exit remote mode. */

#if 1
	mx_status = mxd_marccd_command( marccd, "end_automation",
					MXF_MARCCD_CMD_END_AUTOMATION,
					MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait a moment for the remote end to respond to what we have done. */

	mx_msleep(1000);

#endif
	/* Shutdown the connection. */

	mx_status = MX_SUCCESSFUL_RESULT;

	status_to_marccd = close( marccd->fd_to_marccd );

	if ( status_to_marccd != 0 ) {
		saved_errno = errno;

		mx_status = mx_error( MXE_IPC_IO_ERROR, fname,
	"An error occurred while closing pipe %d to MarCCD.  "
	"Error number = %d, error message = '%s'",
			marccd->fd_to_marccd,
			saved_errno, strerror( saved_errno ) );
	}

	status_from_marccd = close( marccd->fd_from_marccd );

	if ( status_from_marccd != 0 ) {
		saved_errno = errno;

		mx_status = mx_error( MXE_IPC_IO_ERROR, fname,
		"An error occurred while closing pipe %d from MarCCD.  "
		"Error number = %d, error message = '%s'",
			marccd->fd_from_marccd,
			saved_errno, strerror( saved_errno ) );
	}

	marccd->fd_from_marccd = -1;
	marccd->fd_to_marccd = -1;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_arm()";

	MX_MARCCD *marccd = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	MX_RECORD *pulse_generator_record = NULL;
	double exposure_time;
	mx_status_type mx_status;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	sp = &(ad->sequence_parameters);

	if ( sp->sequence_type != MXT_SQ_ONE_SHOT ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Sequence type %ld is not supported for MarCCD detector '%s'.  "
		"Only one-shot sequences are supported.",
			sp->sequence_type, ad->record->name );
	}

	exposure_time = sp->parameter_array[0];

	/* Unconditionally stop the pulse generator. */

	pulse_generator_record = marccd->pulse_generator_record;

	mx_status = mx_pulse_generator_stop( pulse_generator_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Unconditionally stop the area detector. */

	mx_status = mxd_marccd_stop( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the area detector to stop. */

	mx_status = mxd_marccd_wait_for_idle( marccd, 10.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reprogram the pulse generator to generate a gate signal
 	 * for the requested exposure time.
 	 */

	mx_status = mx_pulse_generator_set_pulse_period( pulse_generator_record,
						10.0 * exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_pulse_width( pulse_generator_record,
							exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_num_pulses(
					pulse_generator_record, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_pulse_delay(
						pulse_generator_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_mode( pulse_generator_record,
							MXF_PGN_SQUARE_WAVE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_trigger()";

	MX_MARCCD *marccd = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	double exposure_time;
	MX_CLOCK_TICK clock_ticks_to_wait, start_time;
	mx_status_type mx_status;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	if ( sp->sequence_type != MXT_SQ_ONE_SHOT ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Sequence type %ld is not supported for MarCCD detector '%s'.  "
		"Only one-shot sequences are supported.",
			sp->sequence_type, ad->record->name );
	}

	exposure_time = sp->parameter_array[0];

	/* Start the external gate pulse. */

	mx_status = mx_pulse_generator_start( marccd->pulse_generator_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the finish time for the measurement. */

	clock_ticks_to_wait = 
			mx_convert_seconds_to_clock_ticks( exposure_time );

	start_time = mx_current_clock_tick();

	marccd->finish_time = mx_add_clock_ticks( start_time,
						clock_ticks_to_wait );

	/* Send the start command. */

	mx_status = mxd_marccd_command( marccd, "start",
						MXF_MARCCD_CMD_START,
						MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	marccd->detector_started = TRUE;

	/* Update the MarCCD status variables. */

	mx_status = mxd_marccd_get_extended_status( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_marccd_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_stop()";

	MX_MARCCD *marccd = NULL;
	mx_status_type mx_status;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	marccd->finish_time = mx_current_clock_tick();

	mx_status = mxd_marccd_command( marccd, "abort",
						MXF_MARCCD_CMD_ABORT,
						MXD_MARCCD_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_get_extended_status()";

	MX_MARCCD *marccd = NULL;
	MX_CLOCK_TICK current_time;
	int comparison;
	mx_status_type mx_status;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_mutex_lock(marccd->extended_status_mutex);

#if 0 && MXD_MARCCD_DEBUG_EXTENDED_STATUS
	if ( marccd->old_state != marccd->current_state ) {
		MX_DEBUG(-2,("%s: '%s' current state = %#x",
			fname, ad->record->name, marccd->current_state ));
	}
#endif

#if 0
	mx_status = mx_pulse_generator_is_busy(
			marccd->pulse_generator_record, NULL );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_mutex_unlock(marccd->extended_status_mutex);
		return mx_status;
	}
#endif

	/* Have we reached the end of the exposure time? */

	current_time = mx_current_clock_tick();

	comparison = mx_compare_clock_ticks(current_time, marccd->finish_time);

	if ( comparison < 0 ) {
		ad->last_frame_number = -1;
	} else {
		/* We are after the end of the exposure time. */

		if ( marccd->detector_started ) {

			/* Stop the pulse generator. */

			mx_status = mx_pulse_generator_stop(
					marccd->pulse_generator_record );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_mutex_unlock(marccd->extended_status_mutex);
				return mx_status;
			}

			/* If we just finished an acquisition cycle,
			 * then increment the total number of frames.
			 */

			(ad->total_num_frames)++;

			marccd->detector_started = FALSE;
		}

		ad->last_frame_number = 0;

		ad->status = 0;
	}

	if ( marccd->detector_started ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

#if MXD_MARCCD_DEBUG_EXTENDED_STATUS
	if ( (ad->status != marccd->old_status)
	  || (ad->total_num_frames != marccd->old_total_num_frames) )
	{
		MX_DEBUG(-2,
    ("%s: current_time = (%lu,%lu), finish_time = (%lu,%lu), comparison = %d",
		fname, current_time.high_order, current_time.low_order,
		marccd->finish_time.high_order, marccd->finish_time.low_order,
		comparison ));

		MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
		fname, ad->last_frame_number, ad->total_num_frames,
		ad->status));
	}
#endif
	marccd->old_status = ad->status;
	marccd->old_total_num_frames = ad->total_num_frames;

	mx_mutex_unlock(marccd->extended_status_mutex);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_marccd_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_readout_frame()";

	MX_MARCCD *marccd = NULL;
	unsigned long flags, mask;
	mx_status_type mx_status;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	if ( ad->readout_frame != 0 ) {
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal frame number %ld requested from MarCCD detector '%s'."
		"  Only frame number 0 is valid for detector '%s'",
			ad->readout_frame, ad->record->name, ad->record->name );

		ad->readout_frame = 0;
		return mx_status;
	}

	/* Now tell the detector to read out an image. */

	mx_status = mxd_marccd_command( marccd, "readout,0",
					MXF_MARCCD_CMD_READOUT,
					MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	mx_msleep(2000);
#endif

	/* Wait for the readout to complete. */

	mx_status = mxd_marccd_wait_for_idle( marccd, 10.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if we need to save the image file now. */

	flags = ad->area_detector_flags;

	if ( flags & MXF_AD_SAVE_REMOTE_FRAME_AFTER_ACQUISITION ) {

		mask = MXFT_AD_FLOOD_FIELD_FRAME
				| MXFT_AD_GEOMETRICAL_CORRECTION;

		/* We save here if no correction is to be performed. */

		if ( ( mask & ad->correction_flags ) == 0 ) {
			mx_status = mxd_marccd_writefile( ad, marccd, FALSE );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_correct_frame()";

	MX_MARCCD *marccd = NULL;
	unsigned long flags, mask;
	mx_status_type mx_status;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Return if there is no correction to be performed. */

	mask = MXFT_AD_FLOOD_FIELD_FRAME | MXFT_AD_GEOMETRICAL_CORRECTION;

	if ( ( mask & ad->correction_flags ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Send the correction command. */

	marccd->next_state = MXF_MARCCD_STATE_CORRECT;

	mx_status = mxd_marccd_command( marccd, "correct",
					MXF_MARCCD_CMD_CORRECT,
					MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the correction to complete. */

	mx_status = mxd_marccd_wait_for_idle( marccd, 10.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if we need to save the image file now. */

	flags = ad->area_detector_flags;

	if ( flags & MXF_AD_SAVE_REMOTE_FRAME_AFTER_ACQUISITION ) {

		/* We save here if there _is_ a correction to be performed. */

		if ( ( mask & ad->correction_flags ) != 0 ) {
			mx_status = mxd_marccd_writefile( ad, marccd, TRUE );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_transfer_frame()";

	MX_MARCCD *marccd = NULL;
	char marccd_filename[(2*MXU_FILENAME_LENGTH) + 20];
	size_t dirname_length;
	mx_status_type mx_status;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* We can only handle transferring the image frame. */

	if ( ad->transfer_frame != MXFT_AD_IMAGE_FRAME ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Transferring frame type %lu is not supported for "
		"MarCCD detector '%s'.  Only the image frame (type 0) "
		"is supported.  MarCCD calls it the 'raw frame'.",
			ad->transfer_frame, ad->record->name );
	}

	/* We attempt to read in the most recently saved MarCCD image. */

	dirname_length = strlen( ad->datafile_directory );

	if ( dirname_length > 0 ) {
		snprintf( marccd_filename,
			sizeof(marccd_filename),
			"%s/%s", ad->datafile_directory, ad->datafile_name );
	} else {
		snprintf( marccd_filename,
			sizeof(marccd_filename),
			"%s", ad->datafile_name );
	}

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s: MarCCD filename to open = '%s'",
		fname, marccd_filename));
#endif
	if ( strlen( marccd_filename ) == 0 ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The MarCCD 'datafile_directory' and 'datafile_name' "
		"fields have not been initialized for detector '%s', so "
		"we cannot read in the most recently acquired MarCCD image.",
			ad->record->name );
	}

	mx_status = mx_image_read_marccd_file( &(ad->image_frame),
						marccd_filename );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_get_parameter()";

	MX_MARCCD *marccd = NULL;
	mx_status_type mx_status;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		/* We do not send 'get_size' or 'get_bin' commands to MarCCD
		 * here, since MarCCD will keep us up to date via 'is_size'
		 * and 'is_bin' messages.
		 */

		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		ad->image_format = MXT_IMAGE_FORMAT_GREY16;

		mx_status = mx_image_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_set_parameter()";

	MX_MARCCD *marccd = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	char command[40];
	mx_status_type mx_status;

	static long allowed_binsize_array[] = { 1, 2, 4, 8 };

	static int num_allowed_binsizes = sizeof(allowed_binsize_array)
					/ sizeof(allowed_binsize_array[0]);

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif

	sp = &(ad->sequence_parameters);

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize_array );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command),
			"set_bin,%lu,%lu",
			ad->binsize[0], ad->binsize[1] );

		mx_status = mxd_marccd_command( marccd, command,
						MXF_MARCCD_CMD_SET_BIN,
						MXD_MARCCD_DEBUG );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
		if ( sp->sequence_type != MXT_SQ_ONE_SHOT ) {
			mx_status = mx_error( MXE_UNSUPPORTED, fname,
			"Sequence type %ld is not supported for MarCCD "
			"detector '%s'.  "
			"Only one-shot sequences (type 1) are supported.",
				sp->sequence_type, ad->record->name );

			sp->sequence_type = MXT_SQ_ONE_SHOT;
			return mx_status;
		}
		break;

	case MXLV_AD_TRIGGER_MODE:
		if ( ad->trigger_mode != MXT_IMAGE_INTERNAL_TRIGGER ) {
			mx_status = mx_error( MXE_UNSUPPORTED, fname,
			"Trigger mode %ld is not supported for MarCCD detector "
		    "'%s'.  Only internal trigger mode (mode 1) is supported.",
				ad->trigger_mode, ad->record->name );

			ad->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;
			return mx_status;
		}
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Changing parameter '%s' for area detector '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
		break;

	default:
		mx_status = mx_area_detector_default_set_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_measure_correction()";

	MX_MARCCD *marccd = NULL;
	/* char saved_datafile_name[MXU_FILENAME_LENGTH+1]; */
	mx_status_type mx_status;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	if ( ad->correction_measurement_type != MXFT_AD_DARK_CURRENT_FRAME ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Correction measurement type %lu is not supported for "
		"MarCCD detector '%s'.  Only dark current/background "
		"(type %d) measurements are supported.",
			ad->correction_measurement_type,
			ad->record->name,
			MXFT_AD_DARK_CURRENT_FRAME );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_marccd_command( MX_MARCCD *marccd,
			char *command,
			int command_number,
			unsigned long flags )
{
	static const char fname[] = "mxd_marccd_command()";

	size_t num_bytes_left;
	char *ptr;
	char newline = '\n';
	int saved_errno;
	long num_bytes_written;

	if ( marccd == (MX_MARCCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MARCCD pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command string pointer passed was NULL." );
	}

	if ( flags ) {
		MX_DEBUG(-2,("MarCCD: sent '%s' to '%s'",
				command, marccd->record->name));
	}

	if ( marccd->fd_to_marccd < 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The connection to MarCCD has not been "
		"correctly initialized, fd_to_marccd = %d",
			marccd->fd_to_marccd );
	}

	/* If the command number has changed, display that. */

	marccd->old_command = marccd->current_command;

	marccd->current_command = command_number;

	if ( marccd->old_command != command_number ) {
		MX_DEBUG(-2,
		("%s: MarCCD NEW COMMAND = %d '%s', OLD COMMAND = '%s'",
			fname, command_number,
			mxd_marccd_get_command_name(command_number),
			mxd_marccd_get_command_name(marccd->old_command) ));
	}

	/* Send the command string. */

	ptr            = command;
	num_bytes_left = strlen( command );

	while ( num_bytes_left > 0 ) {

		num_bytes_written = write( marccd->fd_to_marccd,
					ptr, num_bytes_left );

		if ( num_bytes_written < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
		"Error sending command '%s' to MarCCD '%s'.  "
		"Error message = '%s'.",
				command, marccd->record->name,
				strerror( saved_errno ) );
		}

		ptr            += num_bytes_written;
		num_bytes_left -= num_bytes_written;
	}

	/* Write a trailing newline. */

	num_bytes_written = 0;

	while ( num_bytes_written == 0 ) {
		num_bytes_written = write( marccd->fd_to_marccd,
					&newline, 1 );

		if ( num_bytes_written < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
		"Error writing the trailing newline for command '%s' "
		"to MarCCD '%s'.  Error message = '%s'.",
				command, marccd->record->name,
				strerror( saved_errno ) );
		}
	}

#if 0
	MX_DEBUG(-2,("%s: finished sending command '%s'.",
			fname, command));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static const char *
mxd_marccd_get_command_name( int marccd_command )
{
	static const char command_name[][20] = {
		"none",
		"start",
		"readout",
		"dezinger",
		"correct",
		"writefile",
		"abort",
		"header",
		"shutter",
		"get state",
		"set state",
		"get bin",
		"set bin",
		"get size",
		"phi",
		"distance",
		"end_automation" };

	static int num_command_names = sizeof( command_name )
					/ sizeof( command_name[0] );

	static const char illegal[20] = "illegal";

	if (( marccd_command < 0 ) || ( marccd_command > num_command_names )) {
		return &illegal[0];
	}

	return &command_name[ marccd_command ][0];
}

/*--------------------------------------------------------------------------*/

static const char *
mxd_marccd_get_v0_state_name( int marccd_state )
{
	static const char state_name[][20] = {
		"idle",
		"acquire",
		"readout",
		"correct",
		"writing",
		"aborting",
		"unavailable",
		"error",
		"busy" };

	static int num_state_names = sizeof( state_name )
					/ sizeof( state_name[0] );

	static const char illegal[20] = "illegal";

	if ( ( marccd_state < 0 ) || ( marccd_state > num_state_names ) ) {
		return &illegal[0];
	}

	return &state_name[ marccd_state ][0];
}

#define MX_MARCCD_UNHANDLED_COMMAND_MESSAGE \
	mx_warning( "UNHANDLED MarCCD command %d for MarCCD state = %d", \
		marccd->current_command, current_state )


static mx_status_type
mxd_marccd_handle_v0_state_value( MX_AREA_DETECTOR *ad,
				MX_MARCCD *marccd,
				int current_state )
{
	static const char fname[] = "mxd_marccd_handle_v0_state_value()";

	marccd->old_state = marccd->current_state;

	marccd->current_state = current_state;

	if ( marccd->old_state != current_state ) {
		MX_DEBUG(-2,
		("State: MarCCD NEW STATE = %d '%s', OLD STATE = '%s'",
			current_state,
			mxd_marccd_get_v0_state_name(current_state),
			mxd_marccd_get_v0_state_name(marccd->old_state) ));
	}

	/* Check to see if we have been asked to shut down. */

	if ( current_state == MXF_MARCCD_STATE_UNAVAILABLE ) {
		/* Tell MarCCD that we are shutting down. */

		(void) mx_close_hardware( marccd->record );

		/* Stop the MX server nicely. */

		(void) mx_kill_process_id( mx_process_id() );
	}

	/* The interpretation of the state flag depends on the command that
	 * is currently supposed to be executing.
	 */

	ad->status = 0;

	switch( marccd->current_command ) {
	case MXF_MARCCD_CMD_NONE:
	default:
		switch( current_state ) {
		case MXF_MARCCD_STATE_IDLE:
			MX_MARCCD_SHOW_STATE_CHANGE( "MarCCD is idle" );

			ad->status = 0;
			break;
		case MXF_MARCCD_STATE_BUSY:
			MX_MARCCD_SHOW_STATE_CHANGE( "MarCCD is busy" );

			ad->status = MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
			break;
		default:
			MX_MARCCD_UNHANDLED_COMMAND_MESSAGE;
			break;
		}
		break;
	case MXF_MARCCD_CMD_START:
		switch( current_state ) {
		case MXF_MARCCD_STATE_IDLE:
		case MXF_MARCCD_STATE_BUSY:
		case MXF_MARCCD_STATE_ACQUIRE:

			MX_MARCCD_SHOW_STATE_CHANGE(
				"Waiting for start command to finish." );

			ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
			break;
		default:
			MX_MARCCD_UNHANDLED_COMMAND_MESSAGE;
			break;
		}
		break;
	case MXF_MARCCD_CMD_READOUT:
		switch( current_state ) {
		case MXF_MARCCD_STATE_READOUT:
			MX_MARCCD_SHOW_STATE_CHANGE( "Readout in progress." );

			ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
			break;
		case MXF_MARCCD_STATE_IDLE:
			MX_MARCCD_SHOW_STATE_CHANGE( "Readout is complete." );

			ad->status = 0;
			marccd->current_command = MXF_MARCCD_CMD_NONE;
			break;
		default:
			MX_MARCCD_UNHANDLED_COMMAND_MESSAGE;
			break;
		}
		break;
	case MXF_MARCCD_CMD_CORRECT:
		switch( current_state ) {
		case MXF_MARCCD_STATE_IDLE:
			if ( marccd->next_state == MXF_MARCCD_STATE_CORRECT ) {
				MX_MARCCD_SHOW_STATE_CHANGE(
				"Waiting for correct command to finish." );

				ad->status = MXSF_AD_CORRECTION_IN_PROGRESS;
			} else {
				MX_MARCCD_SHOW_STATE_CHANGE(
					"Frame correction has completed." );

				ad->status = 0;
			}
			break;
		case MXF_MARCCD_STATE_CORRECT:
			MX_MARCCD_SHOW_STATE_CHANGE(
				"Waiting for correct command to finish." );

			ad->status = MXSF_AD_CORRECTION_IN_PROGRESS;

			marccd->next_state = MXF_MARCCD_STATE_IDLE;
			break;
		case MXF_MARCCD_STATE_ERROR:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
"An error occurred while trying to correct a frame for MarCCD device '%s'.",
				ad->record->name );
		default:
			MX_MARCCD_UNHANDLED_COMMAND_MESSAGE;
			break;
		}
		break;
	case MXF_MARCCD_CMD_WRITEFILE:
		switch( current_state ) {
		case MXF_MARCCD_STATE_IDLE:
			if ( marccd->next_state == MXF_MARCCD_STATE_WRITING ) {
				MX_MARCCD_SHOW_STATE_CHANGE(
					"Waiting for frame write to complete.");

				ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
			} else {
				MX_MARCCD_SHOW_STATE_CHANGE(
					"Frame write has completed." );

				ad->status = 0;
			}
			break;
		case MXF_MARCCD_STATE_WRITING:
			MX_MARCCD_SHOW_STATE_CHANGE(
				"Waiting for frame write to complete." );

			ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;

			marccd->next_state = MXF_MARCCD_STATE_IDLE;
			break;
		case MXF_MARCCD_STATE_ERROR:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
"An error occurred while trying to write a frame for MarCCD device '%s'.",
				ad->record->name );
		default:
			MX_MARCCD_UNHANDLED_COMMAND_MESSAGE;
			break;
		}
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_marccd_handle_v1_state_value( MX_AREA_DETECTOR *ad,
				MX_MARCCD *marccd,
				int current_state )
{
	static const char fname[] = "mxd_marccd_handle_v1_state_value()";

	int command_state, acquire_state, read_state;
	int correct_state, write_state, dezinger_state;

	marccd->old_state = marccd->current_state;

	marccd->current_state = current_state;

	command_state  = current_state & 0xf;
	acquire_state  = ( current_state >>  4 ) & 0xf;
	read_state     = ( current_state >>  8 ) & 0xf;
	correct_state  = ( current_state >> 12 ) & 0xf;
	write_state    = ( current_state >> 16 ) & 0xf;
	dezinger_state = ( current_state >> 20 ) & 0xf;

	if ( marccd->old_state != current_state ) {
		MX_DEBUG(-2,
			("State: MarCCD NEW STATE = %#x, OLD STATE = %#x",
			current_state, marccd->old_state));

		MX_DEBUG(-2,
    ("State: dez = %#x, wri = %#x, cor = %#x, rea = %#x, acq = %#x, cmd = %#x",
			dezinger_state, write_state, correct_state,
			read_state, acquire_state, command_state ));
	}

	ad->status = 0;

	if ( marccd->detector_started ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

	/* Command processing. */

	switch( command_state ) {
	case MXF_MARCCD_STATE_IDLE:
		break;
	case MXF_MARCCD_STATE_UNAVAILABLE:
		ad->status |= MXSF_AD_ERROR;
		break;
	case MXF_MARCCD_STATE_ERROR:
		ad->status |= MXSF_AD_ERROR;
		break;
	case MXF_MARCCD_STATE_BUSY:
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
		break;
	default:
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"Illegal command state %#x seen in controller status %#x "
		"for area detector '%s'.",
			command_state, current_state,
			ad->record->name );
		break;
	}

	/* Acquisition task. */

	if ( acquire_state & MXF_MARCCD_TASK_STATUS_QUEUED ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	} else
	if ( acquire_state & MXF_MARCCD_TASK_STATUS_EXECUTING ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	} else
	if ( acquire_state & MXF_MARCCD_TASK_STATUS_ERROR ) {
		ad->status |= MXSF_AD_ERROR;
	}

	/* Read task. */

	if ( read_state & MXF_MARCCD_TASK_STATUS_QUEUED ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	} else
	if ( read_state & MXF_MARCCD_TASK_STATUS_EXECUTING ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	} else
	if ( read_state & MXF_MARCCD_TASK_STATUS_ERROR ) {
		ad->status |= MXSF_AD_ERROR;
	}

	/* Correction task. */

	if ( correct_state & MXF_MARCCD_TASK_STATUS_QUEUED ) {
		ad->status |= MXSF_AD_CORRECTION_IN_PROGRESS;
	} else
	if ( correct_state & MXF_MARCCD_TASK_STATUS_EXECUTING ) {
		ad->status |= MXSF_AD_CORRECTION_IN_PROGRESS;
	} else
	if ( correct_state & MXF_MARCCD_TASK_STATUS_ERROR ) {
		ad->status |= MXSF_AD_ERROR;
	}

	/* Write task. */

	if ( write_state & MXF_MARCCD_TASK_STATUS_QUEUED ) {
		ad->status |= MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
	} else
	if ( write_state & MXF_MARCCD_TASK_STATUS_EXECUTING ) {
		ad->status |= MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
	} else
	if ( write_state & MXF_MARCCD_TASK_STATUS_ERROR ) {
		ad->status |= MXSF_AD_ERROR;
	}

	/* Dezinger task. */

	if ( dezinger_state & MXF_MARCCD_TASK_STATUS_QUEUED ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	} else
	if ( dezinger_state & MXF_MARCCD_TASK_STATUS_EXECUTING ) {
		ad->status |= MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
	} else
	if ( dezinger_state & MXF_MARCCD_TASK_STATUS_ERROR ) {
		ad->status |= MXSF_AD_ERROR;
	}

	if ( marccd->old_state != current_state ) {
		MX_DEBUG(-2,("%s: '%s' status = %#lx, last = %ld, total = %ld",
			fname, ad->record->name, ad->status,
			ad->last_frame_number, ad->total_num_frames));
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

#if MXD_MARCCD_DEBUG_SHOW_STATE_CHANGES
static void
mxd_marccd_show_state_change( MX_MARCCD *marccd,
				const char *message )
{
	if ( ( marccd->old_command != marccd->current_command )
	  || ( marccd->old_state   != marccd->current_state   ) )
	{
		MX_DEBUG(-2,("State: %s", message));
	}

	return;
}
#endif

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_marccd_handle_response( MX_AREA_DETECTOR *ad,
				MX_MARCCD *marccd,
				unsigned long response_flags )
{
	static const char fname[] = "mxd_marccd_handle_response()";

	char response[100];
	size_t length;
	int i, num_items, saved_errno, marccd_state, int_value;
	long num_bytes_read;
	unsigned long marccd_flags;
	mx_status_type mx_status;

#if 0
	MX_DEBUG(-2,("%s invoked.",fname));
#endif

	if ( marccd == (MX_MARCCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MARCCD pointer passed was NULL." );
	}

	/* Read the response line from MarCCD. */

	/* Read until we get a newline. */

	if ( marccd->fd_from_marccd < 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The connection from MarCCD has not been fully initialized." );
	}

	for ( i = 0; i < sizeof(response); i++ ) {

		num_bytes_read = read( marccd->fd_from_marccd,
					&(response[i]), 1 );

		if ( num_bytes_read < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"Error reading from MarCCD '%s'.  "
			"Error message = '%s'.",
				marccd->record->name,
				strerror( saved_errno ) );
		}
		if ( num_bytes_read == 0 ) {
			response[i] = '\0';
			break;		/* Exit the for() loop. */
		}
		if ( response[i] == '\n' ) {
			/* Found the end of the line. */

			response[i] = '\0';
			break;		/* Exit the for() loop. */
		}
	}

	/* If the response was too long, print an error message. */

	if ( i >= sizeof(response) ) {
		response[ sizeof(response) - 1 ] = '\0';

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The response from MarCCD '%s' is too long to fit into "
		"our input buffer.  The response begins with '%s'.",
			marccd->record->name, response );
	}

	/* Delete any trailing newline if present. */

	length = strlen( response );

	if ( response[length-1] == '\n' )
		response[length-1] = '\0';

	if ( response_flags ) {

#if MXD_MARCCD_DEBUG_SHOW_STATE_RESPONSE
		MX_DEBUG(-2,("MarCCD: received '%s' from '%s'",
				response, marccd->record->name ));
#else
		if ( strncmp( response, "is_state", 8 ) != 0 ) {
			MX_DEBUG(-2,("MarCCD: received '%s' from '%s'",
					response, marccd->record->name ));
		}
#endif
	}

	marccd_flags = marccd->marccd_flags;

	/* Handle the message from the MarCCD detector. */

	if ( strncmp( response, "is_size_bkg", 11 ) == 0 ) {

		if ( marccd->marccd_version < MX_MARCCD_VER_GET_SIZE_BKG ) {
			marccd->marccd_version = MX_MARCCD_VER_GET_SIZE_BKG;
		}

		mx_warning( "Handler for '%s' is not yet implemented.",
			response);
	} else
	if ( strncmp( response, "is_frameshift", 11 ) == 0 ) {

		if ( marccd->marccd_version < MX_MARCCD_VER_FRAMESHIFT ) {
			marccd->marccd_version = MX_MARCCD_VER_FRAMESHIFT;
		}

		num_items = sscanf( response, "is_frameshift,%d", &int_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the MarCCD frameshift status in the "
			"response '%s'.", response );
		}

		if ( int_value ) {
			marccd->frameshift_enabled = TRUE;
		} else {
			marccd->frameshift_enabled = FALSE;
		}
	} else
	if ( strncmp( response, "is_stability", 11 ) == 0 ) {

		if ( marccd->marccd_version < MX_MARCCD_VER_STABILITY ) {
			marccd->marccd_version = MX_MARCCD_VER_STABILITY;
		}

		num_items = sscanf( response, "is_stability,%d", &int_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the MarCCD stability status in the "
			"response '%s'.", response );
		}

		if ( int_value ) {
			marccd->stability_enabled = TRUE;
		} else {
			marccd->stability_enabled = FALSE;
		}
	} else
	if ( strncmp( response, "is_size", 7 ) == 0 ) {

		num_items = sscanf( response, "is_size,%ld,%ld",
				&(ad->framesize[0]), &(ad->framesize[1]) );

		if ( num_items != 2 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Cannot find the MarCCD data frame size in the "
				"response '%s'.", response );
		}

		ad->bytes_per_frame =
			mx_round( ad->framesize[0] * ad->framesize[1]
							* ad->bytes_per_pixel );

#if 0
		MX_DEBUG(-2,
		("%s: ad->framesize[0] = %ld, ad->framesize[1] = %ld",
			fname, ad->framesize[0], ad->framesize[0]));
#endif

		if ( ad->maximum_framesize[0] == 0 ) {
		  ad->maximum_framesize[0] = ad->framesize[0] * ad->binsize[0];
		  ad->maximum_framesize[1] = ad->framesize[1] * ad->binsize[1];
		}
	} else
	if ( strncmp( response, "is_bin", 6 ) == 0 ) {

		num_items = sscanf( response, "is_bin,%ld,%ld",
				&(ad->binsize[0]), &(ad->binsize[1]) );

		if ( num_items != 2 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Cannot find the MarCCD bin size in the "
				"response '%s'.", response );
		}

#if 0
		MX_DEBUG(-2,("%s: ad->binsize[0] = %ld, ad->binsize[1] = %ld",
			fname, ad->binsize[0], ad->binsize[1]));
#endif

		if ( ad->maximum_framesize[0] == 0 ) {
		  ad->maximum_framesize[0] = ad->framesize[0] * ad->binsize[0];
		  ad->maximum_framesize[1] = ad->framesize[1] * ad->binsize[1];
		}
	} else
	if ( strncmp( response, "is_preset", 9 ) == 0 ) {

		MX_DEBUG(-2,("%s: preset '%s' seen. WHAT DO WE DO WITH IT?",
			fname, response));

	} else
	if ( strncmp( response, "is_state", 8 ) == 0 ) {
	
		num_items = sscanf( response, "is_state,%d", &marccd_state );
	
		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Cannot find the MarCCD status in the "
				"response '%s'.", response );
		}

		if ( marccd_flags & MXF_MARCCD_REMOTE_MODE_1 ) {
			mx_status = mxd_marccd_handle_v1_state_value( ad,
						marccd, marccd_state );
		} else {
			mx_status = mxd_marccd_handle_v0_state_value( ad,
						marccd, marccd_state );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else
	if ( strncmp( response, "is_temp", 7 ) == 0 ) {

		num_items = sscanf( response, "is_temp,%lf",
						&(marccd->temperature) );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Cannot find the MarCCD temperature in the "
				"response '%s'.", response );
		}

	} else
	if ( strncmp( response, "is_press", 8 ) == 0 ) {

		num_items = sscanf( response, "is_press,%lf",
						&(marccd->pressure) );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Cannot find the MarCCD pressure in the "
				"response '%s'.", response );
		}
	
	} else {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable response '%s' from MarCCD '%s'.",
			response, ad->record->name );
	}

	/* If the maximum framesize has never been set, then we check
	 * to see if we have both the framesize and binsize values
	 * necessary to set it.
	 */

	if ( ad->maximum_framesize[0] == 0 ) {
		if ( ( ad->framesize[0] != 0 ) && ( ad->binsize[0] != 0 ) ) {
			ad->maximum_framesize[0]
				= ad->framesize[0] * ad->binsize[0];
			ad->maximum_framesize[1]
				= ad->framesize[1] * ad->binsize[1];
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static int
mxd_marccd_input_is_available( MX_MARCCD *marccd )
{
	static const char fname[] = "mxd_marccd_input_is_available()";

	int fd, select_status;
	struct timeval timeout;
#if HAVE_FD_SET
	fd_set mask;
#else
	long mask;
#endif

	fd = marccd->fd_from_marccd;

	if ( fd < 0 ) {
		(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Attempted to use select() on invalid socket %d", fd );

		return FALSE;
	}

#if HAVE_FD_SET
	FD_ZERO( &mask );
	FD_SET( fd, &mask );
#else
	mask = 1 << fd;
#endif
	timeout.tv_usec = 0;
	timeout.tv_sec = 0;

	select_status = select( 1 + fd, &mask, NULL, NULL, &timeout );

	if ( select_status ) {

#if MXD_MARCCD_DEBUG_INPUT_IS_AVAILABLE
		MX_DEBUG(-2,("%s: select_status = %d", fname, select_status));
#endif
		return TRUE;
	} else {
		return FALSE;
	}
}

