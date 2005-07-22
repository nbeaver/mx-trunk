/*
 * Name:    pr_ccd.c
 *
 * Purpose: Functions used to process MX CCD record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2003-2004 Illinois Institute of Technology
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
#include "mx_ccd.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_ccd_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_ccd_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_CCD_DATA_FRAME_SIZE:
		case MXLV_CCD_BIN_SIZE:
		case MXLV_CCD_STATUS:
		case MXLV_CCD_PRESET_TIME:
		case MXLV_CCD_STOP:
		case MXLV_CCD_READOUT:
		case MXLV_CCD_DEZINGER:
		case MXLV_CCD_CORRECT:
		case MXLV_CCD_WRITEFILE:
		case MXLV_CCD_HEADER_VARIABLE_CONTENTS:
			record_field->process_function
					    = mx_ccd_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_ccd_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_ccd_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_CCD *ccd;
	mx_status_type status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	ccd = (MX_CCD *) record->record_class_struct;

	status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_CCD_DATA_FRAME_SIZE:
			status = mx_ccd_get_data_frame_size(record, NULL, NULL);
			break;
		case MXLV_CCD_BIN_SIZE:
			status = mx_ccd_get_bin_size( record, NULL, NULL );
			break;
		case MXLV_CCD_STATUS:
			status = mx_ccd_get_status( record, NULL );
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
		case MXLV_CCD_BIN_SIZE:
			status = mx_ccd_set_bin_size( record,
					ccd->bin_size[0], ccd->bin_size[1] );
			break;
		case MXLV_CCD_PRESET_TIME:
			status = mx_ccd_start_for_preset_time( record,
							ccd->preset_time );
			break;
		case MXLV_CCD_STOP:
			status = mx_ccd_stop( record );
			break;
		case MXLV_CCD_READOUT:
			status = mx_ccd_readout( record, ccd->ccd_flags );
			break;
		case MXLV_CCD_DEZINGER:
			status = mx_ccd_dezinger( record, ccd->ccd_flags );
			break;
		case MXLV_CCD_CORRECT:
			status = mx_ccd_correct( record );
			break;
		case MXLV_CCD_WRITEFILE:
			status = mx_ccd_writefile( record, ccd->writefile_name,
							ccd->ccd_flags );
			break;
		case MXLV_CCD_HEADER_VARIABLE_CONTENTS:
			status = mx_ccd_write_header_variable( record,
						ccd->header_variable_name,
						ccd->header_variable_contents );
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

