/*
 * Name:    d_merlin_medipix.c
 *
 * Purpose: MX area detector driver for the Merlin Medipix series of
 *          detectors from Quantum Detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015-2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MERLIN_MEDIPIX_DEBUG			FALSE

#define MXD_MERLIN_MEDIPIX_DEBUG_MONITOR_THREAD		TRUE

#define MXD_MERLIN_MEDIPIX_DEBUG_WAIT_FOR_MESSAGES	TRUE

#define MXD_MERLIN_MEDIPIX_DEBUG_FRAME_ADDRESSES	TRUE

#define MXD_MERLIN_MEDIPIX_DEBUG_DETECTOR_STATUS	TRUE

#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__)
#  define __USE_XOPEN
#endif

#include "mx_util.h"
#include "mx_time.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_atomic.h"
#include "mx_bit.h"
#include "mx_socket.h"
#include "mx_array.h"
#include "mx_thread.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_merlin_medipix.h"

/*---*/

/* The following define is used to specifying how much of the
 * beginning of an MPX message to read while figuring out
 * the message type and things like the frame number that
 * are used in the copying of data.
 */

#define MXU_MPX_HEADER_LENGTH			14
#define MXU_MPX_SEPARATOR_LENGTH		1
#define MXU_MPX_INITIAL_READ_LENGTH		12
#define MXU_MPX_MESSAGE_TYPE_STRING_LENGTH	4

/*---*/

/* Identifiers for the type of Merlin message we are receiving. */

#define MXT_MPX_UNKNOWN_MESSAGE		0
#define MXT_MPX_ACQUISITION_MESSAGE	1
#define MXT_MPX_IMAGE_MESSAGE		2

/*---*/

MX_RECORD_FUNCTION_LIST mxd_merlin_medipix_record_function_list = {
	mxd_merlin_medipix_initialize_driver,
	mxd_merlin_medipix_create_record_structures,
	mx_area_detector_finish_record_initialization,
	NULL,
	NULL,
	mxd_merlin_medipix_open,
};

MX_AREA_DETECTOR_FUNCTION_LIST
mxd_merlin_medipix_ad_function_list = {
	mxd_merlin_medipix_arm,
	mxd_merlin_medipix_trigger,
	NULL,
	mxd_merlin_medipix_stop,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_merlin_medipix_get_extended_status,
	mxd_merlin_medipix_readout_frame,
	mxd_merlin_medipix_correct_frame,
	mxd_merlin_medipix_transfer_frame,
	mxd_merlin_medipix_load_frame,
	mxd_merlin_medipix_save_frame,
	mxd_merlin_medipix_copy_frame,
	NULL,
	mxd_merlin_medipix_get_parameter,
	mxd_merlin_medipix_set_parameter,
	mxd_merlin_medipix_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_merlin_medipix_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_MERLIN_MEDIPIX_STANDARD_FIELDS
};

