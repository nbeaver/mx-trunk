/*
 * Name:    d_bluice_area_detector.c
 *
 * Purpose: MX driver for Blu-Ice controlled area detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008-2012 Illinois Institute of Technology
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
#include "d_bluice_motor.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_bluice_area_detector_record_function_list = {
	mxd_bluice_area_detector_initialize_driver,
	mxd_bluice_area_detector_create_record_structures,
	mx_area_detector_finish_record_initialization,
	NULL,
	NULL,
	mxd_bluice_area_detector_open,
	NULL,
	mxd_bluice_area_detector_finish_delayed_initialization
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_bluice_area_detector_ad_function_list = {
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
	mxd_bluice_area_detector_transfer_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_bluice_area_detector_get_parameter,
	mxd_bluice_area_detector_set_parameter,
	mx_area_detector_default_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_bluice_dcss_area_detector_defaults[]
= {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_BLUICE_DCSS_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_bluice_dcss_area_detector_num_record_fields
	= sizeof( mxd_bluice_dcss_area_detector_defaults )
	    / sizeof( mxd_bluice_dcss_area_detector_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_dcss_area_detector_rfield_def_ptr
		= &mxd_bluice_dcss_area_detector_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxd_bluice_dhs_area_detector_defaults[]
= {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_BLUICE_DHS_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_bluice_dhs_area_detector_num_record_fields
	= sizeof( mxd_bluice_dhs_area_detector_defaults )
	    / sizeof( mxd_bluice_dhs_area_detector_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_dhs_area_detector_rfield_def_ptr
		= &mxd_bluice_dhs_area_detector_defaults[0];

/*-------------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------------*/

#if 0

static MX_RECORD *
mxd_bluice_area_detector_find_motor( MX_RECORD *record_list, char *motor_name )
{
	MX_RECORD *current_record;
	MX_BLUICE_MOTOR *bluice_motor;

	if ( record_list == NULL )
		return NULL;

	/* Make sure this is really the list head record. */

	record_list = record_list->list_head;

	/* Search for the requested Blu-Ice motor name. */

	current_record = record_list->next_record;

	while ( current_record != record_list ) {
	    switch( current_record->mx_type ) {
	    case MXT_MTR_BLUICE_DCSS:
	    case MXT_MTR_BLUICE_DHS:
	    	bluice_motor = current_record->record_type_struct;

		if ( strcmp(bluice_motor->bluice_name, motor_name) == 0 ) {
			return current_record;
		}
	    }
	    current_record = current_record->next_record;
	}

	return NULL;
}

#endif

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_bluice_area_detector_collect_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxd_bluice_area_detector_collect_thread()";

	MX_AREA_DETECTOR *ad;
	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *collect_operation;
	MX_BLUICE_FOREIGN_DEVICE *transfer_operation;
	MX_BLUICE_FOREIGN_DEVICE *oscillation_ready_operation;
	int num_items;
	int current_collect_state, commanded_collect_state;
	int current_transfer_state, current_oscillation_ready_state;
	char command[200];
	char start_oscillation_format[40];
	char prepare_for_oscillation_format[40];
	char request_name[40];
	char motor_name[MXU_BLUICE_NAME_LENGTH+1];
	double oscillation_time, new_motor_position;
	unsigned long client_number;
	int32_t operation_counter;
	mx_status_type mx_status;

	if ( args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) args;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
		&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for Blu-Ice detector '%s'",
		fname, ad->record->name ));
