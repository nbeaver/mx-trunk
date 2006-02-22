/*
 * Name:    mx_generic.c
 *
 * Purpose: MX function library of operations on generic interfaces.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_generic.h"
#include "mx_driver.h"

MX_EXPORT mx_status_type
mx_generic_getchar( MX_GENERIC *generic, char *c, int flags )
{
	static const char fname[] = "mx_generic_getchar()";

	MX_GENERIC_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GENERIC *, char *, int );
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC structure pointer is NULL" );
	}

	fl_ptr = generic->record->class_specific_function_list;

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_GENERIC_FUNCTION_LIST pointer is NULL" );
	}

	fptr = ( fl_ptr->getchar );

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"close function pointer is NULL.");
	}

	/* Invoke the function. */

	status = (*fptr)( generic, c, flags );

	return status;
}
		
MX_EXPORT mx_status_type
mx_generic_putchar( MX_GENERIC *generic, char c, int flags )
{
	static const char fname[] = "mx_generic_putchar()";

	MX_GENERIC_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GENERIC *, char, int );
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC structure pointer is NULL" );
	}

	fl_ptr = generic->record->class_specific_function_list;

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_GENERIC_FUNCTION_LIST pointer is NULL" );
	}

	fptr = ( fl_ptr->putchar );

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"close function pointer is NULL.");
	}

	/* Invoke the function. */

	status = (*fptr)( generic, c, flags );

	return status;
}
		
MX_EXPORT mx_status_type
mx_generic_read( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mx_generic_read()";

	MX_GENERIC_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GENERIC *, void *, size_t );
	mx_status_type status;

	MX_DEBUG( 2, ("mx_generic_read() invoked."));

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC structure pointer is NULL" );
	}

	fl_ptr = generic->record->class_specific_function_list;

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_GENERIC_FUNCTION_LIST pointer is NULL" );
	}

	fptr = ( fl_ptr->read );

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"close function pointer is NULL.");
	}

	/* Invoke the function. */

	status = (*fptr)( generic, buffer, count );

	return status;
}
		
MX_EXPORT mx_status_type
mx_generic_write( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mx_generic_write()";

	MX_GENERIC_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GENERIC *, void *, size_t );
	mx_status_type status;

	MX_DEBUG( 2, ("mx_generic_write() invoked."));

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC structure pointer is NULL" );
	}

	fl_ptr = generic->record->class_specific_function_list;

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_GENERIC_FUNCTION_LIST pointer is NULL" );
	}

	fptr = ( fl_ptr->write );

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"close function pointer is NULL.");
	}

	/* Invoke the function. */

	status = (*fptr)( generic, buffer, count );

	return status;
}

MX_EXPORT mx_status_type
mx_generic_num_input_bytes_available( MX_GENERIC *generic,
				unsigned long *num_input_bytes_available )
{
	static const char fname[] = "mx_generic_num_input_bytes_available()";

	MX_GENERIC_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GENERIC *, unsigned long * );
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC structure pointer is NULL" );
	}

	fl_ptr = generic->record->class_specific_function_list;

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_GENERIC_FUNCTION_LIST pointer is NULL" );
	}

	fptr = ( fl_ptr->num_input_bytes_available );

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"close function pointer is NULL.");
	}

	/* Invoke the function. */

	status = (*fptr)( generic, num_input_bytes_available );

	return status;
}

MX_EXPORT mx_status_type
mx_generic_discard_unread_input( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mx_generic_discard_unread_input()";

	MX_GENERIC_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GENERIC *, int );
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC structure pointer is NULL" );
	}

	fl_ptr = generic->record->class_specific_function_list;

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_GENERIC_FUNCTION_LIST pointer is NULL" );
	}

	fptr = ( fl_ptr->discard_unread_input );

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"close function pointer is NULL.");
	}

	/* Invoke the function. */

	status = (*fptr)( generic, debug_flag );

	return status;
}

MX_EXPORT mx_status_type
mx_generic_discard_unwritten_output( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mx_generic_discard_unwritten_output()";

	MX_GENERIC_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GENERIC *, int );
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC structure pointer is NULL" );
	}

	fl_ptr = generic->record->class_specific_function_list;

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_GENERIC_FUNCTION_LIST pointer is NULL" );
	}

	fptr = ( fl_ptr->discard_unwritten_output );

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"close function pointer is NULL.");
	}

	/* Invoke the function. */

	status = (*fptr)( generic, debug_flag );

	return status;
}

