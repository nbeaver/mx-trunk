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
 * Copyright 2000-2001, 2003-2004, 2006, 2008-2010, 2015, 2017-2021
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "mx_osdef.h"

#include "mx_util.h"
#include "mx_version.h"

/*--------------------------------------------------------------------------*/

#if defined( OS_WIN32 ) && ( _MSC_VER >= 1200 )

/* The following code is inspired by several articles in the Microsoft
 * Systems Journal by Matt Pietrek.  The articles were in the following
 * issues and had the following URLs in March 2009.
 *
 * April 1997 - http://www.microsoft.com/msj/0497/hood/hood0497.aspx
 * May 1997   - http://www.microsoft.com/msj/0597/hood/hood0597.aspx
 * April 1998 - http://www.microsoft.com/msj/0498/bugslayer0498.aspx
 */

#include <windows.h>

#if ( _MSC_VER < 1900 )
#  include <imagehlp.h>
#else
   /* Visual Studio 2015 <imagehlp.h> has warnings due to typedefs that do not
    * actually define a type.  Apparently, Microsoft promises to fix this
    * (Visual Studio bug 1319997) but has not yet done it.  In the meantime,
    * we do the following annoying thing.  (2015-12-03 WML)
    */

#  pragma warning(push)
#  pragma warning(disable : 4091)
#  include <imagehlp.h>
#  pragma warning(pop)
#endif

#include "mx_stdint.h"
#include "mx_unistd.h"

#if (_MSC_VER < 1300)
#  define MX_USE_OLD_IMAGEHLP	TRUE
#else
#  define MX_USE_OLD_IMAGEHLP	FALSE
#endif

#if MX_USE_OLD_IMAGEHLP
# define DWORD64				DWORD
# define IMAGEHLP_LINE64			IMAGEHLP_LINE
# define IMAGEHLP_SYMBOL64			IMAGEHLP_SYMBOL
# define LPSTACKFRAME64				LPSTACKFRAME
# define PDWORD64				PDWORD
# define PFUNCTION_TABLE_ACCESS_ROUTINE64	PFUNCTION_TABLE_ACCESS_ROUTINE
# define PGET_MODULE_BASE_ROUTINE64		PGET_MODULE_BASE_ROUTINE
# define PIMAGEHLP_LINE64			PIMAGEHLP_LINE
# define PIMAGEHLP_SYMBOL64			PIMAGEHLP_SYMBOL
# define PREAD_PROCESS_MEMORY_ROUTINE64		PREAD_PROCESS_MEMORY_ROUTINE
# define PTRANSLATE_ADDRESS_ROUTINE64		PTRANSLATE_ADDRESS_ROUTINE
# define STACKFRAME64				STACKFRAME
#endif

typedef BOOL( __stdcall* MX_STACKWALK_PROC )( DWORD, HANDLE, HANDLE,
					LPSTACKFRAME64, PVOID,
					PREAD_PROCESS_MEMORY_ROUTINE64,
					PFUNCTION_TABLE_ACCESS_ROUTINE64,
					PGET_MODULE_BASE_ROUTINE64,
					PTRANSLATE_ADDRESS_ROUTINE64 );

typedef BOOL ( __stdcall *MX_SYM_CLEANUP_PROC )( HANDLE );

typedef BOOL ( __stdcall *MX_SYM_GET_LINE_FROM_ADDR_PROC )( HANDLE,
					DWORD64, PDWORD, PIMAGEHLP_LINE64 );

typedef BOOL ( __stdcall *MX_SYM_GET_SYM_FROM_ADDR_PROC )( HANDLE,
					DWORD64, PDWORD64, PIMAGEHLP_SYMBOL64 );

typedef BOOL ( __stdcall *MX_SYM_INITIALIZE_PROC )( HANDLE, PSTR, BOOL );

MX_STACKWALK_PROC                _StackWalk = NULL;
MX_SYM_CLEANUP_PROC              _SymCleanup = NULL;
MX_SYM_GET_LINE_FROM_ADDR_PROC   _SymGetLineFromAddr = NULL;
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
#if defined(_WIN64)
	uint64_t module_handle, rva, section_start, section_end;
