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
 * Copyright 2007, 2009, 2011-2012, 2014-2016 Illinois Institute of Technology
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
					&result_ptr, MXF_DYNAMIC_LIBRARY_QUIET);

	if ( mx_status.code != MXE_SUCCESS ) {
		result_ptr = NULL;
	}

	return result_ptr;
}

/************************ Microsoft Win32 ***********************/

#if defined(OS_WIN32)

/* Calls to LoadLibrary() are reference counted, so if you call
 * LoadLibrary() multiple times on the same DLL, then you must
 * call FreeLibrary() the same number of times before it is really
 * unloaded.
 *
 * However, if you passed in a NULL filename, GetModuleHandle(NULL)
 * will be invoked instead to get a handle to the .EXE.  In this
 * case, you should _not_ call FreeLibrary() on that.
 */

#include "windows.h"

MX_EXPORT mx_status_type
mx_dynamic_library_open( const char *filename,
			MX_DYNAMIC_LIBRARY **library,
			unsigned long flags )
{
	static const char fname[] = "mx_dynamic_library_open()";

	DWORD last_error_code;
	TCHAR message_buffer[100];
	long mx_error_code;
	mx_bool_type quiet;

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

	if ( flags & MXF_DYNAMIC_LIBRARY_QUIET ) {
		quiet = TRUE;
	} else {
		quiet = FALSE;
	}

	(*library)->filename[0] = '\0';

	if ( filename == (char *) NULL ) {
		(*library)->object = GetModuleHandle(NULL);
	} else
	if ( mx_strcasecmp( filename, "kernel32.dll" ) == 0 ) {
		(*library)->object = GetModuleHandle( filename );
	} else {
		(*library)->object = LoadLibrary( filename );
	}

	if ( (*library)->object == NULL ) {

		last_error_code = GetLastError();
		
		mx_free( *library );

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		mx_error_code = MXE_OPERATING_SYSTEM_ERROR;

		if ( quiet ) {
			mx_error_code |= MXE_QUIET;
		}

		if ( filename == (char *) NULL ) {
			return mx_error( mx_error_code, fname,
				"Unable to open main executable.  "
				"Win32 error code = %ld, error message = '%s'.",
				last_error_code, message_buffer );
		} else {
			return mx_error( mx_error_code, fname,
				"Unable to open dynamic library '%s'.  "
				"Win32 error code = %ld, error message = '%s'.",
				filename, last_error_code, message_buffer );
		}
	}

	if ( filename == (char *) NULL ) {
		(*library)->filename[0] = '\0';
	} else {
		strlcpy( (*library)->filename, filename,
			sizeof((*library)->filename) );
	}

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

	if ( library->filename[0] != '\0' ) {

		/* Only call FreeLibrary() for handles to DLLs, which are
		 * acquired using LoadLibrary().  FreeLibrary() should
		 * _never_ be called on a handle for the main program,
		 * which is acquired via GetModuleHandle(NULL).
		 */

		os_status = FreeLibrary( library->object );

		if ( os_status == 0 ) {
			last_error_code = GetLastError();
		
			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unable to close dynamic library '%s'.  "
				"Win32 error code = %ld, error message = '%s'.",
				library->filename,
				last_error_code, message_buffer );
		}
	}

	mx_free( library );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_dynamic_library_find_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				unsigned long flags )
{
	static const char fname[] = "mx_dynamic_library_find_symbol()";

	DWORD last_error_code;
	TCHAR message_buffer[100];
	long mx_error_code;
	mx_bool_type quiet;

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

	if ( flags & MXF_DYNAMIC_LIBRARY_QUIET ) {
		quiet = TRUE;
	} else {
		quiet = FALSE;
	}

	*symbol_pointer = GetProcAddress( library->object, symbol_name );

	if ( (*symbol_pointer) == NULL ) {

		last_error_code = GetLastError();
		
		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		mx_error_code = MXE_OPERATING_SYSTEM_ERROR;

		if ( quiet ) {
			mx_error_code |= MXE_QUIET;
		}

		return mx_error( mx_error_code, fname,
		"Unable to find symbol '%s' in dynamic library '%s'.  "
		"Win32 error code = %ld, error message = '%s'.",
				symbol_name, library->filename,
				last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dynamic_library_get_function_name_from_address( void *address,
						char *function_name,
						size_t max_name_length )
{
	static const char fname[] =
		"mx_dynamic_library_get_function_name_from_address()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented for this platform." );
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
			MX_DYNAMIC_LIBRARY **library,
			unsigned long flags )
{
	static const char fname[] = "mx_dynamic_library_open()";

	int fd;
	long mx_error_code;
	mx_bool_type quiet;

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}
	if ( library == (MX_DYNAMIC_LIBRARY **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DYNAMIC_LIBRARY pointer passed was NULL." );
	}

	if ( flags & MXF_DYNAMIC_LIBRARY_QUIET ) {
		quiet = TRUE;
	} else {
		quiet = FALSE;
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

		mx_error_code = MXE_OPERATING_SYSTEM_ERROR;

		if ( quiet ) {
			mx_error_code |= MXE_QUIET;
		}

		return mx_error( mx_error_code, fname,
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
				unsigned long flags )
{
	static const char fname[] = "mx_dynamic_library_find_symbol()";

	int os_status;
	char local_symbol_name[MAX_SYS_SYM_LEN+1];
	long mx_error_code;
	mx_bool_type quiet;

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

	if ( flags & MXF_DYNAMIC_LIBRARY_QUIET ) {
		quiet = TRUE;
	} else {
		quiet = FALSE;
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

#if ( MX_VXWORKS_VERSION < 6009000L )  /* Before VxWorks 6.9 */

	os_status = symFindByName( sysSymTbl,
				local_symbol_name,
				(char **) symbol_pointer, 0 );

#else  /* VxWorks 6.9 and after */
	{
		SYMBOL_DESC symbol_descriptor;
		void *symbol_address;

		memset( &symbol_descriptor, 0, sizeof(SYMBOL_DESC) );

		symbol_descriptor.mask = SYM_FIND_BY_NAME;
		symbol_descriptor.name = local_symbol_name;

		os_status = symFind( sysSymTbl, &symbol_descriptor );

		if ( os_status != ERROR ) {
			symbol_address = symbol_descriptor.value;

			if ( symbol_pointer != NULL ) {
				*symbol_pointer = symbol_address;
			}
		}
	}
#endif

	if ( os_status == ERROR ) {

		mx_error_code = MXE_OPERATING_SYSTEM_ERROR;

		if ( quiet ) {
			mx_error_code |= MXE_QUIET;
		}

		return mx_error( mx_error_code, fname,
			"Unable to find symbol '%s' in dynamic library '%s'.",
				symbol_name, library->filename );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dynamic_library_get_function_name_from_address( void *address,
						char *function_name,
						size_t max_name_length )
{
	static const char fname[] =
		"mx_dynamic_library_get_function_name_from_address()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented for this platform." );
}

/************************ dlopen() ***********************/

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_BSD) || defined(OS_IRIX) || defined(OS_CYGWIN) \
	|| defined(OS_QNX) || defined(OS_VMS) || defined(OS_UNIXWARE) \
	|| defined(OS_HURD) || defined(OS_ANDROID) || defined(OS_MINIX)

/* Note: On most systems, calls to dlopen() are reference counted.  If you
 * dlopen() a library multiple times, then you must dlclose() it the same
 * number of times before it is really unloaded.
 * 
 * However, if you passed in a NULL filename pointer, the handle you get
 * back refers to the process as a whole, so you should not dlclose() that.
 */

#if defined(__GLIBC__)
#  define __USE_GNU
#endif

#include <dlfcn.h>

MX_EXPORT mx_status_type
mx_dynamic_library_open( const char *filename,
			MX_DYNAMIC_LIBRARY **library,
			unsigned long flags )
{
	static const char fname[] = "mx_dynamic_library_open()";

	long mx_error_code;
	mx_bool_type quiet;

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

	if ( flags & MXF_DYNAMIC_LIBRARY_QUIET ) {
		quiet = TRUE;
	} else {
		quiet = FALSE;
	}

	(*library)->object = dlopen( filename, RTLD_LAZY );

	if ( (*library)->object == NULL ) {

		mx_free( *library );

		mx_error_code = MXE_OPERATING_SYSTEM_ERROR;

		if ( quiet ) {
			mx_error_code |= MXE_QUIET;
		}

		return mx_error( mx_error_code, fname,
			"Unable to open dynamic library '%s'.  "
			"dlopen() error message = '%s'.", filename, dlerror() );
	}

	if ( filename == (char *) NULL ) {
		(*library)->filename[0] = '\0';
	} else {
		strlcpy( (*library)->filename, filename,
				sizeof((*library)->filename) );
	}

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

	os_status = dlclose( library->object );

	if ( os_status != 0 ) {
		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unable to close dynamic library '%s'.  "
				"dlclose() error message = '%s'.",
					library->filename, dlerror() );
	}

	mx_free( library );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_dynamic_library_find_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				unsigned long flags )
{
	static const char fname[] = "mx_dynamic_library_find_symbol()";

	long mx_error_code;
	mx_bool_type quiet;

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

	if ( flags & MXF_DYNAMIC_LIBRARY_QUIET ) {
		quiet = TRUE;
	} else {
		quiet = FALSE;
	}

	*symbol_pointer = dlsym( library->object, symbol_name );

	if ( (*symbol_pointer) == NULL ) {

		mx_error_code = MXE_OPERATING_SYSTEM_ERROR;

		if ( quiet ) {
			mx_error_code |= MXE_QUIET;
		}

		return mx_error( mx_error_code, fname,
			"Unable to find symbol '%s' in dynamic library '%s'.  "
			"dlsym() error message = '%s'.",
				symbol_name, library->filename, dlerror() );
	}

	return MX_SUCCESSFUL_RESULT;
}

#if ( defined(__GLIBC__) || defined(MX_MUSL_VERSION) )

MX_EXPORT mx_status_type
mx_dynamic_library_get_function_name_from_address( void *address,
						char *function_name,
						size_t max_name_length )
{
	static const char fname[] =
		"mx_dynamic_library_get_function_name_from_address()";

	Dl_info info;
	int status;

	status = dladdr( address, &info );

	if ( status == 0 ) {
		return mx_error( MXE_NOT_FOUND | MXE_QUIET, fname,
		"No function name was found for address %p.", address );
	}

#if 0
	MX_DEBUG(-2,("%s: address = %p, dli_fname = '%s', dli_fbase = %p, "
		"dli_sname = '%s', dli_saddr = %p",
		fname, address, info.dli_fname, info.dli_fbase,
		info.dli_sname, info.dli_saddr));
#endif

	strlcpy( function_name, info.dli_fname, max_name_length );

	return MX_SUCCESSFUL_RESULT;
}

#else  /* Not __GLIBC__ or MX_MUSL_VERSION */

MX_EXPORT mx_status_type
mx_dynamic_library_get_function_name_from_address( void *address,
						char *function_name,
						size_t max_name_length )
{
	static const char fname[] =
		"mx_dynamic_library_get_function_name_from_address()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented for this platform." );
}

#endif  /* Not __GLIBC__ or MX_MUSL_VERSION */

/************************ Not available ***********************/

#elif defined(OS_ECOS) || defined(OS_RTEMS) || defined(OS_DJGPP)

MX_EXPORT mx_status_type
mx_dynamic_library_open( const char *filename,
			MX_DYNAMIC_LIBRARY **library,
			unsigned long flags )
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
				unsigned long flags )
{
	static const char fname[] = "mx_dynamic_library_find_symbol()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Dynamic loading of libraries is not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_dynamic_library_get_function_name_from_address( void *address,
						char *function_name,
						size_t max_name_length )
{
	static const char fname[] =
		"mx_dynamic_library_get_function_name_from_address()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Finding a function name from an address is not supported "
		"on this platform." );
}

/************************ unknown ***********************/

#else

#error Dynamic loading of libraries has not been implemented for this platform.

#endif

/*=========================================================================*/

                  /* Platform independent functions. */

MX_EXPORT mx_status_type
mx_dynamic_library_get_library_and_symbol( const char *filename,
					const char *symbol_name,
					MX_DYNAMIC_LIBRARY **library,
					void **symbol,
					unsigned long flags )
{
	MX_DYNAMIC_LIBRARY *library_ptr;
	void *symbol_ptr;
	mx_status_type mx_status;

	mx_status = mx_dynamic_library_open( filename, &library_ptr, flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_dynamic_library_find_symbol( library_ptr,
						symbol_name,
						&symbol_ptr, flags );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_dynamic_library_close( library_ptr );

		return mx_status;
	}

	if ( library != (MX_DYNAMIC_LIBRARY **) NULL ) {
		*library = library_ptr;
	}
	if ( symbol != (void **) NULL ) {
		*symbol = symbol_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

