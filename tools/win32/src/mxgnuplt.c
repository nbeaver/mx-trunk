/*
 * Name:    mxgnuplt.c 
 *
 * Purpose: This program is a shim that takes characters on standard input
 *          and redirects to the Windows port of Gnuplot called "wgnuplot".
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#if (_MSC_VER >= 1400)
#pragma warning( disable:4996 )
#endif

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <process.h>

struct update_struct_t {
	DWORD process_id;
	HWND hwnd_top_level;
};

struct enum_struct_t {
	DWORD process_id;
	char *window_class;
	HWND  hwnd;
};

HWND  global_hwnd_graph;

#define WGNUPLOT_NAME "wgnuplot"

void  update_plot_window( void *void_hwnd_top_level );
DWORD start_gnuplot( char *pathname, size_t max_length );
void  find_gnuplot( char *pathname, size_t max_length );
void  display_create_process_error( char *wgnuplot_name );
HWND  find_window_by_processid( DWORD process_id, char *window_class );

int
main( int argc, char *argv[] )
{
	struct update_struct_t update_struct;
	HWND hwnd_text, hwnd_top_level;
	DWORD process_id;
	LPARAM lParam;
	LRESULT status;
	char class_name[50];
	char buffer[200];
	char wgnuplot_name[200];
	int i, c, length, do_exit;
	int plotting_window_initialized;

	plotting_window_initialized = FALSE;
	global_hwnd_graph = NULL;

	/* Set standard input to be line buffered. */

	setvbuf( stdin, (char *)NULL, _IOLBF, 0 );

	/* Start the 'wgnuplot' program. */

	process_id = start_gnuplot( wgnuplot_name,
				sizeof(wgnuplot_name) );

	sprintf( buffer, "Process ID = %ld", process_id );

	/* Get a handle to the top level Gnuplot window.
	 * If we are lucky, it is the right one.
	 */

	for ( i = 0; i < 100; i++ ) {

		hwnd_top_level = find_window_by_processid(
				process_id, "wgnuplot_parent" );

		if ( hwnd_top_level != NULL ) {
			break;
		}

		Sleep(100);
	}

	if ( hwnd_top_level == NULL ) {
		MessageBox( NULL,
		(LPTSTR) "Unable to find 'wgnuplot' top level window.",
		    NULL, MB_OK );
		exit(1);
	}

	/* Find the wgnuplot text window. */

	hwnd_text = FindWindowEx( hwnd_top_level, (HWND) NULL,
			(LPCTSTR) "wgnuplot_text", (LPCTSTR) "gnuplot" );

	if ( hwnd_text == NULL ) {
		MessageBox( NULL,
		(LPTSTR) "Unable to find 'wgnuplot' text input window.",
		    NULL, MB_OK );
		exit(1);
	}

	/* Start sending commands to the wgnuplot text window. */

	do_exit = FALSE;

	fgets( buffer, sizeof buffer, stdin );

	while ( !feof(stdin) && !ferror(stdin) ) {

		if ( strcmp( buffer, "exit\n" ) == 0 ) {
			do_exit = TRUE;
		}

		if ( plotting_window_initialized == FALSE ) {
			if ( strncmp( buffer, "plot", 4 ) == 0 ) {

				update_struct.process_id = process_id;
				update_struct.hwnd_top_level
					= hwnd_top_level;

				_beginthread( update_plot_window,
					0, (void *) &update_struct );

				plotting_window_initialized = TRUE;
			}
		}

		length = strlen( buffer );

		for ( i = 0; i < length; i++ ) {
			c = buffer[i];

			status = SendMessage( hwnd_text, WM_CHAR, (long)c, 1L );

			if ( status != 0 ) {
				sprintf( buffer,
	"mxgnuplt: Error sending char '%c' to Gnuplot.  status = %ld\n",
					c, status );
				MessageBox( NULL, (LPTSTR) buffer,
						NULL, MB_OK );
			}
		}

		if ( do_exit ) {
			/* If Gnuplot has been sent an 'exit' command,
			 * sending the graph window a mouse move message
			 * will cause Gnuplot to process the 'exit'
			 * command and terminate.
			 */

			if ( global_hwnd_graph != NULL ) {
				lParam = ( 5 << 16 ) | 5;

				PostMessage( global_hwnd_graph,
					WM_MOUSEMOVE,
					(WPARAM)0, lParam );
			}
			exit(0);
		}
		Sleep(10);

		fgets( buffer, sizeof buffer, stdin );
	}
	MessageBox( NULL, (LPTSTR) "mxgnuplt exiting abnormally.",
			NULL, MB_OK );

	exit(1);

	/* Suppress the Borland compiler's 'Function should return a value...'
	 * message.
	 */

	return 1;
}

