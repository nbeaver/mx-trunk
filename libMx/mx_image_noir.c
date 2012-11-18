/*
 * Name:    mx_image_noir.c
 *
 * Purpose: Functions used to generate NOIR format image files.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_IMAGE_NOIR_DEBUG		TRUE

#define MX_IMAGE_NOIR_DEBUG_UPDATE	TRUE

#if defined(OS_WIN32)
#  include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_unistd.h"
#include "mx_motor.h"
#include "mx_variable.h"
#include "mx_image_noir.h"

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_image_noir_setup( MX_RECORD *record_list,
		char *static_noir_header_name,
		MX_IMAGE_NOIR_INFO **image_noir_info_ptr )
{
	static const char fname[] = "mx_image_noir_setup()";

	MX_IMAGE_NOIR_INFO *image_noir_info;
	MX_RECORD *image_noir_string_record;
	MX_RECORD_FIELD *value_field;
	char *image_noir_string;
	char *duplicate;
	int split_status, saved_errno;
	int argc;
	char **argv;
	mx_status_type mx_status;

#if MX_IMAGE_NOIR_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif
	mx_breakpoint();

	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The record_list pointer passed was NULL." );
	}
	if ( image_noir_info_ptr == (MX_IMAGE_NOIR_INFO **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_noir_info_ptr pointer passed was NULL." );
	}
	if ( (*image_noir_info_ptr) != (MX_IMAGE_NOIR_INFO *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The value of *image_noir_info_ptr passed was not NULL.  "
		"Instead, it had the value %#p", *image_noir_info_ptr );
	}

	image_noir_info = calloc( 1, sizeof(MX_IMAGE_NOIR_INFO) );

	if ( image_noir_info == (MX_IMAGE_NOIR_INFO *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_IMAGE_NOIR_INFO structure." );
	}

	*image_noir_info_ptr = image_noir_info;

	/* Look for a string variable record in the database called
	 * 'mx_image_noir_records'.  It should contain a list of the
	 * records used to generate the information used by NOIR
	 * image headers.
	 */

	image_noir_string_record = mx_get_record( record_list,
						"mx_image_noir_records" );

	if ( image_noir_string_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Cannot setup NOIR header information, since the MX database "
		"does not contain a record called 'mx_image_noir_records'." );
	}

	/* Is this a string variable?  If so, it should have a 1-dimensional
	 * 'value' field with a type of MXFT_STRING.
	 */

	mx_status = mx_find_record_field( image_noir_string_record,
					"value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ( value_field->num_dimensions != 1 )
	  || ( value_field->datatype != MXFT_STRING ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The 'value' field of record '%s' used to generate "
		"NOIR image headers is NULL.",
			image_noir_string_record->name );
	}

	image_noir_string = mx_get_field_value_pointer( value_field );

	if ( image_noir_string == (char *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The field value pointer for field '%s.%s' is NULL.",
			image_noir_string_record->name,
			value_field->name );
	}

#if MX_IMAGE_NOIR_DEBUG
	MX_DEBUG(-2,("%s: '%s' string = '%s'", fname, 
		image_noir_string_record->name,
		image_noir_string ));