#else
	DWORD module_handle, rva, section_start, section_end;
#endif
	size_t num_bytes_returned;
	unsigned int i;

	module_name[0] = '\0';
	*section = 0;
	*offset = 0;

	/* Get some information about this memory address. */

	num_bytes_returned = VirtualQuery( address, &mbi, sizeof(mbi) );

	if ( num_bytes_returned == 0 ) {
		return FALSE;
	}

	/* mbi.AllocationBase may be a handle to the module for this address. */

#if defined(_WIN64)
	module_handle = (uint64_t) mbi.AllocationBase;
#else
	module_handle = (DWORD) mbi.AllocationBase;
#endif

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

	if ( module_handle == 0 ) {
		return FALSE;
	}

	/* Point to the DOS header in memory. */

	dos_header = (IMAGE_DOS_HEADER *) module_handle;

	/* Use the DOS header to find the NT (PE) header. */

	nt_header = (IMAGE_NT_HEADERS *)(module_handle + dos_header->e_lfanew);

	/* Use the NT header to find the first section pointer. */

	section_pointer = IMAGE_FIRST_SECTION( nt_header );

	/* RVA is the offset from the module load address.  MSDN calls
	 * it a 'relative virtual address'.
	 */

#if defined(_WIN64)
	rva = (uint64_t) address - module_handle;
#else
	rva = (DWORD) address - module_handle;
#endif

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

#define MXP_STACK_WALK			0
#define MXP_SYM_CLEANUP			1
#define MXP_SYM_FUNCTION_TABLE_ACCESS	2
#define MXP_SYM_GET_LINE_FROM_ADDR	3
#define MXP_SYM_GET_MODULE_BASE		4
#define MXP_SYM_GET_SYM_FROM_ADDR	5
#define MXP_SYM_INITIALIZE		6

#if MX_USE_OLD_IMAGEHLP

static char mxp_name[][30] = {
	"StackWalk",
	"SymCleanup",
	"SymFunctionTableAccess",
	"SymGetLineFromAddr",
	"SymGetModuleBase",
	"SymGetSymFromAddr",
	"SymInitialize"
};

#else /* not MX_USE_OLD_IMAGEHLP */

static char mxp_name[][30] = {
	"StackWalk64",
	"SymCleanup",
	"SymFunctionTableAccess64",
	"SymGetLineFromAddr64",
	"SymGetModuleBase64",
	"SymGetSymFromAddr64",
	"SymInitialize"
};

#endif /* not MX_USE_OLD_IMAGEHLP */

#define MXP_INIT( s, t, n ) \
        do { \
		(s) = (t) GetProcAddress( ih_handle, mxp_name[(n)] );  \
                                                                       \
                if ( (s) == NULL ) {                                   \
                        (void) mx_error( MXE_NOT_FOUND, fname,         \
                        "Could not find function %s", mxp_name[(n)] ); \
                        return FALSE;                                  \
		}                                                      \
        } while (0)

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

	MXP_INIT( _SymInitialize, MX_SYM_INITIALIZE_PROC, MXP_SYM_INITIALIZE );

	MXP_INIT( _SymCleanup, MX_SYM_CLEANUP_PROC, MXP_SYM_CLEANUP ); 

	MXP_INIT( _StackWalk, MX_STACKWALK_PROC, MXP_STACK_WALK ); 

	MXP_INIT( _SymFunctionTableAccess, PFUNCTION_TABLE_ACCESS_ROUTINE64,
					MXP_SYM_FUNCTION_TABLE_ACCESS ); 


	MXP_INIT( _SymGetModuleBase, PGET_MODULE_BASE_ROUTINE64,
					MXP_SYM_GET_MODULE_BASE ); 


	MXP_INIT( _SymGetSymFromAddr, MX_SYM_GET_SYM_FROM_ADDR_PROC,
					MXP_SYM_GET_SYM_FROM_ADDR ); 

	/* SymGetLineFromAddr() was added to later versions of IMAGEHLP.DLL.
	 * Do not abort if it is not found.
	 */

	_SymGetLineFromAddr = (MX_SYM_GET_LINE_FROM_ADDR_PROC) GetProcAddress(
					ih_handle, "SymGetLineFromAddr64" );

	if ( _SymGetLineFromAddr == NULL ) {
		mx_warning(
		"Stack traceback cannot report the source file name "
		"and line number on this computer." );
	}

	/* Initialize the symbol support. */

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