#endif
	collect_operation = bluice_area_detector->collect_operation;

	if ( collect_operation == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
	    "No collect operation has been configured for area detector '%s'.",
	    		ad->record->name );
	}

	transfer_operation = bluice_area_detector->transfer_operation;

	if ( transfer_operation == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
	    "No transfer operation has been configured for area detector '%s'.",
	    		ad->record->name );
	}

	oscillation_ready_operation =
		bluice_area_detector->oscillation_ready_operation;

	if ( oscillation_ready_operation == (MX_BLUICE_FOREIGN_DEVICE *) NULL )
	{
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
"No oscillation_ready operation has been configured for area detector '%s'.",
	    		ad->record->name );
	}

	snprintf( start_oscillation_format,
		sizeof(start_oscillation_format),
			"%%%ds %%%ds %%lg",
			(int) sizeof(request_name)+1,
			(int) sizeof(motor_name)+1 );

	snprintf( prepare_for_oscillation_format,
		sizeof(prepare_for_oscillation_format),
			"%%%ds %%lg",
			(int) sizeof(request_name)+1 );

	/*-------------------------------------------*/

	/* Send the collect command which was formatted in the main thread
	 * in the function mxd_bluice_area_detector_trigger().
	 */

	commanded_collect_state = MXSF_BLUICE_OPERATION_STARTED;

	mx_bluice_set_operation_state( collect_operation,
						commanded_collect_state );

	mx_status = mx_bluice_send_message( bluice_server->record,
				bluice_area_detector->collect_command, NULL, 0);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Collect operation may perform multiple exposures,
	 * so we loop over the exposures here.
	 */

	while (1) {
		/*-------------------------------------------*/

		/* Wait until the Collect operation changes its state. */

		current_collect_state = mx_bluice_get_operation_state(
							collect_operation );

		while ( current_collect_state == commanded_collect_state ) {
			mx_msleep(1);

			current_collect_state = mx_bluice_get_operation_state(
							collect_operation );
		}

		if ( current_collect_state != MXSF_BLUICE_OPERATION_UPDATED ) {
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Unexpected operation state %d seen for the collect "
			"operation of Blu-Ice area detector '%s'.",
				current_collect_state, ad->record->name );
		}

		/*-------------------------------------------*/

		/* We should have received a start_oscillation request here.
		 * Try to parse it.
		 */

		num_items = sscanf(
			collect_operation->u.operation.arguments_buffer,
			start_oscillation_format,
			request_name, motor_name, &oscillation_time );

		if ( num_items != 3 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"The operation update string '%s' for operation '%s' "
			"of area detector '%s' could not be parsed into a "
			"request name, motor name, and oscillation time.",
				collect_operation->u.operation.arguments_buffer,
				collect_operation->name,
				ad->record->name );
		}

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
	("%s: request_name = '%s', motor_name = '%s', oscillation_time = %f",
			fname, request_name, motor_name, oscillation_time));
#endif

		if ( strcmp( request_name, "start_oscillation" ) != 0 ) {
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Unexpected '%s' request seen in update '%s' for "
			"operation '%s' of area detector '%s'.  "
			"We expected to see 'start_oscillation' here.",
				request_name,
				collect_operation->u.operation.arguments_buffer,
				collect_operation->name,
				ad->record->name );
		}

		/*-------------------------------------------*/

		/* Acknowledge the update. */

		commanded_collect_state =
			MXSF_BLUICE_OPERATION_UPDATE_ACKNOWLEDGED;

		mx_bluice_set_operation_state( collect_operation,
						commanded_collect_state );

		/*-------------------------------------------*/

		/* FIXME: Here we either invoke an 'expose' operation or the
		 * 'stoh_start_oscillation' command.
		 */

		mx_warning( "FIXME FIXME FIXME!\n"
	"****** This is where we would do the 'expose' operation. ******\n"
	"FIXME FIXME FIXME!" );

		/* FIXME: Now we claim that the oscillation is over. */

		/*-------------------------------------------*/

		/* Send 'detector_transfer_image' to the area detector to tell
		 * it to read out the image.
		 */

		client_number = mx_bluice_get_client_number( bluice_server );

		operation_counter = mx_bluice_update_operation_counter(
							bluice_server );

		snprintf( command, sizeof(command),
			"stoh_start_operation detector_transfer_image %lu.%lu",
			client_number, (unsigned long) operation_counter );

		mx_bluice_set_operation_state( transfer_operation,
						MXSF_BLUICE_OPERATION_STARTED );

		mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/*-------------------------------------------*/

		/* Wait until the Transfer operation changes its state. */

		current_transfer_state =
			mx_bluice_get_operation_state( transfer_operation );

		while (current_transfer_state == MXSF_BLUICE_OPERATION_STARTED)
		{
			mx_msleep(1);

			current_transfer_state = mx_bluice_get_operation_state(
							transfer_operation );
		}

		if ( current_transfer_state != MXSF_BLUICE_OPERATION_COMPLETED )		{
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Unexpected operation state %d seen for the transfer "
			"operation of Blu-Ice area detector '%s'.",
				current_transfer_state, ad->record->name );
		}

		/*-------------------------------------------*/

		/* Wait until the Command operation changes its state. */

		current_collect_state =
			mx_bluice_get_operation_state( collect_operation );

		while ( current_collect_state == commanded_collect_state )
		{
			mx_msleep(1);

			current_collect_state = mx_bluice_get_operation_state(
							collect_operation );
		}

		switch( current_collect_state ) {
		case MXSF_BLUICE_OPERATION_UPDATED:
			/* We will be acquiring another frame. */
			break;
		case MXSF_BLUICE_OPERATION_COMPLETED:
			
#if MXD_BLUICE_AREA_DETECTOR_DEBUG
			MX_DEBUG(-2,
			("%s: COLLECT HAS SUCCEEDED! for detector '%s'.  "
			"This thread will exit now.",
				fname, ad->record->name ));
#endif

			/* We have acquired the LAST frame, so the collect
			 * operation is over.  We can now exit this thread.
			 */

			return MX_SUCCESSFUL_RESULT;

			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Unexpected operation state %d seen for the collect "
			"operation of Blu-Ice area detector '%s'.",
				current_collect_state, ad->record->name );
		}

		/*-------------------------------------------*/

		/* We should have received a prepare_for_oscillation request
		 * here.  Try to parse it.
		 */

		num_items = sscanf(
			collect_operation->u.operation.arguments_buffer,
			prepare_for_oscillation_format,
			request_name, &new_motor_position );

		if ( num_items != 2 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"The operation update string '%s' for operation '%s' "
			"of area detector '%s' could not be parsed into a "
			"request name and a new motor position.",
				collect_operation->u.operation.arguments_buffer,
				collect_operation->name,
				ad->record->name );
		}

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: request_name = '%s', new_motor_position = %f",
			fname, request_name, new_motor_position));
