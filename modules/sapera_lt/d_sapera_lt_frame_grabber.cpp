/*
 * Name:    d_sapera_lt_frame_grabber.c
 *
 * Purpose: MX video input driver for a DALSA Sapera LT frame grabber.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG			FALSE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN			TRUE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_ARM			TRUE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_TRIGGER		TRUE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_GET_FRAME		TRUE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_EXTENDED_STATUS	TRUE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_CALLBACK		TRUE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS	FALSE

#define MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_MX_PARAMETERS		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(OS_WIN32)
#  include <io.h>
#endif

#include "mx_util.h"
#include "mx_record.h"
#include "mx_unistd.h"
#include "mx_array.h"
#include "mx_bit.h"
#include "mx_memory.h"
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

	if ( sapera_lt_frame_grabber->num_frames_left_to_acquire > 0 ) {
		sapera_lt_frame_grabber->num_frames_left_to_acquire--;
		vinput->total_num_frames++;
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_CALLBACK
	{
		uint32_t divisor, remainder, result;
		char ascii_digit;
		mx_bool_type suppress_output;

		/* We do not necessarily know what context this callback
		 * is invoked from, so we use write() which is invokable
		 * from pretty much anywhere.
		 */

		write( 2, "CAPTURE: Total num frames = ", 28 );

		remainder = vinput->total_num_frames;

		divisor = 1000000000L;

		suppress_output = TRUE;

		while( divisor > 0 ) {

			result = remainder / divisor;

			if ( suppress_output ) {
				if ( result != 0 ) {
					suppress_output = FALSE;
				}
			}

			if ( suppress_output == FALSE ) {
				ascii_digit = '0' + result;

				write( 2, &ascii_digit, 1 );
			}

			remainder = remainder % divisor;

			divisor /= 10L;
		}

		write( 2, "\n", 1 );
	}
#endif /* MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_CALLBACK */

	return;
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

