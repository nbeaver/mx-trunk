/*
 * Name:    i_zwo_efw.c
 *
 * Purpose: MX driver for ZWO Electronic Filter Wheels.
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

#define MXI_ZWO_EFW_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* Vendor include file. */
#include "EFW_filter.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_motor.h"
#include "i_zwo_efw.h"
#include "d_zwo_efw_motor.h"

MX_RECORD_FUNCTION_LIST mxi_zwo_efw_record_function_list = {
	NULL,
	mxi_zwo_efw_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_zwo_efw_open
};

MX_RECORD_FIELD_DEFAULTS mxi_zwo_efw_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_ZWO_EFW_STANDARD_FIELDS
};

long mxi_zwo_efw_num_record_fields
		= sizeof( mxi_zwo_efw_record_field_defaults )
			/ sizeof( mxi_zwo_efw_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_zwo_efw_rfield_def_ptr
			= &mxi_zwo_efw_record_field_defaults[0];

/*---*/

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_zwo_efw_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_zwo_efw_create_record_structures()";

	MX_ZWO_EFW *zwo_efw = NULL;

	/* Allocate memory for the necessary structures. */

	zwo_efw = (MX_ZWO_EFW *) malloc( sizeof(MX_ZWO_EFW) );

	if ( zwo_efw == (MX_ZWO_EFW *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for an MX_ZWO_EFW structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = zwo_efw;

	record->record_function_list = &mxi_zwo_efw_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	zwo_efw->record = record;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_zwo_efw_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_zwo_efw_open()";

	MX_ZWO_EFW *zwo_efw = NULL;
	MX_RECORD_FIELD *filter_wheel_record_array_field = NULL;
	MX_RECORD_FIELD *filter_wheel_id_array_field = NULL;
	unsigned long i, num_filter_wheels;
	long id_array_length, id_array_length_2;
	int *id_array = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	zwo_efw = (MX_ZWO_EFW *) record->record_type_struct;

	if ( zwo_efw == (MX_ZWO_EFW *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ZWO_EFW pointer for record '%s' is NULL.",
			record->name );
	}

#if MXI_ZWO_EFW_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));

	strlcpy( zwo_efw->sdk_version_name, EFWGetSDKVersion(),
			sizeof(zwo_efw->sdk_version_name) );
#endif

	num_filter_wheels = EFWGetNum();

	zwo_efw->maximum_num_filter_wheels = num_filter_wheels;

#if MXI_ZWO_EFW_DEBUG
	MX_DEBUG(-2,("%s: %s.maximum_num_filter_wheels = %lu'.",
		fname, record->name, zwo_efw->maximum_num_filter_wheels ));
#endif

	zwo_efw->current_num_filter_wheels = 0;

	/* Create an array of pointers to individual filter wheel records. */

	zwo_efw->filter_wheel_record_array = (MX_RECORD **)
		calloc( num_filter_wheels, sizeof(MX_RECORD *) );

	if ( zwo_efw->filter_wheel_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array of "
		"MX_RECORD pointers for %s.filter_wheel_record_array.",
			num_filter_wheels, record->name );
	}

	/* Update the dimensions of the MX 'filter_wheel_record_array' field
	 * to match 'maximum_num_filter_wheels'
	 */

	mx_status = mx_find_record_field( record,
				"filter_wheel_record_array",
				&filter_wheel_record_array_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	filter_wheel_record_array_field->dimension[0] = num_filter_wheels;

	filter_wheel_record_array_field->data_element_size[0]
							= sizeof(MX_RECORD *);

	/* Create an array of pointers to individual filter wheel records. */

	zwo_efw->filter_wheel_id_array = (long *)
		calloc( num_filter_wheels, sizeof(long ) );

	if ( zwo_efw->filter_wheel_id_array == (long *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array of "
		"indices for %s.filter_wheel_id_array.",
			num_filter_wheels, record->name );
	}

	/* Update the dimensions of the MX 'filter_wheel_id_array' field
	 * to match 'maximum_num_filter_wheels'
	 */

	mx_status = mx_find_record_field( record,
				"filter_wheel_id_array",
				&filter_wheel_id_array_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	filter_wheel_id_array_field->dimension[0] = num_filter_wheels;

	filter_wheel_id_array_field->data_element_size[0]
							= sizeof(MX_RECORD *);

	/* Display the IDs of all the filter wheels. */

	id_array_length = EFWGetProductIDs( NULL );

	MX_DEBUG(-2,("%s: id_array_length = %ld", fname, id_array_length));

	id_array = (int *) calloc( id_array_length, sizeof(int) );

	if ( id_array == (int *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array of "
		"filter wheel PIDs for record '%s'.",
			id_array_length, record->name );
	}

	id_array_length_2 = EFWGetProductIDs( id_array );

	if ( id_array_length_2 != id_array_length ) {
		mx_warning( "For some unknown reason, the length returned by "
		"EFWGetProductIDs(NULL) = %lu is different from the value "
		"returned by EFWGetProductIDs( id_array ) = %lu "
		"for record '%s'.", id_array_length, id_array_length_2,
			record->name );
	}

	for ( i = 0; i < num_filter_wheels; i++ ) {
		zwo_efw->filter_wheel_id_array[i] = id_array[i];

		MX_DEBUG(-2,("%s: PID '%lu' = %ld",
			fname, i, zwo_efw->filter_wheel_id_array[i] ));
	}

	mx_free( id_array );

	return MX_SUCCESSFUL_RESULT;
}