#endif

		if ( strcmp( request_name, "prepare_for_oscillation" ) != 0 ) {
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Unexpected '%s' request seen in update '%s' for "
			"operation '%s' of area detector '%s'.  "
			"We expected to see 'prepare_for_oscillation' here.",
				request_name,
				collect_operation->u.operation.arguments_buffer,
				collect_operation->name,
				ad->record->name );
		}

		/*-------------------------------------------*/

		/* Acknowledge the update. */

		commanded_collect_state =
			MXSF_BLUICE_OPERATION_UPDATE_ACKNOWLEDGED;

		mx_bluice_set_operation_state( collect_operation,
						commanded_collect_state );

		/*-------------------------------------------*/

		/* FIXME: Here we would move the motor to a new position. */

		mx_warning( "FIXME FIXME FIXME!\n"
	"****** This is where we would do the 'move' operation. ******\n"
	"FIXME FIXME FIXME!" );

		/* FIXME: Now we claim that the move is over. */

		/*-------------------------------------------*/

		/* Send 'detector_oscillation_ready' to the area detector
		 * to tell it that the motor has reached its new position.
		 */

		client_number = mx_bluice_get_client_number( bluice_server );

		operation_counter = mx_bluice_update_operation_counter(
							bluice_server );

		snprintf( command, sizeof(command),
		"stoh_start_operation detector_oscillation_ready %lu.%lu",
			client_number, (unsigned long) operation_counter );

		mx_bluice_set_operation_state( oscillation_ready_operation,
						MXSF_BLUICE_OPERATION_STARTED );

		mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/*-------------------------------------------*/

		/* Wait until the Oscillation_ready operation changes
		 * its state.
		 */

		current_oscillation_ready_state =
		    mx_bluice_get_operation_state(oscillation_ready_operation);

		while ( current_oscillation_ready_state
				== MXSF_BLUICE_OPERATION_STARTED )
		{
			mx_msleep(1);

			current_oscillation_ready_state =
				mx_bluice_get_operation_state(
					oscillation_ready_operation );
		}

		if ( current_oscillation_ready_state
				!= MXSF_BLUICE_OPERATION_COMPLETED )
		{
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Unexpected operation state %d seen for the "
			"oscillation_ready operation of Blu-Ice "
			"area detector '%s'.",
				current_oscillation_ready_state,
				ad->record->name );
		}

		/*-------------------------------------------*/

		/* Go back to the top of the loop for another pass. */

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
		("%s: Going back to the top of the loop for another frame.",
			fname));
