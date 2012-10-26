/*
 * Name:    pr_sample_changer.c
 *
 * Purpose: Functions used to process MX sample_changer record events.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004, 2006, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_sample_changer.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_sample_changer_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_sample_changer_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_CHG_CONTROL_MODE:
		case MXLV_CHG_STATUS:
		case MXLV_CHG_CHANGER_FLAGS:
		case MXLV_CHG_INITIALIZE:
		case MXLV_CHG_SHUTDOWN:
		case MXLV_CHG_MOUNT_SAMPLE:
		case MXLV_CHG_UNMOUNT_SAMPLE:
		case MXLV_CHG_GRAB_SAMPLE:
		case MXLV_CHG_UNGRAB_SAMPLE:
		case MXLV_CHG_SELECT_SAMPLE_HOLDER:
		case MXLV_CHG_UNSELECT_SAMPLE_HOLDER:
		case MXLV_CHG_SOFT_ABORT:
		case MXLV_CHG_IMMEDIATE_ABORT:
		case MXLV_CHG_IDLE:
		case MXLV_CHG_RESET:
		case MXLV_CHG_COOLDOWN:
		case MXLV_CHG_DEICE:
			record_field->process_function
					= mx_sample_changer_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_sample_changer_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_sample_changer_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_SAMPLE_CHANGER *changer;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	changer = (MX_SAMPLE_CHANGER *) record->record_class_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	MX_DEBUG(-2,("**************************************************"));
	MX_DEBUG(-2,("%s: operation = %d, record_field = '%s'",
		fname, operation, record_field->name ));

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_CHG_STATUS:
			mx_status = mx_sample_changer_get_status(record, NULL);
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
		case MXLV_CHG_INITIALIZE:
			mx_status = mx_sample_changer_initialize( record );
			break;
		case MXLV_CHG_SHUTDOWN:
			mx_status = mx_sample_changer_shutdown( record );
			break;
		case MXLV_CHG_MOUNT_SAMPLE:
			mx_status = mx_sample_changer_mount_sample( record );
			break;
		case MXLV_CHG_UNMOUNT_SAMPLE:
			mx_status = mx_sample_changer_unmount_sample( record );
			break;
		case MXLV_CHG_GRAB_SAMPLE:
			mx_status = mx_sample_changer_grab_sample( record,
						changer->requested_sample_id );
			break;
		case MXLV_CHG_UNGRAB_SAMPLE:
			mx_status = mx_sample_changer_ungrab_sample( record );
			break;
		case MXLV_CHG_SELECT_SAMPLE_HOLDER:
			mx_status = mx_sample_changer_select_sample_holder(
				record, changer->requested_sample_holder);
			break;
		case MXLV_CHG_UNSELECT_SAMPLE_HOLDER:
			mx_status = mx_sample_changer_unselect_sample_holder(
								record );
			break;
		case MXLV_CHG_SOFT_ABORT:
			mx_status = mx_sample_changer_soft_abort( record );
			break;
		case MXLV_CHG_IMMEDIATE_ABORT:
			mx_status = mx_sample_changer_immediate_abort( record );
			break;
		case MXLV_CHG_IDLE:
			mx_status = mx_sample_changer_idle( record );
			break;
		case MXLV_CHG_RESET:
			mx_status = mx_sample_changer_reset( record );
			break;
		case MXLV_CHG_COOLDOWN:
			mx_status = mx_sample_changer_cooldown( record );
			break;
		case MXLV_CHG_DEICE:
			mx_status = mx_sample_changer_deice( record );
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