/*--------------------------------------------------------------------------*/

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
		    "Capability %#x is not available for frame grabber '%s'.",
		    capability, sapera_lt_frame_grabber->record->name );
	    }
	}

	parameter_valid = acq->IsParameterValid( parameter );

	if ( parameter_valid == FALSE ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"Parameter %#x is not available for frame grabber '%s'.",
			parameter, sapera_lt_frame_grabber->record->name );
	}

	get_parameter_status = acq->GetParameter( parameter, parameter_value );

	if ( get_parameter_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to read parameter %#x from "
			"frame grabber '%s' failed.",
			parameter, sapera_lt_frame_grabber->record->name );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS
	MX_DEBUG(-2,("%s: Frame grabber '%s', parameter %#x = %#0x",
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
		    "Capability %#x is not available for frame grabber '%s'.",
		    capability, sapera_lt_frame_grabber->record->name );
	    }
	}

	parameter_valid = acq->IsParameterValid( parameter );

	if ( parameter_valid == FALSE ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"Parameter %#x is not available for frame grabber '%s'.",
			parameter, sapera_lt_frame_grabber->record->name );
	}

	get_parameter_status = acq->GetParameter( parameter, parameter_value );

	if ( get_parameter_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to read parameter %#x from "
			"frame grabber '%s' failed.",
			parameter, sapera_lt_frame_grabber->record->name );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS
	MX_DEBUG(-2,("%s: Frame grabber '%s', parameter %#x = %lu",
		fname, sapera_lt_frame_grabber->record->name,
		parameter, (unsigned long) *parameter_value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

/* This version is for writing memory addresses. */

static mx_status_type
mxd_sapera_lt_frame_grabber_set_lowlevel_parameter(
			MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber,
			int capability,
			int parameter,
			void *parameter_value,
			BOOL update_now = TRUE )
{
	static const char fname[] =
		"mxd_sapera_lt_frame_grabber_set_lowlevel_parameter()";

	BOOL capability_valid, parameter_valid, set_parameter_status;

	if ( sapera_lt_frame_grabber == ( MX_SAPERA_LT_FRAME_GRABBER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SAPERA_LT_FRAME_GRABBER pointer passed was NULL." );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS
	MX_DEBUG(-2,("%s invoked by '%s' for writing a void pointer.",
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
		    "Capability %#x is not available for frame grabber '%s'.",
		    capability, sapera_lt_frame_grabber->record->name );
	    }
	}

	parameter_valid = acq->IsParameterValid( parameter );

	if ( parameter_valid == FALSE ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"Parameter %#x is not available for frame grabber '%s'.",
			parameter, sapera_lt_frame_grabber->record->name );
	}

	set_parameter_status = acq->SetParameter( parameter,
						parameter_value, update_now );

	if ( set_parameter_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to write %p to parameter %#x of "
			"frame grabber '%s' failed.",
			parameter_value, parameter,
			sapera_lt_frame_grabber->record->name );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS
	MX_DEBUG(-2,("%s: Frame grabber '%s', parameter %#x = %#0x",
		fname, sapera_lt_frame_grabber->record->name,
		parameter, parameter_value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/* This version is for writing 32-bit integers. */

static mx_status_type
mxd_sapera_lt_frame_grabber_set_lowlevel_parameter(
			MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber,
			int capability,
			int parameter,
			UINT32 parameter_value,
			BOOL update_now = TRUE )
{
	static const char fname[] =
		"mxd_sapera_lt_frame_grabber_set_lowlevel_parameter()";

	BOOL capability_valid, parameter_valid, set_parameter_status;

	if ( sapera_lt_frame_grabber == ( MX_SAPERA_LT_FRAME_GRABBER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SAPERA_LT_FRAME_GRABBER pointer passed was NULL." );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS
	MX_DEBUG(-2,("%s invoked by '%s' for writing a UINT32 value.",
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
		    "Capability %#x is not available for frame grabber '%s'.",
		    capability, sapera_lt_frame_grabber->record->name );
	    }
	}

	parameter_valid = acq->IsParameterValid( parameter );

	if ( parameter_valid == FALSE ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"Parameter %#x is not available for frame grabber '%s'.",
			parameter, sapera_lt_frame_grabber->record->name );
	}

	set_parameter_status = acq->SetParameter( parameter,
						parameter_value, update_now );

	if ( set_parameter_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to write %d to parameter %#x of "
			"frame grabber '%s' failed.",
			parameter_value, parameter,
			sapera_lt_frame_grabber->record->name );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS
	MX_DEBUG(-2,("%s: Frame grabber '%s', parameter %#x = %lu",
		fname, sapera_lt_frame_grabber->record->name,
		parameter, (unsigned long) parameter_value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_sapera_lt_frame_grabber_get_capability(
			MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber,
			int capability,
			UINT32 *capability_value )
{
	static const char fname[] =
		"mxd_sapera_lt_frame_grabber_get_capability()";

	BOOL sapera_status;

	if ( sapera_lt_frame_grabber == ( MX_SAPERA_LT_FRAME_GRABBER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SAPERA_LT_FRAME_GRABBER pointer passed was NULL." );
	}
	if ( capability_value == (UINT32 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The capability_value pointer passed was NULL." );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_LOWLEVEL_PARAMETERS
	MX_DEBUG(-2,("%s invoked by '%s' for writing a UINT32 value.",
		fname, sapera_lt_frame_grabber->record->name));
#endif

	SapAcquisition *acq = sapera_lt_frame_grabber->acquisition;

	if ( acq == (SapAcquisition *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The SapAcquisition pointer for frame grabber '%s' is NULL.",
			sapera_lt_frame_grabber->record->name );
	}

	sapera_status = acq->GetCapability( capability, capability_value );

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		    "Capability %#x is not available for frame grabber '%s'.",
		    capability, sapera_lt_frame_grabber->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

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
	MX_SYSTEM_MEMINFO system_meminfo;
	UINT32 min_freq_millihz, max_freq_millihz;
	BOOL sapera_status;
	long bytes_per_frame, max_image_frames, max_frames_threshold;
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

	/*---------------------------------------------------------------*/

	/* Create the high level SapAcquisition object.
	 * 
	 * Soon we will call the functions mx_video_input_get_framesize()
	 * and mx_video_input_get_bytes_per_frame().  Those two functions
	 * depend on the presence of an initialized SapAcquisition object,
	 * so we must create that object before calling the video input
	 * functions below.
	 */

	SapLocation location( sapera_lt->server_name,
				sapera_lt_frame_grabber->frame_grabber_number );

	sapera_lt_frame_grabber->acquisition = new SapAcquisition( location,
				sapera_lt_frame_grabber->config_filename );

	if ( sapera_lt_frame_grabber->acquisition == NULL ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "Unable to create a SapAcquisition object for frame grabber '%s'.",
			record->name );
	}

	/* -------- */

	sapera_status = sapera_lt_frame_grabber->acquisition->Create();

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: sapera_lt_frame_grabber->acquisition->Create() = %d",
		fname, (int) sapera_status));
#endif

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Unable to read from the configuration file '%s' "
		"used by frame grabber '%s'.",
			sapera_lt_frame_grabber->config_filename,
			record->name );
	}

	/*---------------------------------------------------------------*/

	/* Check to see if enough free memory is available to allocate all
	 * of the frame buffers that we have requested to use.
	 */

	mx_status = mx_get_system_meminfo( &system_meminfo );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	mx_display_system_meminfo( &system_meminfo );
#endif

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: available free memory = %lu",
		fname, system_meminfo.free_bytes));
#endif
	/* Invoke some video input initialization steps here that we
	 * will need in order to compute the maximum allowed number of
	 * image frames below.
	 */

	mx_status = mx_video_input_get_framesize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( record,
							&bytes_per_frame );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: bytes_per_frame = %ld", fname, bytes_per_frame ));
#endif

	/* Approximately how many image frame buffers will fit into
	 * the available free memory?
	 */

	max_image_frames = mx_round(
		mx_divide_safely( system_meminfo.free_bytes, bytes_per_frame ));

	max_frames_threshold = mx_round( 0.95 * max_image_frames );

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: max_image_frames = %lu, max_frames_threshold = %lu",
			fname, max_image_frames, max_frames_threshold));
#endif
	if ( sapera_lt_frame_grabber->max_frames > max_frames_threshold ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The maximum number of frames (%ld) requested for "
		"frame grabber '%s' is larger than the maximum number of "
		"frames (%ld) that can fit in the available computer memory.",
			sapera_lt_frame_grabber->max_frames,
			record->name,
			max_frames_threshold );
	}

	/*---------------------------------------------------------------*/

	/* Create the SapBuffer object which contains the raw image data
	 * from all of the frames we take in a sequence.
	 */

	sapera_lt_frame_grabber->buffer =
		new SapBuffer( sapera_lt_frame_grabber->max_frames,
				sapera_lt_frame_grabber->acquisition );

	if ( sapera_lt_frame_grabber->buffer == NULL ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Unable to create a %ld frame SapBuffer object for "
		"frame grabber '%s'.",
			sapera_lt_frame_grabber->max_frames, record->name );
	}

	/* -------- */

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

#if ( 1 && MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN )
	{
		SapBuffer *buffer;
		int pixel_depth, num_buffers, width, height;

		buffer = sapera_lt_frame_grabber->buffer;

		pixel_depth = buffer->GetPixelDepth();
		num_buffers = buffer->GetCount();
		width       = buffer->GetWidth();
		height      = buffer->GetHeight();

		MX_DEBUG(-2,("%s: SapBuffer pixel_depth = %d, num_buffers = %d",
			fname, pixel_depth, num_buffers));

		MX_DEBUG(-2,("%s: SapBuffer width = %d, height = %d",
			fname, width, height));
	}
#endif

#if 1
	/* Verify that the image buffers created by the SapBuffer object
	 * are long enough to contain the MX images.
	 */

	{
		int sapera_buffer_is_valid;
		void *sapera_data_address;

		mx_status = MX_SUCCESSFUL_RESULT;

		/* Get the address of the first buffer. */

		sapera_status = sapera_lt_frame_grabber->buffer->GetAddress(
						0, &sapera_data_address );

		if ( sapera_status == FALSE ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to get the buffer data address for "
			"frame grabber '%s' failed.", record->name );
		}

		sapera_buffer_is_valid =
			mx_pointer_is_valid( sapera_data_address,
				bytes_per_frame, R_OK );

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN 
		MX_DEBUG(-2,("%s: sapera_buffer_is_valid = %d",
			fname, sapera_buffer_is_valid));
#endif

		if ( sapera_buffer_is_valid == FALSE ) {
			mx_status = mx_error(
				MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
			"The image buffer returned by buffer->GetAddress() "
			"is not long enough to hold the number of bytes (%ld) "
			"in an MX image for detector '%s'.",
				bytes_per_frame, record->name );
		}

		/* Release the buffer. */

		sapera_status = sapera_lt_frame_grabber->buffer->ReleaseAddress(
							&sapera_data_address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( sapera_status == FALSE ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to release the Sapera data buffer "
			"failed for detector '%s'.", record->name );
		}
	}
