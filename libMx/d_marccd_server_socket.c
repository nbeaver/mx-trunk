/*
 * Name:    d_marccd_server_socket.c
 *
 * Purpose: MX driver for MarCCD remote control using the Mar-USA provided
 *          'marccd_server_socket' program.
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
#include "mx_ascii.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_marccd_server_socket.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_marccd_server_socket_record_function_list = {
	mxd_marccd_server_socket_initialize_type,
	mxd_marccd_server_socket_create_record_structures,
	mxd_marccd_server_socket_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_marccd_server_socket_open,
	mxd_marccd_server_socket_close
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_marccd_server_socket_function_list = {
	NULL,
	mxd_marccd_server_socket_trigger,
	mxd_marccd_server_socket_stop,
	mxd_marccd_server_socket_stop,
	NULL,
	NULL,
	NULL,
	mxd_marccd_server_socket_get_extended_status,
	mxd_marccd_server_socket_readout_frame,
	mxd_marccd_server_socket_correct_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_marccd_server_socket_get_parameter,
	mxd_marccd_server_socket_set_parameter,
	mx_area_detector_default_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_marccd_server_socket_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_MARCCD_SERVER_SOCKET_STANDARD_FIELDS
};

long mxd_marccd_server_socket_num_record_fields
		= sizeof( mxd_marccd_server_socket_record_field_defaults )
		/ sizeof( mxd_marccd_server_socket_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_marccd_server_socket_rfield_def_ptr
			= &mxd_marccd_server_socket_record_field_defaults[0];

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_marccd_server_socket_get_pointers( MX_AREA_DETECTOR *ad,
			MX_MARCCD_SERVER_SOCKET **mss,
			const char *calling_fname )
{
	static const char fname[] = "mxd_marccd_server_socket_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mss == (MX_MARCCD_SERVER_SOCKET **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MARCCD_SERVER_SOCKET pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mss = (MX_MARCCD_SERVER_SOCKET *) ad->record->record_type_struct;

	if ( *mss == (MX_MARCCD_SERVER_SOCKET *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MARCCD_SERVER_SOCKET pointer for record '%s' "
		"passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_marccd_server_socket_initialize_type( long record_type )
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
mxd_marccd_server_socket_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_marccd_server_socket_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_MARCCD_SERVER_SOCKET *mss;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	mss = (MX_MARCCD_SERVER_SOCKET *)
			malloc( sizeof(MX_MARCCD_SERVER_SOCKET) );

	if ( mss == (MX_MARCCD_SERVER_SOCKET *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for an MX_MARCCD_SERVER_SOCKET structure.");
	}

	record->record_class_struct = ad;
	record->record_type_struct = mss;
	record->class_specific_function_list = 
			&mxd_marccd_server_socket_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	mss->record = record;

	mss->marccd_socket = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_finish_record_initialization( MX_RECORD *record )
{
	return mx_area_detector_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_marccd_server_socket_open()";

	MX_AREA_DETECTOR *ad;
	MX_MARCCD_SERVER_SOCKET *mss;
	unsigned long mar_framesize[2], mar_binsize[2];
	char response[40];
	int num_items;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	ad->bytes_per_pixel = 2;
	ad->bits_per_pixel = 16;

	ad->image_format = MXT_IMAGE_FORMAT_GREY16;

	mx_status = mx_image_get_format_name_from_type( ad->image_format,
						ad->image_format_name,
						MXU_IMAGE_FORMAT_NAME_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;

	/*---*/

	mx_status = mx_tcp_socket_open_as_client( &(mss->marccd_socket),
					mss->marccd_host,
					mss->marccd_port, 0,
					MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s: connected to remote server '%s', port %lu, socket %d",
		fname, mss->marccd_host, mss->marccd_port,
		mss->marccd_socket->socket_fd ));
