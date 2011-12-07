/*
 * Name:    d_sapera_lt_frame_grabber.c
 *
 * Purpose: MX video input driver for a DALSA Sapera LT frame grabber.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG			TRUE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN			TRUE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_ARM			TRUE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_TRIGGER		TRUE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_EXTENDED_STATUS	TRUE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS	FALSE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_MX_PARAMETERS		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_array.h"
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_sapera_lt.h"
#include "d_sapera_lt_frame_grabber.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_sapera_lt_frame_grabber_record_function_list = {
	NULL,
	mxd_sapera_lt_frame_grabber_create_record_structures,
	mxd_sapera_lt_frame_grabber_finish_record_initialization,
	NULL,
	NULL,
	mxd_sapera_lt_frame_grabber_open,
	mxd_sapera_lt_frame_grabber_close
};

MX_VIDEO_INPUT_FUNCTION_LIST
mxd_sapera_lt_frame_grabber_video_input_function_list = {
	mxd_sapera_lt_frame_grabber_arm,
	mxd_sapera_lt_frame_grabber_trigger,
	NULL,
	mxd_sapera_lt_frame_grabber_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_sapera_lt_frame_grabber_get_extended_status,
	mxd_sapera_lt_frame_grabber_get_frame,
	mxd_sapera_lt_frame_grabber_get_parameter,
	mxd_sapera_lt_frame_grabber_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_sapera_lt_frame_grabber_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_SAPERA_LT_FRAME_GRABBER_STANDARD_FIELDS
};

long mxd_sapera_lt_frame_grabber_num_record_fields
	= sizeof( mxd_sapera_lt_frame_grabber_record_field_defaults )
	/ sizeof( mxd_sapera_lt_frame_grabber_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sapera_lt_frame_grabber_rfield_def_ptr
			= &mxd_sapera_lt_frame_grabber_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_sapera_lt_frame_grabber_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_SAPERA_LT_FRAME_GRABBER **sapera_lt_frame_grabber,
			MX_SAPERA_LT **sapera_lt,
			const char *calling_fname )
{
	static const char fname[] =
		"mxd_sapera_lt_frame_grabber_get_pointers()";

	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber_ptr;
	MX_RECORD *sapera_lt_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	sapera_lt_frame_grabber_ptr = (MX_SAPERA_LT_FRAME_GRABBER *)
				vinput->record->record_type_struct;

	if ( sapera_lt_frame_grabber_ptr
		== (MX_SAPERA_LT_FRAME_GRABBER *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SAPERA_LT_FRAME_GRABBER pointer for "
			"record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}

	if ( sapera_lt_frame_grabber != (MX_SAPERA_LT_FRAME_GRABBER **) NULL ) {
		*sapera_lt_frame_grabber = sapera_lt_frame_grabber_ptr;
	}

	if ( sapera_lt != (MX_SAPERA_LT **) NULL ) {
		sapera_lt_record =
			sapera_lt_frame_grabber_ptr->sapera_lt_record;

		if ( sapera_lt_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The sapera_lt_record pointer for record '%s' "
			"is NULL.",
			vinput->record->name, calling_fname );
		}

		*sapera_lt = (MX_SAPERA_LT *)
					sapera_lt_record->record_type_struct;

		if ( *sapera_lt == (MX_SAPERA_LT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SAPERA_LT pointer for record '%s' used "
			"by record '%s' is NULL.",
				vinput->record->name,
				sapera_lt_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static void
mxd_sapera_lt_frame_grabber_acquisition_callback( SapXferCallbackInfo *info )
{
	static const char fname[] =
		"mxd_sapera_lt_frame_grabber_acquisition_callback()";

	MX_RECORD *record;
	MX_VIDEO_INPUT *vinput;
	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber;

	sapera_lt_frame_grabber =
		(MX_SAPERA_LT_FRAME_GRABBER *) info->GetContext();

	record = sapera_lt_frame_grabber->record;

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	MX_DEBUG(-2,("%s invoked for frame grabber '%s'.",
		fname, sapera_lt_frame_grabber->record->name ));

	if ( sapera_lt_frame_grabber->num_frames_left_to_acquire > 0 ) {
		sapera_lt_frame_grabber->num_frames_left_to_acquire--;
		vinput->total_num_frames++;
	}
}

/*---*/

