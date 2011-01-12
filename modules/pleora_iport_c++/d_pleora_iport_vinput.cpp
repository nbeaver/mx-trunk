/*
 * Name:    d_pleora_iport_vinput.c
 *
 * Purpose: MX video input driver for a Pleora iPORT video capture device.
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

#define MXD_PLEORA_IPORT_VINPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_array.h"
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_pleora_iport.h"
#include "d_pleora_iport_vinput.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_pleora_iport_vinput_record_function_list = {
	NULL,
	mxd_pleora_iport_vinput_create_record_structures,
	mxd_pleora_iport_vinput_finish_record_initialization,
	NULL,
	NULL,
	mxd_pleora_iport_vinput_open,
	mxd_pleora_iport_vinput_close
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_pleora_iport_vinput_video_input_function_list = {
	mxd_pleora_iport_vinput_arm,
	mxd_pleora_iport_vinput_trigger,
	mxd_pleora_iport_vinput_stop,
	mxd_pleora_iport_vinput_abort,
	mxd_pleora_iport_vinput_asynchronous_capture,
	NULL,
	NULL,
	NULL,
	mxd_pleora_iport_vinput_get_extended_status,
	mxd_pleora_iport_vinput_get_frame,
	mxd_pleora_iport_vinput_get_parameter,
	mxd_pleora_iport_vinput_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_pleora_iport_vinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_PLEORA_IPORT_VINPUT_STANDARD_FIELDS
};

long mxd_pleora_iport_vinput_num_record_fields
		= sizeof( mxd_pleora_iport_vinput_record_field_defaults )
			/ sizeof( mxd_pleora_iport_vinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pleora_iport_vinput_rfield_def_ptr
			= &mxd_pleora_iport_vinput_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_pleora_iport_vinput_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_PLEORA_IPORT_VINPUT **pleora_iport_vinput,
			MX_PLEORA_IPORT **pleora_iport,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pleora_iport_vinput_get_pointers()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput_ptr;
	MX_RECORD *pleora_iport_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pleora_iport_vinput_ptr = (MX_PLEORA_IPORT_VINPUT *)
				vinput->record->record_type_struct;

	if ( pleora_iport_vinput_ptr == (MX_PLEORA_IPORT_VINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PLEORA_IPORT_VINPUT pointer for record '%s' "
			"passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}

	if ( pleora_iport_vinput != (MX_PLEORA_IPORT_VINPUT **) NULL ) {
		*pleora_iport_vinput = pleora_iport_vinput_ptr;
	}

	if ( pleora_iport != (MX_PLEORA_IPORT **) NULL ) {
		pleora_iport_record =
			pleora_iport_vinput_ptr->pleora_iport_record;

		if ( pleora_iport_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The pleora_iport_record pointer for record '%s' "
			"is NULL.",
			vinput->record->name, calling_fname );
		}

		*pleora_iport = (MX_PLEORA_IPORT *)
					pleora_iport_record->record_type_struct;

		if ( *pleora_iport == (MX_PLEORA_IPORT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PLEORA_IPORT pointer for record '%s' used "
			"by record '%s' is NULL.",
				vinput->record->name,
				pleora_iport_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pleora_iport_vinput_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	pleora_iport_vinput = (MX_PLEORA_IPORT_VINPUT *)
				malloc( sizeof(MX_PLEORA_IPORT_VINPUT) );

	if ( pleora_iport_vinput == (MX_PLEORA_IPORT_VINPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_PLEORA_IPORT_VINPUT structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = pleora_iport_vinput;
	record->class_specific_function_list = 
			&mxd_pleora_iport_vinput_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	pleora_iport_vinput->record = record;

	vinput->trigger_mode = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pleora_iport_vinput_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	mx_status = mx_video_input_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pleora_iport_vinput_open()";

	MX_VIDEO_INPUT *vinput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_PLEORA_IPORT *pleora_iport = NULL;
	long i, num_devices;
	char device_address_string[MXU_HOSTNAME_LENGTH+1];
	mx_status_type mx_status;

	const CyDeviceFinder::DeviceEntry *device_entry;
	CyConfig *config;
	CyResult cy_result;

	int offset_x;

	mx_breakpoint();

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
				&pleora_iport_vinput, &pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	/* Find the camera in the device array. */

	num_devices = pleora_iport->num_devices;

	for ( i = 0; i < num_devices; i++ ) {
		device_entry = pleora_iport->device_array[i];

		MX_DEBUG(-2,("%s: i = %ld, device_entry = %p",
			fname, i, device_entry));
		MX_DEBUG(-2,("%s:     device_entry->mAddressIP = %p",
			fname, device_entry->mAddressIP));
		MX_DEBUG(-2,("%s:     device_entry->mAddressIP.c_str_ascii = %p",
			fname, device_entry->mAddressIP.c_str_ascii));
		MX_DEBUG(-2,("%s:     device_entry->mAddressIP.c_str_ascii() = '%s'",
			fname, device_entry->mAddressIP.c_str_ascii() ));

		strlcpy( device_address_string,
			device_entry->mAddressIP.c_str_ascii(),
			sizeof(device_address_string) );

		if ( strcmp( device_address_string,
			pleora_iport_vinput->ip_address_string ) == 0 )
		{
			/* We have found the correct IP Engine. */

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
			MX_DEBUG(-2,("%s: Entry %ld matches address '%s'",
			fname, pleora_iport_vinput->ip_address_string));
#endif
			break;
		}
	}

	if ( i >= num_devices ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"iPORT host '%s' was not found in the scan by record '%s' "
		"of iPORT devices for record '%s'.",
			pleora_iport_vinput->ip_address_string,
			pleora_iport->record->name,
			record->name );
	}