long mxd_merlin_medipix_num_record_fields
		= sizeof( mxd_merlin_medipix_rf_defaults )
		  / sizeof( mxd_merlin_medipix_rf_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_merlin_medipix_rfield_def_ptr
			= &mxd_merlin_medipix_rf_defaults[0];

/*---*/

static mx_status_type
mxd_merlin_medipix_get_pointers( MX_AREA_DETECTOR *ad,
			MX_MERLIN_MEDIPIX **merlin_medipix,
			const char *calling_fname )
{
	static const char fname[] = "mxd_merlin_medipix_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (merlin_medipix == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MERLIN_MEDIPIX pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*merlin_medipix = (MX_MERLIN_MEDIPIX *) ad->record->record_type_struct;

	if ( *merlin_medipix == (MX_MERLIN_MEDIPIX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_MERLIN_MEDIPIX pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

/* mxd_merlin_medipix_raw_wait_for_message_header() keeps reading from the
 * data socket until it finds a Merlin Medipix header.
 *
 * FIXME: This implementation uses single character I/O, which is probably
 *        _very_ inefficient.  It is done this way to meet a deadline for
 *        testing on an X-ray beamline.  This code should be replaced.
 */

static mx_status_type
mxd_merlin_medipix_raw_wait_for_message_header( MX_SOCKET *data_socket,
					unsigned long *message_body_length )
{
	static const char fname[] =
		"mxd_merlin_medipix_raw_wait_for_message_header()";

	static char mpx_label[] = "MPX,";

	unsigned long mpx_label_length;
	unsigned long num_matching_chars;
	size_t num_bytes_received;
	int i, num_items;
	char c;
	char ascii_message_body_length[12];
	mx_status_type mx_status;

	if ( data_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}
	if ( message_body_length == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The message_body_length pointer passed was NULL." );
	}

#if MXD_MERLIN_MEDIPIX_DEBUG_WAIT_FOR_MESSAGES
	MX_DEBUG(-2,("%s: WAIT started.", fname));
#endif

	mpx_label_length = strlen( mpx_label );

	num_matching_chars = 0;

	for ( i = 0; ; i++ ) {
		/* Try to read a character. */

		mx_status = mx_socket_receive( data_socket,
					&c, 1, NULL, NULL, 0, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( c == mpx_label[ num_matching_chars ] ) {
			num_matching_chars++;
		} else {
			num_matching_chars = 0;
		}

		if ( num_matching_chars >= mpx_label_length ) {
			/* We have found all of the characters in
			 * 'mpx_label', so we break out of the loop
			 * to read in the length of the rest of the
			 * message.
			 */

			break;
		}

		mx_usleep(1);
	}

#if MXD_MERLIN_MEDIPIX_DEBUG_WAIT_FOR_MESSAGES
	MX_DEBUG(-2,("%s: WAIT finished.", fname));
#endif

	/* The ASCII header length field is 10 bytes long and is followed
	 * by a comma ',' character.  We read in the next 11 bytes and
	 * extract the message body length from that.
	 */

	mx_status = mx_socket_receive( data_socket,
				ascii_message_body_length,
				sizeof(ascii_message_body_length) - 1,
		  		&num_bytes_received, NULL, 0, 0 );
			  		
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_bytes_received != (sizeof(ascii_message_body_length) - 1) ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
		"The ascii_message_body_length read in for socket %d is "
		"shorter (%lu) than the expected length of %lu.",
			data_socket->socket_fd,
			(unsigned long) num_bytes_received,
			(unsigned long) sizeof(ascii_message_body_length) - 1 );
	}

	num_items = sscanf( ascii_message_body_length,
			"%lu,", message_body_length );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The string '%s' read from socket %d, does not appear "
		"to contain a message body length.",
			ascii_message_body_length, data_socket->socket_fd );
	}

	/* The length returned by the Merlin server apparently includes the
	 * comma ',' character at the end of the length.  We are skipping
	 * over that comma, so we need to reduce the reported message body
	 * length by 1.
	 */

	*message_body_length = *message_body_length - 1;

#if MXD_MERLIN_MEDIPIX_DEBUG_WAIT_FOR_MESSAGES
	MX_DEBUG(-2,("%s: WAIT message body length = %lu",
			fname, *message_body_length ));
#endif

	/* We have found the message body length, so return now. */

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_merlin_medipix_setup_data_socket( MX_AREA_DETECTOR *ad,
					MX_MERLIN_MEDIPIX *merlin_medipix )
{
	static const char fname[] = "mxd_merlin_medipix_setup_data_socket()";

	MX_RECORD_FIELD *acquisition_header_field = NULL;
	MX_RECORD_FIELD *image_message_array_field = NULL;
	MX_SOCKET *data_socket = NULL;
	char message_type_string[MXU_MPX_MESSAGE_TYPE_STRING_LENGTH+1];
	char initial_image_header_bytes[200];
	unsigned long acquisition_message_body_length;
	unsigned long image_message_body_length;
	int argc;
	char **argv;
	size_t num_bytes_received;
	mx_status_type mx_status;

	/* We have no direct way of asking for the size of messages from
	 * the Merlin data socket or even the dimensions of a Merlin image
	 * frame.  So we have to tell the Merlin to send us a frame, so that
	 * we can infer this from the headers of those messages.
	 *
	 * Since this function's job is to figure out parameters that will be
	 * needed by the Merlin data monitor thread, we cannot rely on that
	 * monitor thread for information.
	 */

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}
	if ( merlin_medipix == (MX_MERLIN_MEDIPIX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The MX_MERLIN_MEDIPIX pointer passed for detector '%s' is NULL.",
			ad->record->name );
	}

	data_socket = merlin_medipix->data_socket;

	if ( data_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Somehow the Merlin data socket has not yet been initialized "
		"for detector '%s'.  This should not be able to happen!",
			ad->record->name );
	}

	/* Configure the detector for 1 frame with internal triggering
	 * and an exposure time of 0.01 seconds, since that should always
	 * be possible to do without user intervention.
	 */

	/* Set the Merlin to internal triggering. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,TRIGGERSTART,0", NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,TRIGGERSTOP,0", NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the detector for an exposure time of 10 milliseconds. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
				"SET,ACQUISITIONTIME,10", NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
				"SET,ACQUISITIONPERIOD,11", NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the detector for 1 frame. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
				"SET,NUMFRAMESTOACQUIRE,1", NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
				"SET,NUMFRAMESPERTRIGGER,1", NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell Merlin to acquire the frame. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
				"CMD,STARTACQUISITION", NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*-----------------------------------------------------------------*/

	/* Wait for the data socket to send us a message. */

	mx_status = mxd_merlin_medipix_raw_wait_for_message_header(
			merlin_medipix->data_socket,
			&acquisition_message_body_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* This first message _should_ be a HDR message.  Let's read the
	 * first 4 bytes of it to make sure.
	 */

	mx_status = mx_socket_receive( data_socket,
				message_type_string,
				MXU_MPX_MESSAGE_TYPE_STRING_LENGTH,
				&num_bytes_received, NULL, 0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure the message type is null terminated. */

	message_type_string[ MXU_MPX_MESSAGE_TYPE_STRING_LENGTH ] = '\0';

	if ( strncmp( message_type_string, "HDR,", 4 ) != 0 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"The first 4 bytes '%s' of the first data message received "
		"from Merlin detector '%s' is not 'HDR,'.  Somehow we are "
		"not correctly synchronized with the detector.",
			message_type_string, ad->record->name );
	}

	/* Since we appear to have received a valid HDR message, we now
	 * create an array to copy acquisition header messages to.
	 */

	merlin_medipix->acquisition_header =
		calloc( 1, acquisition_message_body_length );

	if ( merlin_medipix->acquisition_header == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu byte "
		"acquisition header array for Merlin detector '%s'.",
			acquisition_message_body_length, ad->record->name );
	}

	merlin_medipix->acquisition_header_length
			= acquisition_message_body_length;

	/* Configure the 'acquisition_header' MX record field to have the
	 * correct length.
	 */

	mx_status = mx_find_record_field( ad->record,
					"acquisition_header",
					&acquisition_header_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	acquisition_header_field->dimension[0]
			= acquisition_message_body_length;

	/*-----------------------------------------------------------------*/

	/* We have now done everything we need with the HDR message, so we
	 * now wait for the MQ1 image frame message.
	 */

	mx_status = mxd_merlin_medipix_raw_wait_for_message_header(
					merlin_medipix->data_socket,
					&image_message_body_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* This message _should_ be an MQ1 message. */

	mx_status = mx_socket_receive( data_socket,
				message_type_string,
				MXU_MPX_MESSAGE_TYPE_STRING_LENGTH,
				&num_bytes_received, NULL, 0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure the message type is null terminated. */

	message_type_string[ MXU_MPX_MESSAGE_TYPE_STRING_LENGTH ] = '\0';

	if ( strncmp( message_type_string, "MQ1,", 4 ) != 0 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"The first 4 bytes '%s' of the second data message received "
		"from Merlin detector '%s' is not 'MQ1,'.  Somehow we are "
		"not correctly synchronized with the detector.",
			message_type_string, ad->record->name );
	}

	merlin_medipix->image_message_length = image_message_body_length;

	/* Create an array big enough to hold _all_ of the images
	 * specified by the MX configuration file.
	 */

	merlin_medipix->image_message_array =
		calloc( merlin_medipix->num_image_buffers,
			image_message_body_length );

	if ( merlin_medipix->image_message_array == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
	       	"of %lu byte image messages for Merlin detector '%s'.",
			merlin_medipix->num_image_buffers,
			image_message_body_length,
			ad->record->name );
	}

	merlin_medipix->image_message_array_length =
		merlin_medipix->num_image_buffers
		* image_message_body_length;

	/* Configure the 'image_message_array' MX record field to have the
	 * correct length.
	 */

	mx_status = mx_find_record_field( ad->record,
					"image_message_array",
					&image_message_array_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	image_message_array_field->dimension[0]
			= merlin_medipix->image_message_array_length;

#if MXD_MERLIN_MEDIPIX_DEBUG_FRAME_ADDRESSES
	MX_DEBUG(-2,
	    ("%s: num_image_buffers = %lu, image_message_length = %lu",
	    fname, merlin_medipix->num_image_buffers,
	    merlin_medipix->image_message_length));
	MX_DEBUG(-2,("%s: merlin_medipix->image_message_array_length = %lu",
	    fname, merlin_medipix->image_message_array_length));
	MX_DEBUG(-2,("%s: merlin_medipix->image_message_array = %p",
	    fname, merlin_medipix->image_message_array));
#endif

	/* The MQ1 header also contains much more information about the
	 * configuration of the detector.  So let's read in more of the
	 * MQ1 header.
	 */

	mx_status = mx_socket_receive( data_socket,
					initial_image_header_bytes,
					sizeof(initial_image_header_bytes)-1,
					&num_bytes_received, NULL, 0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_string_split( initial_image_header_bytes, ",", &argc, &argv );

#if 0
	{
		int i;

		for ( i = 0; i < argc; i++ ) {
			MX_DEBUG(-2,("%s: argv[%d] = '%s'", fname, i, argv[i]));
		}
	}
#endif

	merlin_medipix->image_data_offset = strtoul( argv[1], NULL, 10 );

	merlin_medipix->number_of_chips = strtoul( argv[2], NULL, 10 );

	ad->framesize[0] = strtol( argv[3], NULL, 10 );

	ad->framesize[1] = strtol( argv[4], NULL, 10 );

	ad->bits_per_pixel = strtoul( argv[5]+1, NULL, 10 );

	switch( ad->bits_per_pixel ) {
	case  8:
		ad->bytes_per_pixel = 1;
		break;
	case 16:
		ad->bytes_per_pixel = 2;
		break;
	case 32:
		ad->bytes_per_pixel = 4;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported number of bits per pixel (%lu) requested for "
		"area detector '%s'.  Only 8, 16, or 32 bits per pixel "
		"are supported.", ad->bits_per_pixel, ad->record->name );
		break;
	}

	ad->bytes_per_frame = mx_round( ad->bytes_per_pixel
				* ad->framesize[0] * ad->framesize[1] );

	strlcpy( merlin_medipix->sensor_layout, argv[6],
			sizeof(merlin_medipix->sensor_layout) );

	merlin_medipix->chip_select = mx_hex_string_to_unsigned_long( argv[7] );

	merlin_medipix->counter = strtoul( argv[10], NULL, 10 );

	merlin_medipix->color_mode = strtoul( argv[11], NULL, 10 );

	merlin_medipix->gain_mode = strtoul( argv[12], NULL, 10 );

	/* argv uses malloc-ed memory, so it must be freed. */

	mx_free( argv );

	/* We do not need any more of this image frame, so we discard it. */

	mx_status = mx_socket_discard_unread_input( data_socket );

	return mx_status;
}

/*---*/

/* Only the thread running mxd_merlin_medipix_monitor_thread_fn() is
 * allowed to read from the Merlin data port after it has been set up
 * by mxd_merlin_medipix_setup_data_socket().
 */

#define MXM_MPX_UNKNOWN_MESSAGE		(-1)
#define MXM_MPX_NOT_YET_READ		0
#define MXM_MPX_HEADER_MESSAGE		1
#define MXM_MPX_IMAGE_MESSAGE		2

static mx_status_type
mxd_merlin_medipix_monitor_thread_fn( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxd_merlin_medipix_monitor_thread_fn()";

	MX_RECORD *record;
	MX_AREA_DETECTOR *ad;
	MX_MERLIN_MEDIPIX *merlin_medipix;
	long merlin_frame_number, mx_sequence_frame_number;
	long total_num_frames_at_start;
	long absolute_frame_number, modulo_frame_number;
	unsigned long sleep_us;
	unsigned long message_body_length, bytes_left;
	char *dest_message_ptr, *remaining_dest_ptr;
	int num_items, message_type;
	size_t num_bytes_read;
	char initial_read_buffer[ MXU_MPX_INITIAL_READ_LENGTH + 1 ];
	mx_status_type mx_status;

	if ( args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record = (MX_RECORD *) args;

#if MXD_MERLIN_MEDIPIX_DEBUG_MONITOR_THREAD
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, record->name));
#endif
	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR pointer for record '%s' is NULL.",
			record->name );
	}

	merlin_medipix = (MX_MERLIN_MEDIPIX *) record->record_type_struct;

	if ( merlin_medipix == (MX_MERLIN_MEDIPIX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MERLIN_MEDIPIX pointer for record '%s' is NULL.",
			record->name );
	}

	sleep_us = 1000;		/* in microseconds */

	while (TRUE) {

	    /* Wait for a Merlin message. */

	    mx_status = mxd_merlin_medipix_raw_wait_for_message_header(
			    			merlin_medipix->data_socket,
						&message_body_length );

	    if ( mx_status.code != MXE_SUCCESS )
		    return mx_status;

	    /* Read in the rest of this message so that we can figure out
	     * what kind of message this is and then copy the data to
	     * whereever it has to go.
	     */

	    mx_status = mx_socket_receive( merlin_medipix->data_socket,
			    		initial_read_buffer,
					MXU_MPX_INITIAL_READ_LENGTH,
					&num_bytes_read, NULL, 0,
					MXF_SOCKET_RECEIVE_WAIT );

	    if ( mx_status.code != MXE_SUCCESS )
		    return mx_status;

	    /* Make sure the initial read buffer is null terminated. */

	    initial_read_buffer[MXU_MPX_INITIAL_READ_LENGTH] = '\0';

	    /*-----------------------------------------------------------*/

	    message_type = MXM_MPX_NOT_YET_READ;

	    if ( strncmp( initial_read_buffer, "HDR,", 4 ) == 0 ){

		    dest_message_ptr = merlin_medipix->acquisition_header;

		    message_type = MXM_MPX_HEADER_MESSAGE;
	    } else
	    if ( strncmp( initial_read_buffer, "MQ1,", 4 ) == 0 ) {

		    /* We need to find out what frame number this is
		     * in order to compute the address the frame must
		     * be copied to.
		     */

		    num_items = sscanf( initial_read_buffer, "MQ1,%lu",
				    			&merlin_frame_number );

		    if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Did not find the Merlin frame number in "
				"the Merlin message body '%s...' for "
				"area detector '%s'.",
					initial_read_buffer,
					record->name );
		    }

		    mx_sequence_frame_number = merlin_frame_number - 1L;

		    total_num_frames_at_start = mx_atomic_read32(
			&(merlin_medipix->total_num_frames_at_start) );

		    absolute_frame_number = total_num_frames_at_start
			    + mx_sequence_frame_number;

		    modulo_frame_number = absolute_frame_number
			    % (merlin_medipix->num_image_buffers);

