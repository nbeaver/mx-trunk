/*
 * Name:    mx_image_noir.c
 *
 * Purpose: Functions used to generate NOIR format image files.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2012-2013, 2015-2016, 2018, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_IMAGE_NOIR_DEBUG_SETUP	FALSE

#define MX_IMAGE_NOIR_DEBUG_STRINGS	FALSE

#define MX_IMAGE_NOIR_DEBUG_UPDATE	FALSE

#define MX_IMAGE_NOIR_DEBUG_READ	FALSE

#define MX_IMAGE_NOIR_DEBUG_WRITE	FALSE

#if defined(OS_WIN32)
#  include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_unistd.h"
#include "mx_array.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_motor.h"
#include "mx_scaler.h"
#include "mx_variable.h"
#include "mx_image_noir.h"

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_image_noir_setup( MX_RECORD *mx_imaging_device_record,
		char *detector_name_for_header,
		char *dynamic_header_template_name,
		char *static_header_file_name,
		MX_IMAGE_NOIR_INFO **image_noir_info_ptr )
{
	static const char fname[] = "mx_image_noir_setup()";

	MX_IMAGE_NOIR_INFO *image_noir_info;
	MX_RECORD **record_array;
	MX_RECORD *dynamic_header_string_record;
	MX_RECORD_FIELD *value_field;
	char *dynamic_header_string;
	char *duplicate;
	int split_status, saved_errno;
	int argc, item_argc;
	char **argv, **item_argv;
	int i, j, num_aliases, max_aliases;
	int string_length, local_max_string_length, max_string_length;
	char *ptr, *new_ptr;
	long *alias_dimension_array;
	char ***alias_array;
	MX_IMAGE_NOIR_DYNAMIC_HEADER_VALUE *value_array;
	size_t char_sizeof[3] =
		{ sizeof(char), sizeof(char *), sizeof(char **) };
	mx_status_type mx_status;

#if MX_IMAGE_NOIR_DEBUG_SETUP
	MX_DEBUG(-2,("%s invoked.", fname));
#endif
	if ( mx_imaging_device_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The mx_imaging_device_record pointer passed was NULL." );
	}
	if ( image_noir_info_ptr == (MX_IMAGE_NOIR_INFO **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_noir_info_ptr pointer passed was NULL." );
	}
	if ( (*image_noir_info_ptr) != (MX_IMAGE_NOIR_INFO *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The value of *image_noir_info_ptr passed was not NULL.  "
		"Instead, it had the value %p", *image_noir_info_ptr );
	}

	image_noir_info = calloc( 1, sizeof(MX_IMAGE_NOIR_INFO) );

	if ( image_noir_info == (MX_IMAGE_NOIR_INFO *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_IMAGE_NOIR_INFO structure." );
	}

	*image_noir_info_ptr = image_noir_info;

	image_noir_info->mx_imaging_device_record = mx_imaging_device_record;

	if ( detector_name_for_header == NULL ) {
		image_noir_info->detector_name_for_header[0] = '\0';
	} else {
		strlcpy( image_noir_info->detector_name_for_header,
			detector_name_for_header,
			sizeof( image_noir_info->detector_name_for_header ) );
	}

	/*========= Setup static header information =========*/

	if ( ( static_header_file_name == NULL )
	  || ( strlen(static_header_file_name) == 0 ) )
	{
		image_noir_info->file_monitor = NULL;

		mx_warning( "No static NOIR header file was specified "
		"for record '%s'.", mx_imaging_device_record->name );
	} else {
		/* Setup a file monitor for the static part
		 * of the NOIR header.
		 */

		unsigned long access_type = R_OK | MXF_FILE_MONITOR_QUIET;

#if MX_IMAGE_NOIR_DEBUG_SETUP
		MX_DEBUG(-2,
			("%s: Setting up a file monitor for the static NOIR "
			"header file '%s'.",
			fname, static_header_file_name));
#endif

		mx_status = mx_create_file_monitor(
				&(image_noir_info->file_monitor),
				access_type,
				static_header_file_name );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			/* Do nothing here. */
			break;

		case MXE_NOT_FOUND:
			image_noir_info->file_monitor = NULL;

			mx_warning( "Static NOIR header file '%s' was "
				"not found and will not be monitored.",
				static_header_file_name );
			break;

		default:
			return mx_status;
			break;
		}
	}

	image_noir_info->static_header_text = NULL;
	image_noir_info->static_header_length = 0;

	/*========= Setup dynamic header information =========*/

	image_noir_info->dynamic_header_num_records = 0;

	/* Look for a string variable record in the database whose name
	 * is given by the string 'dynamic_header_template_name'.  It
	 * should contain a list of the records used to generate the
	 * dynamic information used by NOIR image headers.
	 */

	if ( ( dynamic_header_template_name == NULL )
	  || ( strlen(dynamic_header_template_name) == 0 ) )
	{
		mx_warning( "Cannot setup dynamic NOIR header information, "
		"since no dynamic header template name record was specified." );

		return MX_SUCCESSFUL_RESULT;
	}

	dynamic_header_string_record = mx_get_record( mx_imaging_device_record,
						dynamic_header_template_name );

	if ( dynamic_header_string_record == (MX_RECORD *) NULL ) {
		mx_warning( "Cannot setup NOIR dynamic header information, "
		"since the MX database does not contain a record called '%s'.",
		dynamic_header_template_name );

		return MX_SUCCESSFUL_RESULT;
	}

	/* Is this a string variable?  If so, it should have a 1-dimensional
	 * 'value' field with a type of MXFT_STRING.
	 */

	mx_status = mx_find_record_field( dynamic_header_string_record,
						"value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ( value_field->num_dimensions != 1 )
	  || ( value_field->datatype != MXFT_STRING ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The 'value' field of record '%s' used to generate "
		"NOIR image headers is NULL.",
			dynamic_header_string_record->name );
	}

	dynamic_header_string = mx_get_field_value_pointer( value_field );

	if ( dynamic_header_string == (char *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The field value pointer for field '%s.%s' is NULL.",
			dynamic_header_string_record->name,
			value_field->name );
	}