#endif

	duplicate = strdup( image_noir_string );

	if ( duplicate == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory trying to make a copy of image_noir_string.");
	}

	split_status = mx_string_split( duplicate, " ", &argc, &argv );

	if ( split_status != 0 ) {
		saved_errno = errno;

		mx_free( duplicate );

		switch( saved_errno ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"One or more of the arguments to mx_string_split() "
			"were invalid." );
			break;
		case ENOMEM:
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to parse image_noir_string "
			"into separate strings." );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"An unexpected errno value %d was returned when "
			"trying to parse image_noir_string." );
			break;
		}
	}

	if ( argc != 5 ) {
		mx_free( duplicate );
		mx_free( argv );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"%d arguments were specified for '%s', but 5 are required.",
			argc, image_noir_string_record->name );
	}

	/* Look for the 'energy' record. */

	image_noir_info->energy_motor_record =
			mx_get_record( record_list, argv[0] );

	if ( image_noir_info->energy_motor_record == (MX_RECORD *) NULL ) {
		mx_free( duplicate );
		mx_free( argv );

		return mx_error( MXE_NOT_FOUND, fname,
		"The record name '%s' specified by '%s' was not found.",
			argv[0], image_noir_string_record->name );
	}

	if ( image_noir_info->energy_motor_record->mx_class != MXC_MOTOR ) {
		mx_free( duplicate );
		mx_free( argv );

		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' specified by '%s' is not a motor record.",
			argv[0], image_noir_string_record->name );
	}

	/* Look for the 'wavelength' record. */

	image_noir_info->wavelength_motor_record =
			mx_get_record( record_list, argv[0] );

	if ( image_noir_info->wavelength_motor_record == (MX_RECORD *) NULL ) {
		mx_free( duplicate );
		mx_free( argv );

		return mx_error( MXE_NOT_FOUND, fname,
		"The record name '%s' specified by '%s' was not found.",
			argv[0], image_noir_string_record->name );
	}

	if ( image_noir_info->wavelength_motor_record->mx_class != MXC_MOTOR ) {
		mx_free( duplicate );
		mx_free( argv );

		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' specified by '%s' is not a motor record.",
			argv[0], image_noir_string_record->name );
	}

	/* Look for the 'beam_x' record. */

	image_noir_info->beam_x_motor_record =
			mx_get_record( record_list, argv[0] );

	if ( image_noir_info->beam_x_motor_record == (MX_RECORD *) NULL ) {
		mx_free( duplicate );
		mx_free( argv );

		return mx_error( MXE_NOT_FOUND, fname,
		"The record name '%s' specified by '%s' was not found.",
			argv[0], image_noir_string_record->name );
	}

	if ( image_noir_info->beam_x_motor_record->mx_class != MXC_MOTOR ) {
		mx_free( duplicate );
		mx_free( argv );

		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' specified by '%s' is not a motor record.",
			argv[0], image_noir_string_record->name );
	}

	/* Look for the 'beam_y' record. */

	image_noir_info->beam_y_motor_record =
			mx_get_record( record_list, argv[0] );

	if ( image_noir_info->beam_y_motor_record == (MX_RECORD *) NULL ) {
		mx_free( duplicate );
		mx_free( argv );

		return mx_error( MXE_NOT_FOUND, fname,
		"The record name '%s' specified by '%s' was not found.",
			argv[0], image_noir_string_record->name );
	}

	if ( image_noir_info->beam_y_motor_record->mx_class != MXC_MOTOR ) {
		mx_free( duplicate );
		mx_free( argv );

		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' specified by '%s' is not a motor record.",
			argv[0], image_noir_string_record->name );
	}

	/* Look for the 'oscillation_distance' record. */

	image_noir_info->oscillation_distance_record =
			mx_get_record( record_list, argv[0] );

	if ( image_noir_info->oscillation_distance_record
			== (MX_RECORD *) NULL )
	{
		mx_free( duplicate );
		mx_free( argv );

		return mx_error( MXE_NOT_FOUND, fname,
		"The record name '%s' specified by '%s' was not found.",
			argv[0], image_noir_string_record->name );
	}

	if ( image_noir_info->oscillation_distance_record->mx_superclass
			!= MXC_MOTOR )
	{
		mx_free( duplicate );
		mx_free( argv );

		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' specified by '%s' is not a variable record.",
			argv[0], image_noir_string_record->name );
	}

	mx_free( duplicate );
	mx_free( argv );

	image_noir_info->energy = 0;
	image_noir_info->wavelength = 0;
	image_noir_info->beam_x = 0;
	image_noir_info->beam_y = 0;
	image_noir_info->oscillation_distance = 0;

	/* Setup the file monitor for the static part of the NOIR header. */

#if MX_IMAGE_NOIR_DEBUG
	MX_DEBUG(-2,
	("%s: Setting up a file monitor for the static NOIR header file '%s'.",
		fname, static_noir_header_name));