static mx_status_type
mxd_sapera_lt_frame_grabber_setup_frame_counters( MX_VIDEO_INPUT *vinput,
			MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber )
{
	long num_frames_in_sequence;
	mx_status_type mx_status;

	/* Setup the Sapera frame counters. */

	sapera_lt_frame_grabber->total_num_frames_at_start
					= vinput->total_num_frames;

	mx_status = mx_sequence_get_num_frames( &(vinput->sequence_parameters),
						&num_frames_in_sequence );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sapera_lt_frame_grabber->num_frames_left_to_acquire
					= num_frames_in_sequence;

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

/* This version is for reading memory addresses. */

static mx_status_type
mxd_sapera_lt_frame_grabber_get_lowlevel_parameter(
			MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber,
			int capability,
			int parameter,
			void *parameter_value )
{
	static const char fname[] =
		"mxd_sapera_lt_frame_grabber_get_lowlevel_parameter()";

	BOOL capability_valid, parameter_valid, get_parameter_status;

	if ( sapera_lt_frame_grabber == ( MX_SAPERA_LT_FRAME_GRABBER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SAPERA_LT_FRAME_GRABBER pointer passed was NULL." );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS
	MX_DEBUG(-2,("%s invoked by '%s' for reading a void pointer.",
		fname, sapera_lt_frame_grabber->record->name));
#endif

	SapAcquisition *acq = sapera_lt_frame_grabber->acquisition;

	if ( acq == (SapAcquisition *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The SapAcquisition pointer for frame grabber '%s' is NULL.",
			sapera_lt_frame_grabber->record->name );
	}

	if ( capability >= 0 ) {
	    capability_valid = acq->IsCapabilityValid( capability );

	    if ( capability_valid == FALSE ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		    "Capability %d is not available for frame grabber '%s'.",
		    capability, sapera_lt_frame_grabber->record->name );
	    }
	}

	parameter_valid = acq->IsParameterValid( parameter );

	if ( parameter_valid == FALSE ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"Parameter %d is not available for frame grabber '%s'.",
			parameter, sapera_lt_frame_grabber->record->name );
	}

	get_parameter_status = acq->GetParameter( parameter, parameter_value );

	if ( get_parameter_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to read parameter %d from "
			"frame grabber '%s' failed.",
			parameter, sapera_lt_frame_grabber->record->name );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS
	MX_DEBUG(-2,("%s: Frame grabber '%s', parameter %d = %#0x",
		fname, sapera_lt_frame_grabber->record->name,
		parameter, parameter_value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/* This version is for reading 32-bit integers. */

static mx_status_type
mxd_sapera_lt_frame_grabber_get_lowlevel_parameter(
			MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber,
			int capability,
			int parameter,
			UINT32 *parameter_value )
{
	static const char fname[] =
		"mxd_sapera_lt_frame_grabber_get_lowlevel_parameter()";

	BOOL capability_valid, parameter_valid, get_parameter_status;

	if ( sapera_lt_frame_grabber == ( MX_SAPERA_LT_FRAME_GRABBER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SAPERA_LT_FRAME_GRABBER pointer passed was NULL." );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS
	MX_DEBUG(-2,("%s invoked by '%s' for reading a UINT32 value.",
		fname, sapera_lt_frame_grabber->record->name));
#endif

	SapAcquisition *acq = sapera_lt_frame_grabber->acquisition;

	if ( acq == (SapAcquisition *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The SapAcquisition pointer for frame grabber '%s' is NULL.",
			sapera_lt_frame_grabber->record->name );
	}

	if ( capability >= 0 ) {
	    capability_valid = acq->IsCapabilityValid( capability );

	    if ( capability_valid == FALSE ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		    "Capability %d is not available for frame grabber '%s'.",
		    capability, sapera_lt_frame_grabber->record->name );
	    }
	}

	parameter_valid = acq->IsParameterValid( parameter );

	if ( parameter_valid == FALSE ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"Parameter %d is not available for frame grabber '%s'.",
			parameter, sapera_lt_frame_grabber->record->name );
	}

	get_parameter_status = acq->GetParameter( parameter, parameter_value );

	if ( get_parameter_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to read parameter %d from "
			"frame grabber '%s' failed.",
			parameter, sapera_lt_frame_grabber->record->name );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS
	MX_DEBUG(-2,("%s: Frame grabber '%s', parameter %d = %lu",
		fname, sapera_lt_frame_grabber->record->name,
		parameter, (unsigned long) *parameter_value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_sapera_lt_frame_grabber_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	sapera_lt_frame_grabber = (MX_SAPERA_LT_FRAME_GRABBER *)
				malloc( sizeof(MX_SAPERA_LT_FRAME_GRABBER) );

	if ( sapera_lt_frame_grabber == (MX_SAPERA_LT_FRAME_GRABBER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_SAPERA_LT_FRAME_GRABBER structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = sapera_lt_frame_grabber;
	record->class_specific_function_list = 
			&mxd_sapera_lt_frame_grabber_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	sapera_lt_frame_grabber->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_sapera_lt_frame_grabber_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;
	MX_SAPERA_LT *sapera_lt = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_sapera_lt_frame_grabber_get_pointers( vinput,
				&sapera_lt_frame_grabber, &sapera_lt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	mx_status = mx_video_input_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sapera_lt_frame_grabber_open()";

	MX_VIDEO_INPUT *vinput;
	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;
	MX_SAPERA_LT *sapera_lt = NULL;
	BOOL sapera_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_sapera_lt_frame_grabber_get_pointers( vinput,
				&sapera_lt_frame_grabber, &sapera_lt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* Create the Sapera objects necessary for controlling this camera. */

	if ( sapera_lt->num_frame_grabbers <= 0 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Sapera server '%s' does not have any frame grabbers.",
			sapera_lt->server_name );
	} else
	if ( sapera_lt_frame_grabber->frame_grabber_number
			>= sapera_lt->num_frame_grabbers ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested frame grabber number %ld for record '%s' "
		"is outside the allowed range of 0 to %ld for "
		"Sapera server '%s'.",
			sapera_lt_frame_grabber->frame_grabber_number,
			record->name, sapera_lt->server_name );
	}

	/* Create the high level objects. */

	SapLocation location( sapera_lt->server_name,
				sapera_lt_frame_grabber->frame_grabber_number );

	sapera_lt_frame_grabber->acquisition = new SapAcquisition( location,
				sapera_lt_frame_grabber->config_filename );

	if ( sapera_lt_frame_grabber->acquisition == NULL ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "Unable to create a SapAcquisition object for frame grabber '%s'.",
			record->name );
	}

	sapera_lt_frame_grabber->buffer =
		new SapBuffer( 1, sapera_lt_frame_grabber->acquisition );

	if ( sapera_lt_frame_grabber->buffer == NULL ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "Unable to create a SapBuffer object for frame grabber '%s'.",
			record->name );
	}

	sapera_lt_frame_grabber->transfer =
		new SapAcqToBuf( sapera_lt_frame_grabber->acquisition,
				sapera_lt_frame_grabber->buffer,
			mxd_sapera_lt_frame_grabber_acquisition_callback,
				(void *) sapera_lt_frame_grabber );

	if ( sapera_lt_frame_grabber->transfer == NULL ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "Unable to create a SapAcqToBuf object for frame grabber '%s'.",
			record->name );
	}

	/* Create the low level Sapera resources. */

	sapera_status = sapera_lt_frame_grabber->acquisition->Create();

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: sapera_lt_frame_grabber->acquisition->Create() = %d",
		fname, (int) sapera_status));
#endif

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"There is something wrong with the configuration file '%s' "
		"used by frame grabber '%s'.",
			sapera_lt_frame_grabber->config_filename,
			record->name );
	}

	sapera_status = sapera_lt_frame_grabber->buffer->Create();

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: sapera_lt_frame_grabber->buffer->Create() = %d",
		fname, (int) sapera_status));