#if 1
		    MX_DEBUG(-2,("%s: total_num_frames_at_start = %ld, "
			"mx_sequence_frame_number = %ld, "
			"absolute_frame_number = %ld, "
			"modulo_frame_number = %ld",
			fname, mx_sequence_frame_number,
			total_num_frames_at_start,
			absolute_frame_number,
			modulo_frame_number ));
#endif

		    dest_message_ptr = merlin_medipix->image_message_array
			    + (modulo_frame_number * message_body_length);

		    message_type = MXM_MPX_IMAGE_MESSAGE;
	    } else {
		    message_type = MXM_MPX_UNKNOWN_MESSAGE;

		    return mx_error( MXE_PROTOCOL_ERROR, fname,
		    "Message type in '%s' is not recognized for "
		    "data socket %d of area detector '%s'.",
		    	initial_read_buffer,
			(int) merlin_medipix->data_socket->socket_fd,
			record->name );
	    }

	    /* copy the bytes that we have already read to the destination. */

	    memcpy( dest_message_ptr, initial_read_buffer,
			    MXU_MPX_INITIAL_READ_LENGTH );

	    /* Now copy in the rest of the message from the socket. */

	    remaining_dest_ptr
		    = dest_message_ptr + MXU_MPX_INITIAL_READ_LENGTH;

	    bytes_left = message_body_length - MXU_MPX_INITIAL_READ_LENGTH;

	    mx_status = mx_socket_receive( merlin_medipix->data_socket,
			    		remaining_dest_ptr,
					bytes_left,
					&num_bytes_read, NULL, 0,
					MXF_SOCKET_RECEIVE_WAIT );

#if 1
	    switch( message_type ) {
	    case MXM_MPX_HEADER_MESSAGE:
		MX_DEBUG(-2,("%s: HDR pointer = %p",
			fname, dest_message_ptr));
#if 0
		MX_DEBUG(-2,("%s: HDR message = '%s'",
			fname, merlin_medipix->acquisition_header));
#endif
		break;
	    case MXM_MPX_IMAGE_MESSAGE:
	    	MX_DEBUG(-2,(
	"%s: MQ1: mx_sequence_frame_number = %lu, dest_message_ptr = %p",
			fname, mx_sequence_frame_number, dest_message_ptr));
		break;
	    }