/* WARNING - The code in mx_win32_imagehlp_stack_walk() is fragile.
 *           Given that it examines details of the stack layout and
 #           contents, seemingly minor changes to output formatting
 #           statements can cause the output to stop working correctly.
 #           The use of varargs-style functions like fprintf() and
 #           snprintf() is especially problematic.
 */

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

	IMAGEHLP_LINE64 imagehlp_line64;
	DWORD line_displacement;

#if 1
	DWORD last_error_code;
	TCHAR message_buffer[20];
#endif

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

#if defined(_WIN64)
		fprintf( stderr, "%016I64X  ", stack_frame.AddrPC.Offset );
#else
		fprintf( stderr, "%08X  ", stack_frame.AddrPC.Offset );
#endif

		sym_displacement = 0;

		imagehlp_status = _SymGetSymFromAddr( GetCurrentProcess(),
						stack_frame.AddrPC.Offset,
						&sym_displacement,
						imagehlp_symbol_ptr );

		if ( imagehlp_status ) {

#if defined(_WIN64)
			fprintf( stderr, "%hs+%I64X",
				imagehlp_symbol_ptr->Name,
				sym_displacement );
#else
			fprintf( stderr, "%hs+%X",
				imagehlp_symbol_ptr->Name,
				sym_displacement );
#endif

			if ( _SymGetLineFromAddr != NULL ) {
				imagehlp_line64.SizeOfStruct =
						sizeof(IMAGEHLP_LINE64);

				imagehlp_status = _SymGetLineFromAddr(
						GetCurrentProcess(),
						stack_frame.AddrPC.Offset,
						&line_displacement,
						&imagehlp_line64 );

				if ( imagehlp_status != 0 ) {
					char *ptr, *fn;

					/* Keep only the part of the filename
					 * after the last path separator.
					 */

					fn = imagehlp_line64.FileName;

					ptr = strrchr( fn, '\\' );

					if ( ptr != NULL ) {
						ptr++;
					} else {
						ptr = strrchr( fn, '/' );

						if ( ptr != NULL ) {
							ptr++;
						} else {
							ptr = fn;
						}
					}

					fprintf( stderr, "   (%s, line %ld)",
					    ptr, imagehlp_line64.LineNumber );
				}
			}

			fprintf( stderr, "\n" );

		} else {
			last_error_code = GetLastError();

			if ( last_error_code == ERROR_INVALID_ADDRESS )
			{
				fprintf( stderr, "- Invalid address\n" );

				continue;
			}

			module_name[0] = '\0';
			section = 0; offset = 0;

			mx_win32_get_logical_address(
					(void *) stack_frame.AddrPC.Offset,
					module_name, sizeof(module_name),
					&section, &offset );

#if 0
			fprintf( stderr, "%04:%08X %s\n",
				section, offset, module_name );
#else
			fprintf( stderr, "%04:", section );
			fprintf( stderr, "%08X ", offset );
			fprintf( stderr, "%s\n", module_name );
#endif
		}
	}
}

/*--------*/

#if defined(_M_IX86)

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

		ptr_status = mx_win32_get_logical_address(
					(void *) program_counter,
					module_name, sizeof(module_name),
					&section, &offset );

		mx_info( "%08X  %08X  %04X:%08X %s",
			program_counter, frame_pointer, section, offset,
			module_name );

		if ( ptr_status == FALSE ) {
			return;
		}

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

