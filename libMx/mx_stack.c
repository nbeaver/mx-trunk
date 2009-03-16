/*
 * Name:    mx_stack.c
 *
 * Purpose: MX functions to support run-time stack tracebacks.
 *
 *          This functionality is not available on all platforms.  Even if
 *          it is, it may not work reliably if the program is in the process
 *          of crashing when this function is invoked.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2004, 2006, 2008-2009
 *      Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"

#include "mx_util.h"
#include "mx_version.h"

/*--------------------------------------------------------------------------*/

#if defined( OS_WIN32 )

/* The following code is inspired by several articles in the Microsoft
 * Systems Journal by Matt Pietrek.  The articles were in the following
 * issues and had the following URLs in March 2009.
 *
 * April 1997 - http://www.microsoft.com/msj/0497/hood/hood0497.aspx
 * May 1997   - http://www.microsoft.com/msj/0597/hood/hood0597.aspx
 * April 1998 - http://www.microsoft.com/msj/0498/bugslayer0498.aspx
 */

#include <windows.h>
#include <imagehlp.h>

#include "mx_stdint.h"
#include "mx_unistd.h"

typedef BOOL( __stdcall* MX_STACKWALK_PROC )( DWORD, HANDLE, HANDLE,
					LPSTACKFRAME64, PVOID,
					PREAD_PROCESS_MEMORY_ROUTINE64,
					PFUNCTION_TABLE_ACCESS_ROUTINE64,
					PGET_MODULE_BASE_ROUTINE64,
					PTRANSLATE_ADDRESS_ROUTINE64 );

typedef BOOL ( __stdcall *MX_SYM_CLEANUP_PROC )( HANDLE );

typedef BOOL ( __stdcall *MX_SYM_GET_SYM_FROM_ADDR_PROC )( HANDLE,
					DWORD64, PDWORD64, PIMAGEHLP_SYMBOL64 );

typedef BOOL ( __stdcall *MX_SYM_INITIALIZE_PROC )( HANDLE, PSTR, BOOL );

MX_STACKWALK_PROC                _StackWalk = NULL;
MX_SYM_CLEANUP_PROC              _SymCleanup = NULL;
MX_SYM_GET_SYM_FROM_ADDR_PROC    _SymGetSymFromAddr = NULL;
MX_SYM_INITIALIZE_PROC           _SymInitialize = NULL;

PFUNCTION_TABLE_ACCESS_ROUTINE64 _SymFunctionTableAccess = NULL;
PGET_MODULE_BASE_ROUTINE64       _SymGetModuleBase = NULL;

/*--------*/

static mx_bool_type
mx_win32_get_logical_address( void *address,
			char *module_name,
			size_t module_name_length,
			DWORD *section,
			DWORD *offset )
{
	MEMORY_BASIC_INFORMATION mbi;
	IMAGE_DOS_HEADER *dos_header;
	IMAGE_NT_HEADERS *nt_header;
	IMAGE_SECTION_HEADER *section_pointer;
	DWORD module_handle, rva, section_start, section_end;
	size_t num_bytes_returned;
	unsigned int i;

	/* Get some information about this memory address. */

	num_bytes_returned = VirtualQuery( address, &mbi, sizeof(mbi) );

	/* mbi.AllocationBase may be a handle to the module for this address. */

	module_handle = (DWORD) mbi.AllocationBase;

	/* Get the name of the module corresponding to this supposed handle. */

	num_bytes_returned = GetModuleFileName( (HMODULE) module_handle,
						module_name,
						module_name_length );

	/* If GetModuleFileName() returned a zero length filename, then
	 * this handle isn't really for a module.
	 */

	if ( num_bytes_returned == 0 ) {
		return FALSE;
	}

	/* Point to the DOS header in memory. */

	dos_header = (IMAGE_DOS_HEADER *) module_handle;

	/* Use the DOS header to find the NT (PE) header. */

	nt_header = (IMAGE_NT_HEADERS *)(module_handle + dos_header->e_lfanew);

	/* Use the NT header to find the first section pointer. */

	section_pointer = IMAGE_FIRST_SECTION( nt_header );

	/* RVA is the offset from the module load address. */

	rva = (DWORD) address - module_handle;

	/* Loop through the section table, looking for the section that
	 * contains this address.
	 */

	for ( i = 0;
		i < (nt_header->FileHeader.NumberOfSections);
		i++, section_pointer++ )
	{
		section_start = section_pointer->VirtualAddress;

		section_end = section_start +
			max( section_pointer->SizeOfRawData,
				section_pointer->Misc.VirtualSize );

		/* Is the address in this section? */

		if ( (rva >= section_start) && (rva <= section_end) ) {
			/* Yes it is, so compute the section and offset
			 * parameters for our caller.
			 */

			*section = i+1;
			*offset = rva - section_start;

			return TRUE;
		}
	}

	/* We should not get here. */

	return FALSE;
}

