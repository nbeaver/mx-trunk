/*
 * Name:    mx_dynamic_library.c
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
#include <stdlib.h>

#include "mx_util.h"
#include "mx_dynamic_library.h"

MX_EXPORT void *
mx_dynamic_library_get_symbol_pointer( MX_DYNAMIC_LIBRARY *library,
					const char *symbol_name )
{
	void *result_ptr;
	mx_status_type mx_status;

	mx_status = mx_dynamic_library_find_symbol( library, symbol_name,
							&result_ptr, TRUE );

	if ( mx_status.code != MXE_SUCCESS ) {
		result_ptr = NULL;
	}

	return result_ptr;
}

/************************ Microsoft Win32 ***********************/

#if defined(OS_WIN32)

#include "windows.h"

MX_EXPORT mx_status_type
mx_dynamic_library_open( const char *filename,
			MX_DYNAMIC_LIBRARY **library )
{
	static const char fname[] = "mx_dynamic_library_open()";

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
mx_dynamic_library_close( MX_DYNAMIC_LIBRARY *library )
{
	static const char fname[] = "mx_dynamic_library_close()";

	BOOL os_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

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

	if ( os_status == 0 ) {
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
mx_dynamic_library_find_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				mx_bool_type quiet_flag )
{
	static const char fname[] = "mx_dynamic_library_find_symbol()";

	long mx_error_code;
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
			mx_error_code =
				(MXE_OPERATING_SYSTEM_ERROR | MXE_QUIET);
		} else {
			mx_error_code = MXE_OPERATING_SYSTEM_ERROR;
		}

		return mx_error( mx_error_code, fname,
		"Unable to find symbol '%s' in dynamic library '%s'.  "
		"Win32 error code = %ld, error message = '%s'.",
				symbol_name, library->filename,
				last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/************************ VxWorks ***********************/

#elif defined(OS_VXWORKS)

#include <ioLib.h>
#include <symLib.h>
#include <loadLib.h>
#include <unldLib.h>
#include <sysSymTbl.h>

MX_EXPORT mx_status_type
mx_dynamic_library_open( const char *filename,
			MX_DYNAMIC_LIBRARY **library )
{
	static const char fname[] = "mx_dynamic_library_open()";

	int fd;

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}
	if ( library == (MX_DYNAMIC_LIBRARY **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DYNAMIC_LIBRARY pointer passed was NULL." );
	}

	fd = open( filename, O_RDONLY, 0 );

	if ( fd == ERROR ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to open object module '%s'.", filename );
	}

	*library = malloc( sizeof(MX_DYNAMIC_LIBRARY) );

	if ( (*library) == (MX_DYNAMIC_LIBRARY *) NULL ) {
		close(fd);

		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"an MX_DYNAMIC_LIBRARY structure." );
	}

	(*library)->object = NULL;

	(*library)->object = loadModule( fd, LOAD_ALL_SYMBOLS );

	if ( (*library)->object == NULL ) {
		close(fd);
		mx_free( *library );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to open object module '%s'.", filename );
	}

	close(fd);

	strlcpy( (*library)->filename, filename, sizeof((*library)->filename) );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dynamic_library_close( MX_DYNAMIC_LIBRARY *library )
{
	static const char fname[] = "mx_dynamic_library_close()";

	int os_status;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

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

	os_status = unldByModuleId( (MODULE_ID) library->object, 0 );

	if ( os_status != 0 ) {
		mx_free( library );
		
		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unable to close dynamic library '%s'.",
				library->filename );
	}

	mx_free( library );

	return mx_status;
}

/* WARNING: VxWorks uses the same symbol table for all shared objects,
 * so the symbol you find may be from a different shared object!
 */

MX_EXPORT mx_status_type
mx_dynamic_library_find_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				mx_bool_type quiet_flag )
{
	static const char fname[] = "mx_dynamic_library_find_symbol()";

	int os_status;
	long error_code;
	char local_symbol_name[MAX_SYS_SYM_LEN+1];

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
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The object pointer for the MX_DYNAMIC_LIBRARY pointer "
			"passed was NULL." );
	}

	/* Avoid the warning
	 *   passing arg 2 of `symFindByName' discards `const' 
	 *   from pointer target type
	 * by copying the string to a local buffer.
	 */

	if ( strlen(symbol_name) >= sizeof(local_symbol_name) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Symbol name '%s' is longer (%lu) than the maximum allowed "
		"symbol length (%d) for the VxWorks system symbol table.",
			symbol_name, (unsigned long) strlen(symbol_name),
			MAX_SYS_SYM_LEN );
	}

	strlcpy( local_symbol_name, symbol_name, sizeof(local_symbol_name) );

	/* Look up the symbol name. */

	os_status = symFindByName( sysSymTbl,
				local_symbol_name,
				(char **) symbol_pointer, 0 );

	if ( os_status == ERROR ) {

		if ( quiet_flag ) {
			error_code = (MXE_OPERATING_SYSTEM_ERROR | MXE_QUIET);
		} else {
			error_code = MXE_OPERATING_SYSTEM_ERROR;
		}

		return mx_error( error_code, fname,
			"Unable to find symbol '%s' in dynamic library '%s'.",
				symbol_name, library->filename );
	}

	return MX_SUCCESSFUL_RESULT;
}