#endif
	}

}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_bluice_area_detector_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
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
			&mxd_bluice_area_detector_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	bluice_area_detector->record = record;

	return MX_SUCCESSFUL_RESULT;
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
	/* Find the collect operation. */

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

	/* Find the detector stop operation. */

	mx_status = mx_bluice_get_device_pointer( bluice_server,
					"detector_stop",
					bluice_server->operation_array,
					bluice_server->num_operations,
				&(bluice_area_detector->stop_operation) );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_warning( "detector_stop operation not configured "
			"by Blu-Ice server '%s'", bluice_server->record->name );
	}

	/* For the bluice_dhs_area_detector driver, we must also find the
	 * transfer image, oscillation ready, and reset run operations.
	 */

	if ( record->mx_type == MXT_AD_BLUICE_DHS ) {

		mx_status = mx_bluice_get_device_pointer( bluice_server,
					"detector_transfer_image",
					bluice_server->operation_array,
					bluice_server->num_operations,
				&(bluice_area_detector->transfer_operation) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_bluice_get_device_pointer( bluice_server,
					"detector_oscillation_ready",
					bluice_server->operation_array,
					bluice_server->num_operations,
			&(bluice_area_detector->oscillation_ready_operation) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_bluice_get_device_pointer( bluice_server,
					"detector_reset_run",
					bluice_server->operation_array,
					bluice_server->num_operations,
				&(bluice_area_detector->reset_run_operation) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: collect_operation = %p",
		fname, bluice_area_detector->collect_operation));
	MX_DEBUG(-2,("%s: transfer_operation = %p",
		fname, bluice_area_detector->transfer_operation));
	MX_DEBUG(-2,("%s: oscillation_ready_operation = %p",
		fname, bluice_area_detector->oscillation_ready_operation));
	MX_DEBUG(-2,("%s: stop_operation = %p",
		fname, bluice_area_detector->stop_operation));
	MX_DEBUG(-2,("%s: reset_run_operation = %p",
		fname, bluice_area_detector->reset_run_operation));
#endif

	/* Find the string containing the filename of the most recently
	 * collected image.
	 */

	mx_status = mx_bluice_get_device_pointer( bluice_server,
					"lastImageCollected",
					bluice_server->string_array,
					bluice_server->num_strings,
			&(bluice_area_detector->last_image_collected_string) );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_warning( "lastImageCollected string not configured "
			"by Blu-Ice server '%s'", bluice_server->record->name );
	}

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: last_image_collected_string = %p",
		fname, bluice_area_detector->last_image_collected_string));