/*--------*/

static mx_bool_type
mx_win32_initialize_imagehlp( void )
{
	static const char fname[] = "mx_win32_initialize_imagehlp()";

	HMODULE ih_handle = LoadLibrary( "IMAGEHLP.DLL" );
	BOOL win32_status;

	if ( ih_handle == 0 ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
			"Could not load IMAGEHLP.DLL" );
		return FALSE;
	}

	_SymInitialize = (MX_SYM_INITIALIZE_PROC) GetProcAddress( ih_handle,
							"SymInitialize" );

	if ( _SymInitialize == NULL ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
			"Could not find function SymInitialize64()." );
		return FALSE;
	}

	_SymCleanup = (MX_SYM_CLEANUP_PROC) GetProcAddress( ih_handle,
							"SymCleanup" );

	if ( _SymCleanup == NULL ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
			"Could not find function SymCleanup64()." );
		return FALSE;
	}

	_StackWalk = (MX_STACKWALK_PROC) GetProcAddress( ih_handle,
							"StackWalk64" );

	if ( _StackWalk == NULL ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
			"Could not find function StackWalk64()." );
		return FALSE;
	}

	_SymFunctionTableAccess =
		(PFUNCTION_TABLE_ACCESS_ROUTINE64) GetProcAddress(
			ih_handle, "SymFunctionTableAccess64" );

	if ( _SymFunctionTableAccess == NULL ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
			"Could not find function SymFunctionTableAccess64()." );
		return FALSE;
	}

	_SymGetModuleBase = (PGET_MODULE_BASE_ROUTINE64) GetProcAddress(
					ih_handle, "SymGetModuleBase64" );

	if ( _SymGetModuleBase == NULL ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
			"Could not find function SymGetModuleBase64()." );
		return FALSE;
	}

	_SymGetSymFromAddr = (MX_SYM_GET_SYM_FROM_ADDR_PROC) GetProcAddress(
					ih_handle, "SymGetSymFromAddr64" );

	if ( _SymGetSymFromAddr == NULL ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
			"Could not find function SymGetSymFromAddr64()." );
		return FALSE;
	}

	win32_status = _SymInitialize( GetCurrentProcess(), 0, TRUE );

	if ( win32_status == FALSE ) {
		DWORD error_code = GetLastError();
		char message[80];

		mx_win32_error_message( error_code, message, sizeof(message) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"SymInitialize() failed with Win32 error code %ld, "
			"error message = '%s'", error_code, message );

		return FALSE;
	}

	return TRUE;
}

/*--------*/

#define MX_IMAGEHLP_MAX_NAME_LENGTH	512

