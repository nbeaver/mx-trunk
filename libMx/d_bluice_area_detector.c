/*
 * Name:    d_bluice_area_detector.c
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

#define MXD_BLUICE_AREA_DETECTOR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_bluice.h"
#include "n_bluice_dcss.h"
#include "n_bluice_dhs.h"
#include "n_bluice_dhs_manager.h"
#include "d_bluice_area_detector.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_bluice_area_detector_record_function_list = {
	mxd_bluice_area_detector_initialize_type,
	mxd_bluice_area_detector_create_record_structures,
	mxd_bluice_area_detector_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_bluice_area_detector_open,
	NULL,
	mxd_bluice_area_detector_finish_delayed_initialization
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_bluice_area_detector_function_list = {
	mxd_bluice_area_detector_arm,
	mxd_bluice_area_detector_trigger,
	mxd_bluice_area_detector_stop,
	mxd_bluice_area_detector_stop,
	NULL,
	NULL,
	NULL,
	mxd_bluice_area_detector_get_extended_status,
	mxd_bluice_area_detector_readout_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_bluice_area_detector_get_parameter,
	mxd_bluice_area_detector_set_parameter,
	mx_area_detector_default_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_bluice_dcss_area_detector_record_field_defaults[]
= {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_BLUICE_DCSS_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_bluice_dcss_area_detector_num_record_fields
	= sizeof( mxd_bluice_dcss_area_detector_record_field_defaults )
	    / sizeof( mxd_bluice_dcss_area_detector_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_dcss_area_detector_rfield_def_ptr
		= &mxd_bluice_dcss_area_detector_record_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxd_bluice_dhs_area_detector_record_field_defaults[]
= {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_BLUICE_DHS_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_bluice_dhs_area_detector_num_record_fields
	= sizeof( mxd_bluice_dhs_area_detector_record_field_defaults )
	    / sizeof( mxd_bluice_dhs_area_detector_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_dhs_area_detector_rfield_def_ptr
		= &mxd_bluice_dhs_area_detector_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_bluice_area_detector_get_pointers( MX_AREA_DETECTOR *ad,
			MX_BLUICE_AREA_DETECTOR **bluice_area_detector,
			MX_BLUICE_SERVER **bluice_server,
			void **bluice_type_server,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bluice_area_detector_get_pointers()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector_ptr;
	MX_RECORD *bluice_server_record;
	MX_BLUICE_SERVER *bluice_server_ptr;

	/* In this section, we do standard MX ..._get_pointers() logic. */

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( (ad->record) == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_AREA_DETECTOR pointer %p "
		"passed was NULL.", ad );
	}

	bluice_area_detector_ptr = ad->record->record_type_struct;

	if ( bluice_area_detector_ptr == (MX_BLUICE_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_AREA_DETECTOR pointer for record '%s' is NULL.",
			ad->record->name );
	}

	if ( bluice_area_detector != (MX_BLUICE_AREA_DETECTOR **) NULL ) {
		*bluice_area_detector = bluice_area_detector_ptr;
	}

	bluice_server_record = bluice_area_detector_ptr->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' "
		"is NULL.", ad->record->name );
	}

	bluice_server_ptr = bluice_server_record->record_class_struct;

	if ( bluice_server_ptr == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for Blu-Ice server "
		"record '%s' used by record '%s' is NULL.",
			bluice_server_record->name,
			ad->record->name );
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = bluice_server_ptr;
	}

	if ( bluice_type_server != (void *) NULL ) {
		*bluice_type_server = bluice_server_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_bluice_area_detector_initialize_type( long record_type )
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
mxd_bluice_area_detector_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_bluice_area_detector_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	bluice_area_detector = (MX_BLUICE_AREA_DETECTOR *)
				malloc( sizeof(MX_BLUICE_AREA_DETECTOR) );

	if ( bluice_area_detector == (MX_BLUICE_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	  "Cannot allocate memory for an MX_BLUICE_AREA_DETECTOR structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = bluice_area_detector;
	record->class_specific_function_list = 
			&mxd_bluice_area_detector_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	bluice_area_detector->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_finish_record_initialization( MX_RECORD *record )
{
	return mx_area_detector_finish_record_initialization( record );
}

/* The mxp_setup_dhs_record() macro is used to setup the record pointer for the
 * detector_distance, wavelength, detector_x, and detector_y records.
 */

#define mxp_setup_dhs_record(x,y) \
	do {                                                                \
	    if ( ( strlen(x) == 0 ) || ( strcmp((x), "NULL") == 0 ) ) {     \
		name_record = NULL;                                         \
	    } else {                                                        \
		name_record = mx_get_record( record, (x) ) ;                \
		if ( name_record == (MX_RECORD *) NULL ) {                  \
		    return mx_error( MXE_NOT_FOUND, fname,                  \
			"Record '%s' used by record '%s' was not found.",   \
			    (x), record->name );                            \
		}                                                           \
	    }                                                               \
	    (y) = name_record;                                              \
	} while(0)

MX_EXPORT mx_status_type
mxd_bluice_area_detector_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_bluice_area_detector_open()";

	MX_AREA_DETECTOR *ad;
	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	MX_RECORD *name_record;
	mx_status_type mx_status;

	bluice_area_detector = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
			&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	switch( record->mx_type ) {
	case MXT_AD_BLUICE_DCSS:
		bluice_area_detector->detector_distance_record = NULL;
		bluice_area_detector->wavelength_record = NULL;
		bluice_area_detector->detector_x_record = NULL;
		bluice_area_detector->detector_y_record = NULL;
		break;
	case MXT_AD_BLUICE_DHS:
		mxp_setup_dhs_record(
				bluice_area_detector->detector_distance_name,
				bluice_area_detector->detector_distance_record);
		mxp_setup_dhs_record(bluice_area_detector->wavelength_name,
				bluice_area_detector->wavelength_record);
		mxp_setup_dhs_record(bluice_area_detector->detector_x_name,
				bluice_area_detector->detector_x_record);
		mxp_setup_dhs_record(bluice_area_detector->detector_y_name,
				bluice_area_detector->detector_y_record);
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Blu-Ice area detector record '%s' has an illegal "
		"MX type of %ld", record->name, record->mx_type );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_finish_delayed_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_bluice_area_detector_finish_delayed_initialization()";

	MX_AREA_DETECTOR *ad;
	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *detector_type_string;
	char collect_name[MXU_BLUICE_NAME_LENGTH+1];
	char *detector_type;
	char *source_ptr, *dest_ptr, *ptr;
	unsigned long flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
			&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	switch( record->mx_type ) {
	case MXT_AD_BLUICE_DCSS:
		strlcpy( collect_name, "collectFrame", sizeof(collect_name) );
		break;
	case MXT_AD_BLUICE_DHS:
		strlcpy( collect_name, "detector_collect_image",
							sizeof(collect_name) );
		break;
	}

	mx_status = mx_bluice_get_device_pointer( bluice_server,
					collect_name,
					bluice_server->operation_array,
					bluice_server->num_operations,
				&(bluice_area_detector->collect_operation) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bluice_area_detector->last_collect_operation_state =
	  bluice_area_detector->collect_operation->u.operation.operation_state;

	/* See if a string called 'detectorType' has been sent to us.
	 * If it does exist, then we can figure out what the format of
	 * the detector's image frames are.
	 */

	mx_status = mx_bluice_get_device_pointer( bluice_server,
					"detectorType",
					bluice_server->string_array,
					bluice_server->num_strings,
					&detector_type_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (detector_type_string != NULL)
	  && (detector_type_string->foreign_type == MXT_BLUICE_FOREIGN_STRING)
	  && (detector_type_string->u.string.string_buffer != NULL) )
	{
		/* Skip over a leading '{' character if present. */

		source_ptr = detector_type_string->u.string.string_buffer;

		if ( *source_ptr == '{' ) {
			source_ptr++;
		}

		/* Copy the detector type. */

		strlcpy( bluice_area_detector->detector_type, source_ptr,
			sizeof(bluice_area_detector->detector_type) );

		/* Delete a trailing '}' and/or newline if present. */

		dest_ptr = bluice_area_detector->detector_type;

		ptr = strrchr( dest_ptr, '}' );

		if ( ptr != NULL ) {
			*ptr = '\0';
		} else {
			ptr = strrchr( dest_ptr, '\n' );

			if ( ptr != NULL ) {
				*ptr = '\0';
			}
		}
	} else {
		bluice_area_detector->detector_type[0] = '\0';
	}

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Blu-Ice area detector '%s' has detector type '%s'.",
		fname, record->name, bluice_area_detector->detector_type));
#endif
	/* Initialize MX_AREA_DETECTOR parameters depending on the
	 * detector type.
	 *
	 * FIXME: Some of this stuff is sure to be wrong.
	 */

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	detector_type = bluice_area_detector->detector_type;

	if ( detector_type[0] == '\0' ) {
		ad->datafile_format = MXT_IMAGE_FILE_SMV;

		mx_warning( "No detector type was received from Blu-Ice "
		"server '%s', so the image format is assumed to be SMV.",
			bluice_server->record->name );

		ad->framesize[0] = 4096;
		ad->framesize[1] = 4096;
	} else
	if ( strcmp(detector_type, "MAR345") == 0 ) {
#if 0
		ad->datafile_format = MXT_IMAGE_FILE_TIFF;
#else
		ad->datafile_format = MXT_IMAGE_FILE_SMV;

		mx_info(
		"%s: FIXME: MAR345 image type has been forced to SMV.", fname );
#endif
		ad->framesize[0] = 4096;
		ad->framesize[1] = 4096;
	} else {
		ad->datafile_format = MXT_IMAGE_FILE_SMV;

		mx_warning( "Unrecognized detector type '%s' was reported "
		"by Blu-Ice area detector '%s'.  The image format is "
		"assumed to be SMV.", detector_type,
				bluice_server->record->name );

		ad->framesize[0] = 4096;
		ad->framesize[1] = 4096;
	}

	ad->image_format = ad->datafile_format;
	ad->byte_order = mx_native_byteorder();
	ad->header_length = MXT_IMAGE_HEADER_LENGTH_IN_BYTES;
	ad->bytes_per_pixel = 2;

	ad->maximum_framesize[0] = ad->framesize[0];
	ad->maximum_framesize[1] = ad->framesize[1];

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	/* See if the user has requested the loading or saving of
	 * image frames by MX.
	 */

	flags = ad->area_detector_flags;

	if ( flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION ) {
	    ad->area_detector_flags &= (~MXF_AD_SAVE_FRAME_AFTER_ACQUISITION);

	    (void) mx_error( MXE_UNSUPPORTED, fname,
		"Automatic saving of image frames for Blu-Ice "
		"area detector '%s' is not supported, since the image "
		"frame data only exists in remote Blu-Ice server '%s'.  "
		"The save frame flag has been turned off.",
			record->name, bluice_server->record->name );
	}

	if ( flags & MXF_AD_LOAD_FRAME_AFTER_ACQUISITION ) {
	    mx_status = mx_area_detector_setup_datafile_management( ad, NULL );
	}

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

static double
mxp_motor_position( MX_RECORD *record )
{
	double position;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return 0.0;
	}

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return 0.0;
	}

	return position;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_bluice_area_detector_arm()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	ad->last_frame_number = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_bluice_area_detector_trigger()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	MX_BLUICE_FOREIGN_DEVICE *collect_operation;
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time;
	char command[200];
	char dhs_username[MXU_USERNAME_LENGTH+1];
	char datafile_name[MXU_FILENAME_LENGTH+1];
	char *ptr;
	unsigned long dark_current_fu;
	unsigned long client_number, operation_counter;
	double detector_distance, wavelength;
	double detector_x, detector_y;
	mx_status_type mx_status;

	bluice_area_detector = NULL;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
		&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
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

	/* Construct the trigger command. */

#if 1
	dark_current_fu = 15;
#endif

	/* Construct the Blu-Ice datafile name with the trailing filetype
	 * stripped off.
	 */

	strlcpy( datafile_name, ad->datafile_name, sizeof(datafile_name) );

	ptr = strrchr( datafile_name, '.' );

	if ( ptr != NULL ) {
		*ptr = '\0';
	}

	MX_DEBUG(-2,("%s: datafile_name = '%s'", fname, datafile_name));

	client_number = mx_bluice_get_client_number( bluice_server );

	operation_counter = mx_bluice_update_operation_counter( bluice_server );

	switch( ad->record->mx_type ) {
	case MXT_AD_BLUICE_DCSS:
		bluice_dcss_server = bluice_server->record->record_type_struct;

		snprintf( command, sizeof(command),
		"gtos_start_operation collectFrame %lu.%lu %lu %s %s %s "
			"NULL NULL 0 %f 0 1 0",
			client_number,
			operation_counter,
			dark_current_fu,
			datafile_name,
			ad->datafile_directory,
			bluice_dcss_server->bluice_username,
			exposure_time );
		break;

	case MXT_AD_BLUICE_DHS:
		mx_username( dhs_username, sizeof(dhs_username) );

		detector_distance = mxp_motor_position(
			bluice_area_detector->detector_distance_record );

		wavelength = mxp_motor_position(
			bluice_area_detector->wavelength_record );

		detector_x = mxp_motor_position(
			bluice_area_detector->detector_x_record );

		detector_y = mxp_motor_position(
			bluice_area_detector->detector_y_record );

		snprintf( command, sizeof(command),
		"stoh_start_operation detector_collect_image %lu.%lu %lu "
		"%s %s %s NULL %f 0.0 0.0 %f %f %f %f 0 0",
			client_number,
			operation_counter,
			dark_current_fu,
			datafile_name,
			ad->datafile_directory,
			dhs_username,
			exposure_time,
			detector_distance,
			wavelength,
			detector_x,
			detector_y );
		break;
	}

	mx_status = mx_bluice_check_for_master( bluice_server );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	collect_operation = bluice_area_detector->collect_operation;

	if ( collect_operation == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The collect operation for Blu-Ice area detector '%s' "
		"has not yet been initialized.", ad->record->name );
	}

	collect_operation->u.operation.operation_state
				= MXSF_BLUICE_OPERATION_STARTED;

	mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0 );
			