#endif

	/* See if a string called 'detectorType' has been sent to us.
	 * If it does exist, then we can figure out what the format of
	 * the detector's image frames are.
	 */

	mx_status = mx_bluice_get_device_pointer( bluice_server,
					"detectorType",
					bluice_server->string_array,
					bluice_server->num_strings,
					&detector_type_string );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_warning( "detectorType string not configured "
			"by Blu-Ice server '%s'", bluice_server->record->name );
	}

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
		ad->datafile_load_format = MXT_IMAGE_FILE_SMV;

		mx_warning( "No detector type was received from Blu-Ice "
		"server '%s', so the image format is assumed to be SMV.",
			bluice_server->record->name );

		ad->framesize[0] = 4096;
		ad->framesize[1] = 4096;
	} else
	if ( strcmp(detector_type, "MAR345") == 0 ) {
#if 0
		ad->datafile_load_format = MXT_IMAGE_FILE_TIFF;
#else
		ad->datafile_load_format = MXT_IMAGE_FILE_SMV;

		mx_info(
		"%s: FIXME: MAR345 image type has been forced to SMV.", fname );
#endif
		ad->framesize[0] = 4096;
		ad->framesize[1] = 4096;
	} else {
		ad->datafile_load_format = MXT_IMAGE_FILE_SMV;

		mx_warning( "Unrecognized detector type '%s' was reported "
		"by Blu-Ice area detector '%s'.  The image format is "
		"assumed to be SMV.", detector_type,
				bluice_server->record->name );

		ad->framesize[0] = 4096;
		ad->framesize[1] = 4096;
	}

	ad->image_format = MXT_IMAGE_FORMAT_GREY16;
	ad->byte_order = (long) mx_native_byteorder();
	ad->header_length = MXT_IMAGE_HEADER_LENGTH_IN_BYTES;

	ad->bytes_per_pixel = 2;
	ad->bits_per_pixel = 16;
	ad->bytes_per_frame = mx_round( ad->framesize[0] * ad->framesize[1]
				* ad->bytes_per_pixel );

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
	    mx_status = 
	    	mx_area_detector_setup_datafile_management( record, NULL );

	    if ( mx_status.code != MXE_SUCCESS )
	    	return mx_status;
	}

	bluice_area_detector->initialize_datafile_number = TRUE;

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

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	mx_status_type mx_status;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
		&bluice_area_detector, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	ad->last_frame_number = -1;

	if ( bluice_area_detector->initialize_datafile_number ) {
		mx_status =
		    mx_area_detector_initialize_datafile_number( ad->record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bluice_area_detector->initialize_datafile_number = FALSE;
	}

	mx_status = mx_area_detector_construct_next_datafile_name( ad->record );

	return mx_status;
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
	char dhs_username[MXU_USERNAME_LENGTH+1];
	char datafile_name[MXU_FILENAME_LENGTH+1];
	char datafile_directory[MXU_FILENAME_LENGTH+1];
	char *ptr;
	char motor_name[MXU_BLUICE_NAME_LENGTH+1];
	unsigned long dark_current_fu;
	unsigned long client_number;
	int32_t operation_counter;
	unsigned long flags;
	double detector_distance, wavelength;
	double detector_x, detector_y;
	mx_bool_type reuse_dark;
	mx_status_type mx_status;

	bluice_area_detector = NULL;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
		&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	collect_operation = bluice_area_detector->collect_operation;

	if ( collect_operation == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The collect operation for Blu-Ice area detector '%s' "
		"has not yet been initialized.", ad->record->name );
	}

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	if ( sp->sequence_type != MXT_SQ_ONE_SHOT ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Sequence type %ld is not supported for Blu-Ice detector '%s'."
		"  Only one-shot sequences are supported.",
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

	if ( strlen(datafile_name) == 0 ) {
		strlcpy( datafile_name, "{}", sizeof(datafile_name) );
	}

	MX_DEBUG(-2,("%s: datafile_name = '%s'", fname, datafile_name));

	strlcpy( datafile_directory, ad->datafile_directory,
			sizeof(datafile_directory) );

	if ( strlen(datafile_directory) == 0 ) {
		strlcpy( datafile_directory, "{}", sizeof(datafile_directory) );
	}

	MX_DEBUG(-2,("%s: datafile_directory = '%s'",
			fname, datafile_directory));

	client_number = mx_bluice_get_client_number( bluice_server );

	operation_counter = mx_bluice_update_operation_counter( bluice_server );

	flags = bluice_area_detector->bluice_flags;

	if ( flags & MXF_BLUICE_AD_REUSE_DARK ) {
		reuse_dark = TRUE;
	} else {
		reuse_dark = FALSE;
	}

	MX_DEBUG(-2,("%s: flags = %#lx, reuse_dark = %d",
		fname, flags, (int) reuse_dark));

	switch( ad->record->mx_type ) {
	case MXT_AD_BLUICE_DCSS:
		bluice_dcss_server = bluice_server->record->record_type_struct;

		snprintf( bluice_area_detector->collect_command,
			sizeof(bluice_area_detector->collect_command),
		"gtos_start_operation collectFrame %lu.%lu %lu %s %s %s "
			"NULL NULL 0 %f 0 1 %d",
			client_number,
			(unsigned long) operation_counter,
			dark_current_fu,
			datafile_name,
			datafile_directory,
			bluice_dcss_server->bluice_username,
			exposure_time,
			(int) reuse_dark );
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

		strlcpy( motor_name, "NULL", sizeof(motor_name) );

		snprintf( bluice_area_detector->collect_command,
			sizeof(bluice_area_detector->collect_command),
		"stoh_start_operation detector_collect_image %lu.%lu %lu "
		"%s %s %s %s %f 0.0 0.0 %f %f %f %f 0 %d",
			client_number,
			(unsigned long) operation_counter,
			dark_current_fu,
			datafile_name,
			ad->datafile_directory,
			dhs_username,
			motor_name,
			exposure_time,
			detector_distance,
			wavelength,
			detector_x,
			detector_y,
			(int) reuse_dark );

		/* If this is a DHS area detector record, we manage
		 * the detector_collect_image operation in a
		 * separate thread.
		 */

		collect_operation->u.operation.operation_state
				= MXSF_BLUICE_OPERATION_STARTED;

		mx_status = mx_thread_create(
			&(bluice_area_detector->collect_thread),
			mxd_bluice_area_detector_collect_thread,
			ad );

		return mx_status;
		break;
	}

	mx_status = mx_bluice_check_for_master( bluice_server );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	collect_operation->u.operation.operation_state
				= MXSF_BLUICE_OPERATION_STARTED;

	mx_status = mx_bluice_send_message( bluice_server->record,
				bluice_area_detector->collect_command, NULL, 0);
			
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
	unsigned long client_number;
	int32_t operation_counter;
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
			client_number, (unsigned long) operation_counter );
		break;
	case MXT_AD_BLUICE_DHS:
		snprintf( command, sizeof(command),
		"stoh_start_operation detector_stop %lu.%lu",
			client_number, (unsigned long) operation_counter );
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
	MX_BLUICE_FOREIGN_DEVICE *last_image_collected_string;
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
	    (unsigned long) collect_operation->u.operation.operation_counter));

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

	last_image_collected_string =
		bluice_area_detector->last_image_collected_string;

	if ( ( last_image_collected_string != NULL )
	  && ( last_image_collected_string->u.string.string_buffer != NULL ) )
	{
		strlcpy( bluice_area_detector->last_datafile_name,
			last_image_collected_string->u.string.string_buffer,
			sizeof(bluice_area_detector->last_datafile_name) );

		MX_DEBUG(-2,("%s: string_buffer = '%s'",
		fname, last_image_collected_string->u.string.string_buffer ));

		MX_DEBUG(-2,("%s: last_datafile_name = '%s'",
			fname, bluice_area_detector->last_datafile_name));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_bluice_area_detector_readout_frame()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	unsigned long client_number;
	int32_t operation_counter;
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

	if ( ad->record->mx_type == 0 /*MXT_AD_BLUICE_DHS*/ ) {
		client_number = mx_bluice_get_client_number( bluice_server );

		operation_counter = mx_bluice_update_operation_counter(
							bluice_server );

		snprintf( command, sizeof(command),
			"stoh_start_operation detector_transfer_image %lu.%lu",
			client_number,
			(unsigned long) operation_counter );

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
mxd_bluice_area_detector_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_bluice_area_detector_transfer_frame()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *last_image_collected_string;
	char image_filename[MXU_FILENAME_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
		&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Read in the last image written by Blu-Ice. */

	last_image_collected_string =
		bluice_area_detector->last_image_collected_string;

	if ( ( last_image_collected_string != NULL )
	  && ( last_image_collected_string->u.string.string_buffer != NULL ) )
	{
		strlcpy( image_filename,
			last_image_collected_string->u.string.string_buffer,
			sizeof(image_filename) );
	} else {
		snprintf( image_filename, sizeof(image_filename),
			"%s/%s.img", ad->datafile_directory,
				ad->datafile_name );
	}

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: image_filename = '%s'", fname, image_filename ));
#endif

	mx_status = mx_image_read_file( &(ad->image_frame),
					ad->datafile_load_format,
					image_filename );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: image framesize = (%lu,%lu)", fname,
		(unsigned long) MXIF_ROW_FRAMESIZE(ad->image_frame),
		(unsigned long) MXIF_COLUMN_FRAMESIZE(ad->image_frame) ));
#endif

	/* Update the area detector parameters. */

	ad->header_length   = (long) MXIF_HEADER_BYTES(ad->image_frame);

	ad->framesize[0]    = (long) MXIF_ROW_FRAMESIZE(ad->image_frame);
	ad->framesize[1]    = (long) MXIF_COLUMN_FRAMESIZE(ad->image_frame);

	ad->binsize[0]      = (long) MXIF_ROW_BINSIZE(ad->image_frame);
	ad->binsize[1]      = (long) MXIF_COLUMN_BINSIZE(ad->image_frame);

	ad->image_format    = (long) MXIF_IMAGE_FORMAT(ad->image_frame);
	ad->byte_order      = (long) MXIF_BYTE_ORDER(ad->image_frame);
	ad->bytes_per_pixel = MXIF_BYTES_PER_PIXEL(ad->image_frame);
	ad->bits_per_pixel  = (long) MXIF_BITS_PER_PIXEL(ad->image_frame);

	ad->bytes_per_frame = mx_round( ad->framesize[0] * ad->framesize[1]
						* ad->bytes_per_pixel );

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						sizeof(ad->image_format_name) );
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
			"Sequence type %ld is not supported for Blu-Ice "
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