#endif

	    /* If this is an image message, then update the frame counter. */

	    if ( message_type == MXM_MPX_IMAGE_MESSAGE ) {
		    mx_atomic_increment32(&(merlin_medipix->total_num_frames));

#if 1
		    MX_DEBUG(-2,("CAPTURE: Total num frames = %lu",
		    (unsigned long) merlin_medipix->total_num_frames));
#endif
	    }

	    mx_usleep( sleep_us );

	    mx_status = mx_thread_check_for_stop_request( thread );
	}

	MXW_NOT_REACHED( return MX_SUCCESSFUL_RESULT );
}

/*---*/

/* Since the Merlin frame numbers are stored in variables that are not
 * part of the standard MX_AREA_DETECTOR structure, we cannot use the
 * standard mx_area_detector_vctest_extended_status() function by itself.
 * Instead, we need this shim that explicitly calls the get_extended_status()
 * function to generate the MX_AREA_DETECTOR status values from the
 * Merlin-specific variables managed by the monitor thread.
 */

static mx_status_type
mxd_merlin_medipix_vctest_extended_status( MX_RECORD_FIELD *record_field,
					int direction,
					mx_bool_type *value_changed_ptr )
{
	static const char fname[] =
		"mxd_merlin_medipix_vctest_extended_status()";

	MX_RECORD *record = NULL;
	MX_AREA_DETECTOR *ad = NULL;
	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	int32_t new_total_num_frames_at_start;
	int32_t new_total_num_frames;
	mx_bool_type need_update;
	mx_status_type mx_status;

	record = record_field->record;
	ad = record->record_class_struct;
	merlin_medipix = record->record_type_struct;

	need_update = FALSE;

	/* mxd_merlin_medipix_get_extended_status() is an expensive
	 * function, so we only want to invoke it when absolutely
	 * necessary.
	 */

	new_total_num_frames_at_start =
	    mx_atomic_read32( &(merlin_medipix->total_num_frames_at_start) );

	new_total_num_frames =
	    mx_atomic_read32( &(merlin_medipix->total_num_frames) );

	if ( merlin_medipix->detector_just_started ) {
		need_update = TRUE;

		merlin_medipix->detector_just_started = FALSE;
	} else
	if ( new_total_num_frames_at_start
	     != merlin_medipix->old_total_num_frames_at_start )
	{
		need_update = TRUE;
	} else
	if ( new_total_num_frames != merlin_medipix->old_total_num_frames )
	{
		need_update = TRUE;
	}

	merlin_medipix->old_total_num_frames_at_start
		= new_total_num_frames_at_start;

	merlin_medipix->old_total_num_frames = new_total_num_frames;

	if ( need_update ) {
		MX_DEBUG(-2,("%s: '%s.%s', need_update = %d",
		  fname, record->name, record_field->name, (int) need_update));
	}

	/* If an update is not needed, then return without doing any more. */

	if ( need_update == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If needed, directly invoked the get_extended_status() function
	 * to update the values in the MX_AREA_DETECTOR structure.
	 */

	mx_status = mxd_merlin_medipix_get_extended_status( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now call the standard extended status test function. */

	mx_status = mx_area_detector_vctest_extended_status( record_field,
					direction, value_changed_ptr );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mxd_merlin_medipix_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_merlin_medipix_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	merlin_medipix = (MX_MERLIN_MEDIPIX *)
				malloc( sizeof(MX_MERLIN_MEDIPIX) );

	if ( merlin_medipix == (MX_MERLIN_MEDIPIX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_MERLIN_MEDIPIX structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = merlin_medipix;
	record->class_specific_function_list = 
			&mxd_merlin_medipix_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	merlin_medipix->record = record;

	ad->trigger_mode = 0;
	ad->initial_correction_flags = 0;

	merlin_medipix->acquisition_header_length = 0;
	merlin_medipix->acquisition_header = NULL;

	merlin_medipix->image_message_array_length = 0;
	merlin_medipix->image_message_array = NULL;

	merlin_medipix->external_trigger_debounce_time = 0.01;  /* in seconds */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_merlin_medipix_open()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	MX_RECORD_FIELD *field = NULL;
	unsigned long mask;
	char response[100];
	int num_items;
	unsigned long version_major, version_minor, version_update;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG( 2,("%s invoked for area detector '%s'.",
		fname, record->name ));
#endif

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	merlin_medipix->old_total_num_frames_at_start = (int32_t) (-1L);

	merlin_medipix->old_total_num_frames = (int32_t) (-1L);

	merlin_medipix->detector_just_started = TRUE;

	/* Replace the normal mx_area_detector_vctest_extended_status()
	 * function with our custom front end for the various status fields.
	 */

	field =
	    &(record->record_field_array[ ad->total_num_frames_field_number ]);

	field->value_changed_test_function =
		mxd_merlin_medipix_vctest_extended_status;

	field =
	    &(record->record_field_array[ ad->last_frame_number_field_number ]);

	field->value_changed_test_function =
		mxd_merlin_medipix_vctest_extended_status;

	field = &(record->record_field_array[ ad->status_field_number ]);

	field->value_changed_test_function =
		mxd_merlin_medipix_vctest_extended_status;

	field =
	    &(record->record_field_array[ ad->extended_status_field_number ]);

	field->value_changed_test_function =
		mxd_merlin_medipix_vctest_extended_status;

	/*---*/

	merlin_medipix->old_detector_status = (unsigned long)(-1L);

	/* Set the resolution in mm/pixel (There are 55 um per pixel) . */

	ad->resolution[0] = 0.055;
	ad->resolution[1] = 0.055;

	/* Set some starting guesses at the area detector parameters. */

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	ad->bytes_per_pixel = 2;
	ad->bits_per_pixel = 16;
	ad->byte_order = mx_native_byteorder();

	ad->framesize[0] = 512;
	ad->framesize[1] = 512;

	ad->maximum_framesize[0] = ad->framesize[0];
	ad->maximum_framesize[1] = ad->framesize[1];

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	ad->bytes_per_frame =
	  mx_round( ad->bytes_per_pixel * ad->framesize[0] * ad->framesize[1] );

	ad->image_format = MXT_IMAGE_FORMAT_GREY16;

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						MXU_IMAGE_FORMAT_NAME_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->correction_flags = 0;

	mx_status = mx_area_detector_set_trigger_mode( record,
						MXT_IMAGE_INTERNAL_TRIGGER );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make the connections to the detector controller. */

	mx_status = mx_tcp_socket_open_as_client(
				&(merlin_medipix->command_socket),
				merlin_medipix->hostname,
				merlin_medipix->command_port,
				0, 1000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_tcp_socket_open_as_client(
				&(merlin_medipix->data_socket),
				merlin_medipix->hostname,
				merlin_medipix->data_port,
				0, 100000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* NOTE: The Merlin seems to need a bit of a delay after we connect
	 * to its data socket.  If we try configuring the socket buffers
	 * too quickly after connecting to the data port, then sometimes
	 * the detector will not send frames to us.
	 */

	mx_msleep(1000);

	/* We need to create buffers for receiving messages on the data socket
	 * from the Merlin detector, so lets create them.
	 */

	mx_status = mxd_merlin_medipix_setup_data_socket( ad, merlin_medipix );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set up internal parameters of the 'merlin_medipix' driver. */

	ad->maximum_framesize[0] = ad->framesize[0];
	ad->maximum_framesize[1] = ad->framesize[1];

	if ( ad->bits_per_pixel != 16 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The Merlin Medipix detector '%s' is currently configured "
		"for %lu bits per pixel images.  At this time, the "
		"'merlin_medipix' driver only supports 16-bit images.",
			ad->record->name, ad->bits_per_pixel );
	}

	/* Get the software version of the Merlin controller. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"GET,SOFTWAREVERSION",
					response, sizeof(response), FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "GET,SOFTWAREVERSION,%lu.%lu.%lu",
			&version_major, &version_minor, &version_update );

	if ( num_items != 3 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"Did not find the Merlin software version in the "
		"response '%s' to the 'GET,SOFTWAREVERSION' command "
		"sent to Merlin Medipix detector '%s'.",
			response, record->name );
	}

	merlin_medipix->merlin_software_version =
	  (1000000L * version_major) + (1000L * version_minor) + version_update;

	/*=== Configure some default parameters for the detector. ===*/

	/* 12-bit counting with both counters */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,COUNTERDEPTH,12",
					NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,ENABLECOUNTER1,0",
					NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off continuous acquisition. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,CONTINUOUSRW,0",
					NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off colour mode. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,COLOURMODE,0",
					NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off charge summing. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,CHARGESUMMING,0",
					NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Num frames per trigger. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,NUMFRAMESPERTRIGGER,1",
					NULL, 0, FALSE );

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

	/* Set the detector trigger input mode to 'internal'. */

	mx_status = mx_area_detector_set_trigger_mode( record,
						MXT_IMAGE_INTERNAL_TRIGGER );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the detector output to go high when
	 * the detector is busy (7).
	 */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
						"SET,TriggerOutTTL,7",
						NULL, 0, FALSE );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Optional configuration steps. */

	if ( merlin_medipix->merlin_flags & MXF_MERLIN_CONFIGURE_DETECTOR ) {
		char command[40];

		snprintf( command, sizeof(command),
			"SET,GAIN,%lu", merlin_medipix->gain );

		mx_status = mxd_merlin_medipix_command( merlin_medipix,
						command, NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command),
			"SET,THRESHOLD0,%lu", merlin_medipix->threshold0 );

		mx_status = mxd_merlin_medipix_command( merlin_medipix,
						command, NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command),
			"SET,THRESHOLD1,%lu", merlin_medipix->threshold1 );

		mx_status = mxd_merlin_medipix_command( merlin_medipix,
						command, NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command),
			"SET,HVBIAS,%lu", merlin_medipix->high_voltage );

		mx_status = mxd_merlin_medipix_command( merlin_medipix,
						command, NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Initialize the frame counters. */

	mx_atomic_write32( &(merlin_medipix->total_num_frames_at_start), 0 );
	mx_atomic_write32( &(merlin_medipix->total_num_frames), 0 );

	/* Create a thread to manage the reading of data from the data port. */

	mx_status = mx_thread_create( &(merlin_medipix->monitor_thread),
					mxd_merlin_medipix_monitor_thread_fn,
					record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_arm()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	char command[80];
	double exposure_time, frame_time;
	unsigned long num_frames, debounce_milliseconds;
	int32_t total_num_frames_at_start;
	mx_status_type mx_status;

	merlin_medipix = NULL;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	merlin_medipix->detector_just_started = TRUE;

	sp = &(ad->sequence_parameters);

	/* Set the exposure sequence parameters. */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		frame_time = 1.01 * exposure_time;
		break;
	case MXT_SQ_MULTIFRAME:
		num_frames = mx_round( sp->parameter_array[0] );
		exposure_time = sp->parameter_array[1];
		frame_time = sp->parameter_array[2];
		break;
	case MXT_SQ_STROBE:
		num_frames = mx_round( sp->parameter_array[0] );
		exposure_time = sp->parameter_array[1];
		frame_time = 1.001 * exposure_time;
		break;
	case MXT_SQ_DURATION:
		num_frames = mx_round( sp->parameter_array[0] );
		exposure_time = 0;
		frame_time = 0;
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Detector '%s' sequence types other than 'one-shot' "
		"are not yet implemented.", ad->record->name );
		break;
	}

	/* Update the frame counters. */

	total_num_frames_at_start =
		mx_atomic_read32( &(merlin_medipix->total_num_frames) );

	mx_atomic_write32( &(merlin_medipix->total_num_frames_at_start),
					total_num_frames_at_start );

	/* Configure the type of trigger mode we want. */

	switch( ad->trigger_mode ) {
	case MXT_IMAGE_INTERNAL_TRIGGER:
		mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,TRIGGERSTART,0", NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,TRIGGERSTOP,0", NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXT_IMAGE_EXTERNAL_TRIGGER:
		mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,TRIGGERSTART,1", NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( sp->sequence_type == MXT_SQ_DURATION ) {

			/* Stop the frame when the trigger goes away. */

			mx_status = mxd_merlin_medipix_command( merlin_medipix,
							"SET,TRIGGERSTOP,2",
							NULL, 0, FALSE );
		} else {
			/* Stop the frame when the exposure time expires. */

			mx_status = mxd_merlin_medipix_command(
				merlin_medipix, "SET,TRIGGERSTOP,0",
						NULL, 0, FALSE );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Unsupported trigger mode %#lx requested for area detector '%s'.",
			ad->trigger_mode, ad->record->name );
		break;
	}

	/* Setup the sequence parameters. */

	snprintf( command, sizeof(command),
		"SET,NUMFRAMESTOACQUIRE,%lu", num_frames );

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
						command, NULL, 0, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_STROBE:
		snprintf( command, sizeof(command), "SET,ACQUISITIONTIME,%lu",
			mx_round( 1000.0 * exposure_time ) );

		mx_status = mxd_merlin_medipix_command( merlin_medipix,
						command, NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command), "SET,ACQUISITIONPERIOD,%lu",
			mx_round( 1000.0 * frame_time ) );

		mx_status = mxd_merlin_medipix_command( merlin_medipix,
						command, NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( sp->sequence_type == MXT_SQ_MULTIFRAME )
		  && ( ad->trigger_mode == MXT_IMAGE_EXTERNAL_TRIGGER ) )
		{
			snprintf( command, sizeof(command),
				"SET,NUMFRAMESPERTRIGGER,%lu", num_frames );

			mx_status = mxd_merlin_medipix_command(
					merlin_medipix, command,
					NULL, 0, FALSE );
		} else {
			mx_status = mxd_merlin_medipix_command(
					merlin_medipix,
					"SET,NUMFRAMESPERTRIGGER,1",
					NULL, 0, FALSE );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	case MXT_SQ_DURATION:
		/* In duration mode, the Frame Time (ACQUISITIONTIME)
		 * acts instead as a debouncing time.
		 */

		debounce_milliseconds = mx_round( 1000.0
			* (merlin_medipix->external_trigger_debounce_time) );

		snprintf( command, sizeof(command), "SET,ACQUISITIONTIME,%lu",
						debounce_milliseconds );

		mx_status = mxd_merlin_medipix_command( merlin_medipix,
						command, NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command), "SET,ACQUISITIONPERIOD,%lu",
			debounce_milliseconds + 100L );

		mx_status = mxd_merlin_medipix_command( merlin_medipix,
						command, NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,NUMFRAMESPERTRIGGER,1",
					NULL, 0, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Sequence type %ld is not supported for area detector '%s'.",
			sp->sequence_type, ad->record->name );
		break;
	}

	/* If we are configured for external triggering, tell the
	 * acquisition sequence to start.
	 */

	if ( ad->trigger_mode == MXT_IMAGE_EXTERNAL_TRIGGER ) {
		mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"CMD,STARTACQUISITION", NULL, 0, FALSE);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_trigger()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	merlin_medipix = NULL;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	merlin_medipix->detector_just_started = TRUE;

	/* If we are configured for internal triggering, tell the
	 * acquisition sequence to start.
	 */

	if ( ad->trigger_mode == MXT_IMAGE_INTERNAL_TRIGGER ) {
		mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"CMD,STARTACQUISITION", NULL, 0, FALSE);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_stop()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"CMD,STOPACQUISITION", NULL, 0, FALSE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_get_extended_status()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	char response[80];
	int num_items;
	long detector_status;
	long num_data_bytes_available;
	long total_num_frames_at_start;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG( 2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Update the frame counters. */

	total_num_frames_at_start =
	    mx_atomic_read32( &(merlin_medipix->total_num_frames_at_start) );

	ad->total_num_frames =
	    mx_atomic_read32( &(merlin_medipix->total_num_frames ) );

	ad->last_frame_number =
		ad->total_num_frames - total_num_frames_at_start - 1L;

	/* Update the detector status. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"GET,DETECTORSTATUS",
					response, sizeof(response), TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response,
			"GET,DETECTORSTATUS,%lu", &detector_status );

	if ( num_items != 1 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"Did not see the detector status in the response '%s' "
		"to a 'GET,DETECTORSTATUS' command sent to detector '%s'.",
			response, ad->record->name );
	}

	if ( detector_status != (merlin_medipix->old_detector_status) ) {

#if MXD_MERLIN_MEDIPIX_DEBUG_DETECTOR_STATUS
		MX_DEBUG(-2,("%s: detector_status changed from %lu to %lu",
			fname, merlin_medipix->old_detector_status,
			detector_status));
#endif
	}

	merlin_medipix->old_detector_status = detector_status;

	/* Sometimes this detector returns undocumented status codes 3 and 4.
	 * I think that 4 has something to do with external triggering.
	 * I do not know what 3 means, but if it happens you will need to
	 * restart the Merlin application program on the Windows box.
	 */
		
	switch( detector_status ) {
	case 0:
		ad->status = 0;
		break;
	case 1:
	case 4:
		ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
		break;
	case 2:
		ad->status = MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
		break;
	default:
		ad->status = MXSF_AD_ERROR;
		break;
	}

	/* Check for any new data from the data_socket. */

	mx_status = mx_socket_num_input_bytes_available(
					merlin_medipix->data_socket,
					&num_data_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_data_bytes_available > 0 ) {
		ad->status |= MXSF_AD_UNSAVED_IMAGE_FRAMES;
	}

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s: ad->status = %#lx, num_data_bytes_available = %lu",
		fname, ad->status, num_data_bytes_available));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_readout_frame()"; 
	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	char *image_message_array;
	char *ptr, *image_binary_ptr, *image_message_ptr;
	char copy_of_beginning_of_message[200];
	long mx_frame_number;
	unsigned long bits_per_pixel, buffer_mx_frame_number;
	double exposure_time;
	char *timestamp_string = NULL;
	struct timespec exposure_timespec;
	struct timespec timestamp;
	struct tm tm;
	int num_items;
	unsigned long microseconds;
	int argc;
	char **argv;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', frame %ld.",
		fname, ad->record->name, ad->readout_frame ));
