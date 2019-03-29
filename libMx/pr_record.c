/*
 * Name:    pr_record.c
 *
 * Purpose: Functions used to process generic MX events.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2006-2007, 2012-2013, 2019
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_motor.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_record_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_record_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_REC_PRECISION:
		case MXLV_REC_RESYNCHRONIZE:
			record_field->process_function
					    = mx_record_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_record_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mx_record_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FUNCTION_LIST *record_function_list;
	mx_status_type ( *resynchronize_fn ) ( MX_RECORD * );
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	record_function_list = (MX_RECORD_FUNCTION_LIST *)
					( record->record_function_list );

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_REC_PRECISION:
			record->long_precision = record->precision;
			break;
		case MXLV_REC_RESYNCHRONIZE:
			break;
		default:
			MX_DEBUG(-2,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_REC_PRECISION:
			record->precision = (int) record->long_precision;
			break;
		case MXLV_REC_RESYNCHRONIZE:
			resynchronize_fn = record_function_list->resynchronize;

			if ( resynchronize_fn == NULL ) {

				mx_status = mx_error( MXE_UNSUPPORTED, fname,
			    "Resynchronize is not supported for record '%s'",
					record->name );
			} else {
				mx_status = ( *resynchronize_fn ) ( record );
			}
			break;
		default:
			MX_DEBUG(-2,(
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