#endif

	/*---------------------------------------------------------------*/

	/* Create a SapAcqToBuf object.  This object manages the transfer
	 * of data from the frame grabber to the SapBuffer object that
	 * created just above.
	 */

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

	/* -------- */

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

	/*---------------------------------------------------------------*/

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
	/* Get the minimum and maximum exposure_times. */

	mx_status = mxd_sapera_lt_frame_grabber_get_capability(
					sapera_lt_frame_grabber,
					CORACQ_CAP_INT_FRAME_TRIGGER_FREQ_MAX,
					&max_freq_millihz );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sapera_lt_frame_grabber->minimum_exposure_time =
		mx_divide_safely( 1000.0, max_freq_millihz );

	mx_status = mxd_sapera_lt_frame_grabber_get_capability(
					sapera_lt_frame_grabber,
					CORACQ_CAP_INT_FRAME_TRIGGER_FREQ_MIN,
					&min_freq_millihz );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sapera_lt_frame_grabber->maximum_exposure_time =
		mx_divide_safely( 1000.0, min_freq_millihz );

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: frame grabber '%s' max_freq_millihz = %lu",
		fname, record->name, (unsigned long) max_freq_millihz ));

	MX_DEBUG(-2,("%s: frame grabber '%s' min_freq_millihz = %lu",
		fname, record->name, (unsigned long) min_freq_millihz ));

	MX_DEBUG(-2,("%s: frame grabber '%s' minimum exposure_time = %g",
		fname, record->name,
		sapera_lt_frame_grabber->minimum_exposure_time ));

	MX_DEBUG(-2,("%s: frame grabber '%s' maximum exposure_time = %g",
		fname, record->name,
		sapera_lt_frame_grabber->maximum_exposure_time ));
