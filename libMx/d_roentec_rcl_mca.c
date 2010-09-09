/*
 * Name:    d_roentec_rcl_mca.c
 *
 * Purpose: MX multichannel analyzer driver for Roentec MCAs that use the
 *          RCL 2.2 command language.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ROENTEC_RCL_DEBUG		FALSE

#define MXD_ROENTEC_RCL_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_hrt.h"
#include "mx_hrt_debug.h"
#include "mx_bit.h"
#include "mx_rs232.h"
#include "mx_mca.h"
#include "d_roentec_rcl_mca.h"

#if MXD_ROENTEC_RCL_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

/* Initialize the MCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_roentec_rcl_record_function_list = {
	mxd_roentec_rcl_initialize_type,
	mxd_roentec_rcl_create_record_structures,
	mxd_roentec_rcl_finish_record_initialization,
	mxd_roentec_rcl_delete_record,
	NULL,
	mxd_roentec_rcl_open,
	NULL,
	NULL,
	mxd_roentec_rcl_resynchronize,
	mxd_roentec_rcl_special_processing_setup
};

MX_MCA_FUNCTION_LIST mxd_roentec_rcl_mca_function_list = {
	mxd_roentec_rcl_start,
	mxd_roentec_rcl_stop,
	mxd_roentec_rcl_read,
	mxd_roentec_rcl_clear,
	mxd_roentec_rcl_busy,
	mxd_roentec_rcl_get_parameter,
	mxd_roentec_rcl_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_roentec_rcl_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCA_STANDARD_FIELDS,
	MXD_ROENTEC_RCL_STANDARD_FIELDS,
};

long mxd_roentec_rcl_num_record_fields
		= sizeof( mxd_roentec_rcl_record_field_defaults )
		  / sizeof( mxd_roentec_rcl_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_roentec_rcl_rfield_def_ptr
			= &mxd_roentec_rcl_record_field_defaults[0];

static mx_status_type mxd_roentec_rcl_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/* Private functions for the use of the driver. */

static mx_status_type
mxd_roentec_rcl_get_pointers( MX_MCA *mca,
			MX_ROENTEC_RCL_MCA **roentec_rcl_mca,
			const char *calling_fname )
{
	static const char fname[] = "mxd_roentec_rcl_get_pointers()";

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( roentec_rcl_mca == (MX_ROENTEC_RCL_MCA **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ROENTEC_RCL_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCA pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*roentec_rcl_mca = (MX_ROENTEC_RCL_MCA *)
				mca->record->record_type_struct;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_roentec_rcl_get_energy_gain_and_offset( MX_ROENTEC_RCL_MCA *roentec_rcl_mca,
					double *energy_gain,
					double *energy_offset )
{
	static const char fname[] =
			"mxd_roentec_rcl_get_energy_gain_and_offset()";

	int num_items;
	char response[80];
	mx_status_type mx_status;

	if ( energy_gain == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The energy_gain pointer passed was NULL." );
	}
	if ( energy_offset == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The energy_offset pointer passed was NULL." );
	}

	/* Get the energy calibration parameters. */

	mx_status = mxd_roentec_rcl_command( roentec_rcl_mca, "$FC",
					response, sizeof(response),
					MXD_ROENTEC_RCL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "!FC %lg %lg",
				energy_offset, energy_gain );

	if ( num_items != 2 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to find the energy offset and gain in the "
		"response to the '$FC' command sent to Roentec MCA '%s'.  "
		"Response = '%s'", roentec_rcl_mca->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_read_32bit_array( MX_ROENTEC_RCL_MCA *roentec_rcl_mca,
				size_t num_32bit_values,
				uint32_t *value_array,
				int transfer_flags )
{
	static const char fname[] = "mxd_roentec_rcl_read_32bit_array()";

	uint32_t original_value, new_value;
	size_t i, num_bytes_to_read, bytes_read;
	unsigned long byteorder;
	int debug_flag;
	mx_status_type mx_status;

#if MXD_ROENTEC_RCL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	if ( roentec_rcl_mca == (MX_ROENTEC_RCL_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ROENTEC_RCL_MCA pointer passed was NULL." );
	}
	if ( value_array == (uint32_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The value_array pointer passed was NULL." );
	}

	debug_flag = transfer_flags & MXD_ROENTEC_RCL_DEBUG;

	byteorder = mx_native_byteorder();

	num_bytes_to_read = 4 * num_32bit_values;

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: About to read %lu values from Roentec MCA '%s'.",
			fname, (unsigned long) num_32bit_values,
			roentec_rcl_mca->record->name));
	}

#if MXD_ROENTEC_RCL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_rs232_read( roentec_rcl_mca->rs232_record,
					(char *) value_array,
					num_bytes_to_read, &bytes_read, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_read != num_bytes_to_read ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not read %lu bytes from '%s' "
		"for '%s', instead read %lu bytes.",
			(unsigned long) num_bytes_to_read,
			roentec_rcl_mca->rs232_record->name,
			roentec_rcl_mca->record->name,
			(unsigned long) bytes_read );
	}

	/* The transferred 32bit data is binary in big-endian format.
	 * If we are not on a big-endian computer, we must convert the
	 * data to the native byte order.
	 */
	
	switch( byteorder ) {
	case MX_DATAFMT_BIG_ENDIAN:

		/* Nothing needs to be done for this case. */
	
		break;
	case MX_DATAFMT_LITTLE_ENDIAN:

		/* Convert the data to little-endian format. */

		for ( i = 0; i < num_32bit_values; i++ ) {

			original_value = value_array[i];

			new_value = mx_32bit_byteswap( original_value );
	
			value_array[i] = new_value;
		}
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"MX byte order %lu is not currently supported.  "
		"You should never see this message.",
			mx_native_byteorder() );
	}