#endif
	if ( merlin_medipix->image_message_array_length == 0 ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Somehow the image message array length has not "
		"been correctly set up for area detector '%s'.",
			ad->record->name );
	}

	image_message_array = merlin_medipix->image_message_array;

	MX_DEBUG(-2,("%s: merlin_medipix->image_message_array = %p",
		fname, merlin_medipix->image_message_array));

	if ( image_message_array == (char *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The merlin_medipix->image_message_array pointer "
		"for detector '%s' is NULL.  This should not happen.",
			ad->record->name );
	}

	mx_frame_number = ad->readout_frame;

	image_message_ptr = image_message_array +
		(mx_frame_number * merlin_medipix->image_message_length);

#if MXD_MERLIN_MEDIPIX_DEBUG_FRAME_ADDRESSES
	MX_DEBUG(-2,("%s: merlin_medipix->image_message_array = %p",
		fname, merlin_medipix->image_message_array));
	MX_DEBUG(-2,("%s: merlin_medipix->image_message_length = %lu",
		fname, merlin_medipix->image_message_length));
	MX_DEBUG(-2,("%s: mx_frame_number = %lu, image_message_ptr = %p",
		fname, mx_frame_number, image_message_ptr));
#endif
	/* Make a copy of the beginning of the message so that we can run
	 * mx_string_split() on it.  Bear in mind that mx_string_split()
	 * modifies the string passed to it, so we want to work off of
	 * a copy of the data rather than the original data.
	 */

	memcpy( copy_of_beginning_of_message, image_message_ptr,
		sizeof(copy_of_beginning_of_message) );

	/* Parse some information we need from the beginning of the
	 * Merlin image frame.
	 */

	mx_string_split( copy_of_beginning_of_message, ",", &argc, &argv );

	if ( argc < 11 ) {
		mx_status = mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The '%s' record apparently received a frame from the "
		"Merlin detector, but there does not appear to be an image "
		"header in the image data received from the detector.",
			ad->record->name );

		return mx_status;
	}

	/* MX starts frame numbers at 0 rather than 1. */

	buffer_mx_frame_number = atol( argv[1] ) - 1;

	if ( buffer_mx_frame_number != mx_frame_number ) {
		mx_warning( "The MX frame number %lu computed from the "
		"image message array does not match the MX frame number %lu "
		"requested for area detector '%s'.",
			buffer_mx_frame_number, mx_frame_number,
			ad->record->name );
	}

	bits_per_pixel = atof( argv[6]+1 );

	if ( bits_per_pixel != ad->bits_per_pixel ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The number of bits per pixel %lu for frame %lu of "
		"area detector '%s' does not match the bits per pixel %lu "
		"found when this MX server started.  Changing the data "
		"format of the detector image while the MX server is "
		"running is _not_ supported.  You must shutdown the MX "
		"server, change the data format in the Merlin "
		"vendor-provided GUI and then start the MX server again.",
			bits_per_pixel, mx_frame_number,
			ad->record->name, ad->bits_per_pixel );
	}

	timestamp_string = argv[9];

	exposure_time = atof( argv[10] );

	MX_DEBUG(-2,("%s: timestamp_string = '%s'", fname, timestamp_string));
	MX_DEBUG(-2,("%s: exposure_time = %g", fname, exposure_time));

	/* Construct the timestamp for the image. */

	ptr = strptime( timestamp_string, "%Y-%m-%d %H:%M:%S", &tm );

	if ( ptr == NULL ) {
		mx_free( argv );
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"Cannot parse time string '%s' for area detector '%s'.",
			timestamp_string, ad->record->name );
	}

	timestamp.tv_sec = mktime( &tm );

	if ( *ptr == '.' ) {
		ptr++;

		num_items = sscanf( ptr, "%lu", &microseconds );

		if ( num_items == 0 ) {
			timestamp.tv_nsec = 0;
		} else {
			timestamp.tv_nsec = 1000L * microseconds;
		}
	} else {
		timestamp.tv_nsec = 0;
	}

	/*----*/

	mx_free( argv );

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						sizeof(ad->image_format_name) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->bytes_per_frame = mx_round( ad->bytes_per_pixel
				* ad->framesize[0] * ad->framesize[1] );

	ad->header_length = MXT_IMAGE_HEADER_LENGTH_IN_BYTES;

	mx_status = mx_image_alloc( &(ad->image_frame),
				ad->framesize[0],
				ad->framesize[1],
				ad->image_format,
				ad->byte_order,
				ad->bytes_per_pixel,
				ad->header_length,
				ad->bytes_per_frame,
				ad->dictionary,
				ad->record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	exposure_timespec =
		mx_convert_seconds_to_high_resolution_time( exposure_time );

	MXIF_EXPOSURE_TIME_SEC( ad->image_frame )  = exposure_timespec.tv_sec;
	MXIF_EXPOSURE_TIME_NSEC( ad->image_frame ) = exposure_timespec.tv_nsec;

	MXIF_TIMESTAMP_SEC( ad->image_frame )  = timestamp.tv_sec;
	MXIF_TIMESTAMP_NSEC( ad->image_frame ) = timestamp.tv_nsec;

	/* Finally, we finish by copying the image data from the
	 * Merlin Medipix's data structure to MX's data structure.
	 */

	image_binary_ptr = image_message_ptr
				+ merlin_medipix->image_data_offset;

	memcpy( ad->image_frame->image_data,
			image_binary_ptr,
			ad->bytes_per_frame );

#if 1
	/* FIXME: Is this really the most efficient way of doing
	 *        the byteswapping?
	 */

	if (1) {
		unsigned long i, num_words;
		uint16_t *uint16_array;

		uint16_array = (uint16_t *) ad->image_frame->image_data;

		num_words = (ad->bytes_per_frame) / 2L;

		for ( i = 0; i < num_words; i++ ) {
			uint16_array[i] = mx_16bit_byteswap( uint16_array[i] );
		}
	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_correct_frame()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,
		("%s invoked for area detector '%s', correction_flags=%#lx.",
		fname, ad->record->name, ad->correction_flags ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_merlin_medipix_transfer_frame()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', transfer_frame = %ld.",
		fname, ad->record->name, ad->transfer_frame ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_load_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_load_frame()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	merlin_medipix = NULL;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_save_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_save_frame()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_copy_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_copy_frame()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,
	("%s invoked for area detector '%s', source = %#lx, destination = %#lx",
		fname, ad->record->name, ad->copy_frame[0], ad->copy_frame[1]));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_get_parameter()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	{
		char parameter_name_buffer[80];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type( ad->record,
					ad->parameter_type,
					parameter_name_buffer,
					sizeof(parameter_name_buffer) ) ));
	}
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		break;

	case MXLV_AD_MAXIMUM_FRAMESIZE:
		break;

	case MXLV_AD_FRAMESIZE:
		break;

	case MXLV_AD_BINSIZE:
		break;

	case MXLV_AD_CORRECTION_FLAGS:
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
#if MXD_MERLIN_MEDIPIX_DEBUG
		MX_DEBUG(-2,("%s: image format = %ld, format name = '%s'",
		    fname, ad->image_format, ad->image_format_name));
