/*
 * Name:    d_file_dinput.c
 *
 * Purpose: MX keyboard digital input device driver.  This driver
 *          uses values read from a file to simulate a digital input.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_FILE_DINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_record.h"
#include "mx_digital_input.h"
#include "d_file_dinput.h"

MX_RECORD_FUNCTION_LIST mxd_file_dinput_record_function_list = {
	NULL,
	mxd_file_dinput_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_file_dinput_digital_input_function_list = {
	mxd_file_dinput_read,
	mxd_file_dinput_clear
};

MX_RECORD_FIELD_DEFAULTS mxd_file_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_FILE_DINPUT_STANDARD_FIELDS
};

long mxd_file_dinput_num_record_fields
		= sizeof( mxd_file_dinput_record_field_defaults )
			/ sizeof( mxd_file_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_file_dinput_rfield_def_ptr
			= &mxd_file_dinput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_file_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_FILE_DINPUT **file_dinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_file_dinput_get_pointers()";

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( file_dinput == (MX_FILE_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*file_dinput = (MX_FILE_DINPUT *)
				dinput->record->record_type_struct;

	if ( *file_dinput == (MX_FILE_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_FILE_DINPUT pointer for record '%s' is NULL.",
			dinput->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_file_dinput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_file_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_FILE_DINPUT *file_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        file_dinput = (MX_FILE_DINPUT *)
				malloc( sizeof(MX_FILE_DINPUT) );

        if ( file_dinput == (MX_FILE_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_FILE_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = file_dinput;
        record->class_specific_function_list
			= &mxd_file_dinput_digital_input_function_list;

        digital_input->record = record;

	digital_input->value = 0;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_file_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_file_dinput_read()";

	MX_FILE_DINPUT *file_dinput = NULL;
	FILE *file;
	int saved_errno, num_items;
	char buffer[80];
	mx_status_type mx_status;

	mx_status = mxd_file_dinput_get_pointers( dinput,
						&file_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Try to open the digital input file. */

	file = fopen( file_dinput->filename, "r" );

	saved_errno = errno;

#if MXD_FILE_DINPUT_DEBUG
	MX_DEBUG(-2,("%s: fopen( '%s', ... ) = %p, errno = %d",
		fname, file_dinput->filename, file, saved_errno));
#endif

	if ( file == NULL ) {
		dinput->value = 0;

		if ( saved_errno != ENOENT ) {
			mx_warning( "The attempt to open '%s' by '%s' "
			"failed.  errno = %d, error message = '%s'",
				file_dinput->filename,
				dinput->record->name,
				saved_errno, strerror(saved_errno) );
		}
	} else {
		mx_fgets( buffer, sizeof(buffer), file );

		if ( ferror(file) ) {
			fclose( file );
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while reading from '%s' for '%s'.",
				file_dinput->filename, dinput->record->name );
		}

		fclose(file);

		num_items = sscanf( buffer, "%lu", &(dinput->value) );

		if ( num_items == 0 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Did not find an integer value in the line '%s' "
			"read from '%s' for record '%s'.",
				buffer, file_dinput->filename,
				dinput->record->name );
		}
	}
				
	if ( file_dinput->debug ) {
		MX_DEBUG(-2,("%s: '%s' value = %lu",
			fname, dinput->record->name, dinput->value ));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_file_dinput_clear( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_file_dinput_clear()";

	MX_FILE_DINPUT *file_dinput = NULL;
	int os_status, saved_errno;
	mx_status_type mx_status;

	mx_status = mxd_file_dinput_get_pointers( dinput,
						&file_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We 'clear' the digital input by removing its file. */

	os_status = remove( file_dinput->filename );

	if ( os_status < 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case ENOENT:
			/* No warning will be displayed for trying to
			 * remove a nonexistent file.
			 */
			break;
		case EPERM:
			mx_warning( "This process does not have permission "
				"to delete the file '%s' for '%s'.",
				file_dinput->filename,
				dinput->record->name );
			break;
		default:
			mx_warning( "The attempt to open '%s' by '%s' "
			"failed.  errno = %d, error message = '%s'",
				file_dinput->filename,
				dinput->record->name,
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

#if MXD_FILE_DINPUT_DEBUG
	MX_DEBUG(-2,("%s: remove( '%s' ) = %d",
		fname, file_dinput->filename, os_status ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