/************************ dlopen() ***********************/

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_BSD) || defined(OS_IRIX) || defined(OS_CYGWIN) \
	|| defined(OS_QNX)

#include <errno.h>
#include <dlfcn.h>

MX_EXPORT mx_status_type
mx_dynamic_library_open( const char *filename,
			MX_DYNAMIC_LIBRARY **library )
{
	static const char fname[] = "mx_dynamic_library_open()";

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
mx_dynamic_library_close( MX_DYNAMIC_LIBRARY *library )
{
	static const char fname[] = "mx_dynamic_library_close()";

	int os_status, saved_errno;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

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

		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unable to close dynamic library '%s'.  "
				"Error code = %d, error message = '%s'.",
					library->filename,
					saved_errno, strerror(saved_errno) );
	}

	mx_free( library );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_dynamic_library_find_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				mx_bool_type quiet_flag )
{
	static const char fname[] = "mx_dynamic_library_find_symbol()";

	int saved_errno;
	long error_code;

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
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The object pointer for the MX_DYNAMIC_LIBRARY pointer "
			"passed was NULL." );
	}

	*symbol_pointer = dlsym( library->object, symbol_name );

	if ( (*symbol_pointer) == NULL ) {

		saved_errno = errno;
		
		if ( quiet_flag ) {
			error_code = (MXE_OPERATING_SYSTEM_ERROR | MXE_QUIET);
		} else {
			error_code = MXE_OPERATING_SYSTEM_ERROR;
		}

		return mx_error( error_code, fname,
			"Unable to find symbol '%s' in dynamic library '%s'.  "
			"Error code = %d, error message = '%s'.",
				symbol_name, library->filename,
				saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/************************ Not available ***********************/

#elif defined(OS_ECOS) || defined(OS_RTEMS) || defined(OS_DJGPP)

MX_EXPORT mx_status_type
mx_dynamic_library_open( const char *filename,
			MX_DYNAMIC_LIBRARY **library )
{
	static const char fname[] = "mx_dynamic_library_open()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Dynamic loading of libraries is not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_dynamic_library_close( MX_DYNAMIC_LIBRARY *library )
{
	static const char fname[] = "mx_dynamic_library_close()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Dynamic loading of libraries is not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_dynamic_library_find_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				mx_bool_type quiet_flag )
{
	static const char fname[] = "mx_dynamic_library_find_symbol()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Dynamic loading of libraries is not supported on this platform." );
}

/************************ unknown ***********************/

#else

#error Dynamic loading of libraries has not been implemented for this platform.

#endif