#endif /* defined(_M_IX86) */

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

	EXCEPTION_POINTERS *einfo;
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

	if ( _RtlCaptureContext != NULL ) {
		_RtlCaptureContext( &context );

	} else {
		/* If RtlCaptureContext() is not supported here, then we can
		 * get the context by intentionally generating an exception
		 * and then copying the CONTEXT information from the
		 * EXCEPTION_POINTERS structure returned by the macro
		 * GetExceptionInformation().
		 */

		__try {
			int bad = 0;
			bad = 1/bad;
		}
		__except( (einfo = GetExceptionInformation())
		    ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_EXECUTE_HANDLER )
		{
			if ( einfo != NULL ) {
				memcpy( &context, einfo->ContextRecord,
						sizeof(context) );
			}
		}
	}

	mx_win32_stack_walk( &context );
}

#else
#  error Unrecognized Win32 platform.
#endif

/*--------------------------------------------------------------------------*/

#elif defined( OS_SOLARIS )

#include <unistd.h>

/* Solaris 2 has a stack traceback program called 'pstack' that uses the
 * /proc filesystem to do the traceback.
 */

MX_EXPORT void
mx_stack_traceback( void )
{
	FILE *file;
	static char buffer[200];

	snprintf( buffer, sizeof(buffer),
		"/usr/proc/bin/pstack %ld", (long) getpid() );

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

#include "mx_signal.h"

MX_EXPORT void
mx_stack_traceback( void )
{
	__djgpp_traceback_exit( SIGABRT );
}

/*--------------------------------------------------------------------------*/

#elif ( ( defined(MX_DARWIN_VERSION) && (MX_DARWIN_VERSION >= 9000000L) ) \
     || (MX_GLIBC_VERSION >= 2001000L) )

/* GNU Glibc 2.1 introduces the backtrace() family of functions.
 *
 * MacOS X 10.5 and above has it too.
 */

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

	mx_info( "Stack traceback:\n" );

	for ( i = 0; i < num_addresses; i++ ) {
		mx_info( "%d: %s", i, names[i] );
	}

	if ( num_addresses < MAXDEPTH ) {
		mx_info( "Stack traceback complete." );
	} else {
		mx_info(
		"Stack traceback exceeded the maximum level of %d.",
			MAXDEPTH );
	}

	free( names );

	return;
}

/*--------------------------------------------------------------------------*/

#elif ( (defined(MX_GCC_VERSION) || defined(MX_CLANG_VERSION)) \
		&& !defined(__FreeBSD__) && !defined(__minix__) )

/* FIXME: It may be necessary to use -funwind-tables on ARM to get stack traces.
 */

#include <unwind.h>

static _Unwind_Reason_Code mxp_stack_traceback_fn( struct _Unwind_Context *ctx,
								void *d )
{
	int *depth = (int *) d;
	fprintf( stderr, "#%d: program counter at %#lx\n",
			*depth, _Unwind_GetIP(ctx) );
	(*depth)++;
	return _URC_NO_REASON;
}

MX_EXPORT void
mx_stack_traceback( void )
{
	int depth = 0;

	fprintf( stderr, "\nStack traceback:\n\n" );

	_Unwind_Backtrace( &mxp_stack_traceback_fn, &depth );
}

/*--------------------------------------------------------------------------*/

#else

MX_EXPORT void
mx_stack_traceback( void )
{
	mx_info( "Stack traceback is not available." );

	return;
}

#endif

/*--------------------------------------------------------------------------*/

#if defined(OS_WIN32) && defined(_M_AMD64)

/* Warning: mxp_stack_available_cs and mxp_stack_available_cs_initialized
 * are set up in mx_initialize_runtime().  If you call the following functions
 * without having called mx_initialize_runtime() first before any other threads
 * are created, then the following implementations of mx_stack_available()
 * and mx_stack_check() will not be thread safe.
 */

static CRITICAL_SECTION mxp_stack_available_cs;

static int mxp_stack_available_cs_initialized = FALSE;

/* Inspired by https://stackoverflow.com/questions/12905316/how-can-i-find-out-how-much-space-is-left-on-stack-of-the-current-thread-using-v
 */