#if 0
	/* Setup the connectivity information needed 
	 * to connect to the IP Engine.
	 */

	config = CyConfig_Init();

	if ( config == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The attempt to initialize a CyConfig structure "
		"for record '%s' failed.", record->name );
	}

	cy_result = CyConfig_AddDevice( config );

	if ( cy_result != CY_RESULT_OK ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The attempt to add a device to CyConfig %p "
		"for record '%s'.", record->name );
	}

	/* First set the required parameters. */

	CyParameterRepository_SetParameterIntByID( config,
		CY_CONFIG_PARAM_ACCESS_MODE, device_entry->mMode, FALSE );

	CyParameterRepository_SetParameterStringByID( config,
		CY_CONFIG_PARAM_ADDRESS_MAC, device_entry->mAddressMAC, FALSE );

	CyParameterRepository_SetParameterStringByID( config,
		CY_CONFIG_PARAM_ADDRESS_IP, device_entry->mAddressIP, FALSE );

	CyParameterRepository_SetParameterStringByID( config,
		CY_CONFIG_PARAM_ADAPTER_ID, device_entry->mAdapterID, FALSE );

	/* Then set the optional parameters. */

	CyParameterRepository_SetParameterIntByID( config,
		CY_CONFIG_PARAM_PACKET_SIZE, 1440, FALSE );

	CyParameterRepository_SetParameterIntByID( config,
		CY_CONFIG_PARAM_ANSWER_TIMEOUT, 1000, FALSE );

	CyParameterRepository_SetParameterIntByID( config,
		CY_CONFIG_PARAM_FIRST_PACKET_TIMEOUT, 1500, FALSE );

	CyParameterRepository_SetParameterIntByID( config,
		CY_CONFIG_PARAM_PACKET_TIMEOUT, 500, FALSE );

	CyParameterRepository_SetParameterIntByID( config,
		CY_CONFIG_PARAM_REQUEST_TIMEOUT, 5000, FALSE );

	/* Set the connection topology to unicast. */

	CyParameterRepository_SetParameterIntByID( config,
		CY_CONFIG_PARAM_DATA_SENDING_MODE,
			CY_DEVICE_DSM_UNICAST, FALSE );

	CyParameterRepository_SetParameterIntByID( config,
		CY_CONFIG_PARAM_DATA_SENDING_MODE_MASTER, 1, FALSE );

	/* Create and connect to the grabber. */

	pleora_iport_vinput->grabber = CyGrabber_Init( 0, 0 );

	if ( pleora_iport_vinput->grabber == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The attempt to initialize a CyGrabber structure "
		"for record '%s' failed.", record->name );
	}

	cy_result = CyGrabber_Connect( pleora_iport_vinput->grabber,
					config, 0 );

	if ( cy_result != CY_RESULT_OK ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The attempt to connect configuration %p to grabber %p "
		"for record '%s' failed.", record->name );
	}

	/* CyGrabber_Connect() copied all the information it needed
	 * out of the CyConfig object, so we can now dispose of the
	 * CyConfig object.
	 */

	CyConfig_Destroy( config );

	/* Initialize the image properties. */

	mx_status = mx_video_input_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bits_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_set_framesize( record,
						vinput->framesize[0],
						vinput->framesize[1] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	offset_x = 40;	/* FIXME: Where does this number come from??? */

	CyParameterRepository_SetParameterIntByID(
		pleora_iport_vinput->grabber,
		CY_GRABBER_PARAM_OFFSET_X, offset_x, FALSE );

	CyParameterRepository_SetParameterIntByID(
		pleora_iport_vinput->grabber,
		CY_GRABBER_PARAM_OFFSET_Y, 0, FALSE );

	CyParameterRepository_SetParameterIntByID(
		pleora_iport_vinput->grabber,
		CY_GRABBER_PARAM_TAP_QUANTITY, 1, FALSE );

	CyParameterRepository_SetParameterIntByID(
		pleora_iport_vinput->grabber,
		CY_GRABBER_PARAM_PACKED, FALSE, FALSE );

	CyParameterRepository_SetParameterIntByID(
		pleora_iport_vinput->grabber,
		CY_GRABBER_PARAM_NORMALIZED, FALSE, FALSE );

	cy_result = CyGrabber_SaveConfig( pleora_iport_vinput->grabber );

	if ( cy_result != CY_RESULT_OK ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to initialize some miscellaneous parameters "
		"for record '%s' failed.  cy_result = %d",
			record->name, cy_result );
	}

	/* Initialize a bunch of MX driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->byte_order     = (long) mx_native_byteorder();
	vinput->trigger_mode   = MXT_IMAGE_NO_TRIGGER;

	vinput->last_frame_number = -1;
	vinput->total_num_frames = 0;
	vinput->status = 0x0;

	/* FIXME: It should be possible to get maximum_frame_number
	 * from the Pleora iPORT software.
	 */

	vinput->maximum_frame_number = 0;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
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