static void
mx_win32_imagehlp_stack_walk( CONTEXT *context )
{
	/* IMAGEHLP requires a pointer to an IMAGEHLP_SYMBOL structure
	 * that we call 'imagehlp_symbol_ptr'.  At run time, we arrange
	 * for 'imagehlp_symbol_ptr' to point to 'symbol_buffer'.
	 */

	BYTE symbol_buffer[ sizeof(IMAGEHLP_SYMBOL64)
				+ MX_IMAGEHLP_MAX_NAME_LENGTH ];
	PIMAGEHLP_SYMBOL64 imagehlp_symbol_ptr;
	DWORD64 sym_displacement;

	STACKFRAME64 stack_frame;
	BOOL stackwalk_status, imagehlp_status;
	DWORD machine_type;

	char module_name[MAX_PATH];
	DWORD section, offset;

	memset( &stack_frame, 0, sizeof(stack_frame) );

	/* Initialize the STACKFRAME structure for the first call. */

	stack_frame.AddrPC.Mode      = AddrModeFlat;
	stack_frame.AddrStack.Mode   = AddrModeFlat;
	stack_frame.AddrFrame.Mode   = AddrModeFlat;

#if defined(_M_IX86)
	machine_type = IMAGE_FILE_MACHINE_I386;

	stack_frame.AddrPC.Offset    = context->Eip;
	stack_frame.AddrStack.Offset = context->Esp;
	stack_frame.AddrFrame.Offset = context->Ebp;

#elif defined(_M_AMD64)
	machine_type = IMAGE_FILE_MACHINE_AMD64;

	stack_frame.AddrPC.Offset    = context->Rip;
	stack_frame.AddrStack.Offset = context->Rsp;

	/* FIXME - Apparently VC2005B2 uses Rdi instead of Rbp? */
	stack_frame.AddrFrame.Offset = context->Rbp;

#elif defined(_M_IA64)
	machine_type = IMAGE_FILE_MACHINE_IA64;

	stack_frame.AddrPC.Offset    = context->StIIP;
	stack_frame.AddrStack.Offset = context->IntSP;
	stack_frame.AddrFrame.Offset = context->RsBSP;
	stack_frame.AddrBStore.Offset = context->RsBSP;
	stack_frame.AddrBStore.Mode   = AddrModeFlat;

#else
#  error Unrecognized Win32 machine type.
#endif

	while (1) {
		stackwalk_status = _StackWalk( machine_type,
						GetCurrentProcess(),
						GetCurrentThread(),
						&stack_frame,
						context,
						0,
						_SymFunctionTableAccess,
						_SymGetModuleBase,
						0 );

		if ( stackwalk_status == 0 ) {
			return;
		}

		/* If the frame offset is 0, then this is not a valid
		 * stack frame.
		 */

		if ( stack_frame.AddrFrame.Offset == 0 ) {
			return;
		}

		/* Initialize some variables before getting the symbol. */ 

		symbol_buffer[0] = '\0';

		imagehlp_symbol_ptr = (PIMAGEHLP_SYMBOL64) symbol_buffer;

		imagehlp_symbol_ptr->SizeOfStruct = sizeof(symbol_buffer);
		imagehlp_symbol_ptr->MaxNameLength =
						MX_IMAGEHLP_MAX_NAME_LENGTH;

		/* sym_displacement is the offset of the input address,
		 * relative to the start of the symbol.
		 */

		sym_displacement = 0;

		imagehlp_status = _SymGetSymFromAddr( GetCurrentProcess(),
						stack_frame.AddrPC.Offset,
						&sym_displacement,
						imagehlp_symbol_ptr );

		if ( imagehlp_status ) {
			mx_info( "%hs+%X",
				imagehlp_symbol_ptr->Name,
				sym_displacement );
		} else {
			module_name[0] = '\0';
			section = 0; offset = 0;

			mx_win32_get_logical_address(
					(void *) stack_frame.AddrPC.Offset,
					module_name, sizeof(module_name),
					&section, &offset );

		}
	}
}

/*--------*/

static void
mx_win32_x86_stack_walk( CONTEXT *context )
{
	DWORD program_counter = context->Eip;
	DWORD *frame_pointer, *previous_frame_pointer;
	char module_name[MAX_PATH];
	DWORD section, offset;
	mx_bool_type ptr_status;

	frame_pointer = (DWORD *) context->Ebp;

	do {
		module_name[0] = '\0';

		mx_win32_get_logical_address( (void *) program_counter,
					module_name, sizeof(module_name),
					&section, &offset );

		mx_info( "%08X  %08X  %04X:%08X %s",
			program_counter, frame_pointer, section, offset,
			module_name );

		program_counter = frame_pointer[1];

		previous_frame_pointer = frame_pointer;

		/* Go to the next stack frame. */

		frame_pointer = (DWORD *)( frame_pointer[0] );

		/* Frame pointers must be aligned on a DWORD boundary.
		 * If this is not so, then return.
		 */

		if ( ((DWORD) frame_pointer & 0x3 ) != 0 ) {
			return;
		}

		if ( frame_pointer <= previous_frame_pointer ) {
			return;
		}

		/* Is the frame pointer address readable? */

		ptr_status = mx_pointer_is_valid( frame_pointer,
						2 * sizeof(void *), R_OK );

		if ( ptr_status == FALSE ) {
			return;
		}
	} while (1);
}

/*--------*/

static void
mx_win32_stack_walk( CONTEXT *context )
{
	mx_bool_type init_status;

	if ( _SymInitialize == NULL ) {
		init_status = mx_win32_initialize_imagehlp();
	} else {
		init_status = TRUE;
	}

	if ( init_status == FALSE ) {

#if defined(_M_IX86)
		mx_warning( "Cannot access IMAGEHLP.DLL functions.  "
		"Using x86 specific traceback." );

		mx_win32_x86_stack_walk( context );
#else
		mx_warning( "Cannot access IMAGEHLP.DLL functions, so "
		"no stack traceback will be provided." );
#endif
		return;
	}

	mx_win32_imagehlp_stack_walk( context );
}

/*--------*/

#if defined(_M_AMD64) || defined(_M_IA64)

MX_EXPORT void
mx_stack_traceback( void )
{
	CONTEXT context;

	mx_info( "\nStack Traceback:\n" );

	RtlCaptureContext( &context );

	mx_win32_stack_walk( &context );
}

#elif defined(_M_IX86)