MX_EXPORT long
mx_stack_available( void )
{
	long stack_available = -1L;

	/* Create an mbi object on the stack. */

	MEMORY_BASIC_INFORMATION mbi;

	/*---*/
	if ( mxp_stack_available_cs_initialized == FALSE ) {
		InitializeCriticalSection( &mxp_stack_available_cs );
		mxp_stack_available_cs_initialized = TRUE;
	}

	EnterCriticalSection( &mxp_stack_available_cs );

	/* Get the page range that includes the pointer to this object
	 * on the stack.
	 */

	VirtualQuery( (PVOID) &mbi, &mbi, sizeof(mbi) );

	/* The stack grows down on Windows. */

	stack_available = (UINT_PTR) &mbi - (UINT_PTR) mbi.AllocationBase;

	LeaveCriticalSection( &mxp_stack_available_cs );

	return stack_available;
}

MX_EXPORT int
mx_stack_check( void )
{
	if ( mx_stack_available() > 0 ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

#else

MX_EXPORT long
mx_stack_available( void )
{
	return (-1L);
}

MX_EXPORT int
mx_stack_check( void )
{
	return TRUE;
}

#endif

/*--------------------------------------------------------------------------*/

static const void *mxp_stack_base_ptr = NULL;

static mx_bool_type mxp_stack_grows_up = FALSE;

/* Note: We must call mx_initialize_stack_calc() with the address of
 * a variable that is actually on the stack!  For example, passing the
 * address of something like
 *
 *     static const char fname[] = "main()";
 *
 * will _NOT_ work, since fname is declared 'static', which means that
 * it is not on the stack.
 */

MX_EXPORT void
mx_initialize_stack_calc( const void *stack_base )
{
	int stackvar1;
	int stackvar2;

	mxp_stack_base_ptr = stack_base;

	/* FIXME: The following test probably does not work on all
	 * conceivable targets.  And even if it works within a given
	 * stack frame, it may not work between the stack frames of
	 * a calling function and the called function.  For example,
	 * S/390 mainframes do not put return addresses on a stack.
	 * So don't try to push the validity of this function too far.
	 */

	if ( ( (ptrdiff_t) &stackvar2 ) < ( (ptrdiff_t) &stackvar1 ) ) {
		mxp_stack_grows_up = FALSE;
	} else {
		mxp_stack_grows_up = TRUE;
	}
}

MX_EXPORT const void *
mx_get_stack_base( void )
{
	return mxp_stack_base_ptr;
}

MX_EXPORT int
mx_stack_grows_up( void )
{
	return mxp_stack_grows_up;
}

/* mx_get_stack_offset() computes the offset of an address on the stack
 * relative to a specified 'stack_base'.  If 'stack_base' is NULL, then
 * we perform the offset relative to mxp_stack_base_ptr, which is assumed
 * to have been set "correctly" at program startup time.  If it was not
 * set up correctly, then the value reported is almost certainly garbage.
 *
 * This function also uses mx_stack_grows_up() to try and make the reported
 * offset a positive integer.  If we get a negative integer, then either
 * mx_stack_grows_up() is implemented incorrectly on this architecture or
 * we have set mxp_stack_base_ptr incorrectly or somehow we have popped
 * more stuff off of the stack than was initially pushed to it!
 */

/* Note: If we have just overflowed the stack and then try to call the function
 * mx_get_stack_offset(), the C runtime library or the operating system itself
 * may intervene to interrupt the program before we can actually enter
 * mx_get_stack_offset() itself.  In that case, we suggest making a call to
 * mx_get_stack_offset() in the caller to the current routine, rather than
 * the current routine itself.
 */

MX_EXPORT long
mx_get_stack_offset( const void *stack_base, const void *stack_address )
{
	ptrdiff_t offset;

	if ( stack_base == (void *) NULL ) {
		if ( mxp_stack_base_ptr == (void *) NULL ) {
			(void) mx_error( MXE_INITIALIZATION_ERROR, "",
			"Stack offset reporting has not yet been set up "
			"for this program." );

			return (-1L);
		} else {
			stack_base = mxp_stack_base_ptr;
		}
	}

	if ( mxp_stack_grows_up ) {
		offset = (ptrdiff_t) stack_address - (ptrdiff_t) stack_base;
	} else {
		offset = (ptrdiff_t) stack_base - (ptrdiff_t) stack_address;
	}

	return (long) offset;
}