#endif

	mx_status = mx_create_file_monitor( &(image_noir_info->file_monitor),
						R_OK, static_noir_header_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	image_noir_info->static_header_text = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_image_noir_update( MX_IMAGE_NOIR_INFO *image_noir_info )
{
	static const char fname[] = "mx_image_noir_update()";

	struct stat noir_header_stat_buf;
	int os_status, saved_errno;
	mx_bool_type read_static_header_file;
	mx_bool_type static_header_file_has_changed;
	FILE *noir_header_file;
	size_t noir_header_file_size;
	size_t bytes_read;
	mx_status_type mx_status;

	mx_breakpoint();

	/* See if we need to update the contents of the static header. */

	static_header_file_has_changed = mx_file_has_changed(
					image_noir_info->file_monitor );

	if ( image_noir_info->static_header_text == NULL ) {
		read_static_header_file = TRUE;
	} else
	if ( static_header_file_has_changed ) {
		read_static_header_file = TRUE;
	} else {
		read_static_header_file = FALSE;
	}

#if MX_IMAGE_NOIR_DEBUG_UPDATE
	if ( read_static_header_file ) {

		MX_DEBUG(-2,("%s: updating static NOIR header from '%s'.",
			fname, image_noir_info->file_monitor->filename ));
	} else {
		MX_DEBUG(-2,("%s: static NOIR header will not be updated.",
			fname));
	}
#endif

	if ( read_static_header_file ) {
		mx_free( image_noir_info->static_header_text );

		os_status = stat( image_noir_info->file_monitor->filename,
					&noir_header_stat_buf );

		if ( os_status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An attempt to get the current file status of "
			"NOIR static header file '%s' failed with "
			"errno = %d, error message = '%s'.",
				image_noir_info->file_monitor->filename,
				saved_errno, strerror(saved_errno) );
		}

		/* How big is the file? */

		noir_header_file_size = noir_header_stat_buf.st_size;

		/* Allocate a buffer big enough to read the file into. */

		image_noir_info->static_header_text =
				malloc( noir_header_file_size );

		if ( image_noir_info->static_header_text == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %lu byte "
			"buffer to contain the contents of NOIR static "
			"header file '%s'.",
				noir_header_file_size,
				image_noir_info->file_monitor->filename );
		}

		/* Read in the contents of the NOIR static file header. */

		noir_header_file =
			fopen( image_noir_info->file_monitor->filename, "r" );

		if ( noir_header_file == (FILE *) NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The attempt to read the NOIR static file header "
			"from file '%s' failed.  "
			"Errno = %d, error message = '%s'.",
				image_noir_info->file_monitor->filename,
				saved_errno, strerror(saved_errno) );
		}

		bytes_read = fread( image_noir_info->static_header_text,
					1, noir_header_file_size,
					noir_header_file );

		fclose( noir_header_file );

		if ( bytes_read != noir_header_file_size ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The number of bytes read (%lu) from NOIR static "
			"header file '%s' was different than the reported "
			"file size of %lu bytes.",
				bytes_read,
				image_noir_info->file_monitor->filename,
				noir_header_file_size );
		}
	}

	/* Now update the motor positions and other values used
	 * by the NOIR header.
	 */

	mx_status = mx_motor_get_position(
			image_noir_info->energy_motor_record,
			&(image_noir_info->energy) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position(
			image_noir_info->wavelength_motor_record,
			&(image_noir_info->wavelength) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position(
			image_noir_info->beam_x_motor_record,
			&(image_noir_info->beam_x) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position(
			image_noir_info->beam_y_motor_record,
			&(image_noir_info->beam_y) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_double_variable(
			image_noir_info->oscillation_distance_record,
			&(image_noir_info->oscillation_distance) );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_image_noir_write_header( FILE *file,
			MX_IMAGE_NOIR_INFO *image_noir_info )
{
	static const char fname[] = "mx_image_noir_write_header()";

	int fputs_status, saved_errno;

	mx_breakpoint();

	if ( file == (FILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The FILE pointer passed was NULL." );
	}
	if ( image_noir_info == (MX_IMAGE_NOIR_INFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_NOIR_INFO pointer passed was NULL." );
	}

	/* Write out the special values. */

	fprintf( file, "ENERGY=%f;\n", image_noir_info->energy );
	fprintf( file, "WAVELENGTH=%f;\n", image_noir_info->wavelength );
	fprintf( file, "BEAM_X=%f;\n", image_noir_info->beam_x );
	fprintf( file, "BEAM_Y=%f;\n", image_noir_info->beam_y );
	fprintf( file, "OSCILLATION_DISTANCE=%f;\n",
				image_noir_info->oscillation_distance );

	/* Write out the static header text. */

	fputs_status = fputs( image_noir_info->static_header_text, file );

	if ( fputs_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while writing the NOIR static header "
		"to the file.  Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