#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_bluice_area_detector_stop()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	char command[200];
	unsigned long client_number, operation_counter;
	mx_status_type mx_status;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
		&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	
	client_number = mx_bluice_get_client_number( bluice_server );

	operation_counter = mx_bluice_update_operation_counter( bluice_server );

	switch( ad->record->mx_type ) {
	case MXT_AD_BLUICE_DCSS:
		snprintf( command, sizeof(command),
		"gtos_start_operation detector_stop %lu.%lu",
			client_number, operation_counter );
		break;
	case MXT_AD_BLUICE_DHS:
		snprintf( command, sizeof(command),
		"stoh_start_operation detector_stop %lu.%lu",
			client_number, operation_counter );
		break;
	}

	mx_status = mx_bluice_check_for_master( bluice_server );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0 );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_bluice_area_detector_get_extended_status()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *collect_operation;
	int operation_state;
	mx_status_type mx_status;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
		&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	collect_operation = bluice_area_detector->collect_operation;

	if ( collect_operation == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The collect operation for Blu-Ice area detector '%s' "
		"has not yet been initialized.", ad->record->name );
	}

	operation_state = collect_operation->u.operation.operation_state;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: client_number = %lu, operation_counter = %lu",
		fname, collect_operation->u.operation.client_number,
		collect_operation->u.operation.operation_counter));

	switch( operation_state ) {
	case MXSF_BLUICE_OPERATION_ERROR:
		MX_DEBUG(-2,
		("%s: operation_state = %d (MXSF_BLUICE_OPERATION_ERROR)",
			fname, operation_state));
		break;
	case MXSF_BLUICE_OPERATION_COMPLETED:
		MX_DEBUG(-2,
		("%s: operation_state = %d (MXSF_BLUICE_OPERATION_COMPLETED)",
			fname, operation_state));
		break;
	case MXSF_BLUICE_OPERATION_STARTED:
		MX_DEBUG(-2,
		("%s: operation_state = %d (MXSF_BLUICE_OPERATION_STARTED)",
			fname, operation_state));
		break;
	case MXSF_BLUICE_OPERATION_UPDATED:
		MX_DEBUG(-2,
		("%s: operation_state = %d (MXSF_BLUICE_OPERATION_UPDATED)",
			fname, operation_state));
		break;
	default:
		MX_DEBUG(-2,
		("%s: operation_state = %d (MXSF_BLUICE_OPERATION_UNKNOWN)",
			fname, operation_state));
		break;
	}

	MX_DEBUG(-2,("%s: last_collect_operation_state = %d",
		fname, bluice_area_detector->last_collect_operation_state));

	if ( collect_operation->u.operation.arguments_buffer == NULL ) {
		MX_DEBUG(-2,
		("%s: arguments_length = %ld, arguments_buffer = '(nil)'",
		fname, (long) collect_operation->u.operation.arguments_length))
	} else {
		MX_DEBUG(-2,
		("%s: arguments_length = %ld, arguments_buffer = '%s'",
		fname, (long) collect_operation->u.operation.arguments_length,
		collect_operation->u.operation.arguments_buffer));
	}
