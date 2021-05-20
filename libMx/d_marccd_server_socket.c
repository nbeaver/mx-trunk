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
 * Copyright 2008-2010, 2013, 2015-2016, 2018-2019, 2021
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"

#define MXD_MARCCD_DEBUG			TRUE

#define MXD_MARCCD_DEBUG_FRAME_CORRECTION	FALSE

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
	mxd_marccd_server_socket_initialize_driver,
	mxd_marccd_server_socket_create_record_structures,
	mxd_marccd_server_socket_finish_record_initialization,
	NULL,
	NULL,
	mxd_marccd_server_socket_open,
	mxd_marccd_server_socket_close,
	NULL,
	mxd_marccd_server_socket_resynchronize
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_marccd_server_socket_ad_function_list = {
	NULL,
	mxd_marccd_server_socket_trigger,
	NULL,
	mxd_marccd_server_socket_stop,
	mxd_marccd_server_socket_stop,
	NULL,
	NULL,
	NULL,
	mxd_marccd_server_socket_get_extended_status,
	mxd_marccd_server_socket_readout_frame,
	mxd_marccd_server_socket_correct_frame,
	mxd_marccd_server_socket_transfer_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_marccd_server_socket_get_parameter,
	mxd_marccd_server_socket_set_parameter,
	mxd_marccd_server_socket_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_marccd_server_socket_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_MARCCD_SERVER_SOCKET_STANDARD_FIELDS
};