void
update_plot_window( void *update_struct_ptr )
{
	struct update_struct_t *update_struct;
	HWND hwnd_graph, hwnd_top_level;
	DWORD process_id;
	LPARAM lParam;

	update_struct = ( struct update_struct_t * ) update_struct_ptr;

	process_id = update_struct->process_id;
	hwnd_top_level = update_struct->hwnd_top_level;

	/* The top level window _must_ be visible for Gnuplot
	 * to create the graph window.
	 */

	ShowWindow( hwnd_top_level, SW_SHOWNORMAL );

	/* Loop waiting for the Gnuplot graph window to show up. */

	while (1) {
		hwnd_graph = find_window_by_processid(
				process_id, "wgnuplot_graph" );

		if ( hwnd_graph != NULL ) {
			ShowWindow( hwnd_top_level, SW_MINIMIZE );

			break;		/* exit the while() loop */
		}

		Sleep( 100 );
	}

	/* Put a copy of the handle where the main thread can find it. */

	global_hwnd_graph = hwnd_graph;

	/* Send WM_MOUSEMOVE messages to the
	 * Gnuplot graph window.
	 */

	while (1) {
		lParam = ( 5 << 16 ) | 5;

		PostMessage( hwnd_graph, WM_MOUSEMOVE,
					(WPARAM)0, lParam );

		Sleep( 100 );
	}
	return;
}

DWORD
start_gnuplot( char *wgnuplot_name, size_t max_length )
{
	BOOL status;
	DWORD CreationFlags;
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInformation;
	
	/* Look for 'wgnuplot' in the PATH. */

	strncpy( wgnuplot_name, WGNUPLOT_NAME, max_length );

	wgnuplot_name[max_length - 1] = '\0';

	/* Create the Gnuplot process. */

	CreationFlags = 0;

	memset( &StartupInfo, 0, sizeof(StartupInfo) );
	StartupInfo.cb = sizeof(StartupInfo);

	StartupInfo.dwFlags = ( STARTF_USESHOWWINDOW
				| STARTF_FORCEOFFFEEDBACK );

	StartupInfo.wShowWindow = SW_MINIMIZE;

	status = CreateProcess( NULL,
				(LPTSTR) wgnuplot_name,
				NULL,
				NULL,
				TRUE,
				CreationFlags,
				NULL,
				NULL,
				&StartupInfo,
				&ProcessInformation );
				
	/* Was there an error while creating the process? */

	if ( status == 0 ) {
		display_create_process_error( wgnuplot_name );
		exit(1);
	}

	/* Return the process id. */

	return ( ProcessInformation.dwProcessId );
}

void
display_create_process_error( char *wgnuplot_name )
{
	char *system_buffer;
	char buffer[200];
	size_t length;

	FormatMessage(
	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &system_buffer, 0, NULL );

	length = strlen( system_buffer );

	if ( length > 0 ) {
	    /* The message may be terminated with ., CR, LF.
	     * Delete those characters.
	     */
	    if ( system_buffer[ length - 1 ] == 0xA ) {
	        system_buffer[ length - 1 ] = '\0';

		if ( system_buffer[ length - 2 ] == 0xD ) {
		    system_buffer[ length - 2 ] = '\0';

		    if ( system_buffer[ length - 3 ] == '.' ) {
		        system_buffer[ length - 3 ] = '\0';
		    }
		}
	    }
	}

	sprintf( buffer,
	    "mxgnuplt: Could not start the program '%s'.\n"
	    "Reason is '%s'.", wgnuplot_name, system_buffer );

	MessageBox( NULL, (LPCTSTR) buffer,
		(LPCTSTR) "Cannot Start Gnuplot",
		( MB_OK | MB_ICONWARNING ) );

	LocalFree( system_buffer );
}

BOOL CALLBACK
find_window_enum_proc( HWND hwnd, LPARAM lParam )
{
	struct enum_struct_t *enum_struct;
	DWORD window_process_id;
	char class_name[80];
	int status;

	enum_struct = (struct enum_struct_t *) lParam;

	/* Get this window's process id. */

	(void) GetWindowThreadProcessId( hwnd, &window_process_id );

	if ( window_process_id != enum_struct->process_id ) {
		return TRUE;
	}

	/* This is the correct process.
	 * Is it the correct window class?
	 */

	status = GetClassName( hwnd,
			(LPTSTR) class_name, sizeof(class_name) );

	if ( status == 0 ) {
		MessageBox( NULL, (LPTSTR) "Could not get class name",
			(LPTSTR) "Error in find_window_enum_proc",
			MB_OK );

		return TRUE;
	}

	if ( strcmp( class_name, enum_struct->window_class ) != 0 ) {
		return TRUE;
	} else {
		enum_struct->hwnd = hwnd;

		return FALSE;
	}
}

HWND
find_window_by_processid( DWORD process_id, char *window_class )
{
	struct enum_struct_t enum_struct;
	BOOL result;

	enum_struct.process_id = process_id;
	enum_struct.window_class = window_class;
	enum_struct.hwnd = NULL;

	EnumWindows( find_window_enum_proc, (LPARAM) &enum_struct );

	return enum_struct.hwnd;
}