#endif
	/*---------------------------------------------------------------*/

	/* Does this frame grabber support internal triggers? */

	UINT32 internal_trigger_available;

	mx_status = mxd_sapera_lt_frame_grabber_get_capability(
						sapera_lt_frame_grabber,
						CORACQ_CAP_INT_FRAME_TRIGGER,
						&internal_trigger_available );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( internal_trigger_available ) {
		sapera_lt_frame_grabber->internal_trigger_available = TRUE;
	} else {
		sapera_lt_frame_grabber->internal_trigger_available = FALSE;
	}

	/* Does this frame grabber support external triggers? */

	UINT32 external_trigger_available;

	mx_status = mxd_sapera_lt_frame_grabber_get_capability(
						sapera_lt_frame_grabber,
						CORACQ_CAP_EXT_TRIGGER,
						&external_trigger_available );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( external_trigger_available ) {
		sapera_lt_frame_grabber->external_trigger_available = TRUE;
	} else {
		sapera_lt_frame_grabber->external_trigger_available = FALSE;
	}

	/* Does this frame grabber support software triggers? */

	UINT32 software_trigger_available;

	mx_status = mxd_sapera_lt_frame_grabber_get_capability(
						sapera_lt_frame_grabber,
						CORACQ_CAP_SOFTWARE_TRIGGER,
						&software_trigger_available );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( software_trigger_available ) {
		sapera_lt_frame_grabber->software_trigger_available = TRUE;
	} else {
		sapera_lt_frame_grabber->software_trigger_available = FALSE;
	}

	/* Does this frame grabber support acquisition of multiple frames
	 * on one external trigger?
	 */

	UINT32 multiframe_trigger_available;

	mx_status = mxd_sapera_lt_frame_grabber_get_capability(
					sapera_lt_frame_grabber,
					CORACQ_CAP_EXT_TRIGGER_FRAME_COUNT,
					&multiframe_trigger_available );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( multiframe_trigger_available ) {
		sapera_lt_frame_grabber->multiframe_trigger_available = TRUE;
	} else {
		sapera_lt_frame_grabber->multiframe_trigger_available = FALSE;
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: Sapera LT frame grabber '%s'.\n"
			"    internal_trigger_available = %d\n"
			"    external_trigger_available = %d\n"
			"    software_trigger_available = %d\n"
			"    multiframe_trigger_available = %d",
		fname, record->name,
		(int) sapera_lt_frame_grabber->internal_trigger_available,
		(int) sapera_lt_frame_grabber->external_trigger_available,
		(int) sapera_lt_frame_grabber->software_trigger_available,
		(int) sapera_lt_frame_grabber->multiframe_trigger_available ));