#endif

	/* Get the current framesize. */

	mx_status = mxd_marccd_server_socket_command( mss, "get_size",
						response, sizeof(response),
						MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu,%lu",
			&(mar_framesize[0]), &(mar_framesize[1]) );

	if ( num_items != 2 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"A 'get_size' command send to '%s' did not return "
		"a recognizable response.  Response = '%s'",
			record->name, response );
	}

	/* Get the current binsize. */

	mx_status = mxd_marccd_server_socket_command( mss, "get_bin",
						response, sizeof(response),
						MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu,%lu",
			&(mar_binsize[0]), &(mar_binsize[1]) );

	if ( num_items != 2 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"A 'get_size' command send to '%s' did not return "
		"a recognizable response.  Response = '%s'",
			record->name, response );
	}

	/* Set the local copy of the area detector frame and bin sizes. */

	ad->maximum_framesize[0] = mar_framesize[0] * mar_binsize[0];
	ad->maximum_framesize[1] = mar_framesize[1] * mar_binsize[1];

	ad->framesize[0] = mar_framesize[0];
	ad->framesize[1] = mar_framesize[1];

	ad->binsize[0] = mar_binsize[0];
	ad->binsize[1] = mar_binsize[1];

	ad->bytes_per_frame =
	  mx_round( ad->framesize[0] * ad->framesize[1] * ad->bytes_per_pixel );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_marccd_server_socket_close()";

	MX_AREA_DETECTOR *ad;
	MX_MARCCD_SERVER_SOCKET *mss;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.", fname, record->name));

	/* Shutdown the connection. */

	mx_status = mx_socket_close( mss->marccd_socket );

	mss->marccd_socket = NULL;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_trigger()";

	MX_MARCCD_SERVER_SOCKET *mss;
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time;
	MX_CLOCK_TICK clock_ticks_to_wait, start_time;
	char response[40];
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

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

	/* Compute the finish time for the measurement. */

	clock_ticks_to_wait = 
			mx_convert_seconds_to_clock_ticks( exposure_time );

	start_time = mx_current_clock_tick();

	mss->finish_time = mx_add_clock_ticks( start_time,
						clock_ticks_to_wait );

	/* Send the start command. */

	mx_status = mxd_marccd_server_socket_command( mss, "start",
						response, sizeof(response),
						MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_stop()";

	MX_MARCCD_SERVER_SOCKET *mss;
	char response[40];
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mss->finish_time = mx_current_clock_tick();

	mx_status = mxd_marccd_server_socket_command( mss, "abort",
						response, sizeof(response),
						MXD_MARCCD_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_marccd_server_socket_get_extended_status()";

	MX_MARCCD_SERVER_SOCKET *mss;
	MX_CLOCK_TICK current_time;
	int comparison;
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	current_time = mx_current_clock_tick();

	comparison = mx_compare_clock_ticks( current_time, mss->finish_time );

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

		ad->status &= (~MXSF_AD_ACQUISITION_IN_PROGRESS);
	}

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,
    ("%s: current_time = (%lu,%lu), finish_time = (%lu,%lu), comparison = %d",
		fname, current_time.high_order, current_time.low_order,
		mss->finish_time.high_order, mss->finish_time.low_order,
		comparison ));

	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
		fname, ad->last_frame_number, ad->total_num_frames,
		ad->status));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_readout_frame()";

	MX_MARCCD_SERVER_SOCKET *mss;
	char response[40];
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mxd_marccd_server_socket_command( mss, "readout,0",
						response, sizeof(response),
						MXD_MARCCD_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_correct_frame()";

	MX_MARCCD_SERVER_SOCKET *mss;
	char response[40];
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mxd_marccd_server_socket_command( mss, "correct",
						response, sizeof(response),
						MXD_MARCCD_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_get_parameter()";

	MX_MARCCD_SERVER_SOCKET *mss;
	char response[40];
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

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
		mx_status = mxd_marccd_server_socket_command( mss, "get_size",
						response, sizeof(response),
						MXD_MARCCD_DEBUG );
		break;

	case MXLV_AD_BINSIZE:
		mx_status = mxd_marccd_server_socket_command( mss, "get_bin",
						response, sizeof(response),
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
mxd_marccd_server_socket_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_set_parameter()";

	MX_MARCCD_SERVER_SOCKET *mss;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

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
mxd_marccd_server_socket_command( MX_MARCCD_SERVER_SOCKET *mss,
			char *command,
			char *response,
			size_t response_buffer_length,
			unsigned long flags )
{
	static const char fname[] = "mxd_marccd_server_socket_command()";

	char response_terminators[3];
	size_t length;
	mx_status_type mx_status;

	if ( mss == (MX_MARCCD_SERVER_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MARCCD_SERVER_SOCKET pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command string pointer passed was NULL." );
	}

	if ( flags ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, mss->record->name));
	}

	mx_status = mx_socket_putline( mss->marccd_socket, command, "\n" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (response != NULL) && (response_buffer_length > 0) ) {

		/* Responses by the MarCCD server socket are terminated
		 * by a line-feed character followed by a null character.
		 */

		response_terminators[0] = '\n';
		response_terminators[1] = '\0';
		response_terminators[2] = '\0';

		mx_status = mx_socket_receive( mss->marccd_socket,
					response, response_buffer_length, NULL,
					response_terminators,
					sizeof(response_terminators) - 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Strip off the trailing newline. */

		length = strlen(response);

		if ( length > 0 ) {
			if ( response[length-1] == '\n' ) {
				response[length-1] = '\0';
			}
		}

		if ( flags ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, response, mss->record->name));
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