#endif
		break;

	case MXLV_AD_BYTE_ORDER:
		break;

	case MXLV_AD_TRIGGER_MODE:
		break;

	case MXLV_AD_EXPOSURE_MODE:
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		break;

	case MXLV_AD_BITS_PER_PIXEL:
		break;

	case MXLV_AD_DETECTOR_READOUT_TIME:
		break;

	case MXLV_AD_SEQUENCE_START_DELAY:
		break;

	case MXLV_AD_TOTAL_ACQUISITION_TIME:
		break;

	case MXLV_AD_TOTAL_SEQUENCE_TIME:
		break;

	case MXLV_AD_SEQUENCE_TYPE:

#if MXD_MERLIN_MEDIPIX_DEBUG
		MX_DEBUG(-2,("%s: GET sequence_type = %ld",
			fname, ad->sequence_parameters.sequence_type));
#endif

#if MXD_MERLIN_MEDIPIX_DEBUG
		MX_DEBUG(-2,("%s: sequence type = %ld",
			fname, ad->sequence_parameters.sequence_type));
#endif
		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		break;

	case MXLV_AD_ROI:
		break;

	case MXLV_AD_SUBFRAME_SIZE:
		break;

	case MXLV_AD_USE_SCALED_DARK_CURRENT:
		break;

	case MXLV_AD_REGISTER_VALUE:
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		break;

	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		break;

	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		break;

	case MXLV_AD_DATAFILE_DIRECTORY:
		break;

	case MXLV_AD_DATAFILE_NAME:
		break;

	case MXLV_AD_DATAFILE_NUMBER:
		break;

	case MXLV_AD_DATAFILE_PATTERN:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		break;

	case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
		break;

	case MXLV_AD_FRAME_FILENAME:
		break;

	default:
		mx_status =
			mx_area_detector_default_get_parameter_handler( ad );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_set_parameter()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	{
		char parameter_name_buffer[80];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type( ad->record,
					ad->parameter_type,
					parameter_name_buffer,
					sizeof(parameter_name_buffer) ) ));
	}
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
		break;

	case MXLV_AD_BINSIZE:
		break;

	case MXLV_AD_CORRECTION_FLAGS:
		break;

	case MXLV_AD_TRIGGER_MODE:
		break;

	case MXLV_AD_EXPOSURE_MODE:
		break;

	case MXLV_AD_SEQUENCE_TYPE:
		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		break;

	case MXLV_AD_SEQUENCE_ONE_SHOT:
		break;

	case MXLV_AD_SEQUENCE_STREAM:
		break;

	case MXLV_AD_SEQUENCE_MULTIFRAME:
		break;

	case MXLV_AD_SEQUENCE_STROBE:
		break;

	case MXLV_AD_SEQUENCE_DURATION:
		break;

	case MXLV_AD_SEQUENCE_GATED:
		break;

	case MXLV_AD_ROI:
		break;

	case MXLV_AD_SUBFRAME_SIZE:
		break;

	case MXLV_AD_USE_SCALED_DARK_CURRENT:
		break;

	case MXLV_AD_GEOM_CORR_AFTER_FLAT_FIELD:
		break;

	case MXLV_AD_CORRECTION_FRAME_GEOM_CORR_LAST:
		break;

	case MXLV_AD_CORRECTION_FRAME_NO_GEOM_CORR:
		break;

	case MXLV_AD_REGISTER_VALUE:
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		break;

	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		break;

	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		break;

	case MXLV_AD_DATAFILE_DIRECTORY:
		break;

	case MXLV_AD_DATAFILE_NAME:
		break;

	case MXLV_AD_DATAFILE_NUMBER:
		break;

	case MXLV_AD_DATAFILE_PATTERN:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		break;

	case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
		break;

	case MXLV_AD_FRAME_FILENAME:
		break;

	case MXLV_AD_RAW_LOAD_FRAME:
		break;

	case MXLV_AD_RAW_SAVE_FRAME:
		break;

	case MXLV_AD_BYTES_PER_FRAME:
	case MXLV_AD_BYTES_PER_PIXEL:
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
	case MXLV_AD_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Changing the parameter '%s' for area detector '%s' "
		"is not supported.", mx_get_field_label_string( ad->record,
			ad->parameter_type ), ad->record->name );
	default:
		mx_status =
			mx_area_detector_default_set_parameter_handler( ad );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_merlin_medipix_measure_correction()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
	MX_DEBUG(-2,("%s: type = %ld, time = %g, num_measurements = %ld",
		fname, ad->correction_measurement_type,
		ad->correction_measurement_time,
		ad->num_correction_measurements ));