#if MXD_ROENTEC_RCL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, roentec_rcl_mca->record->name );
#endif

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: %lu values successfully read from Roentec MCA '%s'.",
			fname, (unsigned long) num_32bit_values,
			roentec_rcl_mca->record->name));
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_roentec_rcl_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_channels_varargs_cookie;
	long maximum_num_rois_varargs_cookie;
	long num_soft_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mca_initialize_type( record_type,
				&num_record_fields,
				&record_field_defaults,
				&maximum_num_channels_varargs_cookie,
				&maximum_num_rois_varargs_cookie,
				&num_soft_rois_varargs_cookie );

	return mx_status;
}


MX_EXPORT mx_status_type
mxd_roentec_rcl_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_roentec_rcl_create_record_structures()";

	MX_MCA *mca;
	MX_ROENTEC_RCL_MCA *roentec_rcl_mca;

	/* Allocate memory for the necessary structures. */

	mca = (MX_MCA *) malloc( sizeof(MX_MCA) );

	if ( mca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA structure." );
	}

	roentec_rcl_mca = (MX_ROENTEC_RCL_MCA *)
				malloc( sizeof(MX_ROENTEC_RCL_MCA) );

	if ( roentec_rcl_mca == (MX_ROENTEC_RCL_MCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ROENTEC_RCL_MCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mca;
	record->record_type_struct = roentec_rcl_mca;
	record->class_specific_function_list =
				&mxd_roentec_rcl_mca_function_list;

	mca->record = record;
	roentec_rcl_mca->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_roentec_rcl_finish_record_initialization()";

	MX_MCA *mca;
	MX_ROENTEC_RCL_MCA *roentec_rcl_mca = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) record->record_class_struct;

	mx_status = mxd_roentec_rcl_get_pointers(mca, &roentec_rcl_mca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->maximum_num_rois > MX_ROENTEC_RCL_MAX_SCAS ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested maximum number of ROIs (%ld) for Roentec MCA '%s' is greater "
"than the maximum allowed value of %d.",
			mca->maximum_num_rois, record->name,
			MX_ROENTEC_RCL_MAX_SCAS );
	}

	if ( mca->maximum_num_channels > MX_ROENTEC_RCL_MAX_BINS ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested maximum number of channels (%ld) for Roentec MCA '%s' is greater "
"than the maximum allowed value of %d.",
			mca->maximum_num_channels, record->name,
			MX_ROENTEC_RCL_MAX_BINS );
	}

	mca->channel_array = ( unsigned long * )
		malloc( mca->maximum_num_channels * sizeof( unsigned long ) );

	if ( mca->channel_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an %ld channel data array.",
			mca->maximum_num_channels );
	}

#if ( MX_WORDSIZE != 32 )
	roentec_rcl_mca->channel_32bit_array = ( uint32_t * )
		malloc( mca->maximum_num_channels * sizeof( uint32_t ) );

	if ( roentec_rcl_mca->channel_32bit_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an %lu channel 32bit data array.",
			mca->maximum_num_channels );
	}
