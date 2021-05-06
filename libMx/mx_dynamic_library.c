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
 * Copyright 2007, 2009, 2011-2012, 2014-2016, 2018-2021
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_DYNAMIC_LIBRARY_DEBUG_LIBRARY_POINTERS	FALSE

#define MX_DYNAMIC_LIBRARY_DEBUG_SYMBOL_POINTERS	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_program_model.h"	/* MX_WORDSIZE comes from here. */
#include "mx_dynamic_library.h"
#include "mx_unistd.h"

MX_EXPORT void *
mx_dynamic_library_get_symbol_pointer( MX_DYNAMIC_LIBRARY *library,
					const char *symbol_name )
{
	void *result_ptr;
	mx_status_type mx_status;

	mx_status = mx_dynamic_library_get_address_from_symbol( library, symbol_name,
					&result_ptr, MXF_DYNAMIC_LIBRARY_QUIET);

	if ( mx_status.code != MXE_SUCCESS ) {
		result_ptr = NULL;
	}

	return result_ptr;
}

/************************ Microsoft Win32 ***********************/

#if defined(OS_WIN32)

#include "windows.h"

/* If we receive error code 126 from an attempt to call LoadLibrary(),
 * then this means that either the library we tried to load cannot 
 * be found or that the library uses some _other_ library that could
 * not be found.  We try to distinguish between these here, to make
 * it clearer to the user what the actual error was.
 *
 * FIXME: The following method is probably not 100% reliable.  It
 * may have a race condition if 'library_filename' is changed or
 * deleted inbetween the time we got a code 126 and the time when
 * when we call the investigative function below.  So the function
 * mxp_win32_investigate_code_126() is really only a "best attempt"
 * to determine what really happened.
 *
 * More reliable solutions would be welcome.
 */

static mx_status_type
mxp_win32_investigate_file_access(const char *calling_fname,
					const char *library_filename,
					long win32_error_code,
					char *error_message_buffer )
{
	static const char fname[] = "mxp_win32_investigate_file_access()";

	int access_status, saved_errno;

	if ( calling_fname == (const char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'%s' was called with calling_fname set to NULL.  "
		"This is a program bug and should never happen.  "
		"Please report this error.", fname );
	}
	if ( library_filename == (const char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The calling function '%s' was passed a NULL argument.  "
		"This should not be able to happen.", calling_fname );
	}

	/* Does the file named 'library_filename' actually exist? */

	access_status = _access( library_filename, R_OK );

	if ( access_status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case ENOENT:
			return mx_error( MXE_NOT_FOUND, calling_fname,
			"The requested library '%s' was not found.",
				library_filename );
			break;
		case EACCES:
			return mx_error( MXE_PERMISSION_DENIED, calling_fname,
			"You do not have permission to read library '%s'.",
				library_filename );
			break;
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, calling_fname,
			"Windows seems to think that library '%s' has an "
			"illegal filename.", library_filename );
			break;
		default:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR,
				calling_fname,
			"An unexpected error occurred when trying to access "
			"library '%s'.  errno = %d, error_message = '%s'",
				library_filename,
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	switch( win32_error_code ) {
	case 5:
		return mx_error( MXE_PERMISSION_DENIED, calling_fname,
		"This process has the permissions to access the library file "
		"'%s' itself.  However, this process does not have the "
		"permissions to access some other DLL or resource that is "
		"used indirectly by this library.", library_filename );
		break;

	case 126:
		return mx_error( MXE_NOT_FOUND, calling_fname,
		"The library file '%s' exists.  However, some other DLL or "
		"resource used by this library does not exist.",
		library_filename );
		break;

	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, calling_fname,
		"An unexpected error occurred when trying to access some "
		"DLL or resource that is used by the library '%s'.  "
		"Win32 error code = %ld, error message = '%s'.",
			library_filename, win32_error_code,
			error_message_buffer );
		break;
	}
}

/*----*/

