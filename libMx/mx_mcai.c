/*
 * Name:    mx_mcai.c
 *
 * Purpose: MX function library for multichannel analog input devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2005, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_mcai.h"

static mx_status_type
mx_mcai_get_pointers( MX_RECORD *record,
		MX_MCAI **mcai,
		MX_MCAI_FUNCTION_LIST **flist_ptr,
		const char *calling_fname )
{
	static const char fname[] = "mx_mcai_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( record->mx_class != MXC_MULTICHANNEL_ANALOG_INPUT ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"The record '%s' passed by '%s' is not a multichannel analog input record.  "
"(superclass = %ld, class = %ld, type = %ld)",
			record->name, calling_fname,
			record->mx_superclass,
			record->mx_class,
			record->mx_type );
	}

	if (mcai != (MX_MCAI **) NULL) {
		*mcai = (MX_MCAI *) record->record_class_struct;

		if ( *mcai == (MX_MCAI *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_MCAI pointer for record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

	if (flist_ptr != (MX_MCAI_FUNCTION_LIST **) NULL) {
		*flist_ptr = (MX_MCAI_FUNCTION_LIST *)
				record->class_specific_function_list;

		if ( *flist_ptr ==
			(MX_MCAI_FUNCTION_LIST *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MCAI_FUNCTION_LIST ptr for "
			"record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcai_initialize_driver( MX_DRIVER *driver,
			long *maximum_num_channels_varargs_cookie )
{
	static const char fname[] = "mx_mcai_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}
	if ( maximum_num_channels_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"The maximum_num_channels_varargs_cookie pointer passed was NULL." );
	}

	/*** Construct a varargs cookie for 'maximum_num_channels'. ***/

	mx_status = mx_find_record_field_defaults_index( driver,
						"maximum_num_channels",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
					maximum_num_channels_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** 'channel_array' depends on 'maximum_num_channels'. ***/

	mx_status = mx_find_record_field_defaults( driver,
					"channel_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_channels_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcai_finish_record_initialization( MX_RECORD *mcai_record )
{
	static const char fname[] = "mx_mcai_finish_record_initialization()";

	MX_MCAI *mcai;
	MX_RECORD_FIELD *channel_array_field;
	mx_status_type mx_status;

	if ( mcai_record == ( MX_RECORD * ) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mcai = (MX_MCAI *) mcai_record->record_class_struct;

	if ( mcai == (MX_MCAI *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MCAI pointer for record '%s' is NULL.",
			mcai_record->name );
	}

	/* Since 'channel_array' is a varying length field, but is not
	 * specified in the record description, we must fix up the
	 * dimension[0] value for the field by hand.
	 */

	mx_status = mx_find_record_field( mcai_record,
					"channel_array",
					&channel_array_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	channel_array_field->dimension[0] = (long) mcai->maximum_num_channels;

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_mcai_read( MX_RECORD *record,
		unsigned long *num_channels,
		double **channel_array )
{
	static const char fname[] = "mx_mcai_read()";

	MX_MCAI *mcai;
	MX_MCAI_FUNCTION_LIST *function_list;
	mx_status_type ( *read_fn ) ( MX_MCAI * );
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	mcai = NULL;
	function_list = NULL;

	mx_status = mx_mcai_get_pointers( record,
			&mcai, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_fn = function_list->read;

	if ( read_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "read function ptr for MX_MCAI ptr 0x%p is NULL.", mcai );
	}

	mx_status = (*read_fn)( mcai );

	if ( num_channels != NULL ) {
		*num_channels = mcai->current_num_channels;
	}

	if ( channel_array != NULL ) {
		*channel_array = mcai->channel_array;
	}

	return mx_status;
}

