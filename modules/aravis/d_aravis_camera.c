/*
 * Name:    d_aravis_camera.cpp
 *
 * Purpose: MX video input driver for a camera controlled by Aravis.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ARAVIS_CAMERA_DEBUG			TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
#include "mx_process.h"
#include "mx_thread.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_aravis.h"
#include "d_aravis_camera.h"

/* Vendor include files */

#include "arv.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_aravis_camera_record_function_list = {
	NULL,
	mxd_aravis_camera_create_record_structures,
	mxd_aravis_camera_finish_record_initialization,
	NULL,
	NULL,
	mxd_aravis_camera_open,
	mxd_aravis_camera_close,
	NULL,
	NULL,
	mxd_aravis_camera_special_processing_setup
};

MX_VIDEO_INPUT_FUNCTION_LIST
mxd_aravis_camera_video_input_function_list = {
	mxd_aravis_camera_arm,
	mxd_aravis_camera_trigger,
	mxd_aravis_camera_stop,
	mxd_aravis_camera_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_aravis_camera_get_extended_status,
	mxd_aravis_camera_get_frame,
	mxd_aravis_camera_get_parameter,
	mxd_aravis_camera_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_aravis_camera_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_ARAVIS_CAMERA_STANDARD_FIELDS
};

long mxd_aravis_camera_num_record_fields
	= sizeof( mxd_aravis_camera_record_field_defaults )
	/ sizeof( mxd_aravis_camera_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aravis_camera_rfield_def_ptr
			= &mxd_aravis_camera_record_field_defaults[0];

/*---*/

static mx_status_type mxd_aravis_camera_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );

static mx_status_type
mxd_aravis_camera_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_ARAVIS_CAMERA **aravis_camera,
			MX_ARAVIS **aravis,
			const char *calling_fname )
{
	static const char fname[] =
		"mxd_aravis_camera_get_pointers()";

	MX_ARAVIS_CAMERA *aravis_camera_ptr;
	MX_RECORD *aravis_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	aravis_camera_ptr = (MX_ARAVIS_CAMERA *)
				vinput->record->record_type_struct;

	if ( aravis_camera_ptr == (MX_ARAVIS_CAMERA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_ARAVIS_CAMERA pointer for "
			"record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}

	if ( aravis_camera != (MX_ARAVIS_CAMERA **) NULL ) {
		*aravis_camera = aravis_camera_ptr;
	}

	if ( aravis != (MX_ARAVIS **) NULL ) {
		aravis_record = aravis_camera_ptr->aravis_record;

		if ( aravis_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The aravis_record pointer for record '%s' "
			"is NULL.", vinput->record->name );
		}

		*aravis = (MX_ARAVIS *)
					aravis_record->record_type_struct;

		if ( *aravis == (MX_ARAVIS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_ARAVIS pointer for record '%s' used "
			"by record '%s' is NULL.",
				vinput->record->name,
				aravis_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aravis_camera_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_aravis_camera_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_ARAVIS_CAMERA *aravis_camera = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	aravis_camera = (MX_ARAVIS_CAMERA *)
				malloc( sizeof(MX_ARAVIS_CAMERA) );

	if ( aravis_camera == (MX_ARAVIS_CAMERA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_ARAVIS_CAMERA structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = aravis_camera;
	record->class_specific_function_list = 
			&mxd_aravis_camera_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	aravis_camera->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_aravis_camera_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	MX_ARAVIS *aravis = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
				&aravis_camera, &aravis, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_aravis_camera_open()";

	MX_VIDEO_INPUT *vinput;
	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	MX_ARAVIS *aravis = NULL;

	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXD_ARAVIS_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
				&aravis_camera, &aravis, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---------------------------------------------------------------*/

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_aravis_camera_close()";

	MX_VIDEO_INPUT *vinput = NULL;
	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_arm()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_ARM
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_trigger()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_stop()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_STOP
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_abort()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_STOP
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mxd_aravis_camera_get_extended_status()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput->status = 0;

	if ( vinput->status & MXSF_VIN_IS_BUSY ) {
		vinput->busy = TRUE;
	} else {
		vinput->busy = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_get_frame()";

	MX_ARAVIS_CAMERA *aravis = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*----*/

#if MXD_ARAVIS_CAMERA_DEBUG_GET_FRAME
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aravis_camera_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_aravis_camera_get_parameter()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:

#if MXD_ARAVIS_CAMERA_DEBUG_MX_PARAMETERS
		MX_DEBUG(-2,("%s: camera '%s' framesize = (%ld,%ld).",
			fname, aravis_camera->record->name,
			vinput->framesize[0], vinput->framesize[1]));
#endif

		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		/* FIXME: The 'PixelFormat' enum may have the value we want,
		 *        but for now, we hard code the response.
		 */

		vinput->image_format = MXT_IMAGE_FORMAT_GREY16;
		vinput->bytes_per_pixel = 2;
		vinput->bits_per_pixel = 16;

		mx_status = mx_image_get_image_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );

#if MXD_ARAVIS_CAMERA_DEBUG_MX_PARAMETERS
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
mxd_aravis_camera_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_aravis_camera_set_parameter()";

	MX_ARAVIS_CAMERA *aravis_camera = NULL;
	mx_status_type mx_status;

	mx_status = mxd_aravis_camera_get_pointers( vinput,
					&aravis_camera, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ARAVIS_CAMERA_DEBUG_MX_PARAMETERS
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, vinput->record->name,
	mx_get_field_label_string( vinput->record, vinput->parameter_type ),
			vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_MARK_FRAME_AS_SAVED:

		break;

	case MXLV_VIN_FRAMESIZE:
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		break;

	case MXLV_VIN_BYTE_ORDER:
		break;

	case MXLV_VIN_TRIGGER_MODE:
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

MX_EXPORT mx_status_type
mxd_aravis_camera_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case 888:
			record_field->process_function
				= mxd_aravis_camera_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_aravis_camera_process_function( void *record_ptr,
				void *record_field_ptr,
				int operation )
{
	static const char fname[] = "mxd_aravis_camera_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_VIDEO_INPUT *vinput;
	MX_ARAVIS_CAMERA *aravis_camera;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VIDEO_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	aravis_camera = (MX_ARAVIS_CAMERA *) record->record_type_struct;

	if ( aravis_camera == (MX_ARAVIS_CAMERA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ARAVIS_CAMERA pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		default:
			break;
		}
		break;

	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		default:
			break;
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unknown operation code = %d for record '%s'.",
			operation, record->name );
	}

	return mx_status;
}

