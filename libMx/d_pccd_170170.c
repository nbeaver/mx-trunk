/*
 * Name:    d_pccd_170170.c
 *
 * Purpose: MX driver for the Aviex PCCD-170170 CCD detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PCCD_170170_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_camera_link.h"
#include "mx_area_detector.h"
#include "d_pccd_170170.h"

/*---*/

#if HAVE_CAMERA_LINK

MX_RECORD_FUNCTION_LIST mxd_pccd_170170_record_function_list = {
	NULL,
	mxd_pccd_170170_create_record_structures,
	mxd_pccd_170170_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_pccd_170170_open,
	mxd_pccd_170170_close
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_pccd_170170_function_list = {
	mxd_pccd_170170_arm,
	mxd_pccd_170170_trigger,
	mxd_pccd_170170_stop,
	mxd_pccd_170170_abort,
	mxd_pccd_170170_busy,
	mxd_pccd_170170_get_status,
	mxd_pccd_170170_get_frame,
	mxd_pccd_170170_get_sequence,
	mxd_pccd_170170_get_parameter,
	mxd_pccd_170170_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_pccd_170170_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MXD_PCCD_170170_STANDARD_FIELDS
};

long mxd_pccd_170170_num_record_fields
		= sizeof( mxd_pccd_170170_record_field_defaults )
			/ sizeof( mxd_pccd_170170_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pccd_170170_rfield_def_ptr
			= &mxd_pccd_170170_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_pccd_170170_get_pointers( MX_AREA_DETECTOR *ad,
			MX_PCCD_170170 **pccd_170170,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pccd_170170_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pccd_170170 == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PCCD_170170 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pccd_170170 = (MX_PCCD_170170 *)
				ad->record->record_type_struct;

	if ( *pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_PCCD_170170 pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_pccd_170170_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_pccd_170170_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	pccd_170170 = (MX_PCCD_170170 *)
				malloc( sizeof(MX_PCCD_170170) );

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_PCCD_170170 structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = pccd_170170;
	record->class_specific_function_list = 
			&mxd_pccd_170170_function_list;

	memset( &(ad->sequence_info), 0, sizeof(ad->sequence_info) );

	ad->record = record;
	pccd_170170->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pccd_170170_open()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pccd_170170_get_pointers( ad,
						&pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	/* Set the video input's initial trigger mode (internal/external/etc) */

	mx_status = mx_video_input_set_trigger_mode(
					pccd_170170->video_input_record,
					pccd_170170->initial_trigger_mode );

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_arm()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_arm( pccd_170170->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_trigger()";

	MX_PCCD_170170 *pccd_170170;
	MX_SEQUENCE_INFO *sq;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sq = &(ad->sequence_info);

	switch( sq->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		break;

	case MXT_SQ_CONTINUOUS:
		break;

	case MXT_SQ_MULTI:
		break;

	case MXT_SQ_CONTINUOUS_MULTI:
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Image sequence type %ld is not supported by "
			"area detector '%s'.",
			sq->sequence_type, ad->record->name );
	}

	mx_status = mx_video_input_trigger( pccd_170170->video_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_stop()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_stop( pccd_170170->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_abort()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_abort( pccd_170170->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_busy( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_busy()";

	MX_PCCD_170170 *pccd_170170;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0 && MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_is_busy(
				pccd_170170->video_input_record, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->busy = busy;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_get_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_get_status()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mxd_pccd_170170_busy( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_get_frame( MX_AREA_DETECTOR *ad, MX_IMAGE_FRAME **frame )
{
	static const char fname[] = "mxd_pccd_170170_get_frame()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_get_frame(
				pccd_170170->video_input_record, frame );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_get_sequence( MX_AREA_DETECTOR *ad,
				MX_IMAGE_SEQUENCE **sequence )
{
	static const char fname[] = "mxd_pccd_170170_get_sequence()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sequence == (MX_IMAGE_SEQUENCE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_SEQUENCE pointer passed was NULL." );
	}

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_get_sequence(
				pccd_170170->video_input_record, sequence );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_get_parameter()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
#if 0
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_FORMAT:
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETERS: 
		break;
#endif
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Parameter type %ld not yet implemented for record '%s'.",
			ad->parameter_type, ad->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_set_parameter()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
#if 0
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_FORMAT:

		break;
#endif

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETERS: 
		mx_status = mx_video_input_set_sequence(
					pccd_170170->video_input_record,
					&(ad->sequence_info) );
		break; 
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Parameter type %ld not yet implemented for record '%s'.",
			ad->parameter_type, ad->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_camera_link_command( MX_PCCD_170170 *pccd_170170,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag )
{
	static const char fname[] = "mxd_pccd_170170_camera_link_command()";

	MX_RECORD *camera_link_record;
	size_t command_length, response_length;
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( max_response_length < 1 ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The requested response buffer length %lu for record '%s' "
		"is too short to hold a minimum length response.",
			(unsigned long) max_response_length,
			pccd_170170->record->name );
	}

	camera_link_record = pccd_170170->camera_link_record;

	if ( camera_link_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The camera_link_record pointer for record '%s' is NULL.",
			pccd_170170->record->name );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sent '%s' to '%s'",
			fname, command, camera_link_record->name ));
	}

	command_length = strlen(command);

	mx_status = mx_camera_link_serial_write( camera_link_record,
						command, &command_length, -1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Leave room in the response buffer to null terminate the response. */

	response_length = max_response_length - 1;

	mx_status = mx_camera_link_serial_read( camera_link_record,
						response, &response_length, -1);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	response[response_length] = '\0';

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, camera_link_record->name ));
	}

	if ( response[0] == 'E' ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error response received for command '%s' sent to "
		"Camera Link interface '%s'.",
			command, camera_link_record->name );
	}
	
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_read_register( MX_PCCD_170170 *pccd_170170,
				unsigned long register_address,
				unsigned long *register_value )
{
	static const char fname[] = "mxd_pccd_170170_read_register()";

	char command[10], response[10];
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( register_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The register_value pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( register_address >= 100 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested register address %lu for record '%s' "
		"is outside the allowed range of 0 to 99.",
			register_address, pccd_170170->record->name );
	}

	snprintf( command, sizeof(command), "R%02lu", register_address );

	mx_status = mxd_pccd_170170_camera_link_command( pccd_170170,
					command, response, 5,
					MXD_PCCD_170170_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	response[4] = '\0';

	*register_value = atoi( &response[1] );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_write_register( MX_PCCD_170170 *pccd_170170,
				unsigned long register_address,
				unsigned long register_value )
{
	static const char fname[] = "mxd_pccd_170170_write_register()";

	char command[10], response[10];
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( register_address >= 100 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested register address %lu for record '%s' "
		"is outside the allowed range of 0 to 99.",
			register_address, pccd_170170->record->name );
	}
	if ( register_value >= 300 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested register value %lu for record '%s' "
		"is outside the allowed range of 0 to 299.",
			register_value, pccd_170170->record->name );
	}

	snprintf( command, sizeof(command),
		"W%02lu%03lu", register_address, register_value );

	mx_status = mxd_pccd_170170_camera_link_command( pccd_170170,
					command, response, 2,
					MXD_PCCD_170170_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_read_adc( MX_PCCD_170170 *pccd_170170,
				unsigned long adc_address,
				double *adc_value )
{
	static const char fname[] = "mxd_pccd_170170_read_adc()";

	char command[10], response[10];
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( adc_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The adc_value pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( adc_address >= 8 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested register address %lu for record '%s' "
		"is outside the allowed range of 0 to 7.",
			adc_address, pccd_170170->record->name );
	}

	snprintf( command, sizeof(command), "R%01lu", adc_address );

	mx_status = mxd_pccd_170170_camera_link_command( pccd_170170,
					command, response, 7,
					MXD_PCCD_170170_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	response[4] = '\0';

	*adc_value = atof( &response[1] );

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_CAMERA_LINK */

