/*
 * Name:    d_rdi_mbc_filename.c
 *
 * Purpose: MX driver for video inputs using the Vimba C API
 *          for cameras from Allied Vision Technologies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_AVT_VIMBA_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"

#include "d_rdi_mbc_filename.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_rdi_mbc_filename_record_function_list = {
	NULL,
	mxd_rdi_mbc_filename_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_rdi_mbc_filename_open,
	mxd_rdi_mbc_filename_close
};

MX_RECORD_FIELD_DEFAULTS mxd_rdi_mbc_filename_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_AVT_VIMBA_STANDARD_FIELDS
};

long mxd_rdi_mbc_filename_num_record_fields
		= sizeof( mxd_rdi_mbc_filename_record_field_defaults )
			/ sizeof( mxd_rdi_mbc_filename_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_rdi_mbc_filename_rfield_def_ptr
			= &mxd_rdi_mbc_filename_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_rdi_mbc_filename_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_RDI_MBC_FILENAME **rdi_mbc_filename,
			MX_AVT_VIMBA **rdi_mbc_filename,
			const char *calling_fname )
{
	static const char fname[] = "mxd_rdi_mbc_filename_get_pointers()";

	MX_RDI_MBC_FILENAME *rdi_mbc_filename_ptr;
	MX_RECORD *rdi_mbc_filename_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	rdi_mbc_filename_ptr = vinput->record->record_type_struct;

	if ( rdi_mbc_filename_ptr == (MX_RDI_MBC_FILENAME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RDI_MBC_FILENAME pointer for record '%s' is NULL.",
			vinput->record->name );
	}

	if ( rdi_mbc_filename != (MX_RDI_MBC_FILENAME **) NULL ) {
		*rdi_mbc_filename = rdi_mbc_filename_ptr;
	}

	if ( rdi_mbc_filename != (MX_AVT_VIMBA **) NULL ) {
		rdi_mbc_filename_record = rdi_mbc_filename_ptr->rdi_mbc_filename_record;

		if ( rdi_mbc_filename_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The rdi_mbc_filename_record pointer for record '%s' is NULL.",
				vinput->record->name );
		}

		*rdi_mbc_filename = rdi_mbc_filename_record->record_type_struct;

		if ( (*rdi_mbc_filename) == (MX_AVT_VIMBA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_AVT_VIMBA pointer for record '%s' is NULL.",
				vinput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_rdi_mbc_filename_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_rdi_mbc_filename_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_RDI_MBC_FILENAME *rdi_mbc_filename = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	rdi_mbc_filename = (MX_RDI_MBC_FILENAME *)
				malloc( sizeof(MX_RDI_MBC_FILENAME));

	if ( rdi_mbc_filename == (MX_RDI_MBC_FILENAME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_RDI_MBC_FILENAME structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = rdi_mbc_filename;
	record->class_specific_function_list = 
			&mxd_rdi_mbc_filename_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	rdi_mbc_filename->record = record;

	vinput->bytes_per_frame = 0;
	vinput->bytes_per_pixel = 0;
	vinput->bits_per_pixel = 0;
	vinput->trigger_mode = 0;

	vinput->pixel_clock_frequency = 0.0;	/* in pixels/second */

	vinput->external_trigger_polarity = MXF_VIN_TRIGGER_NONE;

	vinput->camera_trigger_polarity = MXF_VIN_TRIGGER_NONE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rdi_mbc_filename_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_rdi_mbc_filename_open()";

	MX_VIDEO_INPUT *vinput;
	MX_RDI_MBC_FILENAME *rdi_mbc_filename = NULL;
	MX_AVT_VIMBA *rdi_mbc_filename = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_rdi_mbc_filename_get_pointers( vinput,
					&rdi_mbc_filename, &rdi_mbc_filename, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_VIMBA_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	/* Initialize a bunch of driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->byte_order     = mx_native_byteorder();
	vinput->trigger_mode   = MXT_IMAGE_INTERNAL_TRIGGER;

	mx_status = mx_video_input_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bits_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_framesize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_VIMBA_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rdi_mbc_filename_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_rdi_mbc_filename_close()";

	MX_VIDEO_INPUT *vinput;
	MX_RDI_MBC_FILENAME *rdi_mbc_filename = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_rdi_mbc_filename_get_pointers( vinput,
					&rdi_mbc_filename, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVT_VIMBA_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

