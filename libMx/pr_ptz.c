/*
 * Name:    pr_ptz.c
 *
 * Purpose: Functions used to process MX Pan/Tilt/Zoom record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
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
#include "mx_ptz.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_ptz_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_ptz_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG( 2,("%s invoked for record '%s'.", fname, record->name));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_PTZ_COMMAND:
		case MXLV_PTZ_STATUS:
		case MXLV_PTZ_PAN_POSITION:
		case MXLV_PTZ_PAN_DESTINATION:
		case MXLV_PTZ_PAN_SPEED:
		case MXLV_PTZ_TILT_POSITION:
		case MXLV_PTZ_TILT_DESTINATION:
		case MXLV_PTZ_TILT_SPEED:
		case MXLV_PTZ_ZOOM_POSITION:
		case MXLV_PTZ_ZOOM_DESTINATION:
		case MXLV_PTZ_ZOOM_SPEED:
		case MXLV_PTZ_ZOOM_ON:
		case MXLV_PTZ_FOCUS_POSITION:
		case MXLV_PTZ_FOCUS_DESTINATION:
		case MXLV_PTZ_FOCUS_SPEED:
		case MXLV_PTZ_FOCUS_AUTO:
			record_field->process_function
					    = mx_ptz_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_ptz_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_ptz_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_PAN_TILT_ZOOM *ptz;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	ptz = (MX_PAN_TILT_ZOOM *) (record->record_class_struct);

	MX_DEBUG( 2,("%s: PTZ '%s', operation = %d, label_value = %ld",
		fname, record->name, operation, record_field->label_value ));

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_PTZ_STATUS:
			mx_status = mx_ptz_get_status( record, NULL );
			break;
		case MXLV_PTZ_PAN_POSITION:
			mx_status = mx_ptz_get_pan( record, NULL );
			break;
		case MXLV_PTZ_TILT_POSITION:
			mx_status = mx_ptz_get_tilt( record, NULL );
			break;
		case MXLV_PTZ_ZOOM_POSITION:
			mx_status = mx_ptz_get_zoom( record, NULL );
			break;
		case MXLV_PTZ_FOCUS_POSITION:
			mx_status = mx_ptz_get_focus( record, NULL );
			break;
		case MXLV_PTZ_PAN_SPEED:
			mx_status = mx_ptz_get_pan_speed( record, NULL );
			break;
		case MXLV_PTZ_TILT_SPEED:
			mx_status = mx_ptz_get_tilt_speed( record, NULL );
			break;
		case MXLV_PTZ_ZOOM_SPEED:
			mx_status = mx_ptz_get_zoom_speed( record, NULL );
			break;
		case MXLV_PTZ_FOCUS_SPEED:
			mx_status = mx_ptz_get_focus_speed( record, NULL );
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
		case MXLV_PTZ_COMMAND:
			mx_status = mx_ptz_command( record, ptz->command );
			break;
		case MXLV_PTZ_PAN_DESTINATION:
			mx_status = mx_ptz_set_pan( record,
						ptz->pan_destination );
			break;
		case MXLV_PTZ_TILT_DESTINATION:
			mx_status = mx_ptz_set_tilt( record,
						ptz->tilt_destination );
			break;
		case MXLV_PTZ_ZOOM_DESTINATION:
			mx_status = mx_ptz_set_zoom( record,
						ptz->zoom_destination );
			break;
		case MXLV_PTZ_FOCUS_DESTINATION:
			mx_status = mx_ptz_set_focus( record,
						ptz->focus_destination );
			break;
		case MXLV_PTZ_PAN_SPEED:
			mx_status = mx_ptz_set_pan_speed( record,
							ptz->pan_speed );
			break;
		case MXLV_PTZ_TILT_SPEED:
			mx_status = mx_ptz_set_tilt_speed( record,
							ptz->tilt_speed );
			break;
		case MXLV_PTZ_ZOOM_SPEED:
			mx_status = mx_ptz_set_zoom_speed( record,
							ptz->zoom_speed );
			break;
		case MXLV_PTZ_FOCUS_SPEED:
			mx_status = mx_ptz_set_focus_speed( record,
							ptz->focus_speed );
			break;
		case MXLV_PTZ_ZOOM_ON:
			if ( ptz->zoom_on == FALSE ) {
				mx_status = mx_ptz_zoom_off( record );
			} else {
				mx_status = mx_ptz_zoom_on( record );
			}
			break;
		case MXLV_PTZ_FOCUS_AUTO:
			if ( ptz->focus_auto == FALSE ) {
				mx_status = mx_ptz_focus_manual( record );
			} else {
				mx_status = mx_ptz_focus_auto( record );
			}
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