/* Calls to LoadLibrary() are reference counted, so if you call
 * LoadLibrary() multiple times on the same DLL, then you must
 * call FreeLibrary() the same number of times before it is really
 * unloaded.
 *
 * However, if you passed in a NULL filename, GetModuleHandle(NULL)
 * will be invoked instead to get a handle to the .EXE.  In this
 * case, you should _not_ call FreeLibrary() on that.
 */

MX_EXPORT mx_status_type
mx_dynamic_library_open( const char *filename,
			MX_DYNAMIC_LIBRARY **library,
			unsigned long flags )
{
	static const char fname[] = "mx_dynamic_library_open()";

	DWORD last_error_code;
	char error_message_buffer[100];
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

#if MX_DYNAMIC_LIBRARY_DEBUG_LIBRARY_POINTERS
	MX_DEBUG(-2,("%s: filename = '%s', *library = %p, object = %p",
		fname, filename, *library, (*library)->object));
#endif

	if ( (*library)->object == NULL ) {

		last_error_code = GetLastError();
		
		mx_free( *library );

		mx_win32_error_message( last_error_code,
			error_message_buffer, sizeof(error_message_buffer) );

		mx_error_code = MXE_OPERATING_SYSTEM_ERROR;

		if ( quiet ) {
			mx_error_code |= MXE_QUIET;
		}

#if 0
		MX_DEBUG(-2,
		("%s: MX_PROGRAM_MODEL = %lu, last_error_code = %lu",
			fname, MX_PROGRAM_MODEL, last_error_code));
#endif

		if ( filename == (char *) NULL ) {
			return mx_error( mx_error_code, fname,
				"Unable to open main executable.  "
				"Win32 error code = %ld, error message = '%s'.",
				last_error_code, error_message_buffer );
		} else
		if ( (MX_PROGRAM_MODEL == MX_PROGRAM_MODEL_LLP64)
		  && (last_error_code == 193) )
		{
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Received Win32 error code 193 when attempting to load "
			"dynamic library '%s'.  This normally means that "
			"a 64 bit program is trying to load a 32 bit DLL.",
				filename );
		} else
		if ( (last_error_code == 126)
		  || (last_error_code == 5 ) )
		{
			/* In this case, either the library does not exist or
			 * one of the DLLs the library uses could not be found.
			 * We make some attempt to distinguish betweeen these
			 * two cases.
			 */

			return mxp_win32_investigate_file_access( fname,
						filename, last_error_code,
						error_message_buffer );
		} else {
			return mx_error( mx_error_code, fname,
				"Unable to open dynamic library '%s'.  "
				"Win32 error code = %ld, error message = '%s'.",
				filename, last_error_code,
				error_message_buffer );
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
mx_dynamic_library_get_address_from_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				unsigned long flags )
{
	static const char fname[] = "mx_dynamic_library_get_address_from_symbol()";

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

#if MX_DYNAMIC_LIBRARY_DEBUG_SYMBOL_POINTERS
	MX_DEBUG(-2,
	("%s: library_name = '%s', library = %p, symbol_name = '%s', "
		"symbol_pointer = %p, *symbol_pointer = %p",
		fname, library->filename, library, symbol_name,
		symbol_pointer, *symbol_pointer ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dynamic_library_get_symbol_from_address( void *address,
						char *function_name,
						size_t max_name_length )
{
	static const char fname[] =
		"mx_dynamic_library_get_symbol_from_address()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented for this platform." );
}

MX_EXPORT mx_status_type
mx_dynamic_library_get_filename( MX_DYNAMIC_LIBRARY *library,
				char *library_filename,
				size_t max_library_filename_length )
{
	static const char fname[] = "mx_dynamic_library_get_filename()";

	DWORD num_bytes_copied, last_error_code;

	if ( library == (MX_DYNAMIC_LIBRARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The library pointer passed was NULL." );
	}

	if ( library_filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The library_filename_pointer passed was NULL." );
	}

	num_bytes_copied = GetModuleFileName( library->object,
					library_filename,
					max_library_filename_length );

	last_error_code = GetLastError();

#if defined( ERROR_INSUFFICIENT_BUFFER )
	if ( last_error_code == ERROR_INSUFFICIENT_BUFFER ) {
		mx_warning( "The filename returned by '%s' may "
			"have been truncated.  filename = '%s'.",
			fname, library_filename );
	}
#endif

	if ( num_bytes_copied == 0 ) {
		char win32_error_message[200];

		mx_win32_error_message( last_error_code,
			win32_error_message, sizeof(win32_error_message) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to get the full filename of DLL '%s' failed.  "
		"Error code = %ld, error message = '%s'.",
			library->filename, last_error_code,
			win32_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dynamic_library_show_list( FILE *file )
{
	static const char fname[] = "mx_dynamic_library_show_list()";

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
mx_dynamic_library_get_address_from_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				unsigned long flags )
{
	static const char fname[] = "mx_dynamic_library_get_address_from_symbol()";

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
mx_dynamic_library_get_symbol_from_address( void *address,
						char *function_name,
						size_t max_name_length )
{
	static const char fname[] =
		"mx_dynamic_library_get_symbol_from_address()";

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
mx_dynamic_library_get_address_from_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				unsigned long flags )
{
	static const char fname[] =
			"mx_dynamic_library_get_address_from_symbol()";

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

/*----- mx_dynamic_library_get_symbol_from_address() -----*/

#if defined(OS_LINUX) || defined(OS_BSD) || defined(OS_HURD) \
	|| defined(OS_SOLARIS)

MX_EXPORT mx_status_type
mx_dynamic_library_get_symbol_from_address( void *address,
						char *function_name,
						size_t max_name_length )
{
	static const char fname[] =
		"mx_dynamic_library_get_symbol_from_address()";

	Dl_info info;
	int status;

	status = dladdr( address, &info );

	if ( status == 0 ) {
		return mx_error( MXE_NOT_FOUND | MXE_QUIET, fname,
		"No symbol name was found for address %p.", address );
	}

	strlcpy( function_name, info.dli_sname, max_name_length );

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(OS_CYGWIN)

MX_EXPORT mx_status_type
mx_dynamic_library_get_symbol_from_address( void *address,
						char *function_name,
						size_t max_name_length )
{
	static const char fname[] =
		"mx_dynamic_library_get_symbol_from_address()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Not supported for this platform." );
}

#else
#error mx_dynamic_library_get_symbol_from_address() not yet implemented for this platform
#endif 

/*----- MX_dynamic_library_get_filename() -----*/

#if defined(__OpenBSD__)

#include <link.h>

typedef struct {
	MX_DYNAMIC_LIBRARY *library;
	char *filename_of_library;
	size_t max_filename_length;
} mxp_dl_openbsd_library_args;

static int
mxp_dl_openbsd_get_filename_callback( struct dl_phdr_info *current_info,
					size_t size,
					void *library_to_find_args_ptr )
{
	mxp_dl_openbsd_library_args *library_to_find_args = NULL;
	void *object_to_find_ptr = NULL;
	void *current_object_ptr = NULL;

	library_to_find_args = library_to_find_args_ptr;

	object_to_find_ptr = library_to_find_args->library->object;

	/*---*/

	current_object_ptr = (void *) current_info->dlpi_addr;

	MX_DEBUG(-2,("Comparing '%s' %p to '%s' %p",
		library_to_find_args->library->filename, object_to_find_ptr,
		current_info->dlpi_name, current_object_ptr ));

	/* Does this library match the library that we are looking for? */

	if ( current_object_ptr == object_to_find_ptr ) {
		MX_DEBUG(-2,("Object found!"));

		strlcpy( library_to_find_args->filename_of_library,
			current_info->dlpi_name,
			library_to_find_args->max_filename_length );

		return 1;
	}

	return 0;
}

MX_EXPORT mx_status_type
mx_dynamic_library_get_filename( MX_DYNAMIC_LIBRARY *library,
				char *filename_of_library,
				size_t max_filename_length )
{
	static const char fname[] = "mx_dynamic_library_get_filename()";

	mxp_dl_openbsd_library_args library_args;
	int iterate_status;

	mx_warning( "mx_dynamic_library_get_filename() does not yet "
		"work correctly on OpenBSD." );

	library_args.library = library;
	library_args.filename_of_library = filename_of_library;
	library_args.max_filename_length = max_filename_length;

	iterate_status = dl_iterate_phdr( mxp_dl_openbsd_get_filename_callback,
								&library_args );

	MX_DEBUG(-2,("%s: iterate_status = %d", fname, iterate_status ));

	if ( iterate_status == 0 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The dynamic library '%s' does not seem to be loaded in "
		"the current process.",  library->filename );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

#elif defined(OS_LINUX) || defined(OS_BSD) || defined(OS_HURD) \
	|| defined(OS_SOLARIS)

#define _GNU_SOURCE
#include <link.h>
#include <dlfcn.h>

MX_EXPORT mx_status_type
mx_dynamic_library_get_filename( MX_DYNAMIC_LIBRARY *library,
				char *filename_of_library,
				size_t max_filename_length )
{
	static const char fname[] = "mx_dynamic_library_get_filename()";

	struct link_map link_map_struct;
	int status, saved_errno;

	/* Ask the operating system for information about the library. */

	status = dlinfo( library->object, RTLD_DI_LINKMAP, &link_map_struct );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"dlinfo() for library '%s' failed with errno = %d, "
		"error_message = '%s'.",
		library->filename, saved_errno, strerror(saved_errno) );
	}

	strlcpy( filename_of_library, link_map_struct.l_name,
					max_filename_length );

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(OS_CYGWIN)

MX_EXPORT mx_status_type
mx_dynamic_library_get_filename( MX_DYNAMIC_LIBRARY *library,
				char *filename_of_library,
				size_t max_filename_length )
{
	static const char fname[] = "mx_dynamic_library_get_filename()";

	return mx_error( MXE_UNSUPPORTED, fname,
			"Not supported for this platform." );
}

#else
#error mx_dynamic_library_get_filename() not yet implemented for this target.
#endif

/*----- mx_dynamic_library_show_list() -----*/

#if defined(OS_LINUX) || defined(OS_BSD) || defined(OS_HURD)

#define _GNU_SOURCE
#include <link.h>

static int
mxp_dl_show_list_callback( struct dl_phdr_info *info, size_t size, void *file )
{
	fprintf( file, "  '%s'\n", info->dlpi_name );

	return 0;
}

MX_EXPORT mx_status_type
mx_dynamic_library_show_list( FILE *file )
{
	dl_iterate_phdr( mxp_dl_show_list_callback, file );

	return MX_SUCCESSFUL_RESULT;
}

#undef _GNU_SOURCE

/*--------*/

#elif defined(OS_CYGWIN) || defined(OS_SOLARIS)

MX_EXPORT mx_status_type
mx_dynamic_library_show_list( FILE *file )
{
	fprintf( file, "Warning: mx_dynamic_library_show_list() is not "
		"supported for this platform.\n" );

	return MX_SUCCESSFUL_RESULT;
}

#else
#error mx_dynamic_library_show_list() not yet implemented for this target.
#endif

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
mx_dynamic_library_get_address_from_symbol( MX_DYNAMIC_LIBRARY *library,
				const char *symbol_name,
				void **symbol_pointer,
				unsigned long flags )
{
	static const char fname[] = "mx_dynamic_library_get_address_from_symbol()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Dynamic loading of libraries is not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_dynamic_library_get_symbol_from_address( void *address,
						char *function_name,
						size_t max_name_length )
{
	static const char fname[] =
		"mx_dynamic_library_get_symbol_from_address()";

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
mx_dynamic_library_get_library_and_symbol_address( const char *filename,
					const char *symbol_name,
					MX_DYNAMIC_LIBRARY **library,
					void **symbol,
					unsigned long flags )
{
	MX_DYNAMIC_LIBRARY *library_ptr = NULL;
	void *symbol_ptr = NULL;
	mx_status_type mx_status;

	mx_status = mx_dynamic_library_open( filename, &library_ptr, flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_dynamic_library_get_address_from_symbol( library_ptr,
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

