/*
 * Name:    z_rdi_api.c
 *
 * Purpose: Implement's RDI's preferred API for area detectors.
 *
 * Author:  William Lavender
 *
 * WARNING: This code is not even close to finished.  Don't try to use it.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXZ_RDI_API_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_area_detector.h"

#include "z_rdi_api.h"

MX_RECORD_FUNCTION_LIST mxz_rdi_api_record_function_list = {
	NULL,
	mxz_rdi_api_create_record_structures,
	mxz_rdi_api_finish_record_initialization,
	NULL,
	NULL,
	mxz_rdi_api_open,
	NULL,
	NULL,
	NULL,
	mxz_rdi_api_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxz_rdi_api_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXZ_RDI_API_STANDARD_FIELDS
};

long mxz_rdi_api_num_record_fields
	= sizeof( mxz_rdi_api_field_defaults )
	/ sizeof( mxz_rdi_api_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxz_rdi_api_rfield_def_ptr
		= &mxz_rdi_api_field_defaults[0];

/*---*/

static mx_status_type mxz_rdi_api_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/*---*/

static mx_status_type
mxz_rdi_api_get_pointers( MX_RECORD *record,
				MX_RDI_API **rdi_api,
				const char *calling_fname )
{
	static const char fname[] = "mxz_rdi_api_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( rdi_api == (MX_RDI_API **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RDI_API pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*rdi_api = (MX_RDI_API *) record->record_type_struct;

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxz_rdi_api_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxz_rdi_api_create_record_structures()";

	MX_RDI_API *rdi_api;

	/* Allocate memory for the necessary structures. */

	rdi_api = (MX_RDI_API *) malloc( sizeof(MX_RDI_API) );

	if ( rdi_api == (MX_RDI_API *) NULL ) {
	        return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_RDI_API structure.");
	}

	/* Now set up the necessary pointers. */

	record->record_superclass_struct = NULL;
	record->record_class_struct = NULL;
	record->record_type_struct = rdi_api;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	rdi_api->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxz_rdi_api_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxz_rdi_api_finish_record_initialization()";

	MX_RDI_API *rdi_api;
	MX_RECORD_FIELD *command_arguments_field;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mx_status = mxz_rdi_api_get_pointers( record,
						&rdi_api, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}


MX_EXPORT mx_status_type
mxz_rdi_api_open( MX_RECORD *record )
{
	static const char fname[] = "mxz_rdi_api_open()";

	MX_RDI_API *rdi_api;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mx_status = mxz_rdi_api_get_pointers( record,
						&rdi_api, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxz_rdi_api_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxz_rdi_api_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(-2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_RDI_API_SCAN_PARAMETERS:
			record_field->process_function
						= mxz_rdi_api_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxz_rdi_api_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxz_rdi_api_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_RDI_API *rdi_api;
	MX_AREA_DETECTOR *ad;
	unsigned long sequence_type;
	int num_items;
	mx_status_type mx_status;

	MX_SEQUENCE_PARAMETERS sp;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	rdi_api = (MX_RDI_API *) record->record_type_struct;
	ad = (MX_AREA_DETECTOR *)
			rdi_api->area_detector_record->record_class_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:

#if MXZ_RDI_API_DEBUG
		MX_DEBUG(-2,("%s: getting '%s'.", fname,
			mx_get_field_label_string( record,
						record_field->label_value ) ));
#endif
		switch( record_field->label_value ) {
		case MXLV_RDI_API_SCAN_PARAMETERS:
			mx_status = mx_area_detector_get_sequence_parameters(
							ad->record, &sp );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			snprintf( rdi_api->scan_parameters,
				sizeof( rdi_api->scan_parameters ),
				"%lu %f %lu %s",
				sp.sequence_type, 0, 0, "" );
			break;
		default:
			MX_DEBUG(-1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:

#if MXZ_RDI_API_DEBUG
		MX_DEBUG(-2,("%s: putting '%s'.", fname,
			mx_get_field_label_string( record,
						record_field->label_value ) ));
#endif
		switch( record_field->label_value ) {
		case MXLV_RDI_API_SCAN_PARAMETERS:
			num_items = sscanf( rdi_api->scan_parameters,
					"%lu ", &sequence_type );
			switch( sequence_type ) {
			case MXT_SQ_DURATION:
			case MXT_SQ_GATED:
				break;
			default:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Scan mode %lu is not supported by this API "
				"for area detector '%s'.  "
				"Only MXT_SQ_DURATION (%lu) and "
				"MXT_SQ_GATED (%lu) are supported.",
					sequence_type, ad->record->name,
					MXT_SQ_DURATION, MXT_SQ_GATED );
				break;
			}
			break;
		default:
			MX_DEBUG(-1,(
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