#endif

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Unable to create the low-level resources used by the "
		"SapBuffer object of frame grabber '%s'.",
			record->name );
	}

	sapera_status = sapera_lt_frame_grabber->transfer->Create();

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: sapera_lt_frame_grabber->transfer->Create() = %d",
		fname, (int) sapera_status));
#endif

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Unable to create the low-level resources used by the "
		"SapAcqToBuf object of frame grabber '%s'.",
			record->name );
	}

	char label[129];

	mx_status = mxd_sapera_lt_frame_grabber_get_lowlevel_parameter(
					sapera_lt_frame_grabber,
					-1, CORACQ_PRM_LABEL, label );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: '%s' label = '%s'",
		fname, sapera_lt_frame_grabber->record->name, label));
#endif

	/* Get the video parameters from the acquisition object. */

	mx_status = mx_video_input_get_framesize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize a bunch of MX driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->byte_order     = (long) mx_native_byteorder();
	vinput->trigger_mode   = MXT_IMAGE_INTERNAL_TRIGGER;

	vinput->last_frame_number = -1;
	vinput->total_num_frames = 0;
	vinput->status = 0x0;

	vinput->maximum_frame_number = 0;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,
	("%s: vinput->framesize[0] = %ld, vinput->framesize[1] = %ld",
		fname, vinput->framesize[0], vinput->framesize[1] ));

	MX_DEBUG(-2,("%s: vinput->image_format_name = '%s'",
		fname, vinput->image_format_name));

	MX_DEBUG(-2,("%s: vinput->image_format = %ld",
		fname, vinput->image_format));

	MX_DEBUG(-2,("%s: vinput->byte_order = %ld",
		fname, vinput->byte_order));

	MX_DEBUG(-2,("%s: vinput->trigger_mode = %ld",
		fname, vinput->trigger_mode));

	MX_DEBUG(-2,("%s: vinput->bits_per_pixel = %ld",
		fname, vinput->bits_per_pixel));

	MX_DEBUG(-2,("%s: vinput->bytes_per_pixel = %g",
		fname, vinput->bytes_per_pixel));

	MX_DEBUG(-2,("%s: vinput->bytes_per_frame = %ld",
		fname, vinput->bytes_per_frame));