#endif

#if 1
	/* This tells the MX driver to handle "internal" triggers by
	 * doing a software trigger of the external trigger interface.
	 */

	sapera_lt_frame_grabber->use_software_trigger = TRUE;
#endif

	/*---------------------------------------------------------------*/

	UINT32 time_integrate_available;

	mx_status = mxd_sapera_lt_frame_grabber_get_capability(
					sapera_lt_frame_grabber,
					CORACQ_CAP_TIME_INTEGRATE,
					&time_integrate_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: Time Integrate available = %d",
		fname, time_integrate_available));
#endif

	/*---------------------------------------------------------------*/

	UINT32 supported_methods;

	mx_status = mxd_sapera_lt_frame_grabber_get_capability(
					sapera_lt_frame_grabber,
					CORACQ_CAP_TIME_INTEGRATE_METHOD,
					&supported_methods );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: Time Integrate supported methods = %#lx",
		fname, supported_methods ));
#endif

	/*---------------------------------------------------------------*/

	/* Tell the frame grabber to use Time Integrate method 2.  For
	 * this method, the integration time starts on the trailing edge
	 * of a start trigger pulse and ends at the trailing edge of
	 * an end trigger pulse.
	 */

	mx_status = mxd_sapera_lt_frame_grabber_set_lowlevel_parameter(
				sapera_lt_frame_grabber,
				-1, CORACQ_PRM_TIME_INTEGRATE_METHOD,
				CORACQ_VAL_TIME_INTEGRATE_METHOD_2 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: Selected Time Integration method 2.", fname));
#endif
	/*---------------------------------------------------------------*/

	/* Tell the frame grabber to use the Time Integrate
	 * camera control method.
	 */

	mx_status = mxd_sapera_lt_frame_grabber_set_lowlevel_parameter(
				sapera_lt_frame_grabber,
				-1, CORACQ_PRM_TIME_INTEGRATE_ENABLE,
				(UINT32) TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_OPEN
	MX_DEBUG(-2,("%s: Time Integration enabled.", fname));
#endif
	/*---------------------------------------------------------------*/

	/* Initialize the video parameters. */

	mx_status = mx_video_input_get_framesize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;

	mx_status = mx_video_input_set_trigger_mode( record,
						MXT_IMAGE_INTERNAL_TRIGGER );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize a bunch of MX driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->byte_order     = (long) mx_native_byteorder();

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
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time;
	int num_frames;
	UINT32 exposure_time_in_microsec;
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

	mx_status = mxd_sapera_lt_frame_grabber_setup_frame_counters(
					vinput, sapera_lt_frame_grabber );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear any existing frame data in the SapBuffer object. */

	sapera_status = sapera_lt_frame_grabber->buffer->Clear();

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to clear the capture buffers for "
		"frame grabber '%s' failed.",
			vinput->record->name );
	}

	/* For this video input, the input frame buffer must be set up
	 * before we can execute the Snap() method.  For that reason,
	 * we do mx_image_alloc() here to make sure that the frame
	 * is set up.
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

	/* Figure out the number of frames and the exposure time
	 * for the upcoming imaging sequence.
	 */

	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		break;
	case MXT_SQ_MULTIFRAME:
		num_frames = sp->parameter_array[0];
		exposure_time = sp->parameter_array[1];

		/* sp->parameter_array[2] contains a "frame time"
		 * which is the time interval between the start
		 * of two successive frames.  However, Sapera LT
		 * does not seem to provide a way to use this,
		 * so we just skip it here.
		 */
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Sequence type %ld has not yet been implemented for '%s'.",
			sp->sequence_type, vinput->record->name );
		break;
	}

	if ( num_frames > sapera_lt_frame_grabber->max_frames ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The number of frames requested (%d) for this sequence "
		"exceeds the maximum number of frames (%ld) allocated "
		"for detector '%s'.",
			num_frames,
			sapera_lt_frame_grabber->max_frames,
			vinput->record->name );
	}

	if (( exposure_time < sapera_lt_frame_grabber->minimum_exposure_time )
	 || ( exposure_time > sapera_lt_frame_grabber->maximum_exposure_time ))
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested exposure time for frame grabber '%s' "
		"is outside the allowed range of %g seconds to %g seconds.",
			vinput->record->name,
			sapera_lt_frame_grabber->minimum_exposure_time,
			sapera_lt_frame_grabber->maximum_exposure_time );
	}

	/* Specify how long the integration time is supposed to be. */

	exposure_time_in_microsec = mx_round( 1.0e6 * (double) exposure_time );

	mx_status = mxd_sapera_lt_frame_grabber_set_lowlevel_parameter(
				sapera_lt_frame_grabber,
				-1, CORACQ_PRM_TIME_INTEGRATE_DURATION,
				exposure_time_in_microsec );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Specify the number of frames to acquire after a trigger. */

	mx_status = mxd_sapera_lt_frame_grabber_set_lowlevel_parameter(
				sapera_lt_frame_grabber,
				-1, CORACQ_PRM_EXT_TRIGGER_FRAME_COUNT,
				(UINT32) num_frames );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*--------*/

	/* Tell the frame grabber to wait for incoming image frames. */

	sapera_status = sapera_lt_frame_grabber->transfer->Snap( num_frames );

	if ( sapera_status == FALSE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to call Snap() for frame grabber '%s' failed.",
			vinput->record->name );
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_ARM
	MX_DEBUG(-2,("%s complete for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sapera_lt_frame_grabber_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sapera_lt_frame_grabber_trigger()";

	MX_SAPERA_LT_FRAME_GRABBER *sapera_lt_frame_grabber = NULL;
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

	if ( sapera_lt_frame_grabber->use_software_trigger ) {

		SapAcquisition *acq = sapera_lt_frame_grabber->acquisition;

		sapera_status = 
		    acq->SoftwareTrigger( SapAcquisition::SoftwareTriggerExt );

		if ( sapera_status == FALSE ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to send a software trigger to "
			"frame grabber '%s' failed.",
				vinput->record->name );
		}
	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Internal triggering is not yet implemented." );
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
	unsigned long absolute_frame_number;
	unsigned long modulo_frame_number;
	int buffer_resource_index;
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

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_GET_FRAME
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

	/* The SapBuffer appears to behave as a ring buffer (no surprise),
	 * so the offset we pass to GetAddress must be the absolute frame
	 * number modulo the maximum number of frames in the buffer.
	 */

	absolute_frame_number =
		sapera_lt_frame_grabber->total_num_frames_at_start
			+ vinput->frame_number;

	modulo_frame_number =
		absolute_frame_number % (sapera_lt_frame_grabber->max_frames);

	buffer_resource_index = (int) modulo_frame_number;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s: total_num_frames_at_start = %lu, frame_number = %ld",
		fname, sapera_lt_frame_grabber->total_num_frames_at_start,
		vinput->frame_number));

	MX_DEBUG(-2,("%s: absolute_frame_number = %lu, max_frames = %ld",
		fname, absolute_frame_number,
		sapera_lt_frame_grabber->max_frames));

	MX_DEBUG(-2,("%s: buffer_resource_index = %d",
		fname, buffer_resource_index));

	MX_DEBUG(-2,("%s: image_length = %ld",
		fname, vinput->frame->image_length));