#if MX_IMAGE_NOIR_DEBUG_SETUP
	MX_DEBUG(-2,("%s: '%s' string = '%s'", fname, 
		dynamic_header_string_record->name,
		dynamic_header_string ));
#endif

	duplicate = strdup( dynamic_header_string );

	if ( duplicate == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	  "Ran out of memory trying to make a copy of dynamic_header_string.");
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
			"trying to parse image_noir_string.", saved_errno );
			break;
		}
	}

#if MX_IMAGE_NOIR_DEBUG_SETUP
	MX_DEBUG(-2,("%s: argc = %d", fname, argc));
#endif

	image_noir_info->dynamic_header_num_records = argc;

	if ( argc == 0 ) {
		mx_warning( "No dynamic header entries found in record '%s'.",
			dynamic_header_string_record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	record_array = (MX_RECORD **) calloc( argc, sizeof(MX_RECORD *) );

	if ( record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element "
		"array of MX_RECORD * pointers.", argc );
	}

	image_noir_info->dynamic_header_record_array = record_array;

	value_array = (MX_IMAGE_NOIR_DYNAMIC_HEADER_VALUE *)
		calloc( argc, sizeof(MX_IMAGE_NOIR_DYNAMIC_HEADER_VALUE) );

	if ( value_array == (MX_IMAGE_NOIR_DYNAMIC_HEADER_VALUE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element "
		"array of MX_IMAGE_NOIR_DYNAMIC_HEADER_VALUE elements.", argc );
	}

	image_noir_info->dynamic_header_value_array = value_array;

	/* What is the maximum number of aliases for a given header value?
	 * We find that out by counting the number of occurrences of the ':'
	 * each of the argv strings above and then use the maximum of those.
	 */

	max_aliases = 0;
	max_string_length = 0;

	for ( i = 0; i < argc; i++ ) {

		num_aliases = 0;
		local_max_string_length = 0;

		ptr = argv[i];

		new_ptr = strchr( ptr, ':' );

		while ( new_ptr != NULL ) {

			num_aliases++;

			ptr = new_ptr;

			ptr++;

			new_ptr = strchr( ptr, ':' );

			if ( new_ptr == NULL ) {
				string_length = strlen(ptr) + 1;
			} else {
				string_length = new_ptr - ptr + 1;
			}

			if ( string_length > local_max_string_length ) {
				local_max_string_length = string_length;
			}
		}

		if ( num_aliases > max_aliases ) {
			max_aliases = num_aliases;
		}

		if ( local_max_string_length > max_string_length ) {
			max_string_length = local_max_string_length;
		}
	}

#if MX_IMAGE_NOIR_DEBUG_SETUP
	MX_DEBUG(-2,("%s: max_aliases = %d", fname, max_aliases));
#endif

	/* Create an array to store the header alias strings in. */

	alias_dimension_array =
		(long *) image_noir_info->dynamic_header_alias_dimension_array;

	if ( ( image_noir_info->dynamic_header_alias_array != NULL )
	  && ( alias_dimension_array[0] != 0 ) )
	{
		/* If present, destroy the old one. */

		mx_status = mx_free_array(
		    image_noir_info->dynamic_header_alias_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	alias_dimension_array[0] = argc;
	alias_dimension_array[1] = max_aliases;
	alias_dimension_array[2] = max_string_length + 1;

	alias_array = mx_allocate_array( MXFT_STRING, 3,
				alias_dimension_array, char_sizeof );

	if ( alias_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a (%lu,%lu,%lu) array "
		"of NOIR header aliases.",
			alias_dimension_array[0],
			alias_dimension_array[1],
			alias_dimension_array[2] );
	}

	image_noir_info->dynamic_header_alias_array = alias_array;

	/* Fill in the contents of mx_noir_header_record_array
	 * and mx_noir_header_alias_array.
	 */

	for ( i = 0; i < argc; i++ ) {

		split_status = mx_string_split( argv[i], ":",
					&item_argc, &item_argv );

		if ( split_status != 0 ) {
			saved_errno = errno;

			mx_status = mx_error( MXE_UNKNOWN_ERROR, fname,
				"Unexpected errno value %d was returned when "
				"trying to parse argv[%d] = '%s'.",
					saved_errno, i, argv[i] );

			mx_free( argv );
			mx_free( duplicate );

			return mx_status;
		}

		if ( item_argc < 1 ) {
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Entry %d in the 'mx_noir_records_list' variable "
			"is empty.", i );

			mx_free( item_argv );
			mx_free( argv );
			mx_free( duplicate );

			return mx_status;
		}
		if ( item_argc < 2 ) {
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Entry %d '%s' in the 'mx_noir_records_list' "
			"only contains the name of an MX record.  "
			"It does not contain any alias values to be used "
			"to show the value of this record in an "
			"MX NOIR image header.", 0, item_argv[0] );

			mx_free( item_argv );
			mx_free( argv );
			mx_free( duplicate );

			return mx_status;
		}

		/* The first item should be the name of an MX record in
		 * the local MX database.
		 */

		record_array[i] = mx_get_record( mx_imaging_device_record,
							item_argv[0] );

		if ( record_array[i] == (MX_RECORD *) NULL ) {
			mx_status = mx_error( MXE_NOT_FOUND, fname,
			"MX record '%s' for 'mx_noir_record_list entry' %d "
			"was not found in the MX database.",
				item_argv[0], i );

			mx_free( item_argv );
			mx_free( argv );
			mx_free( duplicate );

			return mx_status;
		}

#if MX_IMAGE_NOIR_DEBUG_STRINGS
		fprintf(stderr, "%s: record[%d] = '%s'",
			fname, i, record_array[i]->name);
#endif

		/* The subsequent items are the various aliases to be used
		 * in the NOIR header for this MX record.
		 */

		for ( j = 0; j < max_aliases; j++ ) {

			if ( j >= (item_argc - 1) ) {
				alias_array[i][j][0] = '\0';
			} else {
				strlcpy( alias_array[i][j],
					item_argv[j+1],
					max_string_length );
			}
#if MX_IMAGE_NOIR_DEBUG_STRINGS
			fprintf(stderr, ", alias(%d) = '%s'",
				j, alias_array[i][j] );
#endif
		}

#if MX_IMAGE_NOIR_DEBUG_STRINGS
		fprintf(stderr,"\n");
#endif

		mx_free( item_argv );
	}

	/* Discard some temporary data structures. */

	mx_free( argv );
	mx_free( duplicate );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxp_image_noir_info_update_record_value( MX_RECORD *record,
			MX_IMAGE_NOIR_DYNAMIC_HEADER_VALUE *array_element )
{
	static const char fname[] = "mxp_image_noir_info_update_record_value()";

	MX_RECORD_FIELD *value_field;
	void *value_pointer;
	long num_value_elements;
	long long_value;
	double double_value;
	mx_status_type mx_status;

	switch( record->mx_superclass ) {
	case MXR_DEVICE:
		switch( record->mx_class ) {
		case MXC_MOTOR:
			mx_status = mx_motor_get_position( record,
							&double_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			array_element->datatype = MXFT_DOUBLE;
			array_element->u.double_value = double_value;
			break;

		case MXC_SCALER:
			mx_status = mx_scaler_read( record,
						&long_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			array_element->datatype = MXFT_LONG;
			array_element->u.long_value = long_value;
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Support for MX device class %lu used by "
			"record '%s' is not yet implemented.",
				record->mx_class, record->name );
			break;
		}
		break;
	case MXR_VARIABLE:
		/* Cause the variable's value to be updated. */

		mx_status = mx_receive_variable( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* What datatype is the variable? */

		mx_status = mx_find_record_field( record, "value",
						&value_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Get a pointer to the value of the variable. */

		mx_status = mx_get_1d_array( record,
					value_field->datatype,
					&num_value_elements,
					&value_pointer );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( value_field->datatype ) {
		case MXFT_STRING:
			array_element->datatype = MXFT_STRING;

			strlcpy( array_element->u.string_value,
				value_pointer,
				MXU_MAX_IMAGE_NOIR_STRING_LENGTH );
			break;
		case MXFT_CHAR:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
					*( (char *) value_pointer );
			break;
		case MXFT_UCHAR:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
				*( (unsigned char *) value_pointer );
			break;
		case MXFT_INT8:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
					*( (int8_t *) value_pointer );
			break;
		case MXFT_UINT8:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
					*( (uint8_t *) value_pointer );
			break;
		case MXFT_SHORT:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
				*( (short *) value_pointer );
			break;
		case MXFT_USHORT:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
				*( (unsigned short *) value_pointer );
			break;
		case MXFT_INT16:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
					*( (int16_t *) value_pointer );
			break;
		case MXFT_UINT16:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
					*( (uint16_t *) value_pointer );
			break;
		case MXFT_BOOL:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
				*( (mx_bool_type *) value_pointer );
			break;
		case MXFT_INT32:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
					*( (int32_t *) value_pointer );
			break;
		case MXFT_UINT32:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
					*( (uint32_t *) value_pointer );
			break;
		case MXFT_LONG:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
					*( (long *) value_pointer );
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
				*( (unsigned long *) value_pointer );
			break;
		case MXFT_FLOAT:
			array_element->datatype = MXFT_DOUBLE;
			array_element->u.double_value =
					*( (float *) value_pointer );
			break;
		case MXFT_DOUBLE:
			array_element->datatype = MXFT_DOUBLE;
			array_element->u.double_value =
					*( (double *) value_pointer );
			break;
		case MXFT_INT64:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
					*( (int64_t *) value_pointer );
			break;
		case MXFT_UINT64:
			array_element->datatype = MXFT_LONG;
			array_element->u.long_value =
					*( (uint64_t *) value_pointer );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"MX record field '%s.value' does not have "
			"a datatype compatible with being read out "
			"as a double.", record->name );
			break;
		}
		break;

	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support for MX superclass %lu used by "
		"record '%s' is not yet implemented.",
			record->mx_superclass, record->name );
		break;
	}

#if MX_IMAGE_NOIR_DEBUG_READ
	MX_DEBUG(-2,("%s: record '%s' value read = %f",
		fname, record->name, double_value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_image_noir_update( MX_IMAGE_NOIR_INFO *image_noir_info )
{
	static const char fname[] = "mx_image_noir_update()";

	MX_RECORD *record;
	MX_IMAGE_NOIR_DYNAMIC_HEADER_VALUE *array_element;
	struct stat noir_header_stat_buf;
	int os_status, saved_errno;
	mx_bool_type read_static_header_file;
	mx_bool_type static_header_file_has_changed;
	FILE *noir_static_header_file;
	size_t noir_static_header_file_size;
	size_t bytes_read;
	unsigned long i;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

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

		noir_static_header_file_size = noir_header_stat_buf.st_size;

		/* Allocate a buffer big enough to read the file into. */

		image_noir_info->static_header_text =
				malloc( noir_static_header_file_size + 1 );

		if ( image_noir_info->static_header_text == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %lu byte "
			"buffer to contain the contents of NOIR static "
			"header file '%s'.",
				(unsigned long) noir_static_header_file_size,
				image_noir_info->file_monitor->filename );
		}

		/* Read in the contents of the NOIR static file header. */

		noir_static_header_file =
			fopen( image_noir_info->file_monitor->filename, "r" );

		if ( noir_static_header_file == (FILE *) NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The attempt to read the NOIR static file header "
			"from file '%s' failed.  "
			"Errno = %d, error message = '%s'.",
				image_noir_info->file_monitor->filename,
				saved_errno, strerror(saved_errno) );
		}

		bytes_read = fread( image_noir_info->static_header_text,
					1, noir_static_header_file_size,
					noir_static_header_file );

		fclose( noir_static_header_file );

		if ( bytes_read != noir_static_header_file_size ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The number of bytes read (%lu) from NOIR static "
			"header file '%s' was different than the reported "
			"file size of %lu bytes.",
				(unsigned long) bytes_read,
				image_noir_info->file_monitor->filename,
				(unsigned long) noir_static_header_file_size );
		}

		/* Make sure the text is null terminated. */

		image_noir_info->static_header_text[bytes_read] = '\0';
	}

	/* Now update the motor positions and other values used
	 * by the NOIR header.
	 */

	for ( i = 0; i < image_noir_info->dynamic_header_num_records; i++ ) {

		record = image_noir_info->dynamic_header_record_array[i];

		/* FIXME: The following code might be best handled by
		 * an enhanced version of mx_update_record_values().
		 */

		array_element = &image_noir_info->dynamic_header_value_array[i];

		mx_status = mxp_image_noir_info_update_record_value( record,
								array_element );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_image_noir_write_header( FILE *file,
			MX_IMAGE_NOIR_INFO *image_noir_info )
{
	static const char fname[] = "mx_image_noir_write_header()";

	int fputs_status, saved_errno;
	int i, j, num_records, max_aliases, length;
	char *alias_name;
	MX_IMAGE_NOIR_DYNAMIC_HEADER_VALUE *alias_value_struct = NULL;
	MX_RECORD *referenced_record = NULL;

	MX_RECORD *imaging_device_record = NULL;
	MX_AREA_DETECTOR *ad = NULL;
	char scan_template[2*MXU_FILENAME_LENGTH+3];

	mx_status_type mx_status;

	if ( file == (FILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The FILE pointer passed was NULL." );
	}
	if ( image_noir_info == (MX_IMAGE_NOIR_INFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_NOIR_INFO pointer passed was NULL." );
	}

	imaging_device_record = image_noir_info->mx_imaging_device_record;

	if ( imaging_device_record != (MX_RECORD *) NULL ) {
		if ( imaging_device_record->mx_class == MXC_AREA_DETECTOR ) {
			ad = (MX_AREA_DETECTOR *)
				imaging_device_record->record_class_struct;
		}
	}

	/* If a detector name is specified, write it to the header. */

#if 0
	{
		char *detector_name;

		detector_name = image_noir_info->detector_name_for_header;

		if ( strlen(detector_name) > 0 ) {
			fprintf( file, "DETECTOR_NAMES=%s;\n", detector_name );
		}
	}
#endif

	/* Write out the static header text. */

	if ( image_noir_info->static_header_text != NULL ) {
	    fputs_status = fputs( image_noir_info->static_header_text, file );

	    if ( fputs_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while writing the NOIR static header "
		"to the file.  Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );
	    }
	}

	/* Write out special case stuff. */

	if ( ad != (MX_AREA_DETECTOR *) NULL ) {

		/* For area detectors, write the filename out to the header. */

		fprintf( file, "FILENAME=%s/%s;\n",
			ad->datafile_directory, ad->datafile_name );

		/* Write out the MX datafile pattern to a buffer. */

		snprintf( scan_template, sizeof(scan_template),
		"%s/%s", ad->datafile_directory, ad->datafile_pattern );

		/* Go through the scan template and change all # characters
		 * to ? characters, since that is what SMV header readers
		 * appear to expect.
		 */

		length = strlen( scan_template );

		for ( i = 0; i < length; i++ ) {
			if ( scan_template[i] == '#' ) {
				scan_template[i] = '?';
			}
		}

		fprintf( file, "SCAN_TEMPLATE=%s;\n", scan_template );

		if ( ad->oscillation_motor_record != (MX_RECORD *) NULL ) {
			/* Write the (calculated ?) motor position
			 * for this frame.
			 */

			char temp_buffer[400];
			int c;

			snprintf( temp_buffer, sizeof(temp_buffer),
				"%s=%f;\n",
				ad->oscillation_motor_record->name,
				ad->motor_position );

			length = strlen( temp_buffer );

			for ( i = 0; i < length; i++ ) {
				c = temp_buffer[i];

				if ( islower(c) ) {
					temp_buffer[i] = toupper(c);
				}
			}

			fputs( temp_buffer, file );
		}
	}

	/* Write out the dynamic header values. */

	num_records = image_noir_info->dynamic_header_num_records;

	max_aliases = image_noir_info->dynamic_header_alias_dimension_array[1];

	for ( i = 0; i < num_records; i++ ) {

		for( j = 0; j < max_aliases; j++ ) {

			alias_name =
			    image_noir_info->dynamic_header_alias_array[i][j];

			if ( strlen( alias_name ) < 1 ) {
				/* If the alias name is empty, then we
				 * skip this alias and go back to the
				 * top of the loop for the next one.
				 */
				continue;
			}

			alias_value_struct =
			    &image_noir_info->dynamic_header_value_array[i];

			if ( alias_value_struct->process_field ) {

			    referenced_record =
				image_noir_info->dynamic_header_record_array[i];

			    mx_status = mxp_image_noir_info_update_record_value(
				referenced_record, alias_value_struct );

			    if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			}

#if MX_IMAGE_NOIR_DEBUG_WRITE
			switch( alias_value_struct->datatype ) {
			case MXFT_LONG:
				MX_DEBUG(-2,("%s: %s=%ld;",
					fname, alias_name,
					alias_value_struct->u.long_value));
				break;
			case MXFT_DOUBLE:
				MX_DEBUG(-2,("%s: %s=%f;",
					fname, alias_name,
					alias_value_struct->u.double_value));
				break;
			case MXFT_STRING:
				MX_DEBUG(-2,("%s: %s=%s;",
					fname, alias_name,
					alias_value_struct->u.string_value));
				break;
			}
#endif

			switch( alias_value_struct->datatype ) {
			case MXFT_LONG:
				fprintf( file, "%s=%ld;\n", alias_name,
					alias_value_struct->u.long_value );
				break;
			case MXFT_DOUBLE:
				fprintf( file, "%s=%f;\n", alias_name,
					alias_value_struct->u.double_value );
				break;
			case MXFT_STRING:
				fprintf( file, "%s=%s;\n", alias_name,
					alias_value_struct->u.string_value );
				break;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