#endif

	switch( operation_state ) {
	case MXSF_BLUICE_OPERATION_STARTED:
	case MXSF_BLUICE_OPERATION_UPDATED:
		ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
		break;
	case MXSF_BLUICE_OPERATION_COMPLETED:
		ad->status = 0;
		ad->last_frame_number = 0;

		if ( operation_state
			!= bluice_area_detector->last_collect_operation_state )
		{
			ad->total_num_frames++;
		}
		break;
	case MXSF_BLUICE_OPERATION_ERROR:
	case MXSF_BLUICE_OPERATION_NETWORK_ERROR:
	default:
		ad->status = MXSF_AD_ERROR;
		break;
	}

	bluice_area_detector->last_collect_operation_state = operation_state;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
	fname, ad->last_frame_number, ad->total_num_frames, ad->status));

	MX_DEBUG(-2,("%s: datafile_total_num_frames = %ld",
		fname, ad->datafile_total_num_frames));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_bluice_area_detector_readout_frame()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	unsigned long client_number, operation_counter;
	char command[200];
	mx_status_type mx_status;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
		&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* If necessary, trigger the loading of the image frame
	 * by calling the total_num_frames function.
	 */

	mx_status = mx_area_detector_get_total_num_frames( ad->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ad->record->mx_type == MXT_AD_BLUICE_DHS ) {
		client_number = mx_bluice_get_client_number( bluice_server );

		operation_counter = mx_bluice_update_operation_counter(
							bluice_server );

		snprintf( command, sizeof(command),
			"stoh_start_operation detector_transfer_image %lu.%lu",
			client_number,
			operation_counter );

		mx_status = mx_bluice_check_for_master( bluice_server );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_bluice_area_detector_get_parameter()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	mx_status_type mx_status;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
		&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
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
		break;

	case MXLV_AD_BINSIZE:
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		break;

	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_bluice_area_detector_set_parameter()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
		&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
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