#endif
	/* Get the address. */

	sapera_status = sapera_lt_frame_grabber->buffer->GetAddress(
							buffer_resource_index,
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

	/* Update the exposure time in the image header. */

	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time;

	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		exposure_time = sp->parameter_array[0];
		break;
	case MXT_SQ_MULTIFRAME:
		exposure_time = sp->parameter_array[1];
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Sequence type %ld has not yet been implemented for '%s'.",
			sp->sequence_type, vinput->record->name );
		break;
	}

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s: exposure_time = %f", fname, exposure_time));
#endif

	struct timespec exposure_timespec =
	    mx_convert_seconds_to_high_resolution_time( exposure_time );

	MXIF_EXPOSURE_TIME_SEC( vinput->frame )  = exposure_timespec.tv_sec;

	MXIF_EXPOSURE_TIME_NSEC( vinput->frame ) = exposure_timespec.tv_nsec;

	/*----*/

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s complete.", fname));
#endif

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
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
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
		/* For this, we just return the values set by an earlier
		 * ..._set_parameter() call.
		 */
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
	case MXLV_VIN_BYTES_PER_PIXEL:
	case MXLV_VIN_BITS_PER_PIXEL:
		vinput->bytes_per_pixel = 2;
		vinput->bits_per_pixel = 16;

		vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel
			* vinput->framesize[0] * vinput->framesize[1] );
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
	mx_bool_type internal_trigger_enabled, external_trigger_enabled;
	UINT32 external_trigger_setting;
	mx_status_type mx_status;

	mx_status = mxd_sapera_lt_frame_grabber_get_pointers( vinput,
					&sapera_lt_frame_grabber, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SAPERA_LT_FRAME_GRABBER_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Changing the framesize is not yet implemented for '%s'.",
			vinput->record->name );
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the image format is not supported for "
			"video input '%s'.", vinput->record->name );
		break;

	case MXLV_VIN_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the byte order for video input '%s' "
			"is not supported.", vinput->record->name );
		break;

	case MXLV_VIN_TRIGGER_MODE:
		if ( sapera_lt_frame_grabber->use_software_trigger ) {

			internal_trigger_enabled = FALSE;
			external_trigger_enabled = TRUE;
		} else {
			if ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER )
			{
				internal_trigger_enabled = TRUE;
			} else {
				internal_trigger_enabled = FALSE;
			}

			if ( vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER )
			{
				external_trigger_enabled = TRUE;
			} else {
				external_trigger_enabled = FALSE;
			}
		}

		/* Configure internal triggering. */

		if ( sapera_lt_frame_grabber->internal_trigger_available ) {
			mx_status =
			    mxd_sapera_lt_frame_grabber_set_lowlevel_parameter(
					sapera_lt_frame_grabber,
					-1, CORACQ_PRM_INT_FRAME_TRIGGER_ENABLE,
					(UINT32) internal_trigger_enabled );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		} else {
			if ( internal_trigger_enabled ) {
				mx_warning(
				"Internal trigger mode was requested for "
				"Sapera LT frame grabber '%s', but that frame "
				"grabber does not support internal triggering.",
					vinput->record->name );

				vinput->trigger_mode
					&= (~MXT_IMAGE_INTERNAL_TRIGGER);

				internal_trigger_enabled = FALSE;
			}
		}

		/* Configure external triggering. */

		if ( sapera_lt_frame_grabber->external_trigger_available ) {
			if ( external_trigger_enabled ) {
				external_trigger_setting
					= CORACQ_VAL_EXT_TRIGGER_ON;
			} else {
				external_trigger_setting
					= CORACQ_VAL_EXT_TRIGGER_OFF;
			}

			mx_status =
			    mxd_sapera_lt_frame_grabber_set_lowlevel_parameter(
					sapera_lt_frame_grabber,
					-1, CORACQ_PRM_EXT_TRIGGER_ENABLE,
					external_trigger_setting );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		} else {
			if ( external_trigger_enabled ) {
				mx_warning(
				"External trigger mode was requested for "
				"Sapera LT frame grabber '%s', but that frame "
				"grabber does not support external triggering.",
					vinput->record->name );

				vinput->trigger_mode
					&= (~MXT_IMAGE_EXTERNAL_TRIGGER);

				external_trigger_enabled = FALSE;
			}
		}
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
		break;

	case MXLV_VIN_BITS_PER_PIXEL:
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

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