#endif

	sapera_lt_frame_grabber->total_num_frames_at_start
					= vinput->total_num_frames;

	sapera_lt_frame_grabber->num_frames_left_to_acquire = 0;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_sapera_lt_frame_grabber_close()";

	MX_VIDEO_INPUT *vinput = NULL;
	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_sapera_lt_frame_grabber_get_pointers( vinput,
					&sapera_lt_frame_grabber, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sapera_lt_frame_grabber_arm()";

	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;
	MX_SEQUENCE_PARAMETERS *seq;
	long num_frames_in_sequence;
	BOOL sapera_status;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_frame_grabber_get_pointers( vinput,
					&sapera_lt_frame_grabber, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_ARM
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	/* Clear any existing frame data in the SapBuffer object. */

	sapera_status = sapera_lt_frame_grabber->buffer->Clear();

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to clear the capture buffers for "
		"frame grabber '%s' failed.",
			vinput->record->name );
	}

	/* For this video input, the input frame buffer must be set up
	 * before we can execute the grabber->Grab() method.  For that
	 * reason, we do mx_image_alloc() here to make sure that the
	 * frame is set up.
	 */

	mx_status = mx_image_alloc( &(vinput->frame),
					vinput->framesize[0],
					vinput->framesize[1],
					vinput->image_format,
					vinput->byte_order,
					vinput->bytes_per_pixel,
					MXT_IMAGE_HEADER_LENGTH_IN_BYTES,
					vinput->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_ARM
	MX_DEBUG(-2,("%s: Prepare for trigger mode %d",
		fname, vinput->trigger_mode ));
#endif

	if ( (vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) == 0 ) {

		/* If external triggering is not enabled,
		 * return without doing anything further.
		 */

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_ARM
		MX_DEBUG(-2,
		("%s: external trigger disabled for video input '%s'",
			fname, vinput->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we are doing external triggering. */

	seq = &(vinput->sequence_parameters);

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_ARM
	MX_DEBUG(-2,("%s: Prepare for sequence type %d",
		fname, seq->sequence_type));
#endif

	mx_status = mxd_sapera_lt_frame_grabber_setup_frame_counters(
					vinput, sapera_lt_frame_grabber );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sapera_lt_frame_grabber_trigger()";

	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;
	MX_IMAGE_FRAME *frame;
	BOOL sapera_status;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_frame_grabber_get_pointers( vinput,
					&sapera_lt_frame_grabber, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_TRIGGER
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	if ( ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) == 0 ) {

		/* If internal triggering is not enabled,
		 * return without doing anything.
		 */

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_TRIGGER
		MX_DEBUG(-2,
		("%s: internal trigger disabled for video input '%s'",
			fname, vinput->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we are doing internal triggering. */

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_TRIGGER
	MX_DEBUG(-2,("%s: Sending internal trigger for '%s'.",
		fname, vinput->record->name));
#endif
	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"No image frame has been allocated for video input '%s'.",
			vinput->record->name );
	}

	mx_status = mxd_sapera_lt_frame_grabber_setup_frame_counters(
					vinput, sapera_lt_frame_grabber );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	int num_frames = 1;

	sapera_status = sapera_lt_frame_grabber->transfer->Snap( num_frames );

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to trigger frame grabber '%s' failed.",
			vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sapera_lt_frame_grabber_abort()";

	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;
	BOOL sapera_status;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_frame_grabber_get_pointers( vinput,
					&sapera_lt_frame_grabber, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	sapera_status = sapera_lt_frame_grabber->transfer->Abort();

	if ( sapera_lt_frame_grabber->num_frames_left_to_acquire > 0 ) {
		sapera_lt_frame_grabber->num_frames_left_to_acquire = 0;
	}

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to abort frame grabber '%s' failed.",
			vinput->record->name );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mxd_sapera_lt_frame_grabber_get_extended_status()";

	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;
	unsigned long timeout_ms = 1L;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_frame_grabber_get_pointers( vinput,
					&sapera_lt_frame_grabber, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput->status = 0;

	if ( sapera_lt_frame_grabber->num_frames_left_to_acquire > 0 ) {
		vinput->status |= MXSF_VIN_IS_BUSY;
	}

	vinput->last_frame_number = vinput->total_num_frames
		- sapera_lt_frame_grabber->total_num_frames_at_start - 1;

	if ( vinput->status & MXSF_VIN_IS_BUSY ) {
		vinput->busy = TRUE;
	} else {
		vinput->busy = FALSE;
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_EXTENDED_STATUS
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
		fname, vinput->last_frame_number, vinput->total_num_frames,
		vinput->status));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sapera_lt_frame_grabber_get_frame()";

	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;
	MX_IMAGE_FRAME *frame;
	void *mx_data_address;
	void *sapera_data_address;
	BOOL sapera_status;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_frame_grabber_get_pointers( vinput,
					&sapera_lt_frame_grabber, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	/* Get the address of the MX image frame buffer. */

	if ( vinput->frame == (	MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No image frames have been acquired yet "
		"for frame grabber '%s'.",
			vinput->record->name );
	}

	mx_data_address = vinput->frame->image_data;

	/* Get the SapBuffer's address for the buffer data. */

	sapera_status = sapera_lt_frame_grabber->buffer->GetAddress(
							&sapera_data_address );

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to get the buffer data address for "
		"frame grabber '%s' failed.",
			vinput->record->name );
	}

	/* Transfer the data from the Sapera buffer to the MX buffer. */

	memcpy( mx_data_address, sapera_data_address,
			vinput->frame->image_length );

	/* Release the Sapera buffer address. */

	sapera_status = sapera_lt_frame_grabber->buffer->ReleaseAddress(
							&sapera_data_address );

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to get the buffer data address for "
		"frame grabber '%s' failed.",
			vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_sapera_lt_frame_grabber_get_parameter()";

	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;
	UINT32 pixels_per_line, lines_per_frame, output_format;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_frame_grabber_get_pointers( vinput,
					&sapera_lt_frame_grabber, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		mx_status = mxd_sapera_lt_frame_grabber_get_lowlevel_parameter(
						sapera_lt_frame_grabber,
						-1,
						CORACQ_PRM_CROP_WIDTH,
						&pixels_per_line );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_sapera_lt_frame_grabber_get_lowlevel_parameter(
						sapera_lt_frame_grabber,
						-1,
						CORACQ_PRM_CROP_HEIGHT,
						&lines_per_frame );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		vinput->framesize[0] = pixels_per_line;
		vinput->framesize[1] = lines_per_frame;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG
		MX_DEBUG(-2,("%s: frame grabber '%s' framesize = (%ld,%ld).",
			fname, sapera_lt_frame_grabber->record->name,
			vinput->framesize[0], vinput->framesize[1]));
#endif

		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		mx_status = mxd_sapera_lt_frame_grabber_get_lowlevel_parameter(
						sapera_lt_frame_grabber,
						-1, CORACQ_PRM_OUTPUT_FORMAT,
						&output_format );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( output_format ) {
		case CORACQ_VAL_OUTPUT_FORMAT_MONO8:
			vinput->image_format = MXT_IMAGE_FORMAT_GREY8;
			vinput->bytes_per_pixel = 1;
			vinput->bits_per_pixel  = 8;
			break;

		case CORACQ_VAL_OUTPUT_FORMAT_MONO16:
			vinput->image_format = MXT_IMAGE_FORMAT_GREY16;
			vinput->bytes_per_pixel = 2;
			vinput->bits_per_pixel  = 16;
			break;
		
		case CORACQ_VAL_OUTPUT_FORMAT_MONO32:
			vinput->image_format = MXT_IMAGE_FORMAT_GREY32;
			vinput->bytes_per_pixel = 4;
			vinput->bits_per_pixel  = 32;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported output format %lu for video input '%s'.",
				(unsigned long) output_format,
				vinput->record->name );
			break;
		}

		mx_status = mx_image_get_image_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_MX_PARAMETERS
		MX_DEBUG(-2,("%s: video format = %ld, format name = '%s'",
		    fname, vinput->image_format, vinput->image_format_name));
#endif
		break;

	case MXLV_VIN_BYTE_ORDER:
		vinput->byte_order = (long) mx_native_byteorder();
		break;

	case MXLV_VIN_TRIGGER_MODE:
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel
			* vinput->framesize[0] * vinput->framesize[1] );
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
		break;

	case MXLV_VIN_BUSY:
		vinput->busy = 0;
		break;

	case MXLV_VIN_STATUS:
		vinput->status = 0;
		break;

	case MXLV_VIN_SEQUENCE_TYPE:
		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		break;
	default:
		mx_status =
			mx_video_input_default_get_parameter_handler( vinput );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_sapera_lt_frame_grabber_set_parameter()";

	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;
	unsigned long bytes_per_frame;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_frame_grabber_get_pointers( vinput,
					&sapera_lt_frame_grabber, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

#if 0
	CyGrabber *grabber = sapera_lt_frame_grabber->grabber;

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		grabber->SetParameter( CY_GRABBER_PARAM_SIZE_X,
						vinput->framesize[0] );
		grabber->SetParameter( CY_GRABBER_PARAM_SIZE_Y,
						vinput->framesize[1] );
		grabber->SaveConfig();
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the image format is not supported for "
			"video input '%s'.", vinput->record->name );

	case MXLV_VIN_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the byte order for video input '%s' "
			"is not supported.", vinput->record->name );

	case MXLV_VIN_TRIGGER_MODE:
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		/* The documentation for the CyUserBuffer class states that
		 * the image buffer size must be a multiple of 4 bytes.
		 */

		bytes_per_frame = vinput->bytes_per_frame;

		if ( (bytes_per_frame % 4) != 0 ) {
			bytes_per_frame += ( 4 - (bytes_per_frame % 4) );

			mx_warning( "The bytes per frame of video input '%s' "
			"was rounded up to %lu so that it is a multiple of 4.",
				vinput->record->name,
				bytes_per_frame );
		}

		grabber->SetParameter( CY_GRABBER_PARAM_IMAGE_SIZE,
						bytes_per_frame );
		grabber->SaveConfig();
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
		grabber->SetParameter( CY_GRABBER_PARAM_PIXEL_DEPTH,
				mx_round(8.0 * vinput->bytes_per_pixel) );
		grabber->SaveConfig();
		break;

	case MXLV_VIN_BITS_PER_PIXEL:
		grabber->SetParameter( CY_GRABBER_PARAM_PIXEL_DEPTH,
						vinput->bits_per_pixel );
		grabber->SaveConfig();
		break;

	case MXLV_VIN_SEQUENCE_TYPE:
		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		break;

	default:
		mx_status =
			mx_video_input_default_set_parameter_handler( vinput );
		break;
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