typedef VOID ( WINAPI *MX_RTL_CAPTURE_CONTEXT_PROC )( PCONTEXT );

MX_RTL_CAPTURE_CONTEXT_PROC _RtlCaptureContext = NULL;

MX_EXPORT void
mx_stack_traceback( void )
{
	static const char fname[] = "mx_stack_traceback()";

	CONTEXT context;

	mx_info( "\nStack Traceback:\n" );

	if ( _RtlCaptureContext == NULL ) {
		HMODULE kernel32_handle = GetModuleHandle("KERNEL32.DLL");

		if ( kernel32_handle == NULL ) {
			mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"GetModuleHandle('KERNEL32.DLL') failed!  "
			"This should never, ever happen." );

			return;
		}

		_RtlCaptureContext = (MX_RTL_CAPTURE_CONTEXT_PROC)
			GetProcAddress( kernel32_handle, "RtlCaptureContext" );
	}

	_RtlCaptureContext( &context );

	mx_win32_stack_walk( &context );
}

#else
#  error Unrecognized Win32 platform.
#endif

/*--------------------------------------------------------------------------*/

#elif defined( OS_SOLARIS )

#include <unistd.h>

/* Solaris 2 has a stack traceback program called 'ptrace' that uses the
 * /proc filesystem to do the traceback.
 */

MX_EXPORT void
mx_stack_traceback( void )
{
	FILE *file;
	static char buffer[200];

	sprintf( buffer, "/usr/proc/bin/pstack %ld", (long) getpid() );

	file = popen( buffer, "r" );

	if ( file == NULL ) {
		mx_info(
    "Unable to run /usr/proc/bin/pstack to get stack traceback information" );
		return;
	}

	fgets( buffer, sizeof buffer, file );

	while ( !feof( file ) ) {

		mx_info( buffer );

		fgets( buffer, sizeof buffer, file );
	}

	pclose(file);

	return;
}

/*--------------------------------------------------------------------------*/

#elif defined( OS_IRIX )

#   if !defined(__GNUC__)

    /* trace_back_stack_and_print() seems to work right for code compiled
     * with SGI's own C compiler, but not for GCC.
     */

#   include <libexc.h>

    MX_EXPORT void
    mx_stack_traceback( void )
    {
	(void) trace_back_stack_and_print();
    }
#   else /* __GNUC__ */

    MX_EXPORT void
    mx_stack_traceback( void )
    {
	mx_info("Stack traceback is not available on Irix "
		"for code compiled with GCC.");
    }
#   endif /* __GNUC__ */

/*--------------------------------------------------------------------------*/

#elif defined( OS_HPUX )

/* The undocumented function U_STACK_TRACE() does the trick on HP/UX.
 * U_STACK_TRACE() is found in libcl and requires adding -lcl to all
 * link commands.
 */

extern void U_STACK_TRACE(void);

MX_EXPORT void
mx_stack_traceback( void )
{
	U_STACK_TRACE();
}

/*--------------------------------------------------------------------------*/

#elif defined( OS_DJGPP )

#include <signal.h>

MX_EXPORT void
mx_stack_traceback( void )
{
	__djgpp_traceback_exit( SIGABRT );
}

/*--------------------------------------------------------------------------*/

#elif ( MX_GLIBC_VERSION >= 2001000L )

/* GNU Glibc 2.1 introduces the backtrace() family of functions. */

/* MAXDEPTH is the maximum number of stack frames that will be dumped. */

#define MAXDEPTH	100

#include <execinfo.h>

MX_EXPORT void
mx_stack_traceback( void )
{
	static void *addresses[ MAXDEPTH ];
	int i, num_addresses;
	char **names;

	num_addresses = backtrace( addresses, MAXDEPTH );

	if ( num_addresses <= 0 ) {
		mx_info(
	"\nbacktrace() failed to return any stack traceback information." );

		return;
	}

	names = backtrace_symbols( addresses, num_addresses );

	mx_info( "\nStack traceback:\n" );

	for ( i = 0; i < num_addresses; i++ ) {
		mx_info( "%d: %s", i, names[i] );
	}

	if ( num_addresses < MAXDEPTH ) {
		mx_info( "\nStack traceback complete." );
	} else {
		mx_info(
		"\nStack traceback exceeded the maximum level of %d.",
			MAXDEPTH );
	}

	free( names );

	return;
}

#else

/*--------------------------------------------------------------------------*/

MX_EXPORT void
mx_stack_traceback( void )
{
	mx_info( "Stack traceback is not available." );

	return;
}

#endif

/*--------------------------------------------------------------------------*/

#if 1

MX_EXPORT int
mx_stack_check( void )
{
	return TRUE;
}

#endif

