/*
 * Name:    d_marccd.c
 *
 * Purpose: MX driver for MarCCD remote control.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MARCCD_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_thread.h"
#include "mx_unistd.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_marccd.h"

/*---*/

#define MX_CR_LF	"\015\012"

MX_RECORD_FUNCTION_LIST mxd_marccd_record_function_list = {
	mxd_marccd_initialize_type,
	mxd_marccd_create_record_structures,
	mxd_marccd_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_marccd_open,
	mxd_marccd_close
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_marccd_function_list = {
	NULL,
	mxd_marccd_trigger,
	mxd_marccd_stop,
	mxd_marccd_stop,
	NULL,
	NULL,
	NULL,
	mxd_marccd_get_extended_status,
	mxd_marccd_readout_frame,
	mxd_marccd_correct_frame,
	NULL,
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

static mx_status_type mxd_marccd_handle_state_value( MX_AREA_DETECTOR *,
						MX_MARCCD *, int );

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

	sleep_us = 10;

	while (1) {
		/* Check for a new message from MarCCD. */

		input_available = mxd_marccd_input_is_available(marccd);

		if ( input_available ) {
			mx_status = mxd_marccd_handle_response( ad,
						marccd, MXD_MARCCD_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxd_marccd_handle_state_value( ad, marccd,
							marccd->current_state );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

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

static mx_status_type
mxd_marccd_connect_to_server( MX_MARCCD *marccd )
{
	MX_SOCKET *client_socket;
	mx_status_type mx_status;

	/* If this function is called, it means that we are supposed to 
	 * connect via TCP/IP to 'marccd_server_socket' or some workalike
	 * running on the remote MarCCD computer.
	 */

	mx_status = mx_tcp_socket_open_as_client( &client_socket,
					marccd->marccd_host,
					marccd->marccd_port, 0,
					MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	marccd->marccd_socket = client_socket;

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_marccd_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_type( record_type,
					&num_record_fields,
					&record_field_defaults,
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
	record->class_specific_function_list = 
			&mxd_marccd_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	marccd->record = record;

	marccd->marccd_socket = NULL;

	marccd->fd_from_marccd = -1;
	marccd->fd_to_marccd = -1;

	marccd->current_command = MXF_MARCCD_CMD_UNKNOWN;
	marccd->current_state = MXF_MARCCD_STATE_UNKNOWN;
	marccd->next_state = MXF_MARCCD_STATE_UNKNOWN;

	return MX_SUCCESSFUL_RESULT;
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

	MX_AREA_DETECTOR *ad;
	MX_MARCCD *marccd;
	mx_status_type mx_status;

	marccd = NULL;

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

	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	ad->bytes_per_pixel = 2;
	ad->bits_per_pixel = 16;

	ad->image_format = MXT_IMAGE_FORMAT_GREY16;

	/*------------------------------------------------------------*/

	if ( strlen( marccd->marccd_host )  == 0 ) {

		/* If the MarCCD host name is of zero length, then we assume
		 * that the current process was started by MarCCD itself.
		 */

		mx_status = mxd_marccd_find_file_descriptors( marccd );
	} else {
		/* Otherwise, we assume that we need to connect via TCP/IP
		 * to a server spawned by MarCCD on the MarCCD host.
		 */

		mx_status = mxd_marccd_connect_to_server( marccd );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	if ( marccd->marccd_socket != NULL ) {
		MX_DEBUG(-2,("%s: connected to remote socket %d",
			fname, marccd->marccd_socket->socket_fd ));
	} else {
		MX_DEBUG(-2,(
	    "%s: connected via pipes, fd_from_marccd = %d, fd_to_marccd = %d",
			fname, marccd->fd_from_marccd, marccd->fd_to_marccd));
	}
#endif

	marccd->use_finish_time = FALSE;

	/* Start a thread to monitor messages sent by MarCCD. */

	mx_status = mx_thread_create(
			&(marccd->marccd_monitor_thread),
			mxd_marccd_monitor_thread,
			record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_marccd_close()";

	MX_AREA_DETECTOR *ad;
	MX_MARCCD *marccd;
	int saved_errno, status_from_marccd, status_to_marccd;
	mx_status_type mx_status;

	marccd = NULL;

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

	marccd->current_command = MXF_MARCCD_CMD_END_AUTOMATION;

	mx_status = mxd_marccd_command( marccd,
				"end_automation", MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait a moment for the remote end to respond to what we have done. */

	mx_msleep(100);

	/* Shutdown the connection. */

	if ( marccd->marccd_socket != NULL ) {

		mx_status = mx_socket_close( marccd->marccd_socket );

	} else {
		mx_status = MX_SUCCESSFUL_RESULT;

		status_to_marccd = close( marccd->fd_to_marccd );

		if ( status_to_marccd != 0 ) {
			saved_errno = errno;

			mx_status = mx_error( MXE_IPC_IO_ERROR, fname,
		"An error occurred while closing the pipe to MarCCD.  "
		"Error number = %d, error message = '%s'",
				saved_errno, strerror( saved_errno ) );
		}

		status_from_marccd = close( marccd->fd_from_marccd );

		if ( status_from_marccd != 0 ) {
			saved_errno = errno;

			mx_status = mx_error( MXE_IPC_IO_ERROR, fname,
		"An error occurred while closing the pipe from MarCCD.  "
		"Error number = %d, error message = '%s'",
				saved_errno, strerror( saved_errno ) );
		}
	}

	marccd->marccd_socket = NULL;

	marccd->fd_from_marccd = -1;
	marccd->fd_to_marccd = -1;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_trigger()";

	MX_MARCCD *marccd;
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time;
	MX_CLOCK_TICK clock_ticks_to_wait, start_time;
	mx_status_type mx_status;

	marccd = NULL;

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

	/* If needed, compute the finish time. */

	if ( 0 ) {
		marccd->use_finish_time = FALSE;
	} else {
		marccd->use_finish_time = TRUE;

		clock_ticks_to_wait = 
			mx_convert_seconds_to_clock_ticks( exposure_time );

		start_time = mx_current_clock_tick();

		marccd->finish_time = mx_add_clock_ticks( start_time,
							clock_ticks_to_wait );
	}

	/* Send the start command. */

	marccd->current_command = MXF_MARCCD_CMD_START;

	mx_status = mxd_marccd_command( marccd, "start", MXD_MARCCD_DEBUG );

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

	MX_MARCCD *marccd;
	mx_status_type mx_status;

	marccd = NULL;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	marccd->finish_time = mx_current_clock_tick();

	marccd->current_command = MXF_MARCCD_CMD_ABORT;

	mx_status = mxd_marccd_command( marccd, "abort", MXD_MARCCD_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_get_extended_status()";

	MX_MARCCD *marccd;
	MX_CLOCK_TICK current_time;
	int comparison;
	mx_status_type mx_status;

	marccd = NULL;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

#if 0
	/* FIXME: Use mutex here? */

	mx_status = mxd_marccd_check_for_responses( ad,
						marccd, MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	current_time = mx_current_clock_tick();

	comparison = mx_compare_clock_ticks(current_time, marccd->finish_time);

	if ( comparison < 0 ) {
		ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
		ad->last_frame_number = -1;
	} else {
		if ( ad->status & MXSF_AD_ACQUISITION_IN_PROGRESS ) {

			/* If we just finished an acquisition cycle,
			 * then increment the total number of frames.
			 */

			(ad->total_num_frames)++;
		}

		ad->last_frame_number = 0;

		marccd->use_finish_time = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_marccd_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_readout_frame()";

	MX_MARCCD *marccd;
	mx_status_type mx_status;

	marccd = NULL;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	marccd->current_command = MXF_MARCCD_CMD_READOUT;

	mx_status = mxd_marccd_command( marccd, "readout,0",
				MXF_MARCCD_FORCE_READ | MXD_MARCCD_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_correct_frame()";

	MX_MARCCD *marccd;
	mx_status_type mx_status;

	marccd = NULL;

	mx_status = mxd_marccd_get_pointers( ad, &marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	marccd->current_command = MXF_MARCCD_CMD_CORRECT;
	marccd->next_state = MXF_MARCCD_STATE_CORRECT;

	mx_status = mxd_marccd_command( marccd, "correct", MXD_MARCCD_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_get_parameter()";

	MX_MARCCD *marccd;
	mx_status_type mx_status;

	marccd = NULL;

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
		mx_status = mxd_marccd_command( marccd, "get_size",
							MXD_MARCCD_DEBUG );
		break;

	case MXLV_AD_BINSIZE:
		mx_status = mxd_marccd_command( marccd, "get_bin",
							MXD_MARCCD_DEBUG );
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		ad->image_format = MXT_IMAGE_FILE_TIFF;

		mx_status = mx_image_get_format_name_from_type(
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

	MX_MARCCD *marccd;
	MX_SEQUENCE_PARAMETERS *sp;
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

	sp = &(ad->sequence_parameters);

	switch( ad->parameter_type ) {
	case MXLV_AD_SEQUENCE_TYPE:
		if ( sp->sequence_type != MXT_SQ_ONE_SHOT ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Sequence type %ld is not supported for MarCCD "
		      "detector '%s'.  Only one-shot sequences are supported.",
				sp->sequence_type, ad->record->name );
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

/* --- */

MX_EXPORT mx_status_type
mxd_marccd_command( MX_MARCCD *marccd,
			char *command,
			unsigned long flags )
{
	static const char fname[] = "mxd_marccd_command()";

	size_t num_bytes_left;
	char *ptr;
	char newline = '\n';
	int saved_errno;
	long num_bytes_written;
	mx_status_type mx_status;

	if ( marccd == (MX_MARCCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MARCCD pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command string pointer passed was NULL." );
	}

	if ( flags ) {
		MX_DEBUG(-2,("%s: sent '%s' to '%s'",
			fname, command, marccd->record->name));
	}

	if ( marccd->marccd_socket != NULL ) {

		mx_status = mx_socket_putline( marccd->marccd_socket,
						command, MX_CR_LF );
	} else {
		if ( marccd->fd_to_marccd < 0 ) {
			return mx_error( MXE_IPC_IO_ERROR, fname,
			"The connection to MarCCD has not been "
			"correctly initialized, fd_to_marccd = %d",
				marccd->fd_to_marccd );
		}

		ptr            = command;
		num_bytes_left = strlen( command );

		while ( num_bytes_left > 0 ) {
			MX_DEBUG(-2,("%s: num_bytes_left = %ld",
				fname, (long) num_bytes_left));

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

			MX_DEBUG(-2,("%s: num_bytes_written = %ld",
				fname, (long) num_bytes_written));

			ptr            += num_bytes_written;
			num_bytes_left -= num_bytes_written;
		}

		/* Write a trailing newline. */

		MX_DEBUG(-2,("%s: about to write a trailing newline.",fname));

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
	}

	MX_DEBUG(-2,("%s: finished sending command '%s'.",
			fname, command));

	return MX_SUCCESSFUL_RESULT;
}

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

static const char *
mxd_marccd_get_state_name( int marccd_state )
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
mxd_marccd_handle_state_value( MX_AREA_DETECTOR *ad,
				MX_MARCCD *marccd,
				int current_state )
{
	static const char fname[] = "mxd_marccd_handle_state_value()";

	int old_state, next_state, current_command;

	old_state = marccd->current_state;
	next_state = marccd->next_state;

	marccd->current_state = current_state;

	current_command = marccd->current_command;

	if ( old_state != current_state ) {
		MX_DEBUG(-2,("%s: MarCCD NEW STATE = %d '%s', OLD STATE = '%s'",
			fname, current_state,
			mxd_marccd_get_state_name(current_state),
			mxd_marccd_get_state_name(old_state) ));
	}

	MX_DEBUG(-2,("%s: MarCCD command = %d '%s', state = %d '%s'", fname,
	  current_command, mxd_marccd_get_command_name(current_command),
	  current_state, mxd_marccd_get_state_name(current_state) ));

	/* The interpretation of the state flag depends on the command that
	 * is currently supposed to be executing.
	 */

	ad->status = 0;

	switch( current_command ) {
	case MXF_MARCCD_CMD_NONE:
		switch( current_state ) {
		case MXF_MARCCD_STATE_IDLE:
			MX_DEBUG(-2,("%s: MarCCD is idle.", fname));

			ad->status = 0;
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
			MX_DEBUG(-2,("%s: Waiting for start command to finish.",
				fname ));

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
			MX_DEBUG(-2,("%s: Readout in progress.", fname));

			ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
			break;
		case MXF_MARCCD_STATE_IDLE:
			MX_DEBUG(-2,("%s: Readout is complete.", fname));

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
			if ( next_state == MXF_MARCCD_STATE_CORRECT ) {
				MX_DEBUG(-2,
				("%s: Waiting for correct command to finish.",
					fname));

				ad->status = MXSF_AD_CORRECTION_IN_PROGRESS;
			} else {
				MX_DEBUG(-2,
				("%s: Frame correction has completed.", fname));

				ad->status = 0;
			}
			break;
		case MXF_MARCCD_STATE_CORRECT:
			MX_DEBUG(-2,
			("%s: Waiting for correct command to finish.", fname));

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
			if ( next_state == MXF_MARCCD_STATE_WRITING ) {
				MX_DEBUG(-2,
				("%s: Waiting for frame write to complete.",
					fname));

				ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
			} else {
				MX_DEBUG(-2,
				("%s: Frame write has completed.", fname));

				ad->status = 0;
			}
			break;
		case MXF_MARCCD_STATE_WRITING:
			MX_DEBUG(-2,
			("%s: Waiting for frame write to complete.", fname));

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
	default:
		MX_MARCCD_UNHANDLED_COMMAND_MESSAGE;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_marccd_handle_response( MX_AREA_DETECTOR *ad,
				MX_MARCCD *marccd,
				unsigned long flags )
{
	static const char fname[] = "mxd_marccd_handle_response()";

	char response[100];
	size_t length;
	int i, num_items, saved_errno, marccd_state;
	long num_bytes_read;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.",fname));

	if ( marccd == (MX_MARCCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MARCCD pointer passed was NULL." );
	}

	/* Read the response line from MarCCD. */

	if ( marccd->marccd_socket != NULL ) {

		mx_status = mx_socket_getline( marccd->marccd_socket,
						response, sizeof(response),
						MX_CR_LF );
	} else {
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
	}

	/* Delete any trailing newline if present. */

	length = strlen( response );

	if ( response[length-1] == '\n' )
		response[length-1] = '\0';

	if ( flags ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, marccd->record->name ));
	}

	if ( strncmp( response, "is_size", 7 ) == 0 ) {

		num_items = sscanf( response, "is_size,%ld,%ld",
				&(ad->framesize[0]), &(ad->framesize[1]) );

		if ( num_items != 2 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Cannot find the MarCCD data frame size in the "
				"response '%s'.", response );
		}

		MX_DEBUG(-2,
		("%s: ad->framesize[0] = %ld, ad->framesize[1] = %ld",
			fname, ad->framesize[0], ad->framesize[0]));

	} else
	if ( strncmp( response, "is_bin", 6 ) == 0 ) {

		num_items = sscanf( response, "is_bin,%ld,%ld",
				&(ad->binsize[0]), &(ad->binsize[1]) );

		if ( num_items != 2 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Cannot find the MarCCD bin size in the "
				"response '%s'.", response );
		}

		MX_DEBUG(-2,("%s: ad->binsize[0] = %ld, ad->binsize[1] = %ld",
			fname, ad->binsize[0], ad->binsize[1]));

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

		mx_status = mxd_marccd_handle_state_value( ad,
						marccd, marccd_state );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable response '%s' from MarCCD '%s'.",
			response, ad->record->name );
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

	if ( marccd->marccd_socket != NULL ) {
		fd = marccd->marccd_socket->socket_fd;
	} else {
		fd = marccd->fd_from_marccd;
	}

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
		MX_DEBUG(-2,("%s: select_status = %d", fname, select_status));

		return TRUE;
	} else {
		return FALSE;
	}
}