long mxd_marccd_server_socket_num_record_fields
		= sizeof( mxd_marccd_server_socket_rf_defaults )
		/ sizeof( mxd_marccd_server_socket_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_marccd_server_socket_rfield_def_ptr
			= &mxd_marccd_server_socket_rf_defaults[0];

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

static mx_status_type
mxd_marccd_server_socket_writefile( MX_AREA_DETECTOR *ad,
			MX_MARCCD_SERVER_SOCKET *mss,
			mx_bool_type write_corrected_frame )
{
	static const char fname[] = "mxd_marccd_server_socket_writefile()";

	char command[(2*MXU_FILENAME_LENGTH)+20];
	char response[40];
	size_t dirname_length;
	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}
	if ( mss == (MX_MARCCD_SERVER_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MARCCD_SERVER_SOCKET pointer passed was NULL." );
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

	/* Now write the corrected frame. */

	dirname_length = strlen( ad->datafile_directory );

	if ( write_corrected_frame ) {
		if ( dirname_length > 0 ) {
			snprintf( command, sizeof(command),
				"writefile,%s/%s,1",
					ad->datafile_directory,
					ad->datafile_name );
		} else {
			snprintf( command, sizeof(command),
				"writefile,%s,1", ad->datafile_name );
		}
	} else {
		if ( dirname_length > 0 ) {
			snprintf( command, sizeof(command),
				"writefile,%s/%s,0",
					ad->datafile_directory,
					ad->datafile_name );
		} else {
			snprintf( command, sizeof(command),
				"writefile,%s,0", ad->datafile_name );
		}
	}

	/* Send the writefile command.  This call synchronously waits
	 * until the file is written.
	 */

	mx_status = mxd_marccd_server_socket_command( mss, command,
						response, sizeof(response),
						MXD_MARCCD_DEBUG );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_marccd_server_socket_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_marccd_server_socket_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_MARCCD_SERVER_SOCKET *mss = NULL;

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
			&mxd_marccd_server_socket_ad_function_list;

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

	MX_AREA_DETECTOR *ad = NULL;
	MX_MARCCD_SERVER_SOCKET *mss = NULL;
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

#if 1
	mss->remote_mode_version = 1;
#endif

	mss->finish_time = mx_set_clock_tick_to_maximum();

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

	ad->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;

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
		(int) mss->marccd_socket->socket_fd ));
#endif

	/* Get the current framesize. */

	mx_status = mx_area_detector_get_framesize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current binsize. */

	mx_status = mx_area_detector_get_binsize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the maximum framesize and bytes per frame. */

	ad->maximum_framesize[0] = ad->framesize[0] * ad->binsize[0];
	ad->maximum_framesize[1] = ad->framesize[1] * ad->binsize[1];

	ad->bytes_per_frame =
	  mx_round( ad->framesize[0] * ad->framesize[1] * ad->bytes_per_pixel );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_marccd_server_socket_close()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_MARCCD_SERVER_SOCKET *mss = NULL;
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
mxd_marccd_server_socket_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_marccd_server_socket_resynchronize()";

	MX_AREA_DETECTOR *ad;
	MX_MARCCD_SERVER_SOCKET *mss = NULL;
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

	/* Discard any unread messages from the MarCCD server socket. */

	mx_status = mx_socket_discard_unread_input( mss->marccd_socket );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_trigger()";

	MX_MARCCD_SERVER_SOCKET *mss = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	double exposure_time;
	MX_CLOCK_TICK clock_ticks_to_wait, start_time;
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
						NULL, 0, MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;

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

	MX_MARCCD_SERVER_SOCKET *mss = NULL;
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
						NULL, 0, MXD_MARCCD_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_marccd_server_socket_get_extended_status()";

	MX_MARCCD_SERVER_SOCKET *mss = NULL;
	char response[80];
	unsigned long detector_state;
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mxd_marccd_server_socket_command( mss, "get_state",
						response, sizeof(response),
						MXD_MARCCD_DEBUG );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: 'get_state' response = '%s'", fname, response));

	mss->detector_status = mx_hex_string_to_unsigned_long( response );

	/* The low order 4 bits of mss->detector_status for remote mode
	 * version 1 are the same as the detector_state value returned
	 * for remote mode version 0.  Note that the 'detector_state'
	 * value is not a bitmask.  Instead, 'detector_state' is just
	 * a number.  Only 0, 7, and 8 are visible for clients.
	 */

	detector_state = (mss->detector_status) & 0xf;

	ad->status = 0;

	switch( detector_state ) {
	case 0:		/* IDLE */
		/* In MX, we indicate idle by not setting any of the bits. */

		if ( mss->remote_mode_version == 0 ) {
			return MX_SUCCESSFUL_RESULT;
		}
		break;
	case 7:		/* ERROR */

		ad->status = MXSF_AD_ERROR;

		return mx_error( MXE_PROTOCOL_ERROR, fname,
			"The most recent command sent to area detector '%s' "
			"was not understood.", ad->record->name );
		break;
	case 8:		/* BUSY */

		ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
		return MX_SUCCESSFUL_RESULT;
	}

	/* Only more recent remote mode versions are processed here. */

	if ( mss->remote_mode_version >= 1 ) {

		if ( mss->detector_status & 0x30 ) {
			ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
		} else
		if ( mss->detector_status & 0x40 ) {
			ad->status = MXSF_AD_ERROR;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"An error occurred for area detector '%s' "
			"during image acquisition.",  ad->record->name );
		} else
		if ( mss->detector_status & 0x300 ) {
			ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
		} else
		if ( mss->detector_status & 0x400 ) {
			ad->status = MXSF_AD_ERROR;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"An error occurred for area detector '%s' "
			"during image reading.",  ad->record->name );
		} else
		if ( mss->detector_status & 0x3000 ) {
			ad->status = MXSF_AD_CORRECTION_IN_PROGRESS;
		} else
		if ( mss->detector_status & 0x4000 ) {
			ad->status = MXSF_AD_ERROR;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"An error occurred for area detector '%s' "
			"during image correction.",  ad->record->name );
		} else
		if ( mss->detector_status & 0x30000 ) {
			ad->status = MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
		} else
		if ( mss->detector_status & 0x40000 ) {
			ad->status = MXSF_AD_ERROR;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"An error occurred for area detector '%s' "
			"during image writing.",  ad->record->name );
		} else
		if ( mss->detector_status & 0x300000 ) {
			ad->status = MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
		} else
		if ( mss->detector_status & 0x400000 ) {
			ad->status = MXSF_AD_ERROR;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"An error occurred for area detector '%s' "
			"during image dezingering.",  ad->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_readout_frame()";

	MX_MARCCD_SERVER_SOCKET *mss = NULL;
	char command[40];
	char response[40];
	int marccd_state, num_items;
	unsigned long flags, mask;
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

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

	/* If the datafile_name field is empty, then we assume that no
	 * files have been acquired yet and attempt to initialize the
	 * datafile_number based on the contents of the remote directory.
	 */

	if ( strlen( ad->datafile_name ) == 0 ) {
		mx_status = mx_area_detector_initialize_remote_datafile_number(
				ad->record, mss->remote_filename_prefix,
				mss->local_filename_prefix );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Now tell the detector to read out an image.  This call
	 * synchronously waits until the readout is complete.
	 */

	strlcpy( command, "readout,0", sizeof(command) );

	MX_DEBUG(-2,("%s: Before 'readout' command.", fname));

	mx_status = mxd_marccd_server_socket_command( mss, command,
						response, sizeof(response),
						MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: After 'readout' command.", fname));

	num_items = sscanf( response, "%d", &marccd_state );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not find the current MarCCD state in the response '%s' "
		"to the command '%s' sent to MarCCD detector '%s'.",
			response, command, ad->record->name );
	}

	switch( marccd_state ) {
	case 0:			/* State: idle */
		break;
	case 7:			/* State: error */

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to readout an image frame for MarCCD "
		"detector '%s' failed with an error.",
			ad->record->name );
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	    "Received unexpected MarCCD state %d from MarCCD detector '%s'.",
			marccd_state, ad->record->name );
		break;
	}

	/* Check to see if we need to save the image file now. */

	flags = ad->area_detector_flags;

	if ( flags & MXF_AD_SAVE_REMOTE_FRAME_AFTER_ACQUISITION ) {

		mask = MXFT_AD_FLAT_FIELD_FRAME
				| MXFT_AD_GEOMETRICAL_CORRECTION;

		/* We save here if no correction is to be performed. */

		if ( ( mask & ad->correction_flags ) == 0 ) {
			mx_status =
			  mxd_marccd_server_socket_writefile( ad, mss, FALSE );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_correct_frame()";

	MX_MARCCD_SERVER_SOCKET *mss = NULL;
	char command[40];
	char response[40];
	int marccd_state, num_items;
	unsigned long mask, flags;
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MARCCD_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Return if there is no correction to be performed. */

	mask = MXFT_AD_FLAT_FIELD_FRAME | MXFT_AD_GEOMETRICAL_CORRECTION;

	if ( ( mask & ad->correction_flags ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Send the correction command.  This call synchronously waits
	 * until the correction is complete.
	 */

	strlcpy( command, "correct", sizeof(command) );

	mx_status = mxd_marccd_server_socket_command( mss, command,
						response, sizeof(response),
						MXD_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d", &marccd_state );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not find the current MarCCD state in the response '%s' "
		"to the command '%s' sent to MarCCD detector '%s'.",
			response, command, ad->record->name );
	}

	switch( marccd_state ) {
	case 0:			/* State: idle */
		break;
	case 7:			/* State: error */

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to correct an image frame for MarCCD "
		"detector '%s' failed with an error.",
			ad->record->name );
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	    "Received unexpected MarCCD state %d from MarCCD detector '%s'.",
			marccd_state, ad->record->name );
		break;
	}

	/* Check to see if we need to save the image file now. */

	flags = ad->area_detector_flags;

	if ( flags & MXF_AD_SAVE_REMOTE_FRAME_AFTER_ACQUISITION ) {

		/* We save here if there _is_ a correction to be performed. */

		if ( ( mask & ad->correction_flags ) != 0 ) {
			mx_status =
			  mxd_marccd_server_socket_writefile( ad, mss, TRUE );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_transfer_frame()";

	MX_MARCCD_SERVER_SOCKET *mss = NULL;
	size_t dirname_length;
	char remote_marccd_filename[(2*MXU_FILENAME_LENGTH) + 20];
	char local_marccd_filename[(2*MXU_FILENAME_LENGTH) + 20];
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

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
		snprintf( remote_marccd_filename,
			sizeof(remote_marccd_filename),
			"%s/%s", ad->datafile_directory, ad->datafile_name );
	} else {
		snprintf( remote_marccd_filename,
			sizeof(remote_marccd_filename),
			"%s", ad->datafile_name );
	}

#if MXD_MARCCD_DEBUG
	MX_DEBUG(-2,("%s: Remote MarCCD filename = '%s'",
		fname, remote_marccd_filename));
#endif
	if ( strlen( remote_marccd_filename ) == 0 ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The MarCCD 'datafile_directory' and 'datafile_name' "
		"fields have not been initialized for detector '%s', so "
		"we cannot read in the most recently acquired MarCCD image.",
			ad->record->name );
	}

	/* Often, this driver is not running on the same computer as 
	 * the one running the 'marccd' program.  If so, then we can only
	 * load the MarCCD file if it has been exported to us via somthing
	 * like NFS or SMB.  If so, the local filename is probably different
	 * from the filename on the 'marccd' server computer.  We handle
	 * this by stripping off the remote prefix and then prepending that
	 * with the local prefix.
	 */

	mx_status = mx_change_filename_prefix( remote_marccd_filename,
						mss->remote_filename_prefix,
						mss->local_filename_prefix,
						local_marccd_filename,
						sizeof(local_marccd_filename) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now we can read in the MarCCD file. */

	mx_status = mx_image_read_marccd_file( &(ad->image_frame),
						local_marccd_filename );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_marccd_server_socket_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_get_parameter()";

	MX_MARCCD_SERVER_SOCKET *mss = NULL;
	char response[40];
	int num_items;
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

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Sometimes, marccd_server_socket sends us the string '0'
		 * instead of the size that we asked for.  If it does this,
		 * then ask a second time.
		 */

		if ( strcmp( response, "0" ) == 0 ) {
			mx_status = mxd_marccd_server_socket_command(
						mss, "get_size",
						response, sizeof(response),
						MXD_MARCCD_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Parse the response that we got back. */

		num_items = sscanf( response, "%lu,%lu",
				&(ad->framesize[0]), &(ad->framesize[1]) );

		if ( num_items != 2 ) {
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"A 'get_size' command sent to '%s' did not return "
			"a recognizable response.  Response = '%s'",
				ad->record->name, response );
		}

		ad->bytes_per_frame =
	    mx_round(ad->framesize[0] * ad->framesize[1] * ad->bytes_per_pixel);

		break;

	case MXLV_AD_BINSIZE:
		mx_status = mxd_marccd_server_socket_command( mss, "get_bin",
						response, sizeof(response),
						MXD_MARCCD_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Sometimes, marccd_server_socket sends us the string '0'
		 * instead of the binsize that we asked for.  If it does this,
		 * then ask a second time.
		 */

		if ( strcmp( response, "0" ) == 0 ) {
			mx_status = mxd_marccd_server_socket_command(
						mss, "get_bin",
						response, sizeof(response),
						MXD_MARCCD_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Parse the response that we got back. */

		num_items = sscanf( response, "%lu,%lu",
				&(ad->binsize[0]), &(ad->binsize[1]) );

		if ( num_items != 2 ) {
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"A 'get_bin' command sent to '%s' did not return "
			"a recognizable response.  Response = '%s'",
				ad->record->name, response );
		}
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
mxd_marccd_server_socket_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_marccd_server_socket_set_parameter()";

	MX_MARCCD_SERVER_SOCKET *mss = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	char command[40];
	/* char response[40]; */
	mx_status_type mx_status;

	static long allowed_binsize[] = { 1, 2, 4, 8 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

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
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command),
			"set_bin,%lu,%lu",
			ad->binsize[0], ad->binsize[1] );

		mx_status = mxd_marccd_server_socket_command( mss, command,
						NULL, 0, MXD_MARCCD_DEBUG );
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
		if ( ad->trigger_mode != MXF_DEV_INTERNAL_TRIGGER ) {
			mx_status = mx_error( MXE_UNSUPPORTED, fname,
			"Trigger mode %ld is not supported for MarCCD detector "
		    "'%s'.  Only internal trigger mode (mode 1) is supported.",
				ad->trigger_mode, ad->record->name );

			ad->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;
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
mxd_marccd_server_socket_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_marccd_server_socket_measure_correction()";

	MX_MARCCD_SERVER_SOCKET *mss = NULL;
	/* char saved_datafile_name[MXU_FILENAME_LENGTH+1]; */
	mx_status_type mx_status;

	mx_status = mxd_marccd_server_socket_get_pointers( ad, &mss, fname );

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
					sizeof(response_terminators) - 1, 0 );

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