#endif

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
	case MXFT_AD_FLAT_FIELD_FRAME:
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Correction measurement type %ld is not supported "
		"for area detector '%s'.",
			ad->correction_measurement_type, ad->record->name );
	}

	return mx_status;
}

/*==========================================================================*/

MX_EXPORT mx_status_type
mxd_merlin_medipix_command( MX_MERLIN_MEDIPIX *merlin_medipix,
			char *command,
			char *response,
			size_t response_buffer_length,
			mx_bool_type suppress_output )

{
	static const char fname[] = "mxd_merlin_medipix_command()";

	MX_AREA_DETECTOR *ad = NULL;
	char command_buffer[100];
	char response_buffer[500];
	size_t command_length;
	size_t num_integer_characters;
	mx_bool_type is_get_command, debug_flag;
	int num_items, status_code;
	char *body_ptr, *value_ptr, *status_code_ptr;
	unsigned long message_body_length;
	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( merlin_medipix == (MX_MERLIN_MEDIPIX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MERLIN_MEDIPIX pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}

	if ( suppress_output ) {
		debug_flag = FALSE;
	} else
	if ( merlin_medipix->merlin_flags & MXF_MERLIN_DEBUG_COMMAND_PORT ) {
		debug_flag = TRUE;
	} else {
		debug_flag = FALSE;
	}

	ad = merlin_medipix->record->record_class_struct;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
		fname, command, merlin_medipix->record->name ));
	}

	if ( mx_strncasecmp( command, "GET,", 4 ) == 0 ) {
		is_get_command = TRUE;
	} else {
		is_get_command = FALSE;
	}

	/*-----------------------------------------------------------------*/

	/* First construct the raw command string. */

	command_length = strlen( command );

	command_length++;		/* For the leading ',' separator. */

	snprintf( command_buffer, sizeof(command_buffer),
		"MPX,%010ld,%s", (long) command_length, command );

	/* A sent command requires a line terminator.  This terminator
	 * can be either a <CR> or an <LF> character.
	 */

	mx_status = mx_socket_putline( merlin_medipix->command_socket,
					command_buffer, "\n" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*-----------------------------------------------------------------*/

	/* Now read back the response line. */

	mx_status = mx_socket_wait_for_event( merlin_medipix->command_socket,
				merlin_medipix->command_socket_timeout );

	mx_status_code = mx_status.code & (~MXE_QUIET);

	switch( mx_status_code ) {
	case MXE_SUCCESS:
		break;
	case MXE_TIMED_OUT:
		/* Resend the message with the MXE_QUIET flag turned off. */

		return mx_error( MXE_TIMED_OUT,
			mx_status.location,
			"%s", mx_status.message );
		break;
	default:
		return mx_status;
		break;
	}

	/* Read the beginning of the message, including the message
	 * length field.  This part of the message is 15 bytes long
	 * if we include the trailing comma ',' character after the
	 * length field.
	 */

	memset( response_buffer, 0, sizeof(response_buffer) );

	mx_status = mx_socket_receive( merlin_medipix->command_socket,
			response_buffer,
			MXU_MPX_HEADER_LENGTH + MXU_MPX_SEPARATOR_LENGTH,
			NULL, NULL, 0, MXF_SOCKET_RECEIVE_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out how many more bytes there are to read. */

	num_items = sscanf( response_buffer, "MPX,%lu,", &message_body_length );

	if ( num_items != 1 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"Did not see the length of the rest of the message body in "
		"the response '%s' to  a '%s' command sent to '%s'.",
		    response_buffer, command, merlin_medipix->record->name );
	}

	body_ptr = response_buffer +
			MXU_MPX_HEADER_LENGTH + MXU_MPX_SEPARATOR_LENGTH;

	/* Note that the message_body_length value read above does _not_
	 * include the comma ',' character immediately after it.  Since
	 * we have already read that comma in, then we must subtract it
	 * from the value of message_body_length.
	 */

	message_body_length--;

	/* Now read in the rest of the message.  We have to know how many
	 * characters are to be received here, since the message sent to us
	 * by the controller does _not_ have a line terminator at the end.
	 */

	mx_status = mx_socket_receive( merlin_medipix->command_socket,
			body_ptr, message_body_length,
			NULL, NULL, 0, MXF_SOCKET_RECEIVE_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If this was a GET command, find the returned value in
	 * the response.  It should be located after the fourth
	 * comma (,) character in the response.
	 */

	if ( is_get_command == FALSE ) {
		value_ptr = NULL;
	} else {
		value_ptr = body_ptr;
	}

	/* Get the status code for the message. */

	status_code_ptr = strrchr( response_buffer, ',' );

	if ( status_code_ptr == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Did not find any ',' characters in the response '%s' "
		"to command '%s' sent to Merlin Medipix detector '%s'.",
			response_buffer, command_buffer, ad->record->name );
	}

	/* Null terminate the value string here (if it exists). */

	*status_code_ptr = '\0';

	status_code_ptr++;

	/* Parse the status code. */

	num_integer_characters = strspn( status_code_ptr, "0123456789" );

	if ( num_integer_characters != strlen( status_code_ptr ) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Did not find a valid status code in the string '%s' "
		"following the response '%s' to the command '%s' sent "
		"to Merlin Medipix detector '%s'.",
			status_code_ptr, response_buffer, 
			command_buffer, ad->record->name );
	}

	status_code = atoi( status_code_ptr );

	switch( status_code ) {
	case 0:			/* 0 means OK */
		break;
	case 1:			/* 1 means busy */

		return mx_error( MXE_NOT_READY, fname,
		"The Merlin Medipix detector '%s' was not ready for "
		"the command '%s' sent to it.",
			ad->record->name, command_buffer );
		break;
	case 2:			/* 2 means not recognized */

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Command '%s' sent to Merlin Medipix detector '%s' was not "
		"a valid command.", command_buffer, ad->record->name );
		break;
	case 3:			/* 3 means parameter out of range */

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Command '%s' sent to Merlin Medipix detector '%s' attempted "
		"to set a parameter to outside its range.",
			command_buffer, ad->record->name );
		break;
	default:
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Merlin Medipix detector '%s' sent unrecognized status "
		"code %d in response to the command '%s' sent to it.",
			ad->record->name, status_code, command_buffer );
		break;
	}

	/* Copy the response to the caller's buffer. */

	if ( is_get_command ) {
		strlcpy( response, value_ptr, response_buffer_length );
	} else {
		if ( response != NULL ) {
			response[0] = '\0';
		}
	}

	/*---*/

	if ( debug_flag && is_get_command ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'.",
			fname, response, merlin_medipix->record->name ));
	}

	return MX_SUCCESSFUL_RESULT;
}