#endif
	/* FIXME: We are supposed to reprogram the PLC here. */

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_pleora_iport_vinput_close()";

	MX_VIDEO_INPUT *vinput = NULL;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

#if 0
	CyGrabber_Destroy( pleora_iport_vinput->grabber );

	pleora_iport_vinput->grabber = NULL;
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_arm()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_SEQUENCE_PARAMETERS *seq;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	seq = &(vinput->sequence_parameters);

	switch( seq->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
		break;
	case MXT_SQ_MULTIFRAME:
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported sequence type %lu requested for video input '%s'.",
			seq->sequence_type, vinput->record->name );
		break;
	}

	switch( vinput->trigger_mode ) {
	case MXT_IMAGE_INTERNAL_TRIGGER:
		break;
	case MXT_IMAGE_EXTERNAL_TRIGGER:
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_trigger()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_stop()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_abort()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_asynchronous_capture( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_pleora_iport_vinput_asynchronous_capture()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mxd_pleora_iport_vinput_get_extended_status()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( vinput->status & MXSF_VIN_IS_BUSY ) {
		vinput->busy = TRUE;
	} else {
		vinput->busy = FALSE;
	}

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
		fname, vinput->last_frame_number, vinput->total_num_frames,
		vinput->status));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_get_frame()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_IMAGE_FRAME *frame;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_get_parameter()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:

		mx_status = mx_image_get_image_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
#if MXD_PLEORA_IPORT_VINPUT_DEBUG
		MX_DEBUG(-2,("%s: video format = %ld, format name = '%s'",
		    fname, vinput->image_format, vinput->image_format_name));
#endif
		break;

	case MXLV_VIN_BYTE_ORDER:
		vinput->byte_order = (long) mx_native_byteorder();
		break;

	case MXLV_VIN_TRIGGER_MODE:
		vinput->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel
			* vinput->framesize[0] * vinput->framesize[1] );
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
		switch( vinput->image_format ) {
		case MXT_IMAGE_FORMAT_RGB:
			vinput->bytes_per_pixel = 3;
			vinput->bits_per_pixel  = 24;
			break;
		case MXT_IMAGE_FORMAT_GREY8:
			vinput->bytes_per_pixel = 1;
			vinput->bits_per_pixel  = 8;
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			vinput->bytes_per_pixel = 2;
			vinput->bits_per_pixel  = 16;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported image format %ld for video input '%s'.",
				vinput->image_format, vinput->record->name );
		}
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
mxd_pleora_iport_vinput_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_set_parameter()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
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
		return mx_error( MXE_UNSUPPORTED, fname,
			"Directly changing the number of bytes per frame "
			"for video input '%s' is not supported.",
				vinput->record->name );

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

