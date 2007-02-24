/*
 * Name:    mx_hrt.c
 *
 * Purpose: This file defines functions for loading dynamic libraries at
 *          run time and for searching for symbols in them.  Searching for
 *          function pointers is implemented for all platforms.  By contrast,
 *          searching for variable pointers is implemented on Unix-like 
 *          platforms, but not on Windows platforms.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_dynamic_link.h"

/************************ Microsoft Win32 ***********************/

#if defined( OS_WIN32 )

#include "windows.h"

MX_EXPORT mx_status_type
mx_dynamic_link_open_library( const char *filename,
				MX_DYNAMIC_LIBRARY **library )
{
	static const char fname[] = "mx_dynamic_link_open_library()";

	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}
	if ( library == (MX_DYNAMIC_LIBRARY **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DYNAMIC_LIBRARY pointer passed was NULL." );
	}

	*library = malloc( sizeof(MX_DYNAMIC_LIBRARY) );

	if ( (*library) == (MX_DYNAMIC_LIBRARY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"an MX_DYNAMIC_LIBRARY structure." );
	}

	(*library)->object = LoadLibrary( filename );

	if ( (*library)->object == NULL ) {

		last_error_code = GetLastError();
		
		mx_free( *library );

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to open dynamic library '%s'.  "
			"Win32 error code = %ld, error message = '%s'.",
			filename, last_error_code, message_buffer );
	}

	strlcpy( (*library)->filename, filename, sizeof((*library)->filename) );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dynamic_link_close_library( MX_DYNAMIC_LIBRARY *library )
{
	static const char fname[] = "mx_dynamic_link_close_library()";

	BOOL os_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

	if ( library == (MX_DYNAMIC_LIBRARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DYNAMIC_LIBRARY pointer passed was NULL." );
	}

	if ( library->object == NULL ) {
		mx_free( library );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The object pointer for the MX_DYNAMIC_LIBRARY pointer "
			"passed was NULL." );
	}

	os_status = FreeLibrary( library->object );

	if ( os_status != 0 ) {
		mx_status = MX_SUCCESSFUL_RESULT;
	} else {
		last_error_code = GetLastError();
		
		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to close dynamic library '%s'.  "
			"Win32 error code = %ld, error message = '%s'.",
			library->filename, last_error_code, message_buffer );
	}

	mx_free( library );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_dynamic_link_find_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				mx_bool_type quiet_flag )
{
	static const char fname[] = "mx_dynamic_link_find_symbol()";

	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( library == (MX_DYNAMIC_LIBRARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DYNAMIC_LIBRARY pointer passed was NULL." );
	}
	if ( symbol_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The symbol_name pointer passed was NULL." );
	}
	if ( symbol_pointer == (void **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The symbol_pointer argument passed was NULL." );
	}

	if ( library->object == NULL ) {
		mx_free( library );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The object pointer for the MX_DYNAMIC_LIBRARY pointer "
			"passed was NULL." );
	}

	*symbol_pointer = GetProcAddress( library->object, symbol_name );

	if ( (*symbol_pointer) == NULL ) {

		last_error_code = GetLastError();
		
		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		if ( quiet_flag ) {
			return mx_error_quiet(MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to find symbol '%s' in dynamic library '%s'.  "
			"Win32 error code = %ld, error message = '%s'.",
				symbol_name, library->filename,
				last_error_code, message_buffer );
		} else {
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to find symbol '%s' in dynamic library '%s'.  "
			"Win32 error code = %ld, error message = '%s'.",
				symbol_name, library->filename,
				last_error_code, message_buffer );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/************************ dlopen() ***********************/

#elif defined( OS_LINUX )

#include <stdlib.h>
#include <errno.h>
#include <dlfcn.h>

MX_EXPORT mx_status_type
mx_dynamic_link_open_library( const char *filename,
				MX_DYNAMIC_LIBRARY **library )
{
	static const char fname[] = "mx_dynamic_link_open_library()";

	int saved_errno;

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}
	if ( library == (MX_DYNAMIC_LIBRARY **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DYNAMIC_LIBRARY pointer passed was NULL." );
	}

	*library = malloc( sizeof(MX_DYNAMIC_LIBRARY) );

	if ( (*library) == (MX_DYNAMIC_LIBRARY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"an MX_DYNAMIC_LIBRARY structure." );
	}

	(*library)->object = dlopen( filename, RTLD_LAZY );

	if ( (*library)->object == NULL ) {

		saved_errno = errno;

		mx_free( *library );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to open dynamic library '%s'.  "
			"Error code = %d, error message = '%s'.",
			filename, saved_errno, strerror(saved_errno) );
	}

	strlcpy( (*library)->filename, filename, sizeof((*library)->filename) );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dynamic_link_close_library( MX_DYNAMIC_LIBRARY *library )
{
	static const char fname[] = "mx_dynamic_link_close_library()";

	int os_status, saved_errno;

	if ( library == (MX_DYNAMIC_LIBRARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DYNAMIC_LIBRARY pointer passed was NULL." );
	}

	if ( library->object == NULL ) {
		mx_free( library );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The object pointer for the MX_DYNAMIC_LIBRARY pointer "
			"passed was NULL." );
	}

	os_status = dlclose( library->object );

	if ( os_status != 0 ) {
		saved_errno = errno;

		mx_free( library );
		
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to close dynamic library '%s'.  "
			"Error code = %d, error message = '%s'.",
			library->filename, saved_errno, strerror(saved_errno) );
	}

	mx_free( library );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dynamic_link_find_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				mx_bool_type quiet_flag )
{
	static const char fname[] = "mx_dynamic_link_find_symbol()";

	int saved_errno;

	if ( library == (MX_DYNAMIC_LIBRARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DYNAMIC_LIBRARY pointer passed was NULL." );
	}
	if ( symbol_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The symbol_name pointer passed was NULL." );
	}
	if ( symbol_pointer == (void **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The symbol_pointer argument passed was NULL." );
	}

	if ( library->object == NULL ) {
		mx_free( library );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The object pointer for the MX_DYNAMIC_LIBRARY pointer "
			"passed was NULL." );
	}

	*symbol_pointer = dlsym( library->object, symbol_name );

	if ( (*symbol_pointer) == NULL ) {

		saved_errno = errno;
		
		if ( quiet_flag ) {
			return mx_error_quiet(MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to find symbol '%s' in dynamic library '%s'.  "
			"Error code = %d, error message = '%s'.",
				symbol_name, library->filename,
				saved_errno, strerror(saved_errno) );
		} else {
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to find symbol '%s' in dynamic library '%s'.  "
			"Error code = %d, error message = '%s'.",
				symbol_name, library->filename,
				saved_errno, strerror(saved_errno) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

#else

#error Dynamic linking of libraries is not implemented for this platform.

#endif

