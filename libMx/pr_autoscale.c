/*
 * Name:    pr_autoscale.c
 *
 * Purpose: Functions used to process MX autoscale record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_autoscale.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_autoscale_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_autoscale_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_AUT_ENABLED:
		case MXLV_AUT_LOW_LIMIT:
		case MXLV_AUT_HIGH_LIMIT:
		case MXLV_AUT_LOW_DEADBAND:
		case MXLV_AUT_HIGH_DEADBAND:
		case MXLV_AUT_MONITOR_VALUE:
		case MXLV_AUT_GET_CHANGE_REQUEST:
		case MXLV_AUT_CHANGE_CONTROL:
		case MXLV_AUT_MONITOR_OFFSET_INDEX:
		case MXLV_AUT_NUM_MONITOR_OFFSETS:
		case MXLV_AUT_MONITOR_OFFSET_ARRAY:
			record_field->process_function
					    = mx_autoscale_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_autoscale_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_autoscale_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_AUTOSCALE *autoscale;
	mx_status_type status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	autoscale = (MX_AUTOSCALE *) (record->record_class_struct);

	status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_AUT_NUM_MONITOR_OFFSETS:

			/* Nothing to do but just return the value. */

			break;

		case MXLV_AUT_MONITOR_VALUE:
			status = mx_autoscale_read_monitor( record, NULL );
			break;

		case MXLV_AUT_GET_CHANGE_REQUEST:
			status = mx_autoscale_get_change_request(record, NULL);
			break;

		case MXLV_AUT_MONITOR_OFFSET_INDEX:
			status = mx_autoscale_get_offset_index( record, NULL );
			break;

		case MXLV_AUT_MONITOR_OFFSET_ARRAY:
			status = mx_autoscale_get_offset_array( record,
								NULL, NULL );
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
		case MXLV_AUT_NUM_MONITOR_OFFSETS:
			status = mx_error( MXE_CLIENT_REQUEST_DENIED, fname,
				"The 'num_monitor_offsets' field may not be "
				"changed from a client program." );
			break;

		case MXLV_AUT_CHANGE_CONTROL:
			status = mx_autoscale_change_control( record,
					autoscale->change_control );
			break;

		case MXLV_AUT_MONITOR_OFFSET_INDEX:
			status = mx_autoscale_set_offset_index( record,
					autoscale->monitor_offset_index );
			break;

		case MXLV_AUT_MONITOR_OFFSET_ARRAY:
#if 0
			{
				unsigned long i;

				for ( i = 0;
					i < autoscale->num_monitor_offsets;
					i++ )
				{
					MX_DEBUG(-2,(
			"%s: autoscale->monitor_offset_array[%lu] = %g",
					fname, i,
					autoscale->monitor_offset_array[i]));
				}
			}
#endif
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
		break;
	}

	return status;
}