#endif

	mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;

	roentec_rcl_mca = (MX_ROENTEC_RCL_MCA *) record->record_type_struct;

	mx_status = mx_mca_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_delete_record( MX_RECORD *record )
{
	MX_MCA *mca;
	MX_ROENTEC_RCL_MCA *roentec_rcl_mca = NULL;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	roentec_rcl_mca = (MX_ROENTEC_RCL_MCA *) record->record_type_struct;

	if ( roentec_rcl_mca != NULL ) {

#if ( MX_WORDSIZE != 32 )
		mx_free( roentec_rcl_mca->channel_32bit_array );
#endif

		mx_free( record->record_type_struct );

		record->record_type_struct = NULL;
	}

	mca = (MX_MCA *) record->record_class_struct;

	if ( mca != NULL ) {

		if ( mca->channel_array != NULL ) {
			mx_free( mca->channel_array );

			mca->channel_array = NULL;
		}

		mx_free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_roentec_rcl_open()";

	MX_MCA *mca;
	MX_ROENTEC_RCL_MCA *roentec_rcl_mca = NULL;
	long i;
	char response[80];
	mx_status_type mx_status;

	roentec_rcl_mca = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_roentec_rcl_get_pointers(mca, &roentec_rcl_mca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_verify_configuration(roentec_rcl_mca->rs232_record,
			MXF_232_DONT_CARE, 8, 'N', 1, 'H', 0x0d, 0x0d );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca->current_num_channels = mca->maximum_num_channels;
	mca->current_num_rois     = mca->maximum_num_rois;

	for ( i = 0; i < mca->maximum_num_rois; i++ ) {
		mca->roi_array[i][0] = 0;
		mca->roi_array[i][1] = 0;
	}

	/* Verify communication with the controller by asking it how long
	 * it has been since the controller was powered on.
	 */

	mx_status = mxd_roentec_rcl_command( roentec_rcl_mca,
				"$OK", response, sizeof(response),
				MXD_ROENTEC_RCL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the send mode for the copy spectrum to buffer (SS) command.
	 * We set the data format to 4 (4 bytes/channel) and the send mode
	 * to 0 (accumulate spectrum; don't clear after sending)
	 */

	mx_status = mxd_roentec_rcl_command( roentec_rcl_mca,
				"$SM 4 0", NULL, 0, MXD_ROENTEC_RCL_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_roentec_rcl_resynchronize()";

	MX_MCA *mca;
	MX_ROENTEC_RCL_MCA *roentec_rcl_mca = NULL;
	char response[80];
	mx_status_type mx_status;

	roentec_rcl_mca = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_roentec_rcl_get_pointers(mca, &roentec_rcl_mca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ROENTEC_RCL_DEBUG
	/* Display the state of the RS-232 signal lines. */

	mx_status = mx_rs232_print_signal_state(roentec_rcl_mca->rs232_record);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	mx_status = mx_rs232_discard_unwritten_output(
				roentec_rcl_mca->rs232_record, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input(
				roentec_rcl_mca->rs232_record, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a software reset of the controller. */

	mx_status = mx_rs232_putline( roentec_rcl_mca->rs232_record, "$##",
					NULL, MXD_ROENTEC_RCL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Roentec sends a garbage response line. */

	mx_status = mx_rs232_getline( roentec_rcl_mca->rs232_record,
					response, sizeof(response),
					NULL, MXD_ROENTEC_RCL_DEBUG );

	/* Wait a short time for the reset to complete. */

	mx_msleep(1000);

	/* Read the startup banner sent by the Roentec controller. */

	mx_status = mx_rs232_getline( roentec_rcl_mca->rs232_record,
					response, sizeof(response),
					NULL, MXD_ROENTEC_RCL_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_ROENTEC_RCL_COMMAND:
		case MXLV_ROENTEC_RCL_RESPONSE:
		case MXLV_ROENTEC_RCL_COMMAND_WITH_RESPONSE:
			record_field->process_function
					    = mxd_roentec_rcl_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_start( MX_MCA *mca )
{
	static const char fname[] = "mxd_roentec_rcl_start()";

	MX_ROENTEC_RCL_MCA *roentec_rcl_mca = NULL;
	char command[40];
	mx_status_type mx_status;

	roentec_rcl_mca = NULL;

	mx_status = mxd_roentec_rcl_get_pointers(mca, &roentec_rcl_mca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The command sent depends on the preset type. */

	switch( mca->preset_type ) {
	case MXF_MCA_PRESET_NONE:
		sprintf( command, "$MT 0" );
		break;
	case MXF_MCA_PRESET_LIVE_TIME:
		sprintf( command, "$LT %ld",
			mx_round( 1000.0 * mca->preset_live_time ) );
		break;
	case MXF_MCA_PRESET_REAL_TIME:
		sprintf( command, "$MT %ld",
			mx_round( 1000.0 * mca->preset_real_time ) );
		break;
	case MXF_MCA_PRESET_COUNT:
		/* Initialize the window counter. */

		sprintf( command, "$FI %lu 0 %ld",
			mca->preset_count, mca->current_num_channels );

		mx_status = mxd_roentec_rcl_command( roentec_rcl_mca,
				command, NULL, 0, MXD_ROENTEC_RCL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Start counting with an acquisition time of 0.  This will
		 * cause the MCA to continue counting until the preset
		 * window counter value is reached.
		 */

		sprintf( command, "$MT 0" );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Preset type %ld is not supported for record '%s'",
			mca->preset_type, mca->record->name );
	}

	/* Send the command to the MCA. */

	mx_status = mxd_roentec_rcl_command( roentec_rcl_mca,
				command, NULL, 0, MXD_ROENTEC_RCL_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_stop( MX_MCA *mca )
{
	static const char fname[] = "mxd_roentec_rcl_stop()";

	MX_ROENTEC_RCL_MCA *roentec_rcl_mca = NULL;
	mx_status_type mx_status;

	roentec_rcl_mca = NULL;

	mx_status = mxd_roentec_rcl_get_pointers(mca, &roentec_rcl_mca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the MCA. */

	mx_status = mxd_roentec_rcl_command( roentec_rcl_mca,
				"$MP +", NULL, 0, MXD_ROENTEC_RCL_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_roentec_rcl_read()";

	MX_ROENTEC_RCL_MCA *roentec_rcl_mca = NULL;
	char command[40];
	mx_status_type mx_status;

	roentec_rcl_mca = NULL;

	mx_status = mxd_roentec_rcl_get_pointers(mca, &roentec_rcl_mca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Request transfer of the spectrum.  The MCA will acknowledge
	 * the command with the one line response '!SS<cr>'.
	 */

	sprintf( command, "$SS 0,1,1,%ld",
			mca->current_num_channels );

	mx_status = mxd_roentec_rcl_command( roentec_rcl_mca,
				command, NULL, 0, MXD_ROENTEC_RCL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* After the acknowledgement of the command, the actual spectrum
	 * data is transferred.
	 */

#if ( MX_WORDSIZE == 32 )
	mx_status = mxd_roentec_rcl_read_32bit_array( roentec_rcl_mca,
				mca->current_num_channels,
				(uint32_t *) mca->channel_array,
				MXD_ROENTEC_RCL_DEBUG_TIMING );

#else /* MX_WORDSIZE is not 32 bits. */

	mx_status = mxd_roentec_rcl_read_32bit_array( roentec_rcl_mca,
				mca->current_num_channels,
				roentec_rcl_mca->channel_32bit_array,
				MXD_ROENTEC_RCL_DEBUG_TIMING );
	{
		int i;

		for ( i = 0; i < MX_ROENTEC_RCL_MAX_BINS; i++ ) {
			mca->channel_array[i] =
		    (unsigned long) roentec_rcl_mca->channel_32bit_array[i];
		}
	}
#endif
	if ( mx_status.code == MXE_END_OF_DATA )
		return MX_SUCCESSFUL_RESULT;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_roentec_rcl_clear()";

	MX_ROENTEC_RCL_MCA *roentec_rcl_mca = NULL;
	mx_status_type mx_status;

	roentec_rcl_mca = NULL;

	mx_status = mxd_roentec_rcl_get_pointers(mca, &roentec_rcl_mca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_roentec_rcl_command( roentec_rcl_mca, "$CC", NULL, 0,
						MXD_ROENTEC_RCL_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_roentec_rcl_busy()";

	MX_ROENTEC_RCL_MCA *roentec_rcl_mca = NULL;
	char response[80];
	int num_items;
	char pause_status;
	mx_status_type mx_status;

	roentec_rcl_mca = NULL;

	mx_status = mxd_roentec_rcl_get_pointers(mca, &roentec_rcl_mca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_roentec_rcl_command( roentec_rcl_mca, "$FP",
						response, sizeof(response),
						MXD_ROENTEC_RCL_DEBUG );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "!FP %c", &pause_status );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cannot find the pause status for MCA '%s' in the "
		"response to the command '$FP'.  Response = '%s'",
			mca->record->name, response );
	}

	switch( pause_status ) {
	case '+':
		mca->busy = FALSE;
		break;
	case '-':
		mca->busy = TRUE;
		break;
	default:
		mca->busy = FALSE;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable pause status character '%c' seen in "
		"the response '%s' to command '$FP' for MCA '%s'.",
			pause_status, response, mca->record->name );
	}

#if MXD_ROENTEC_RCL_DEBUG
	MX_DEBUG(-2,("%s: pause_status = '%c', mca->busy = %d",
				fname, pause_status, mca->busy));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_get_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_roentec_rcl_get_parameter()";

	MX_ROENTEC_RCL_MCA *roentec_rcl_mca = NULL;
	int num_items, atomic_number;
	unsigned long i, ulong_value;
	unsigned long starting_channel, ending_channel;
	long summation_width;
	uint32_t roi_integral;
	double raw_energy_gain, raw_energy_offset;
	double low_ev, high_ev;
	double raw_low_channel, raw_high_channel;
	double gate_time_in_ms;
	char element_name[40];
	char command[80], response[80];
	mx_status_type mx_status;

	roentec_rcl_mca = NULL;

	mx_status = mxd_roentec_rcl_get_pointers(mca, &roentec_rcl_mca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	switch( mca->parameter_type ) {
	case MXLV_MCA_REAL_TIME:
		mx_status = mxd_roentec_rcl_command( roentec_rcl_mca, "$MS",
					response, sizeof(response),
					MXD_ROENTEC_RCL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "!MS %lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to find the acquisition real time in "
			"the response to the '$MS' command sent to "
			"Roentec MCA '%s'.  Response = '%s'",
				mca->record->name, response );
		}

		mca->real_time = 0.001 * (double) ulong_value;
		break;
	case MXLV_MCA_LIVE_TIME:
		mx_status = mxd_roentec_rcl_command( roentec_rcl_mca, "$LS",
					response, sizeof(response),
					MXD_ROENTEC_RCL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "!LS %lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to find the acquisition live time in "
			"the response to the '$LS' command sent to "
			"Roentec MCA '%s'.  Response = '%s'",
				mca->record->name, response );
		}

		mca->live_time = 0.001 * (double) ulong_value;
		break;
	case MXLV_MCA_CHANNEL_VALUE:
		i = mca->channel_number;

		mca->channel_value = mca->channel_array[i];
		break;
	case MXLV_MCA_ROI:
		/* Get the conversion factors between energy and
		 * channel number units.
		 */

		mx_status = mxd_roentec_rcl_get_energy_gain_and_offset(
			roentec_rcl_mca, &raw_energy_gain, &raw_energy_offset );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Get the ROI boundaries in energy units.  If the ROI has
		 * not yet been set, the 'element_name' field will be
		 * returned as a '\0' character followed by the 'low_ev'
		 * and 'high_ev' fields.  In order to make sure that we
		 * do not leave the trailing 'low_ev' and 'high_ev' fields
		 * behind in the input buffer, we set the MXF_232_IGNORE_NULLS
		 * flag so that '\0' characters are discarded.  The symptom
		 * of this happening is that the sscanf() below will only
		 * return 3 items rather than the 4 specified in the format
		 * string.
		 */

		i = mca->roi_number;

		sprintf( command, "$GK %lu", i+1 );

		mx_status = mxd_roentec_rcl_command( roentec_rcl_mca,
				command, response, sizeof(response),
				MXD_ROENTEC_RCL_DEBUG | MXF_232_IGNORE_NULLS );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "!GK %d %s %lg %lg",
					&atomic_number, element_name,
					&low_ev, &high_ev );

		if ( num_items == 3 ) {
			MX_DEBUG( 2,("%s: ROI %lu not set.", fname, i));

			mca->roi[0] = 0;
			mca->roi[1] = 0;

			return MX_SUCCESSFUL_RESULT;
		}

		if ( num_items != 4 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to find ROI data for ROI %lu "
			"of Roentec MCA '%s' in the response to command '%s'.  "
			"Response = '%s'", i, mca->record->name,
				command, response );
		}

		MX_DEBUG( 2,("%s: low_ev = %g, high_ev = %g",
			fname, low_ev, high_ev));

		raw_low_channel =
			10000.0 * mx_divide_safely( low_ev, raw_energy_gain )
					+ 0.01 * raw_energy_offset;

		raw_high_channel =
			10000.0 * mx_divide_safely( high_ev, raw_energy_gain )
					+ 0.01 * raw_energy_offset;

		mca->roi[0] = mx_round( raw_low_channel );
		mca->roi[1] = mx_round( raw_high_channel );
		break;
	case MXLV_MCA_ROI_INTEGRAL:

		i = mca->roi_number;

		/* Refresh the boundaries of the ROI. */

		mx_status = mx_mca_get_roi( roentec_rcl_mca->record, i, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Send the command to read the integral. */

		starting_channel = mca->roi_array[i][0];
		ending_channel   = mca->roi_array[i][1];

		summation_width = ((long) ending_channel)
					- ((long) starting_channel) + 1L;

		MX_DEBUG( 2,
		("%s: i = %lu, starting_channel = %lu, ending_channel = %lu",
			fname, i, starting_channel, ending_channel));
		MX_DEBUG( 2,("%s: summation_width = %ld",
			fname, summation_width));

		if ( summation_width < 0 ) {
			return mx_error(MXE_HARDWARE_CONFIGURATION_ERROR, fname,
			"The computed summation width %ld for "
			"MCA '%s', ROI %lu is not a positive number.",
				summation_width, mca->record->name, i );
		}

		sprintf( command, "$SS %lu, 1, %ld, 1",
				starting_channel, summation_width );

		mx_status = mxd_roentec_rcl_command( roentec_rcl_mca, command,
						response, sizeof(response),
						MXD_ROENTEC_RCL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* After the acknowledgement of the command, the actual ROI
		 * integral is transferred in binary.
	 	*/

		mx_status = mxd_roentec_rcl_read_32bit_array( roentec_rcl_mca,
				1, &roi_integral, MXD_ROENTEC_RCL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mca->roi_integral = roi_integral;
		break;
	case MXLV_MCA_INPUT_COUNT_RATE:
		/* Get the ICR gate time. */

		mx_status = mxd_roentec_rcl_command( roentec_rcl_mca, "$TC",
						response, sizeof(response),
						MXD_ROENTEC_RCL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "!TC %lg", &gate_time_in_ms );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to find the ICR gate time in "
			"the response to the '$TC' command sent to "
			"Roentec MCA '%s'.  Response = '%s'",
				mca->record->name, response );
		}

		/* Get the input count rate. */

		mx_status = mxd_roentec_rcl_command( roentec_rcl_mca, "$BC",
						response, sizeof(response),
						MXD_ROENTEC_RCL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "!BC %lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to find the input count rate in "
			"the response to the '$BC' command sent to "
			"Roentec MCA '%s'.  Response = '%s'",
				mca->record->name, response );
		}

		mca->input_count_rate = mx_divide_safely( (double) ulong_value,
							gate_time_in_ms );
		break;
	case MXLV_MCA_OUTPUT_COUNT_RATE:
		/* Get the OCR gate time. */

		mx_status = mxd_roentec_rcl_command( roentec_rcl_mca, "$TC",
						response, sizeof(response),
						MXD_ROENTEC_RCL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "!TC %lg", &gate_time_in_ms );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to find the ICR gate time in "
			"the response to the '$TC' command sent to "
			"Roentec MCA '%s'.  Response = '%s'",
				mca->record->name, response );
		}

		/* Get the output count rate. */

		mx_status = mxd_roentec_rcl_command( roentec_rcl_mca, "$NC",
						response, sizeof(response),
						MXD_ROENTEC_RCL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "!NC %lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to find the output count rate in "
			"the response to the '$NC' command sent to "
			"Roentec MCA '%s'.  Response = '%s'",
				mca->record->name, response );
		}

		mca->output_count_rate = mx_divide_safely( (double) ulong_value,
							gate_time_in_ms );
		break;
	default:
		mx_status = mx_mca_default_get_parameter_handler( mca );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_set_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_roentec_rcl_set_parameter()";

	MX_ROENTEC_RCL_MCA *roentec_rcl_mca = NULL;
	unsigned long i;
	double raw_energy_gain, raw_energy_offset;
	double low_ev, high_ev;
	unsigned long low_ev_rounded, high_ev_rounded;
	char command[80];
	mx_status_type mx_status;

	roentec_rcl_mca = NULL;

	mx_status = mxd_roentec_rcl_get_pointers(mca, &roentec_rcl_mca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	switch( mca->parameter_type ) {
	case MXLV_MCA_ROI:
		/* Get the conversion factors between energy and
		 * channel number units.
		 */

		i = mca->roi_number;

		mx_status = mxd_roentec_rcl_get_energy_gain_and_offset(
			roentec_rcl_mca, &raw_energy_gain, &raw_energy_offset );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the ROI limits in energy units. */

		low_ev = 0.0001 * raw_energy_gain
		    * ( ((double) mca->roi[0]) - 0.01 * raw_energy_offset );

		high_ev = 0.0001 * raw_energy_gain
		    * ( ((double) mca->roi[1]) - 0.01 * raw_energy_offset );

		low_ev_rounded = mx_round( low_ev );
		high_ev_rounded = mx_round( high_ev );

		sprintf( command, "$SK %lu %lu %lu %lu %lu",
			i+1, i+1, i+1, low_ev_rounded, high_ev_rounded );

		mx_status = mxd_roentec_rcl_command( roentec_rcl_mca, command,
						NULL, 0, MXD_ROENTEC_RCL_DEBUG);
		break;
	default:
		mx_status = mx_mca_default_set_parameter_handler( mca );
		break;
	}

	return mx_status;
}

/* === Functions specific to this driver. === */

static mx_status_type
mxd_roentec_rcl_handle_error( MX_ROENTEC_RCL_MCA *roentec_rcl_mca,
					char *command,
					char *response_buffer )
{
	static const char fname[] = "mxd_roentec_rcl_handle_error()";

	char *ptr;
	unsigned long error_code;
	int num_items;
	char error_message[80];

	ptr = strchr( response_buffer, ':' );

	if ( ptr == NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cannot parse the error message '%s' from Roentec MCA '%s' "
		"in response to the command '%s'.",
		    response_buffer, roentec_rcl_mca->record->name, command );
	}

	ptr++;

	num_items = sscanf( ptr, "%lu", &error_code );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not find an error code in the response '%s' from "
		"Roentec MCA '%s' to the command '%s'.",
		    response_buffer, roentec_rcl_mca->record->name, command );
	}

	switch( error_code ) {
	case 0:
		strlcpy( error_message,
			"General error or buffer overflow",
			sizeof(error_message) );
		break;
	case 1:
		strlcpy( error_message,
			"Unknown command",
			sizeof(error_message) );
		break;
	case 2:
		strlcpy( error_message,
			"Numeric parameter expected",
			sizeof(error_message) );
		break;
	case 4:
		strlcpy( error_message,
			"Boolean parameter expected",
			sizeof(error_message) );
		break;
	case 5:
		strlcpy( error_message,
			"Additional parameter expected",
			sizeof(error_message) );
		break;
	case 6:
		strlcpy( error_message,
			"Unexpected parameter or character",
			sizeof(error_message) );
		break;
	case 7:
		strlcpy( error_message,
			"Illegal numeric value",
			sizeof(error_message) );
		break;
	case 8:
		strlcpy( error_message,
			"Unknown subcommand",
			sizeof(error_message) );
		break;
	case 9:
		strlcpy( error_message,
			"Function not implemented or no hardware support",
			sizeof(error_message) );
		break;
	case 10:
		strlcpy( error_message,
			"Flash-EPROM programming fault",
			sizeof(error_message) );
		break;
	case 11:
		strlcpy( error_message,
			"Error clearing Flash-EPROM",
			sizeof(error_message) );
		break;
	case 12:
		strlcpy( error_message,
			"Flash-EPROM read error",
			sizeof(error_message) );
		break;
	case 13:
		strlcpy( error_message,
			"Hardware error",
			sizeof(error_message) );
		break;
	case 16:
		strlcpy( error_message,
			"Illegal baud rate",
			sizeof(error_message) );
		break;
	default:
		strlcpy( error_message,
			"Unrecognized error code",
			sizeof(error_message) );
		break;
	}

	return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Error detected in response to command '%s' send to "
		"Roentec MCA '%s'.  Error code = %lu, error message = '%s', "
		"Raw response = '%s'",
			command, roentec_rcl_mca->record->name,
			error_code, error_message, response_buffer );
}

MX_EXPORT mx_status_type
mxd_roentec_rcl_command( MX_ROENTEC_RCL_MCA *roentec_rcl_mca,
		char *command,
		char *response, size_t max_response_length,
		int transfer_flags )
{
	static const char fname[] = "mxd_roentec_rcl_command()";

	char response_buffer[MXU_ROENTEC_RCL_MAX_COMMAND_LENGTH+1];
	MX_RS232 *rs232;
	int debug_flag, modified_transfer_flags;
	mx_status_type mx_status;

#if MXD_ROENTEC_RCL_DEBUG_TIMING	
	MX_HRT_RS232_TIMING command_timing;
	MX_HRT_RS232_TIMING response_timing;
#endif

	MX_DEBUG( 2,("%s invoked for record '%s'.",
		fname, roentec_rcl_mca->record->name));

	debug_flag = transfer_flags & MXD_ROENTEC_RCL_DEBUG;

	modified_transfer_flags = transfer_flags & ( ~ MXD_ROENTEC_RCL_DEBUG );

	if ( roentec_rcl_mca == (MX_ROENTEC_RCL_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ROENTEC_RCL_MCA pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}
	if ( roentec_rcl_mca->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for record '%s' is NULL.",
			roentec_rcl_mca->record->name );
	}

	rs232 = (MX_RS232 *)
		roentec_rcl_mca->rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RS232 pointer for RS232 record '%s' "
		"used by '%s' is NULL.",
			roentec_rcl_mca->rs232_record->name,
			roentec_rcl_mca->record->name );
	}

	/* Send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, roentec_rcl_mca->record->name));
	}

#if 0 && MXD_ROENTEC_RCL_DEBUG
	mx_status = mx_rs232_print_signal_state(roentec_rcl_mca->rs232_record);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

#if MXD_ROENTEC_RCL_DEBUG_TIMING	
	MX_HRT_RS232_START_COMMAND( command_timing, 2 + strlen(command) );
#endif

	mx_status = mx_rs232_putline( roentec_rcl_mca->rs232_record,
					command, NULL, 0 );

#if MXD_ROENTEC_RCL_DEBUG_TIMING
	MX_HRT_RS232_END_COMMAND( command_timing );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response, if one is expected. */

#if MXD_ROENTEC_RCL_DEBUG_TIMING
	MX_HRT_RS232_START_RESPONSE( response_timing,
				roentec_rcl_mca->rs232_record );
#endif

	mx_status = mx_rs232_getline( roentec_rcl_mca->rs232_record,
			response_buffer, sizeof(response_buffer), NULL,
			modified_transfer_flags );

#if MXD_ROENTEC_RCL_DEBUG_TIMING
	MX_HRT_RS232_END_RESPONSE( response_timing, strlen(response_buffer) );

	MX_HRT_RS232_COMMAND_RESULTS( command_timing, command, fname );

	MX_HRT_TIME_BETWEEN_MEASUREMENTS( command_timing,
						response_timing, fname );

	MX_HRT_RS232_RESPONSE_RESULTS( response_timing, response_buffer, fname);
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Was there a warning or error in the response?  If so,
	 * return an appropriate error to the caller.
	 */

	if ( strncmp( response_buffer, "!ERROR", 6 ) == 0 ) {
		return mxd_roentec_rcl_handle_error( roentec_rcl_mca,
						command, response_buffer );
	}

	/* If the caller wanted a response, copy it from the
	 * response buffer.
	 */

	if ( response != (char *) NULL ) {

		strlcpy( response, response_buffer, max_response_length );

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, response,
				roentec_rcl_mca->record->name));
		}
	}

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxd_roentec_rcl_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxd_roentec_rcl_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_ROENTEC_RCL_MCA *roentec_rcl_mca;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	roentec_rcl_mca = (MX_ROENTEC_RCL_MCA *)
					record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_ROENTEC_RCL_RESPONSE:
			/* Nothing to do since the necessary string is
			 * already stored in the 'response' field.
			 */

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_ROENTEC_RCL_COMMAND:
			mx_status = mxd_roentec_rcl_command(
					roentec_rcl_mca,
					roentec_rcl_mca->command,
					NULL, 0, MXD_ROENTEC_RCL_DEBUG );

			break;
		case MXLV_ROENTEC_RCL_COMMAND_WITH_RESPONSE:
			mx_status = mxd_roentec_rcl_command(
					roentec_rcl_mca,
					roentec_rcl_mca->command,
					roentec_rcl_mca->response,
					MXU_ROENTEC_RCL_MAX_COMMAND_LENGTH,
					MXD_ROENTEC_RCL_DEBUG );

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

