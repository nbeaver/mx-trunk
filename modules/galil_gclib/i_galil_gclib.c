/*
 * Name:    i_galil_gclib.c
 *
 * Purpose: MX driver for Galil motor controllers via the gclib library.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_GALIL_GCLIB_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_process.h"
#include "mx_motor.h"
#include "i_galil_gclib.h"
#include "d_galil_gclib.h"

/* Vendor include files. */

#include "gclib.h"

MX_RECORD_FUNCTION_LIST mxi_galil_gclib_record_function_list = {
	NULL,
	mxi_galil_gclib_create_record_structures,
	mxi_galil_gclib_finish_record_initialization,
	NULL,
	NULL,
	mxi_galil_gclib_open,
	NULL,
	NULL,
	NULL,
	mxi_galil_gclib_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_galil_gclib_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_GALIL_GCLIB_STANDARD_FIELDS
};

long mxi_galil_gclib_num_record_fields
		= sizeof( mxi_galil_gclib_record_field_defaults )
			/ sizeof( mxi_galil_gclib_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_galil_gclib_rfield_def_ptr
			= &mxi_galil_gclib_record_field_defaults[0];

/*---*/

static mx_status_type mxi_galil_gclib_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_galil_gclib_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_galil_gclib_create_record_structures()";

	MX_GALIL_GCLIB *galil_gclib = NULL;

	/* Allocate memory for the necessary structures. */

	galil_gclib = (MX_GALIL_GCLIB *) malloc( sizeof(MX_GALIL_GCLIB) );

	if ( galil_gclib == (MX_GALIL_GCLIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for an MX_GALIL_GCLIB structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = galil_gclib;

	record->record_function_list = &mxi_galil_gclib_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	galil_gclib->record = record;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_galil_gclib_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_galil_gclib_finish_record_initialization()";

	MX_GALIL_GCLIB *galil_gclib = NULL;
	unsigned long flags;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	galil_gclib = (MX_GALIL_GCLIB *) record->record_type_struct;

	if ( galil_gclib == (MX_GALIL_GCLIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_GALIL_GCLIB pointer for record '%s' is NULL.",
			record->name );
	}

	flags = galil_gclib->galil_gclib_flags;

	flags = flags;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_galil_gclib_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_galil_gclib_open()";

	MX_GALIL_GCLIB *galil_gclib = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	galil_gclib = (MX_GALIL_GCLIB *) record->record_type_struct;

	if ( galil_gclib == (MX_GALIL_GCLIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_GALIL_GCLIB pointer for record '%s' is NULL.",
			record->name );
	}

#if MXI_GALIL_GCLIB_DEBUG
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_galil_gclib_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case 0:
			record_field->process_function
					= mxi_galil_gclib_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxi_galil_gclib_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxi_galil_gclib_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_GALIL_GCLIB *galil_gclib;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	galil_gclib = (MX_GALIL_GCLIB *) (record->record_type_struct);

	galil_gclib = galil_gclib;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case 0:
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

/*--------------------------------------------------------------------------*/

