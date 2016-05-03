/*
 * Name:     mx_datafile.c
 *
 * Purpose:  Support for writing a data file during a scan.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005, 2008, 2015-2016
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_scan.h"
#include "mx_datafile.h"
#include "mx_driver.h"
#include "f_none.h"
#include "f_child.h"
#include "f_text.h"
#include "f_sff.h"
#include "f_xafs.h"
#include "f_custom.h"

MX_DATAFILE_TYPE_ENTRY mx_datafile_type_list[] = {
	{ MXDF_NONE,  "none",  &mxdf_none_datafile_function_list },
	{ MXDF_CHILD, "child", &mxdf_child_datafile_function_list },
	{ MXDF_TEXT,  "text",  &mxdf_text_datafile_function_list },
	{ MXDF_SFF,   "sff",   &mxdf_sff_datafile_function_list },
	{ MXDF_XAFS,  "xafs",  &mxdf_xafs_datafile_function_list },
	{ MXDF_CUSTOM, "custom", &mxdf_custom_datafile_function_list },
	{ -1, "", NULL }
};

MX_EXPORT mx_status_type
mx_get_datafile_type_by_name( 
	MX_DATAFILE_TYPE_ENTRY *datafile_type_list,
	char *name,
	MX_DATAFILE_TYPE_ENTRY **datafile_type_entry )
{
	static const char fname[] = "mx_get_datafile_type_by_name()";

	char *list_name;
	int i;

	if ( datafile_type_list == NULL ) {
		*datafile_type_entry = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Datafile type list passed was NULL." );
	}

	if ( name == NULL ) {
		*datafile_type_entry = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Plot name pointer passed is NULL.");
	}

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( datafile_type_list[i].type < 0 ) {
			*datafile_type_entry = NULL;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Datafile type '%s' was not found.", name );
		}

		list_name = datafile_type_list[i].name;

		if ( list_name == NULL ) {
			*datafile_type_entry = NULL;

			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"NULL name ptr for datafile type %ld.",
				datafile_type_list[i].type );
		}

		if ( strcmp( name, list_name ) == 0 ) {
			*datafile_type_entry = &( datafile_type_list[i] );

			MX_DEBUG( 8,
			("mx_get_datafile_type_by_name: ptr = 0x%p, type = %ld",
			*datafile_type_entry, (*datafile_type_entry)->type));

			return MX_SUCCESSFUL_RESULT;
		}
	}
}

MX_EXPORT mx_status_type mx_get_datafile_type_by_value( 
	MX_DATAFILE_TYPE_ENTRY *datafile_type_list,
	long datafile_type,
	MX_DATAFILE_TYPE_ENTRY **datafile_type_entry )
{
	static const char fname[] = "mx_get_datafile_type_by_value()";

	int i;

	if ( datafile_type_list == NULL ) {
		*datafile_type_entry = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Datafile type list passed was NULL." );
	}

	if ( datafile_type <= 0 ) {
		*datafile_type_entry = NULL;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Datafile type %ld is illegal.", datafile_type );
	}

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( datafile_type_list[i].type < 0 ) {
			*datafile_type_entry = NULL;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Datafile type %ld was not found.", datafile_type );
		}

		if ( datafile_type_list[i].type == datafile_type ) {
			*datafile_type_entry = &( datafile_type_list[i] );

			MX_DEBUG( 8, ("%s: ptr = 0x%p, type = %ld", fname,
				*datafile_type_entry,
				(*datafile_type_entry)->type));

			return MX_SUCCESSFUL_RESULT;
		}
	}
}

MX_EXPORT mx_status_type
mx_datafile_open( MX_DATAFILE *datafile )
{
	static const char fname[] = "mx_datafile_open()";

	MX_DATAFILE_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_DATAFILE * );
	mx_status_type mx_status;

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	flist
	  = (MX_DATAFILE_FUNCTION_LIST *) (datafile->datafile_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_FUNCTION_LIST pointer for datafile is NULL");
	}

	fptr = flist->open;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"open function pointer for datafile type %ld is NULL",
			datafile->type );
	}

	mx_status = (*fptr) ( datafile );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_datafile_close( MX_DATAFILE *datafile )
{
	static const char fname[] = "mx_datafile_close()";

	MX_DATAFILE_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_DATAFILE * );
	mx_status_type mx_status;

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	flist
	  = (MX_DATAFILE_FUNCTION_LIST *) (datafile->datafile_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_FUNCTION_LIST pointer for datafile is NULL");
	}

	/* Now invoke the close function. */

	fptr = flist->close;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"close function pointer for datafile type %ld is NULL",
			datafile->type );
	}

	mx_status = (*fptr) ( datafile );

	/* If the datafile close succeeded, transform the datafile filename
	 * to be ready for the next scan.
	 */

	if ( mx_status.code == MXE_SUCCESS ) {
		mx_status = mx_update_datafile_name( datafile );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_datafile_write_main_header( MX_DATAFILE *datafile )
{
	static const char fname[] = "mx_datafile_write_main_header()";

	MX_DATAFILE_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_DATAFILE * );
	mx_status_type mx_status;

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	flist
	  = (MX_DATAFILE_FUNCTION_LIST *) (datafile->datafile_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_FUNCTION_LIST pointer for datafile is NULL");
	}

	/* Now invoke the write_main_header function. */

	fptr = flist->write_main_header;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"write_main_header function pointer for datafile type %ld is NULL",
			datafile->type );
	}

	mx_status = (*fptr) ( datafile );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_datafile_write_segment_header( MX_DATAFILE *datafile )
{
	static const char fname[] = "mx_datafile_write_segment_header()";

	MX_DATAFILE_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_DATAFILE * );
	mx_status_type mx_status;

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	flist
	  = (MX_DATAFILE_FUNCTION_LIST *) (datafile->datafile_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_FUNCTION_LIST pointer for datafile is NULL");
	}

	/* Now invoke the write_segment_header function. */

	fptr = flist->write_segment_header;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"write_segment_header function pointer for datafile type %ld is NULL",
			datafile->type );
	}

	mx_status = (*fptr) ( datafile );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_datafile_write_trailer( MX_DATAFILE *datafile )
{
	static const char fname[] = "mx_datafile_write_trailer()";

	MX_DATAFILE_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_DATAFILE * );
	mx_status_type mx_status;

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	flist
	  = (MX_DATAFILE_FUNCTION_LIST *) (datafile->datafile_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_FUNCTION_LIST pointer for datafile is NULL");
	}

	/* Now invoke the write_trailer function. */

	fptr = flist->write_trailer;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"write_trailer function pointer for datafile type %ld is NULL",
			datafile->type );
	}

	mx_status = (*fptr) ( datafile );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_add_measurement_to_datafile( MX_DATAFILE *datafile )
{
	static const char fname[] = "mx_add_measurement_to_datafile()";

	MX_DATAFILE_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_DATAFILE * );
	mx_status_type mx_status;

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	flist
	  = (MX_DATAFILE_FUNCTION_LIST *) (datafile->datafile_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_FUNCTION_LIST pointer for datafile is NULL");
	}

	/* Now invoke the add_measurement_to_datafile function. */

	fptr = flist->add_measurement_to_datafile;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"add_measurement_to_datafile function pointer for datafile type %ld is NULL",
			datafile->type );
	}

	mx_status = (*fptr) ( datafile );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_add_array_to_datafile( MX_DATAFILE *datafile,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array )
{
	static const char fname[] = "mx_add_array_to_datafile()";

	MX_DATAFILE_FUNCTION_LIST *flist;
	mx_status_type (*fptr)(MX_DATAFILE *,
				long, long, void *, long, long, void *);
	mx_status_type mx_status;

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	flist
	  = (MX_DATAFILE_FUNCTION_LIST *) (datafile->datafile_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_FUNCTION_LIST pointer for datafile is NULL");
	}

	/* Now invoke the add_array_to_datafile function. */

	fptr = flist->add_array_to_datafile;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"add_array_to_datafile function pointer for datafile type %ld is NULL",
			datafile->type );
	}

	mx_status = (*fptr) ( datafile,
			position_type, num_positions, position_array,
			data_type, num_data_points, data_array );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_update_datafile_name( MX_DATAFILE *datafile )
{
	static const char fname[] = "mx_update_datafile_name()";

	char *filename;
	char *period_ptr, *number_string_ptr;
	size_t number_length, digits_length;
	unsigned long number_value, old_number_value, divisor;
	int i, max_digits, num_items;

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	/* If the datafile name looks like "arbitrary_string.integer_number",
	 * we increment the integer number field by 1.  Otherwise, we do
	 * nothing to the filename.  For example, "myname.027" will be
	 * incremented to "myname.028".
	 *
	 * The number of digits in the number field is always kept the same.
	 * If the number has a value that would require a larger number
	 * of digits to display than before, the number instead wraps back
	 * to zero.  For example, "myname.999" would wrap back to "myname.000".
	 */

	filename = datafile->filename;

	/* Is there a period in the filename? */

	period_ptr = strrchr( filename, '.' );

	if ( period_ptr == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Is there anything in the string after the period? */

	number_string_ptr = period_ptr + 1;

	if ( *number_string_ptr == '\0' ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Does the part of the string after the period entirely consist
	 * of the digits 0-9 and nothing else?
	 */

	number_length = strlen( number_string_ptr );

	digits_length = strspn( number_string_ptr, "0123456789" );

	if ( number_length != digits_length ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* We have determined that the part of the string after the period 
	 * is composed entirely of the digits 0-9.  Read in the corresponding
	 * number.
	 */

	num_items = sscanf( number_string_ptr, "%lu", &old_number_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Parse error in sscanf() for filename '%s'", filename );
	}

	/* If the string representation of 'number_value' now would require
	 * more characters than before, cause the value to wrap back to zero.
	 * For example, if 'old_number_value' was 999, force 'number_value'
	 * to be 0.  This is done by computing a divisor, which in this case
	 * would be 1000 and then discovering that while 999/1000 == 0,
	 * 1000/1000 == 1 (using integer division).
	 */

	divisor = 1U;
	max_digits = 0;

	for ( i = 0; i < (int) number_length; i++ ) {

		if ( ULONG_MAX / divisor < 10U ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
"There are too many digits (%d) in the number string '%s' in filename '%s'.  "
"Reduce it to %d digits or less.",
				(int) number_length, number_string_ptr,
				filename, max_digits );
		}

		divisor *= 10U;

		max_digits++;
	}

	number_value = old_number_value + 1U;

	if ( number_value / divisor > 0U ) {
		number_value = 0U;
	}

	/* Now update the filename while keeping the same number of digits
	 * in the number string field by the use of zero fill.
	 */

	snprintf( number_string_ptr, strlen(number_string_ptr)+1,
		"%0*lu", (int) number_length, number_value );

	return MX_SUCCESSFUL_RESULT;
}

/*
 * Datafile name handling policy:
 *
 * If the datafile name looks like "arbitrary_string.integer_number",
 * we increment the integer number field by 1.  Otherwise, we do
 * nothing to the filename.  For example, "myname.027" will be
 * incremented to "myname.028".
 *
 * The number of digits in the number field is always kept the same.
 * If the number has a value that would require a larger number
 * of digits to display than before, the number instead wraps back
 * to zero.  For example, "myname.999" would wrap back to "myname.000".
 */

/*
 * The function mx_parse_datafile_name() looks for the presence of a version
 * number at the end of the filename.  If the last period character in
 * the filename is followed only by numerical digits, then the end of
 * the filename is interpreted as a version number.  If the filename
 * has a version number in it, a pointer 'version_number_pointer' to the
 * start of the character string representation of the version number
 * in the supplied buffer 'datafile_name_buffer' is returned.  In addition,
 * a numerical value 'version_number' for the version number is returned,
 * along with the number of characters 'version_number_length' in the
 * string representation of the number.  If the filename has no version
 * number, then 'version_number_pointer' is NULL, while 'version_number'
 * and 'version_number_length' are set to -1.
 */

MX_EXPORT mx_status_type
mx_parse_datafile_name( char *datafile_name, char **version_number_pointer,
			long *version_number, long *version_number_length )
{
	static const char fname[] = "mx_parse_datafile_name()";

	char *period_ptr, *number_string_ptr;
	size_t number_length, digits_length;
	unsigned long old_number_value;
	int num_items;

	*version_number_pointer = NULL;
	*version_number = -1;
	*version_number_length = -1;

	/* Is there a period in the filename? */

	period_ptr = strrchr( datafile_name, '.' );

	if ( period_ptr == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Is there anything in the string after the period? */

	number_string_ptr = period_ptr + 1;

	if ( *number_string_ptr == '\0' ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Does the part of the string after the period entirely consist
	 * of the digits 0-9 and nothing else?
	 */

	number_length = strlen( number_string_ptr );

	digits_length = strspn( number_string_ptr, "0123456789" );

	if ( number_length != digits_length ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* We have determined that the part of the string after the period 
	 * is composed entirely of the digits 0-9.  Read in the corresponding
	 * number.
	 */

	num_items = sscanf( number_string_ptr, "%lu", &old_number_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Parse error in sscanf() for filename '%s'",
			datafile_name );
	}

	*version_number_pointer = number_string_ptr;
	*version_number         = (long) old_number_value;
	*version_number_length  = (long) number_length;

	return MX_SUCCESSFUL_RESULT;
}

/* If the string representation of 'version_number' now would require
 * more characters than before, cause the value to wrap back to zero.
 * For example, if 'old_version_number' was 999, force 'version_number'
 * to be 0.  This is done by computing a divisor, which in this case
 * would be 1000 and then discovering that while 999/1000 == 0,
 * 1000/1000 == 1 (using integer division).
 */

MX_EXPORT long
mx_construct_datafile_version_number( long test_version_number,
					long version_number_length )
{
	static const char fname[] = "mx_construct_datafile_version_number()";

	unsigned long version_number, divisor;
	int i, max_digits;

	divisor = 1U;
	max_digits = 0;

	for ( i = 0; i < (int) version_number_length; i++ ) {

		if ( ULONG_MAX / divisor < 10U ) {
			(void) mx_error( MXE_FUNCTION_FAILED, fname,
"There are too many digits (%ld) in the file version number %ld.  "
"Reduce it to %d digits or less.",
				version_number_length,
				test_version_number,
				max_digits );

			return -1;
		}

		divisor *= 10U;

		max_digits++;
	}

	version_number = test_version_number;

	if ( version_number / divisor > 0U ) {
		version_number = 0U;
	}

	return (long) version_number;
}

static mx_status_type
mx_datafile_do_x_command( MX_DATAFILE *datafile, char *command_arguments )
{
	static const char fname[] = "mx_datafile_do_x_command()";

	MX_RECORD *motor_record;
	MX_SCAN *scan;
	char record_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	long i;
	int valid_type;
	size_t length;
	char *start_ptr, *end_ptr;

	scan = (MX_SCAN *) datafile->scan;

	/* The motor names are separated by comma (,) characters.
	 * By counting the number of commas, we can figure out how
	 * many motors there are.
	 */

	scan->datafile.num_x_motors = 1;

	start_ptr = command_arguments;

	for(;;) {
		end_ptr = strchr( start_ptr, ',' );

		if ( end_ptr == NULL ) {
			break;			/* Exit the for loop. */
		}

		(scan->datafile.num_x_motors)++;

		start_ptr = (++end_ptr);
	}

	scan->datafile.x_motor_array = (MX_RECORD **)
		malloc( scan->datafile.num_x_motors * sizeof(MX_RECORD *) );

	if ( scan->datafile.x_motor_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate a %ld element array of MX_RECORD pointers "
	"for the datafile x_motor_array used by scan '%s'.",
			scan->datafile.num_x_motors, scan->record->name );
	}

	start_ptr = command_arguments;

	for ( i = 0; i < scan->datafile.num_x_motors; i++ ) {
		end_ptr = strchr( start_ptr, ',' );

		if ( end_ptr == NULL ) {
			strlcpy( record_name, start_ptr,
					MXU_RECORD_NAME_LENGTH );
		} else {
			length = end_ptr - start_ptr + 1;

			if ( length > MXU_RECORD_NAME_LENGTH ) {
				length = MXU_RECORD_NAME_LENGTH;
			}

			strlcpy( record_name, start_ptr, length );
		}

		motor_record = mx_get_record( scan->record, record_name );

		if ( motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
		"The motor record '%s' specified by the datafile 'x=' command "
		"for scan '%s' does not exist.",
				record_name, scan->record->name );
		}

		valid_type = mx_verify_driver_type( motor_record,
				MXR_DEVICE, MXC_MOTOR, MXT_ANY );

		if ( valid_type == FALSE ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' specified by the datafile 'x=' command "
		"for scan '%s' is not a motor record.",
				motor_record->name, scan->record->name );
		}

		scan->datafile.x_motor_array[i] = motor_record;

		if ( end_ptr != NULL ) {
			start_ptr = (++end_ptr);
		}
	}

#if 0
	for ( i = 0; i < scan->datafile.num_x_motors; i++ ) {
		MX_DEBUG(-2,("%s: scan '%s', motor[%ld] = '%s'",
			fname, scan->record->name, i,
			scan->datafile.x_motor_array[i]->name));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_datafile_parse_options( MX_DATAFILE *datafile )
{
	static const char fname[] = "mx_datafile_parse_options()";

	MX_SCAN *scan;
	char *options, *start_ptr, *end_ptr;
	char *command_arguments;
	char command_name[80];
	char command_buffer[120];
	int last_command;
	size_t length;
	mx_status_type mx_status;

	if ( datafile == (MX_DATAFILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DATAFILE pointer passed was NULL." );
	}

	scan = (MX_SCAN *) datafile->scan;

	options = datafile->options;

	/* The commands in the type arguments are separated by 
	 * semicolon (;) characters.
	 */

	last_command = FALSE;

	start_ptr = options;

	while ( last_command == FALSE ) {

		end_ptr = strchr( start_ptr, ';' );

		if ( end_ptr == NULL ) {
			strlcpy( command_buffer, start_ptr,
					sizeof(command_buffer) );

			last_command = TRUE;
		} else {
			length = end_ptr - start_ptr + 1;

			if ( length > sizeof(command_buffer) - 1 ) {
				length = sizeof(command_buffer) - 1;
			}

			strlcpy( command_buffer, start_ptr, length );

			start_ptr = (++end_ptr);
		}

		/* If the command has arguments, they are separated from
		 * the command name by an equals sign '=' character.
		 */

		end_ptr = strchr( command_buffer, '=' );

		if ( end_ptr == NULL ) {
			strlcpy( command_name, command_buffer,
					sizeof(command_name) );

			command_arguments = NULL;
		} else {
			length = end_ptr - command_buffer + 1;

			if ( length > ( sizeof(command_name) - 1 ) ) {
				length = sizeof(command_name) - 1;
			}

			strlcpy( command_name, command_buffer, length );

			command_arguments = (++end_ptr);
		}

		/* Figure out which command this is and invoke it. */

		length = strlen( command_name );

		if ( strcmp( command_name, "x" ) == 0 ) {
			mx_status = mx_datafile_do_x_command( datafile,
						command_arguments );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		} else if ( strncmp( command_name, "normalize_data",
					length ) == 0 )
		{
			datafile->normalize_data = TRUE;
		} else if ( strncmp( command_name, "raw_data", length ) == 0 ) {
			datafile->normalize_data = FALSE;
		} else if ( strncmp( command_name, "xafs", length ) == 0 ) {

			/* 'xafs' is a special option that turns on
			 * normalization, and sets the X axis to 'energy'.
			 */

			datafile->normalize_data = TRUE;

			mx_status = mx_datafile_do_x_command( datafile,
								"energy" );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		} else {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal command '%s' passed in datafile options for scan '%s'.",
				command_name, scan->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

