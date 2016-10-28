/*
 * Name:    mx_epics.c
 *
 * Purpose: Some functions for using EPICS with MX.
 *
 * Author:  William Lavender
 *
 * Warning: Note that the EPICS ca_puser() macro triggers a warning message
 *          with GCC due to the -Wcast-qual flag.  Thus, we must specify the
 *          -Wno-cast-qual flag.
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2009-2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_EPICS_DEBUG_IO			FALSE

#define MX_EPICS_DEBUG_HANDLERS			FALSE

#define MX_EPICS_DEBUG_CA_POLL			FALSE

#define MX_EPICS_DEBUG_ATEXIT			FALSE

#define MX_EPICS_DEBUG_PERFORMANCE		FALSE

#define MX_EPICS_DEBUG_PUT_CALLBACK_STATUS	FALSE

#define MX_EPICS_DEBUG_TIMEOUT_SETUP		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "mx_util.h"
#include "mx_unistd.h"

/* EPICS makes its own definition of 'struct timespec' in the EPICS
 * header file "osdTime.h".  This collides with the definition of
 * 'struct timespec' made by MX in "mx_util.h".  However, the MX
 * definition and the EPICS definition are identical, so we just
 * inhibit the including of "osdTime.h" by predefining its wrapper
 * variable 'osdTimeh'.
 *
 * Note: At present, we only do this on platforms where the compiler
 * complains about the redefinition.
 */

#if defined(OS_WIN32)
#  define osdTimeh
#endif

/* Microsoft Visual C++ does not define __STDC__, but the EPICS include
 * files depend on its presence, so we set __STDC__ to 0 here.
 */

#if defined(OS_WIN32) && defined(_MSC_VER)
#  ifndef __STDC__
#     define __STDC__ 0
#  endif
#endif

/* Detect the EPICS version number. */

#include "epicsVersion.h"

#if !defined( EPICS_VERSION )
#error Either EPICS is not installed or it is not installed at the location you configured in modules/epics/Makefile.config
#endif

#define MX_EPICS_VERSION  \
    ( EPICS_VERSION * 1000000L + EPICS_REVISION * 1000L + EPICS_MODIFICATION )

#if ( defined(EPICS_VERSION) && ( MX_EPICS_VERSION < 3014000L ) )
#error You are attempting to build MX support for EPICS with EPICS version 3.13 or before.  This is not supported.  Please upgrade to EPICS 3.14 or later.
#endif

/* The following header files come from the EPICS base source code
 * distribution.
 */

#include "tsDefs.h"
#include "cadef.h"
#include "errlog.h"

/*===*/

#include "mx_util.h"
#include "mx_record.h"
#include "mx_mutex.h"
#include "mx_epics.h"
#include "mx_hrt.h"

#if MX_EPICS_DEBUG_PERFORMANCE
#include "mx_hrt_debug.h"
#endif

/* Forward declaration of some needed functions. */

#define MXF_EPICS_INTERNAL_CAGET	1
#define MXF_EPICS_INTERNAL_CAPUT	2
#define MXF_EPICS_INTERNAL_CAPUT_NOWAIT	3

static mx_status_type
mx_epics_internal_handle_channel_disconnection( const char *calling_fname,
						int function_type,
						MX_EPICS_PV *pv,
						long epics_type,
						unsigned long num_elements,
						void *data_buffer,
						double timeout,
						int max_retries,
						int quiet );

/*---*/

/* mx_epics_connect_timeout_interval contains the timeout interval for
 * the initial connection or reconnection to EPICS process variables.
 */

static double mx_epics_connect_timeout_interval = 1.0;	/* in seconds */

/* The variable mx_epics_io_timeout_interval contains the timeout interval
 * for all other EPICS Channel Access calls.
 */

static double mx_epics_io_timeout_interval = 10.0;    	/* in seconds */

/*---*/

static long mx_epics_max_connection_attempts = 1;

static mx_bool_type mx_epics_connection_retry_warning = FALSE;

/*---*/

/* mx_epics_is_initialized is set to true the first time that
 * mx_epics_internal_connect() is called.
 */

static int mx_epics_is_initialized = FALSE;

static int mx_epics_put_callback_num_null_channel_ids = 0;
static int mx_epics_put_callback_num_null_puser_ptrs = 0;

#if ( MX_EPICS_VERSION >= 3014000L )
static MX_MUTEX *mx_epics_mutex;
#endif

#if ( MX_EPICS_VERSION >= 3014000L )
#  define LOCK_EPICS_MUTEX	(void) mx_mutex_lock( mx_epics_mutex )
#  define UNLOCK_EPICS_MUTEX	(void) mx_mutex_unlock( mx_epics_mutex )
#else
#  define LOCK_EPICS_MUTEX
#  define UNLOCK_EPICS_MUTEX
#endif

/*--------------------------------------------------------------------------*/

/* The variable mx_epics_debug_flag is used to control whether or not
 * EPICS debugging messages are generated.
 */

static int mx_epics_debug_flag = FALSE;

MX_EXPORT int
mx_epics_get_debug_flag( void )
{
	if ( mx_epics_debug_flag == FALSE ) {
		return FALSE;
	} else {
		return TRUE;
	}
}

MX_EXPORT void
mx_epics_set_debug_flag( int flag )
{
	if ( flag == FALSE ) {
		mx_epics_debug_flag = FALSE;
	} else {
		mx_epics_debug_flag = TRUE;
	}
}

/*--------------------------------------------------------------------------*/

static void
mx_epics_atexit_handler( void )
{
#if MX_EPICS_DEBUG_ATEXIT
	static char fname[] = "mx_epics_atexit_handler()";

	MX_DEBUG(-2,("%s: atexit handler invoked.", fname));
#endif

#if MX_EPICS_DEBUG_ATEXIT
	MX_DEBUG(-2,("%s: Calling ca_pend_event( 0.1 )", fname));
#endif

	/* Allow any last straggling events to be processed. */

	ca_pend_event( 0.1 );

	/* If a VME crate is shut down before we exit, we may hang in
	 * ca_context_destroy().  In order to recover from this, we set
	 * up a timeout mechanism for ca_context_destroy().  The timeout
	 * is currently set to 5 seconds.
	 */

#if defined(OS_WIN32) || defined(OS_CYGWIN)

	/* There does not seem to be a reliable way of arranging for
	 * a timeout in an atexit() handler on Windows.  It is possible
	 * to use timeSetEvent() or _beginthreadex() to try to arrange
	 * for something to happen after the timeout.  However, the
	 * event handler or thread never seems to execute.  Apparently,
	 * atexit() handlers in a DLL are invoked after most of the
	 * handles have already been destroyed, which presumably has
	 * something to do with the problem.
	 *
	 * For this reason, we skip trying to invoke ca_context_destroy()
	 * on Windows.
	 */

#if MX_EPICS_DEBUG_ATEXIT
	MX_DEBUG(-2,("%s: atexit handler returning.", fname));
#endif

	return;

#else /* not OS_WIN32 */

	/* On Unix-like systems, we use alarm() and SIGALRM to terminate
	 * after a timeout.
	 */

	signal( SIGALRM, SIG_DFL );

	alarm(5);

#if MX_EPICS_DEBUG_ATEXIT
	MX_DEBUG(-2,("%s: About to shutdown Channel Access.", fname));
#endif

	/* Destroy the Channel Access context. */

#if ( MX_EPICS_VERSION < 3014000L )
	(void) ca_task_exit();
#else
	ca_context_destroy();
#endif

#if MX_EPICS_DEBUG_ATEXIT
	MX_DEBUG(-2,("%s: atexit handler complete.", fname));
#endif

#endif /* not OS_WIN32 */

}

/*--------------------------------------------------------------------------*/

static int
mx_epics_printf_handler( const char *format, va_list args )
{
	char buffer[1000];

	vsnprintf(buffer, sizeof(buffer), format, args);

	/* Print out the text. */

	mx_info( "%s", buffer );

	/* mx_info() returns void, but EPICS expects an int. */

	return 1;
}

/*--------------------------------------------------------------------------*/

static void
mx_epics_exception_handler( struct exception_handler_args args )
{
	static char fname[] = "mx_epics_exception_handler()";

	const char *name, *filename;

	if ( args.chid ) {
		name = ca_name( args.chid );
	} else {
		name = "?";
	}

	if ( args.pFile ) {
		filename = args.pFile;
	} else {
		filename = "(null)";
	}

#if 1 /* MX_EPICS_DEBUG_HANDLERS */
	MX_DEBUG(-2,("%s: \"%s\" for %s - with request chan=%s, op=%ld, "
	"datatype=%s, count=%ld, file=%s, line_number = %u",
	 	fname, ca_message( args.stat ), args.ctx, name, args.op,
		dbr_type_to_text( args.type ), args.count,
		filename, args.lineNo));
#endif

	/* For now we do nothing with the exception except report it. */

	return;
}

/*--------------------------------------------------------------------------*/

static void
mx_epics_connection_state_change_handler( struct connection_handler_args args )
{
	static char fname[] = "mx_epics_connection_state_change_handler()";

	MX_EPICS_PV *pv;

	if ( args.chid == NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The EPICS channel id passed was NULL." );
		return;
	}

	pv = (MX_EPICS_PV *) ca_puser( args.chid );

	if ( pv == (MX_EPICS_PV *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The EPICS PUSER pointer for channel id %p was NULL.",
			args.chid );
	}

	pv->connection_state_last_change_time = mx_high_resolution_time();

	pv->connection_state = args.op;

#if MX_EPICS_DEBUG_HANDLERS
	MX_DEBUG(-2,("%s invoked for PV '%s', connection_state = %ld",
		fname, pv->pvname, pv->connection_state ));
#endif

	return;
}

/*--------------------------------------------------------------------------*/

static void
mx_epics_show_caget_value( MX_EPICS_PV *pv,
			long epics_type,
			unsigned long num_elements,
			void *data_buffer,
			const char *calling_fname )
{
	if ( num_elements > 1 ) {
		if ( epics_type == MX_CA_STRING ) {
			MX_DEBUG(-2,("%s: '%s' value read = '%s'.",
				calling_fname, pv->pvname,
				(char *) data_buffer));
		} else {
#if 0
			MX_DEBUG(-2,
			("%s: multielement array read from '%s'.",
				calling_fname, pv->pvname));
#else
			unsigned long i, num_to_show;
			unsigned long max_to_show = 10;
			char *byte_array = NULL;
			short *short_array = NULL;
			int32_t *int32_array = NULL;
			float *float_array = NULL;
			double *double_array = NULL;

			if ( num_elements > max_to_show ) {
				num_to_show = max_to_show;
			} else {
				num_to_show = num_elements;
			}

			fprintf( stderr, "%s: '%s' array read = ",
				calling_fname, pv->pvname );

			switch( epics_type ) {
			case MX_CA_CHAR:
				byte_array = (char *) data_buffer;

				fprintf( stderr, "%d", byte_array[0] );

				for ( i = 1; i < num_to_show; i++ ) {
				    fprintf( stderr, ", %d", byte_array[i] );
				}
				break;
			case MX_CA_SHORT:
			case MX_CA_ENUM:
				short_array = (short *) data_buffer;

				fprintf( stderr, "%hd", short_array[0] );

				for ( i = 1; i < num_to_show; i++ ) {
				    fprintf( stderr, ", %hd", short_array[i] );
				}
				break;
			case MX_CA_LONG:
				int32_array = (int32_t *) data_buffer;

				fprintf( stderr, "%ld",
						(long) int32_array[0] );

				for ( i = 1; i < num_to_show; i++ ) {
				    fprintf( stderr, ", %ld",
						(long) int32_array[i] );
				}
				break;
			case MX_CA_FLOAT:
				float_array = (float *) data_buffer;

				fprintf( stderr, "%g", float_array[0] );

				for ( i = 1; i < num_to_show; i++ ) {
				    fprintf( stderr, ", %g", float_array[i] );
				}
				break;
			case MX_CA_DOUBLE:
				double_array = (double *) data_buffer;

				fprintf( stderr, "%g", double_array[0] );

				for ( i = 1; i < num_to_show; i++ ) {
				    fprintf( stderr, ", %g", double_array[i] );
				}
				break;
			default:
				fprintf( stderr,
				"has unsupported epics_type %ld",
					epics_type );
				break;
			}

			if ( num_elements > num_to_show ) {
				fprintf( stderr, ", ...\n" );
			} else {
				fprintf( stderr, "\n" );
			}
#endif
		}
	} else {
		char byte_value;
		short short_value;
		int32_t int32_value;
		float float_value;
		double double_value;

		switch( epics_type ) {
		case MX_CA_CHAR:
			byte_value = *((char *) data_buffer);

			MX_DEBUG(-2,("%s: '%s' value read = %d.",
				calling_fname, pv->pvname,
				byte_value));
			break;
		case MX_CA_STRING:
			MX_DEBUG(-2,("%s: '%s' value read = '%s'",
				calling_fname, pv->pvname,
				(char *) data_buffer));
			break;
		case MX_CA_SHORT:
		case MX_CA_ENUM:
			short_value = *((short *) data_buffer);

			MX_DEBUG(-2,("%s: '%s' value read = %hd.",
				calling_fname, pv->pvname,
				short_value));
			break;
		case MX_CA_LONG:
			int32_value = *((int32_t *) data_buffer);

			MX_DEBUG(-2,("%s: '%s' value read = %ld.",
				calling_fname, pv->pvname,
				(long) int32_value));
			break;
		case MX_CA_FLOAT:
			float_value = *((float *) data_buffer);

			MX_DEBUG(-2,("%s: '%s' value read = %g.",
				calling_fname, pv->pvname,
				float_value));
			break;
		case MX_CA_DOUBLE:
			double_value = *((double *) data_buffer);

			MX_DEBUG(-2,("%s: '%s' value read = %g.",
				calling_fname, pv->pvname,
				double_value));
			break;
		default:
			MX_DEBUG(-2,
			("%s: '%s' has unsupported epics_type %ld",
				calling_fname, pv->pvname,
				epics_type));
			break;
		}
	}

	return;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_initialize( void )
{
	static const char fname[] = "mx_epics_initialize()";

	int status, epics_status;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

#if MX_EPICS_DEBUG_IO
	mx_epics_set_debug_flag(TRUE);

	MX_DEBUG(-2,("%s: MX EPICS support is using '%s'",
		fname, EPICS_VERSION_STRING))
#endif

#if ( MX_EPICS_VERSION >= 3014000L )
	/* Make sure the EPICS error log buffer is set up. */

	errlogInit( 1280 );	/* EPICS expects this to be at least 1280. */

	/* Give the errlog thread a while to initialize itself.  Otherwise,
	 * EPICS errors that happen early on can cause segmentation faults
	 * in the errlog thread.
	 */

	/* FIXME: It would be nice to have a somewhat more deterministic
	 * way of knowing when the errlog thread is ready.
	 */

	mx_msleep(100);

	/* Set up a mutex used to protect variables used by the MX
	 * put callback handler.
	 */

	mx_status = mx_mutex_create( &mx_epics_mutex );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

#if ( MX_EPICS_VERSION < 3014000L )
	epics_status = ca_task_initialize();

#else		/* EPICS 3.14 */
	{
		enum ca_preemptive_callback_select callback_type;

		callback_type = ca_enable_preemptive_callback;

#  if MX_EPICS_DEBUG_IO
		if ( callback_type == ca_enable_preemptive_callback ) {
			MX_DEBUG(-2,
			("%s: Enabling preemptive callback.", fname));
		} else {
			MX_DEBUG(-2,
			("%s: Disabling preemptive callback.", fname));
		}
#  endif
		epics_status = ca_context_create( callback_type );
	}
#endif
	
	if ( epics_status != ECA_NORMAL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"The attempt to initialize EPICS failed.  "
		"EPICS status = %d, EPICS error = '%s'.",
			epics_status, ca_message( epics_status ) );
	}

	/* Redirect EPICS output to MX. */

	ca_replace_printf_handler( &mx_epics_printf_handler );

	/* Install an EPICS exception handler. */

	epics_status = ca_add_exception_event( mx_epics_exception_handler, 0 );

	if ( epics_status != ECA_NORMAL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
	"The attempt to install a global exception handler failed.  "
	"EPICS status = %d, EPICS error = '%s'.",
			epics_status, ca_message( epics_status ) );
	}

#if MX_EPICS_DEBUG_IO
	MX_DEBUG(-2,("%s: ca_add_exception_event() succeeded.", fname));
#endif

	/* Setup an atexit() handler to deal with normal EPICS shutdown. */

	status = atexit( mx_epics_atexit_handler );

	if ( status != 0 ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
	"Installing the MX EPICS atexit handler failed for an unknown reason.");
	}

	mx_epics_is_initialized = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT void
mx_epics_set_connection_timeout( double timeout_in_seconds )
{
	static const char fname[] = "mx_epics_set_connection_timeout()";

#if MX_EPICS_DEBUG_TIMEOUT_SETUP
	MX_DEBUG(-2,("%s: timeout_in_seconds = %f, "
		"mx_epics_is_initialized = %d",
		fname, timeout_in_seconds, mx_epics_is_initialized ));
#endif

	if ( mx_epics_is_initialized ) {
		mx_warning( "%s invoked after an MX record has already "
		"initialized EPICS.  Normally, you should make sure that "
		"%s is invoked before any EPICS-related MX records are "
		"set up.  Otherwise, some EPICS PVs may use the previous "
		"connection timeout of %f seconds rather than the "
		"requested value of %f seconds.", fname, fname,
			mx_epics_connect_timeout_interval,
			timeout_in_seconds );
	}

	mx_epics_connect_timeout_interval = timeout_in_seconds;

	return;
}

MX_EXPORT double
mx_epics_get_connection_timeout( void )
{
#if MX_EPICS_DEBUG_TIMEOUT_SETUP
	static const char fname[] = "mx_epics_get_connection_timeout()";
#endif

	double timeout_in_seconds;

	timeout_in_seconds = mx_epics_connect_timeout_interval;

#if MX_EPICS_DEBUG_TIMEOUT_SETUP
	MX_DEBUG(-2,("%s: timeout_in_seconds = %f, "
		"mx_epics_is_initialized = %d",
		fname, timeout_in_seconds, mx_epics_is_initialized ));
#endif

	return timeout_in_seconds;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT void
mx_epics_set_max_connection_attempts( long max_attempts )
{
	mx_epics_max_connection_attempts = max_attempts;

	return;
}

MX_EXPORT long
mx_epics_get_max_connection_attempts( void )
{
	return mx_epics_max_connection_attempts;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT void
mx_epics_set_connection_retry_warning( mx_bool_type show_it )
{
	mx_epics_connection_retry_warning = show_it;

	return;
}

MX_EXPORT mx_bool_type
mx_epics_get_connection_retry_warning( void )
{
	return mx_epics_connection_retry_warning;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_pend_io( double timeout )
{
	static const char fname[] = "mx_epics_pend_io()";

	int epics_status;

	epics_status = ca_pend_io( timeout );

#if MX_EPICS_DEBUG_CA_POLL
	if ( mx_get_debug_level() >= -2 ) {
		fprintf(stderr, ")%d(", epics_status);
	}
#endif

	switch( epics_status ) {
	case ECA_NORMAL:
		break;
	case ECA_TIMEOUT:
		return mx_error( MXE_TIMED_OUT, fname,
			"The selected I/O requests did not complete before "
			"the specified timeout of %g seconds.", timeout );
		break;
	case ECA_EVDISALLOW:
		return mx_error( MXE_UNSUPPORTED, fname,
	  "Cannot execute ca_pend_io() from within an EPICS event handler.");
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"EPICS background event processing via ca_pend_io() "
		"failed.  EPICS status = %d, EPICS error = '%s'",
			epics_status, ca_message( epics_status ) );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_pend_event( double timeout )
{
	static const char fname[] = "mx_epics_pend_event()";

	int epics_status;

	/* Check for put callback errors. */

	LOCK_EPICS_MUTEX;	/* FIXME - Does this really need to be here? */

	if ( mx_epics_put_callback_num_null_channel_ids != 0 ) {
		mx_warning("The EPICS put callback handler received "
			"a NULL channel id.  This bug should be reported "
			"to Bill Lavender." );

#if 0
		mx_epics_put_callback_num_null_channel_ids = 0;
#endif
	}

	if ( mx_epics_put_callback_num_null_puser_ptrs != 0 ) {
		mx_warning("The EPICS put callback handler received "
			"a NULL PUSER pointer.  This bug should be reported "
			"to Bill Lavender." );

#if 0
		mx_epics_put_callback_num_null_puser_ptrs = 0;
#endif
	}

	UNLOCK_EPICS_MUTEX;	/* FIXME - Does this really need to be here? */

	/* Poll EPICS for outstanding events. */

	epics_status = ca_pend_event( timeout );

#if MX_EPICS_DEBUG_CA_POLL
	if ( mx_get_debug_level() >= -2 ) {
		fprintf(stderr, ">%d<", epics_status);
	}
#endif

	switch( epics_status ) {
	case ECA_NORMAL:
	case ECA_TIMEOUT:
		break;		/* no error */
	case ECA_EVDISALLOW:
		return mx_error( MXE_UNSUPPORTED, fname,
	  "Cannot execute ca_pend_event() from within an EPICS event handler.");
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"EPICS background event processing via ca_pend_event() "
		"failed.  EPICS status = %d, EPICS error = '%s'",
			epics_status, ca_message( epics_status ) );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_flush_io( void )
{
	static const char fname[] = "mx_epics_flush_io()";

	int epics_status;

	epics_status = ca_flush_io();

#if MX_EPICS_DEBUG_IO
	MX_DEBUG(-2,("%s: ca_flush_io() status = %d", fname, epics_status));
#endif

	if ( epics_status != ECA_NORMAL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
	"An attempt to force execution of EPICS Channel Access I/O failed.  "
	"EPICS status = %d, EPICS error = '%s'.",
			epics_status, ca_message( epics_status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT void
mx_epics_pvname_init( MX_EPICS_PV *pv, char *name_format, ... )
{
	static const char fname[] = "mx_epics_pvname_init()";

	va_list args;
	char buffer[1000];

	if ( pv == (MX_EPICS_PV *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );

		return;
	}
	if ( name_format == (char *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The EPICS PV name format string pointer passed was NULL." );

		return;
	}

	va_start(args, name_format);
	vsnprintf(buffer, sizeof(buffer), name_format, args);
	va_end(args);

	/* Save the process variable name. */

	strlcpy( pv->pvname, buffer, MXU_EPICS_PVNAME_LENGTH );

	/* Initialize the data structure. */

	pv->channel_id = NULL;

	pv->connect_timeout_interval =
		mx_convert_seconds_to_high_resolution_time(
					mx_epics_connect_timeout_interval );

	pv->reconnect_timeout_interval =
		mx_convert_seconds_to_high_resolution_time(
					mx_epics_connect_timeout_interval );

	pv->io_timeout_interval = mx_convert_seconds_to_high_resolution_time(
					mx_epics_io_timeout_interval );

	pv->put_callback_status = MXF_EPVH_IDLE;

#if MX_EPICS_DEBUG_PUT_CALLBACK_STATUS
	MX_DEBUG(-2,("%s: Initializing PV '%s' put callback status to %d",
		fname, pv->pvname, pv->put_callback_status));
#endif

	pv->application_ptr = NULL;

	return;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_pv_connect( MX_EPICS_PV *pv, unsigned long connect_flags )
{
	static const char fname[] = "mx_epics_pv_connect()";

	chid new_channel_id;
	struct timespec current_time, timeout_time, attempt_start_time;
	int epics_status, time_comparison;
	long error_code;
	mx_bool_type wait_for_connection;
	long attempt;
	double timeout_in_seconds;
	mx_status_type mx_status;

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_TIMING measurement;
#endif

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

	if ( pv->channel_id != NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The EPICS PV '%s' is already connected.", pv->pvname );
	}

	if ( strlen(pv->pvname) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Cannot connect to a zero-length EPICS PV name." );
	}

#if MX_EPICS_DEBUG_IO
	MX_DEBUG(-2,("%s invoked for PV '%s'", fname, pv->pvname));
#endif

	if ( mx_epics_is_initialized == FALSE ) {
		mx_status = mx_epics_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_START( measurement );
#endif
	if ( connect_flags & MXF_EPVC_WAIT_FOR_CONNECTION ) {
		wait_for_connection = TRUE;
	} else {
		wait_for_connection = FALSE;
	}

	timeout_in_seconds = mx_convert_high_resolution_time_to_seconds(
					pv->connect_timeout_interval );

	/* Initialize the connection state variables. */

	pv->connection_state_last_change_time = mx_high_resolution_time();

	pv->connection_state = CA_OP_CONN_DOWN;

	/*---- Start the connection process. ----*/

	/* If you want no heroic measures, then
	 * set mx_epics_max_connection_attempts to 1.
	 */

	for ( attempt = 1;
		attempt <= mx_epics_max_connection_attempts;
		attempt++ )
	{
#if MX_EPICS_DEBUG_IO
		MX_DEBUG(-2,("%s: About to create a channel for PV '%s'",
			fname, pv->pvname));
#endif

		if ( mx_epics_debug_flag ) {
			if ( wait_for_connection == FALSE ) {
				MX_DEBUG(-2,
				("%s: searching for PV '%s' without waiting.",
					fname, pv->pvname));
			}
		}

		attempt_start_time = mx_high_resolution_time();

#if ( MX_EPICS_VERSION < 3014000L )
		epics_status = ca_search_and_connect(
				pv->pvname, &new_channel_id,
				mx_epics_connection_state_change_handler, pv );
#else
		epics_status = ca_create_channel(
				pv->pvname,
				mx_epics_connection_state_change_handler,
				pv, 0, &new_channel_id );
#endif

		if ( epics_status != ECA_NORMAL ) {
			return mx_error( MXE_NOT_FOUND, fname,
				"Unable to connect to EPICS PV '%s'.  "
				"EPICS status = %d, EPICS error = '%s'",
				pv->pvname, epics_status,
				ca_message( epics_status ) );
		}

		pv->channel_id = new_channel_id;

#if MX_EPICS_DEBUG_IO
		MX_DEBUG(-2,("%s: pvname = '%s', channel_id = %p, "
		"connect_timeout_interval = %g",
			fname, pv->pvname, pv->channel_id,
			  mx_convert_high_resolution_time_to_seconds(
				pv->connect_timeout_interval ) ));
#endif

		/* Flush the connection request out to the server. */

		mx_status = mx_epics_flush_io();

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) ca_clear_channel( new_channel_id );
			pv->channel_id = NULL;
			return mx_status;
		}

		/* Return now if we are not waiting for the connection
		 * to complete.
		 */

		if ( wait_for_connection == FALSE ) {
			return MX_SUCCESSFUL_RESULT;
		}

		/* When will the connection timeout interval expire? */

		timeout_time = mx_add_high_resolution_times(
				attempt_start_time,
				pv->connect_timeout_interval );

		/* Wait for the search request to complete. */

		while (1) {
			/* Has the connection request completed? */

			mx_status = mx_epics_poll();

			if ( mx_status.code != MXE_SUCCESS ) {
				(void) ca_clear_channel( new_channel_id );
				pv->channel_id = NULL;
				return mx_status;
			}

			/* If so, then break out of this while loop. */

			if ( pv->connection_state == CA_OP_CONN_UP ) {

#if MX_EPICS_DEBUG_IO
				MX_DEBUG(-2,
				("%s: successfully connected to PV '%s'.",
					fname, pv->pvname ));
#endif
				if ( mx_epics_debug_flag ) {
					MX_DEBUG(-2,
					("%s: connected to PV '%s', chid = %p",
						fname, pv->pvname,
						pv->channel_id ));
				}

				break;		/* Exit the while() loop. */
			}

			/* If not, have we timed out yet? */

			current_time = mx_high_resolution_time();

			time_comparison = mx_compare_high_resolution_times(
						current_time, timeout_time );

			if ( time_comparison >= 0 ) {

				break;	/* Timed out of the while() loop. */
			}
			
			mx_msleep(1);	/* Sleep for at least 1 millisecond. */
		}

		/* Do we need to go back for another attempt? */

		if ( pv->connection_state == CA_OP_CONN_UP ) {
			break;		/* Exit the retry for() loop. */
		}

		/* If we get here, our attempt to connect to the PV
		 * has timed out.  If so, we abandon the existing
		 * attempt to connect and start a new one.
		 */

		epics_status = ca_clear_channel( pv->channel_id );

		pv->channel_id = NULL;

		if ( epics_status != ECA_NORMAL ) {
			(void) mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"The attempt to clear a timed out connection "
			"for EPICS PV '%s' failed.  Discarding...",
				pv->pvname );
		}

		/* Force the command out to the CA server. */

		mx_status = mx_epics_flush_io();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If it is time for us to give up, then do so. */

		if ( connect_flags & MXF_EPVC_QUIET ) {
			error_code = MXE_TIMED_OUT | MXE_QUIET;
		} else {
			error_code = MXE_TIMED_OUT;
		}

		if ( mx_epics_max_connection_attempts <= 1 ) {
			return mx_error( error_code, fname,
			"The attempt to connect to EPICS PV '%s' "
			"has timed out after %g seconds.",
				pv->pvname, timeout_in_seconds );
		} else
		if ( attempt >= mx_epics_max_connection_attempts ) {
			return mx_error( error_code, fname,
			"The attempt to connect to EPICS PV '%s' "
			"failed after %ld attempts of %g seconds each.",
				pv->pvname,
				mx_epics_max_connection_attempts,
				timeout_in_seconds );
		}

		/* Otherwise, tell the user that we are going back
		 * for another attempt.
		 */

		if ( mx_epics_connection_retry_warning ) {
			mx_warning(
		    "Attempt %ld to connect to PV '%s' timed out. Retrying...",
				attempt, pv->pvname );
		}

		/* Wait a while first. */

		mx_msleep(100);
	}

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, pv->pvname );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_pv_disconnect( MX_EPICS_PV *pv )
{
	static const char fname[] = "mx_epics_pv_disconnect()";

	int epics_status;
	mx_status_type mx_status;

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_TIMING measurement;
#endif

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

	if ( pv->channel_id == NULL ) {
#if 0
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The EPICS PV '%s' was already disconnected.", pv->pvname );
#else
		return MX_SUCCESSFUL_RESULT;
#endif
	}


#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_START( measurement );
#endif

	epics_status = ca_clear_channel( pv->channel_id );

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, pv->pvname );
#endif

	if ( epics_status != ECA_NORMAL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Attempt to disconnect from EPICS PV '%s' failed.  "
		"EPICS status = %d, EPICS error = '%s'.", pv->pvname,
		epics_status, ca_message( epics_status ) );
	}

	pv->channel_id = NULL;

	/* Allow background events to execute. */

	mx_status = mx_epics_poll();
		
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_bool_type
mx_epics_pv_is_connected( MX_EPICS_PV *pv )
{
	if ( pv == (MX_EPICS_PV *) NULL ) {
		return FALSE;
	}

	if ( pv->connection_state == CA_OP_CONN_UP ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mx_epics_internal_caget( MX_EPICS_PV *pv,
			long epics_type,
			unsigned long num_elements,
			void *data_buffer,
			double timeout,
			int max_retries )
{
	static const char fname[] = "mx_epics_internal_caget()";

#if 1
	unsigned long max_elements;
#endif
	int epics_status;
	mx_status_type mx_status;

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_TIMING measurement;
#endif

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}
	if ( data_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_buffer pointer passed was NULL." );
	}
	if ( pv->channel_id == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The channel_id passed for EPICS PV '%s' was NULL.",
			pv->pvname );
	}

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_START( measurement );
#endif

	mx_status = mx_epics_poll();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	max_elements = (unsigned long) ca_element_count( pv->channel_id );

	if ( num_elements > max_elements ) {

		/* Make sure that the rest of the buffer is zeroed out. */

		size_t element_size = mx_epics_get_element_size( epics_type );

		size_t num_bytes = element_size * num_elements;

		memset( data_buffer, 0, num_bytes );

		/* Limit the number of elements to the maximum supported
		 * by the EPICS server.
		 */

		num_elements = max_elements;
	}
#endif

	epics_status = ca_array_get( epics_type, num_elements,
					pv->channel_id, data_buffer );

	switch( epics_status ) {
	case ECA_NORMAL:
		break;
	case ECA_BADCHID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"ca_array_get() was called with "
			"illegal EPICS channel id %p", pv->channel_id );
		break;
	case ECA_BADCOUNT:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of elements %lu for "
			"EPICS PV '%s' is larger than the maximum allowed "
			"(%lu) for this process variable.",
				num_elements, pv->pvname,
			(unsigned long) ca_element_count( pv->channel_id ) );
		break;
	case ECA_BADTYPE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"An illegal EPICS datatype was requested for "
			"EPICS PV '%s'.", pv->pvname );
		break;
	case ECA_GETFAIL:
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"An attempt to read the value of local EPICS "
			"database variable '%s' failed.", pv->pvname );
		break;
	case ECA_NORDACCESS:
		return mx_error( MXE_PERMISSION_DENIED, fname,
			"An attempt to read from read-protected EPICS process "
			"variable '%s' failed.", pv->pvname );
		break;
	case ECA_DISCONN:
	case ECA_DISCONNCHID:
		return mx_epics_internal_handle_channel_disconnection(
						fname,
						MXF_EPICS_INTERNAL_CAGET,
						pv,
						epics_type,
						num_elements,
						data_buffer,
						timeout,
						max_retries,
						FALSE );
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"A ca_array_get() call failed for EPICS PV '%s'.  "
			"EPICS status = %d, EPICS error = '%s'.",
				pv->pvname, epics_status,
				ca_message( epics_status ) );
		break;
	}

	epics_status = ca_pend_io( timeout );

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, pv->pvname );
#endif

	switch( epics_status ) {
	case ECA_NORMAL:
		break;
	case ECA_TIMEOUT:
		return mx_error( MXE_TIMED_OUT, fname,
			"caget from EPICS PV '%s' timed out after %g seconds.",
				pv->pvname, timeout );
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"ca_pend_io() failed for EPICS PV '%s'.  "
		"EPICS status = %d, EPICS error = '%s'.",
			pv->pvname, epics_status,
			ca_message( epics_status ) );
		break;
	}


	if ( mx_epics_debug_flag ) {
		mx_epics_show_caget_value( pv, epics_type,
					num_elements, data_buffer, fname );
	}

	/* Allow background events to execute. */

	mx_status = mx_epics_poll();
		
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_caget( MX_EPICS_PV *pv,
		long epics_type,
		unsigned long num_elements,
		void *data_buffer )
{
	static const char fname[] = "mx_caget()";

	mx_status_type mx_status;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

	if ( pv->channel_id == NULL ) {
		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_epics_internal_caget( pv, epics_type,
					num_elements, data_buffer,
					mx_epics_io_timeout_interval, 1 );
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_caget_with_timeout( MX_EPICS_PV *pv,
		long epics_type,
		unsigned long num_elements,
		void *data_buffer,
		double timeout )
{
	static const char fname[] = "mx_caget_with_timeout()";

	mx_status_type mx_status;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

	if ( pv->channel_id == NULL ) {
		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_epics_internal_caget( pv, epics_type,
					num_elements, data_buffer,
					timeout, 1 );
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_caget_by_name( char *pvname,
		long epics_type,
		unsigned long num_elements,
		void *data_buffer )
{
	static const char fname[] = "mx_caget_by_name()";

	MX_EPICS_PV pv;
	mx_status_type mx_status;

	if ( pvname == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pvname pointer passed was NULL." );
	}

	mx_epics_pvname_init( &pv, pvname );

	mx_status = mx_epics_pv_connect( &pv, MXF_EPVC_WAIT_FOR_CONNECTION );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_epics_internal_caget( &pv, epics_type,
					num_elements, data_buffer,
					mx_epics_io_timeout_interval, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_epics_pv_disconnect( &pv );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

static void
mx_epics_ca_array_put_synchronous_callback_handler(
			struct event_handler_args args )
{
#if MX_EPICS_DEBUG_PUT_CALLBACK_STATUS
	static const char fname[] =
		"mx_epics_ca_array_put_synchronous_callback_handler()";
#endif

	MX_EPICS_PV *pv;

	LOCK_EPICS_MUTEX;

	if ( args.chid == NULL ) {
		mx_epics_put_callback_num_null_channel_ids++;
		UNLOCK_EPICS_MUTEX;
		return;
	}

	pv = (MX_EPICS_PV *) ca_puser( args.chid );

	if ( pv == (MX_EPICS_PV *) NULL ) {
		mx_epics_put_callback_num_null_puser_ptrs++;
		UNLOCK_EPICS_MUTEX;
		return;
	}

#if MX_EPICS_DEBUG_PUT_CALLBACK_STATUS
	MX_DEBUG(-2,("%s: PV '%s' put callback status = %d",
		fname, pv->pvname, pv->put_callback_status));
#endif

	if ( pv->put_callback_status != MXF_EPVH_CALLBACK_IN_PROGRESS ) {
		pv->put_callback_status = MXF_EPVH_UNEXPECTED_HANDLER_CALL;
		UNLOCK_EPICS_MUTEX;
		return;
	}

	/* Record that the callback has now occurred. */

	pv->put_callback_status = MXF_EPVH_IDLE;

#if MX_EPICS_DEBUG_PUT_CALLBACK_STATUS
	MX_DEBUG(-2,("%s: Setting PV '%s' put callback status to %d",
		fname, pv->pvname, pv->put_callback_status));
#endif

	UNLOCK_EPICS_MUTEX;

	return;
}

/*--------------------------------------------------------------------------*/

static void
mx_epics_ca_array_put_asynchronous_callback_handler(
			struct event_handler_args args )
{
	static const char fname[] =
		"mx_epics_ca_array_put_asynchronous_callback_handler()";

	MX_EPICS_CALLBACK *callback;
	MX_EPICS_PV *pv;
	mx_status_type (*callback_function)( MX_EPICS_CALLBACK *, void * );

	LOCK_EPICS_MUTEX;

	callback = (MX_EPICS_CALLBACK *) args.usr;

	if ( callback == (MX_EPICS_CALLBACK *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_CALLBACK pointer returned by EPICS was NULL." );

		UNLOCK_EPICS_MUTEX;
		return;
	}

	pv = callback->pv;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		mx_epics_put_callback_num_null_puser_ptrs++;
		mx_free( callback);
		UNLOCK_EPICS_MUTEX;
		return;
	}

#if MX_EPICS_DEBUG_PUT_CALLBACK_STATUS
	MX_DEBUG(-2,("%s: PV '%s' put callback status = %d",
		fname, pv->pvname, pv->put_callback_status));
#endif
	if ( pv->put_callback_status != MXF_EPVH_CALLBACK_IN_PROGRESS ) {
		pv->put_callback_status = MXF_EPVH_UNEXPECTED_HANDLER_CALL;
		mx_free( callback);
		UNLOCK_EPICS_MUTEX;
		return;
	}

	/* Invoke the MX callback function. */

	callback_function = callback->callback_function;

	if ( callback_function != NULL ) {
		(void) (*callback_function)( callback,
					callback->callback_argument );
	}

	/* Record that the callback has now occurred and free the memory
	 * allocated for the MX_EPICS_CALLBACK object.
	 */

	pv->put_callback_status = MXF_EPVH_IDLE;

#if MX_EPICS_DEBUG_PUT_CALLBACK_STATUS
	MX_DEBUG(-2,("%s: Setting PV '%s' put callback status to %d",
		fname, pv->pvname, pv->put_callback_status));
#endif
	mx_free( callback );

	UNLOCK_EPICS_MUTEX;

	return;
}

/*--------------------------------------------------------------------------*/

#define MXI_EPICS_CAPUT			1
#define MXI_EPICS_CAPUT_NOWAIT		2
#define MXI_EPICS_CAPUT_WITH_CALLBACK	3
#define MXI_EPICS_CAPUT_WITH_TIMEOUT	4

static mx_status_type
mx_epics_internal_caput( MX_EPICS_PV *pv,
			long epics_type,
			unsigned long num_elements,
			void *data_buffer,
			double timeout,
			int max_retries,
			int caput_type,
			mx_status_type( *callback_function )
					( MX_EPICS_CALLBACK *, void * ),
			void *callback_argument )
{
	static const char fname[] = "mx_epics_internal_caput()";

	MX_EPICS_CALLBACK *callback_object;
	void (*internal_epics_callback_function)( struct event_handler_args );
	void *internal_epics_callback_argument;
#if 1
	unsigned long max_elements;
#endif
	int epics_status;
	unsigned long i, milliseconds_to_wait, milliseconds_between_polls;
	mx_status_type mx_status;

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_TIMING measurement;
#endif

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}
	if ( data_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_buffer pointer passed was NULL." );
	}
	if ( pv->channel_id == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The channel_id passed for EPICS PV '%s' was NULL.",
			pv->pvname );
	}

	switch( caput_type ) {
	case MXI_EPICS_CAPUT_NOWAIT:
		internal_epics_callback_function = NULL;
		internal_epics_callback_argument = NULL;
		break;
	case MXI_EPICS_CAPUT:
	case MXI_EPICS_CAPUT_WITH_TIMEOUT:
		internal_epics_callback_function =
			    mx_epics_ca_array_put_synchronous_callback_handler;

		internal_epics_callback_argument = NULL;
		break;
	case MXI_EPICS_CAPUT_WITH_CALLBACK:
		internal_epics_callback_function =
			    mx_epics_ca_array_put_asynchronous_callback_handler;

		callback_object = (MX_EPICS_CALLBACK *)
				malloc( sizeof(MX_EPICS_CALLBACK) );

		if ( callback_object == (MX_EPICS_CALLBACK *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"an MX_EPICS_CALLBACK structure for PV '%s'.",
				pv->pvname );
		}

		callback_object->pv = pv;
		callback_object->callback_function = callback_function;
		callback_object->callback_argument = callback_argument;

		internal_epics_callback_argument = callback_object;
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Invoked with illegal caput type %d", caput_type );
		break;
	}

	if ( mx_epics_debug_flag ) {
		if ( num_elements > 1 ) {
			if ( epics_type == MX_CA_STRING ) {
				MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
					fname, (char *) data_buffer,
					pv->pvname ));
			} else {
				MX_DEBUG(-2,
				("%s: sending multielement array to '%s'.",
					fname, pv->pvname ));
			}
		} else {
			char byte_value;
			short short_value;
			int32_t int32_value;
			float float_value;
			double double_value;
	
			switch( epics_type ) {
			case MX_CA_CHAR:
				byte_value = *((char *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %d to '%s'.",
					fname, byte_value,
					pv->pvname ));
				break;
			case MX_CA_STRING:
				MX_DEBUG(-2,("%s: sending '%s' to '%s'",
					fname, (char *) data_buffer,
					pv->pvname ));
				break;
			case MX_CA_SHORT:
			case MX_CA_ENUM:
				short_value = *((short *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %hd to '%s'.",
					fname, short_value,
					pv->pvname ));
				break;
			case MX_CA_LONG:
				int32_value = *((int32_t *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %ld to '%s'.",
					fname, (long) int32_value,
					pv->pvname ));
				break;
			case MX_CA_FLOAT:
				float_value = *((float *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %g to '%s'.",
					fname, float_value,
					pv->pvname ));
				break;
			case MX_CA_DOUBLE:
				double_value = *((double *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %g to '%s'.",
					fname, double_value,
					pv->pvname ));
				break;
			default:
				MX_DEBUG(-2,
				("%s: '%s' has unsupported epics_type %ld",
					fname, pv->pvname,
					epics_type));
				break;
			}
		}
	}

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_START( measurement );
#endif

	mx_status = mx_epics_poll();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	/* Limit the number of elements to the maximum supported
	 * by the EPICS server.
	 */

	max_elements = (unsigned long) ca_element_count( pv->channel_id );

	if ( num_elements > max_elements ) {
		num_elements = max_elements;
	}
#endif

	LOCK_EPICS_MUTEX;

#if MX_EPICS_DEBUG_PUT_CALLBACK_STATUS
	MX_DEBUG(-2,("%s: PV '%s' put callback status = %d",
		fname, pv->pvname, pv->put_callback_status));
#endif

	if ( pv->put_callback_status != MXF_EPVH_IDLE ) {
		mx_warning( "EPICS PV '%s' put callback status has the "
			"unexpected value of %d just before invoking "
			"ca_array_put_callback().",
					pv->pvname, pv->put_callback_status );
	}

	/* Mark this channel ID as having a put callback in progress. */

	switch( caput_type ) {
	case MXI_EPICS_CAPUT_NOWAIT:
		pv->put_callback_status = MXF_EPVH_IDLE;
		break;
	default:
		pv->put_callback_status = MXF_EPVH_CALLBACK_IN_PROGRESS;
		break;
	}

#if MX_EPICS_DEBUG_PUT_CALLBACK_STATUS
	MX_DEBUG(-2,("%s: Setting PV '%s' put callback status to %d",
		fname, pv->pvname, pv->put_callback_status));
#endif

	UNLOCK_EPICS_MUTEX;

	/* Send the request to Channel Access. */

	switch( caput_type ) {
	case MXI_EPICS_CAPUT_NOWAIT:
		epics_status = ca_array_put(
					epics_type, num_elements,
					pv->channel_id, data_buffer );
		break;
	default:
		epics_status = ca_array_put_callback(
					epics_type, num_elements,
					pv->channel_id, data_buffer,
					internal_epics_callback_function,
					internal_epics_callback_argument );
		break;
	}

	switch( epics_status ) {
	case ECA_NORMAL:
		break;
	case ECA_BADCHID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"ca_array_put_callback() was called with an "
			"illegal EPICS channel id" );
		break;
	case ECA_BADCOUNT:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of elements %lu for "
			"EPICS PV '%s' is larger than the maximum allowed "
			"(%lu) for this process variable.",
				num_elements, pv->pvname,
			(unsigned long) ca_element_count( pv->channel_id ) );
		break;
	case ECA_BADTYPE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"An illegal EPICS datatype was requested for "
			"EPICS PV '%s'.", pv->pvname );
		break;
	case ECA_ALLOCMEM:
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory while trying to perform a "
			"ca_array_put_callback() call for EPICS PV '%s'.",
				pv->pvname );
		break;
	case ECA_NOWTACCESS:
		return mx_error( MXE_PERMISSION_DENIED, fname,
			"An attempt to write to read-only EPICS process "
			"variable '%s' failed.", pv->pvname );
		break;
	case ECA_DISCONN:
	case ECA_DISCONNCHID:
		return mx_epics_internal_handle_channel_disconnection(
						fname,
						MXF_EPICS_INTERNAL_CAPUT,
						pv,
						epics_type,
						num_elements,
						data_buffer,
						timeout,
						max_retries,
						FALSE );
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"A ca_array_put_callback() call failed for EPICS "
			"PV '%s'.  EPICS status = %d, EPICS error = '%s'.",
				pv->pvname, epics_status,
				ca_message( epics_status ) );
		break;
	}

	/* If we are not waiting for a synchronous callback, then exit. */

	switch( caput_type ) {
	case MXI_EPICS_CAPUT_NOWAIT:
	case MXI_EPICS_CAPUT_WITH_CALLBACK:

		/* Allow background events to execute. */

		mx_status = mx_epics_poll();

#if MX_EPICS_DEBUG_PERFORMANCE
		MX_HRT_END( measurement );

		MX_HRT_RESULTS( measurement, fname, "%s (async)", pv->pvname );
#endif
		return mx_status;

		break;
	}

	/* Wait for the put to complete. */

	milliseconds_to_wait = mx_round( 1000.0 * timeout );

	milliseconds_between_polls = 10;

	for (i = 0; i < milliseconds_to_wait; i += milliseconds_between_polls)
	{
		/* See if the callback has completed and allow other 
		 * background events to execute too.
		 */

		mx_status = mx_epics_poll();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		LOCK_EPICS_MUTEX;

#if MX_EPICS_DEBUG_PUT_CALLBACK_STATUS
		MX_DEBUG(-2,("%s: PV '%s' put callback status = %d",
			fname, pv->pvname, pv->put_callback_status));
#endif

		if ( pv->put_callback_status == MXF_EPVH_IDLE ) {
			UNLOCK_EPICS_MUTEX;
			break;		/* Exit the for() loop. */
		} else
		if ( pv->put_callback_status != MXF_EPVH_CALLBACK_IN_PROGRESS)
		{
			mx_warning(
			"EPICS PV '%s' put callback status has the "
			"unexpected value of %d while waiting for "
			"a callback to complete.",
					pv->pvname, pv->put_callback_status );
		}

		UNLOCK_EPICS_MUTEX;

		mx_msleep( milliseconds_between_polls );
	}

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "%s (sync)", pv->pvname );
#endif

	if ( i >= milliseconds_to_wait ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"caput to EPICS PV '%s' timed out after %g seconds.",
			pv->pvname, timeout );
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_caput( MX_EPICS_PV *pv,
		long epics_type,
		unsigned long num_elements,
		void *data_buffer )
{
	static const char fname[] = "mx_caput()";

	mx_status_type mx_status;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

	if ( pv->channel_id == NULL ) {
		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_epics_internal_caput( pv, epics_type,
					num_elements, data_buffer,
					mx_epics_io_timeout_interval, 1,
					MXI_EPICS_CAPUT,
					NULL, NULL );
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_caput_with_callback( MX_EPICS_PV *pv,
		long epics_type,
		unsigned long num_elements,
		void *data_buffer,
		mx_status_type (*callback_function)
				( MX_EPICS_CALLBACK *, void * ),
		void *callback_argument )
{
	static const char fname[] = "mx_caput_with_callback()";

	mx_status_type mx_status;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

	if ( pv->channel_id == NULL ) {
		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_epics_internal_caput( pv, epics_type,
					num_elements, data_buffer,
					0.0, 0,
					MXI_EPICS_CAPUT_WITH_CALLBACK,
					callback_function, callback_argument );
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_caput_with_timeout( MX_EPICS_PV *pv,
		long epics_type,
		unsigned long num_elements,
		void *data_buffer,
		double timeout )
{
	static const char fname[] = "mx_caput_with_timeout()";

	mx_status_type mx_status;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

	if ( pv->channel_id == NULL ) {
		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_epics_internal_caput( pv, epics_type,
					num_elements, data_buffer,
					timeout, 1,
					MXI_EPICS_CAPUT_WITH_TIMEOUT,
					NULL, NULL );
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_caput_by_name( char *pvname,
		long epics_type,
		unsigned long num_elements,
		void *data_buffer )
{
	static const char fname[] = "mx_caput_by_name()";

	MX_EPICS_PV pv;
	mx_status_type mx_status;

	if ( pvname == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pvname pointer passed was NULL." );
	}

	mx_epics_pvname_init( &pv, pvname );

	mx_status = mx_epics_pv_connect( &pv, MXF_EPVC_WAIT_FOR_CONNECTION );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_epics_internal_caput( &pv, epics_type,
					num_elements, data_buffer,
					mx_epics_io_timeout_interval, 0,
					MXI_EPICS_CAPUT,
					NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_epics_pv_disconnect( &pv );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mx_epics_internal_caput_nowait( MX_EPICS_PV *pv,
				long epics_type,
				unsigned long num_elements,
				void *data_buffer,
				int max_retries )
{
	static const char fname[] = "mx_epics_internal_caput_nowait()";

	int epics_status;
	mx_status_type mx_status;

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_TIMING measurement;
#endif

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}
	if ( data_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_buffer pointer passed was NULL." );
	}
	if ( pv->channel_id == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,

				/* Wait to see if the channel reconnects. */
		"The channel_id passed for EPICS PV '%s' was NULL.",
			pv->pvname );
	}

	if ( mx_epics_debug_flag ) {
		if ( num_elements > 1 ) {
			if ( epics_type == MX_CA_STRING ) {
				MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
					fname, (char *) data_buffer,
					pv->pvname ));
			} else {
				MX_DEBUG(-2,
				("%s: sending multielement array to '%s'.",
					fname, pv->pvname ));
			}
		} else {
			char byte_value;
			short short_value;
			int32_t int32_value;
			float float_value;
			double double_value;
	
			switch( epics_type ) {
			case MX_CA_CHAR:
				byte_value = *((char *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %d to '%s'.",
					fname, byte_value,
					pv->pvname ));
				break;
			case MX_CA_STRING:
				MX_DEBUG(-2,("%s: sending '%s' to '%s'",
					fname, (char *) data_buffer,
					pv->pvname ));
				break;
			case MX_CA_SHORT:
			case MX_CA_ENUM:
				short_value = *((short *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %hd to '%s'.",
					fname, short_value,
					pv->pvname ));
				break;
			case MX_CA_LONG:
				int32_value = *((int32_t *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %ld to '%s'.",
					fname, (long) int32_value,
					pv->pvname ));
				break;
			case MX_CA_FLOAT:
				float_value = *((float *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %g to '%s'.",
					fname, float_value,
					pv->pvname ));
				break;
			case MX_CA_DOUBLE:
				double_value = *((double *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %g to '%s'.",
					fname, double_value,
					pv->pvname ));
				break;
			default:
				MX_DEBUG(-2,
				("%s: '%s' has unsupported epics_type %ld",
					fname, pv->pvname,
					epics_type ));
				break;
			}
		}
	}

#if MX_EPICS_DEBUG_PUT_CALLBACK_STATUS
	MX_DEBUG(-2,("%s: PV '%s' put callback status = %d",
		fname, pv->pvname, pv->put_callback_status));
#endif

	pv->put_callback_status = MXF_EPVH_IDLE;

#if MX_EPICS_DEBUG_PUT_CALLBACK_STATUS
	MX_DEBUG(-2,("%s: Setting PV '%s' put callback status to %d",
		fname, pv->pvname, pv->put_callback_status));
#endif

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_START( measurement );
#endif

	mx_status = mx_epics_poll();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_status = ca_array_put( epics_type, num_elements,
					pv->channel_id, data_buffer );

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, pv->pvname );
#endif

	switch( epics_status ) {
	case ECA_NORMAL:
		break;
	case ECA_BADCHID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"ca_array_put() was called with an "
			"illegal EPICS channel id" );
		break;
	case ECA_BADCOUNT:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of elements %lu for "
			"EPICS PV '%s' is larger than the maximum allowed "
			"(%lu) for this process variable.",
				num_elements, pv->pvname,
			(unsigned long) ca_element_count( pv->channel_id ) );
		break;
	case ECA_BADTYPE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"An illegal EPICS datatype was requested for "
			"EPICS PV '%s'.", pv->pvname );
		break;
	case ECA_STRTOBIG:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"An unusually long string was supplied for a call "
			"to ca_array_put() for EPICS PV '%s'.",
				pv->pvname );
		break;
	case ECA_NOWTACCESS:
		return mx_error( MXE_PERMISSION_DENIED, fname,
			"An attempt to write to read-only EPICS process "
			"variable '%s' failed.",
			pv->pvname );
		break;
	case ECA_DISCONN:
	case ECA_DISCONNCHID:
		return mx_epics_internal_handle_channel_disconnection( 
						fname,
						MXF_EPICS_INTERNAL_CAPUT_NOWAIT,
						pv,
						epics_type,
						num_elements,
						data_buffer,
						0.0,
						max_retries,
						FALSE );
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"A ca_array_put() call failed for EPICS "
			"PV '%s'.  EPICS status = %d, EPICS error = '%s'.",
				pv->pvname,
				epics_status,
				ca_message( epics_status ) );
		break;
	}

	/* Allow background events to execute. */

	mx_status = mx_epics_poll();
		
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_caput_nowait( MX_EPICS_PV *pv,
		long epics_type,
		unsigned long num_elements,
		void *data_buffer )
{
	static const char fname[] = "mx_caput_nowait()";

	mx_status_type mx_status;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

	if ( pv->channel_id == NULL ) {
		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_epics_internal_caput_nowait( pv, epics_type,
						num_elements, data_buffer, 1 );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_caput_nowait_by_name( char *pvname,
			long epics_type,
			unsigned long num_elements,
			void *data_buffer )
{
	static const char fname[] = "mx_caput_nowait_by_name()";

	MX_EPICS_PV pv;
	mx_status_type mx_status;

	if ( pvname == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pvname pointer passed was NULL." );
	}

	mx_epics_pvname_init( &pv, pvname );

	mx_status = mx_epics_pv_connect( &pv, MXF_EPVC_WAIT_FOR_CONNECTION );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_epics_internal_caput_nowait( &pv, epics_type,
						num_elements, data_buffer, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_epics_pv_disconnect( &pv );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mx_epics_internal_handle_channel_disconnection( const char *calling_fname,
						int function_type,
						MX_EPICS_PV *pv,
						long epics_type,
						unsigned long num_elements,
						void *data_buffer,
						double timeout,
						int max_retries,
						int quiet )
{
	static const char fname[] =
		"mx_epics_internal_handle_channel_disconnection()";

	struct timespec current_time, timeout_time;
	int time_comparison, timed_out;
	unsigned long i;
	mx_status_type mx_status;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

#if MX_EPICS_DEBUG_HANDLERS
	MX_DEBUG(-2,
    ("%s invoked for EPICS PV '%s', function type %d, connection_state = %ld",
		fname, pv->pvname, function_type, pv->connection_state ));
#endif

	/* Wait to see if the channel reconnects.  We timeout if the
	 * reconnection takes too long.
	 */

	timeout_time = mx_add_high_resolution_times(
					pv->connection_state_last_change_time,
					pv->reconnect_timeout_interval );

	timed_out = FALSE;

#if MX_EPICS_DEBUG_HANDLERS
	MX_DEBUG(-2,("%s: checking for automatic reconnection.", fname));
#endif
						
	for ( i = 0; ; i++ ) {

		/* Has the EPICS process variable reconnected? */

		mx_status = mx_epics_poll();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If so, then break out of this while loop. */

		if ( pv->connection_state == CA_OP_CONN_UP ) {

#if MX_EPICS_DEBUG_HANDLERS
			MX_DEBUG(-2,
			("%s: successfully reconnected to PV '%s', i = %lu.",
				fname, pv->pvname, i ));
#endif

			break;			/* Exit the while() loop. */
		}

		/* If not, have we timed out yet? */

		current_time = mx_high_resolution_time();

		time_comparison = mx_compare_high_resolution_times(
						current_time, timeout_time );

		if ( time_comparison >= 0 ) {
			timed_out = TRUE;

#if MX_EPICS_DEBUG_HANDLERS
			MX_DEBUG(-2,("%s: PV '%s' reconnection timeout "
				"of %g seconds has expired, i = %lu.",
				fname, pv->pvname,
				mx_convert_high_resolution_time_to_seconds(
					pv->reconnect_timeout_interval ), i ));
#endif

			break;			/* Exit the while() loop. */
		}
	}

	/* If automatic reconnection has failed, try to reconnect manually. */

	if ( timed_out ) {

#if MX_EPICS_DEBUG_HANDLERS
		MX_DEBUG(-2,("%s: attempting manual reconnection.", fname));
#endif

		/* Discard the old channel id. */

		(void) ca_clear_channel( pv->channel_id );

		pv->channel_id = NULL;

		if ( max_retries <= 0 ) {
			return mx_error( MXE_NETWORK_CONNECTION_LOST,
			    calling_fname, "The connection to EPICS PV '%s' "
				"has been disconnected.", pv->pvname );
		}

		/* Try to reconnect to the PV. */

		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS ) {
			return mx_error( MXE_NETWORK_CONNECTION_LOST,
			    calling_fname, "The connection to EPICS PV '%s' "
				"has been disconnected and "
				"an attempt to reconnect failed.",
				pv->pvname );
		}
	}

	/* Try the EPICS command again. */

#if MX_EPICS_DEBUG_HANDLERS
	MX_DEBUG(-2,("%s: invoking the EPICS command again, max_retries = %d",
		fname, max_retries));
#endif

	switch( function_type ) {
	case MXF_EPICS_INTERNAL_CAGET:
		mx_status = mx_epics_internal_caget( pv,
						epics_type,
						num_elements,
						data_buffer,
						timeout,
						max_retries - 1 );
		break;
	case MXF_EPICS_INTERNAL_CAPUT:
		mx_status = mx_epics_internal_caput( pv,
						epics_type,
						num_elements,
						data_buffer,
						timeout,
						max_retries - 1,
						MXI_EPICS_CAPUT_NOWAIT,
						NULL, NULL );
		break;
	case MXF_EPICS_INTERNAL_CAPUT_NOWAIT:
		mx_status = mx_epics_internal_caput_nowait( pv,
						epics_type,
						num_elements,
						data_buffer,
						max_retries - 1 );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"An illegal function_type %d was passed for EPICS PV '%s'.",
			function_type, pv->pvname );
		break;
	}

#if MX_EPICS_DEBUG_HANDLERS
	MX_DEBUG(-2,
	("%s: returned from invoking the EPICS command again, max_retries = %d",
		fname, max_retries));
#endif

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT long
mx_epics_pv_get_field_type( MX_EPICS_PV *pv )
{
	static const char fname[] = "mx_epics_pv_get_field_type()";

	long field_type;
	mx_status_type mx_status;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_EPICS_PV pointer passed was NULL." );
		return -1;
	}

	if ( pv->channel_id == NULL ) {
		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS )
			return -1;
	}

	field_type = ca_field_type( pv->channel_id );

	return field_type;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT unsigned long
mx_epics_pv_get_element_count( MX_EPICS_PV *pv )
{
	static const char fname[] = "mx_epics_pv_get_element_count()";

	unsigned long element_count;
	mx_status_type mx_status;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_EPICS_PV pointer passed was NULL." );
		return 0;
	}

	if ( pv->channel_id == NULL ) {
		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS )
			return 0;
	}

	element_count = ca_element_count( pv->channel_id );

	return element_count;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT size_t
mx_epics_get_element_size( long field_type )
{
	size_t element_size;

	switch( field_type ) {
	case MX_CA_STRING:
		element_size = MXU_EPICS_STRING_LENGTH + 1;
		break;
	case MX_CA_CHAR:
		element_size = sizeof(char);
		break;
	case MX_CA_SHORT:
	case MX_CA_ENUM:
		element_size = sizeof(short);
		break;
	case MX_CA_LONG:
		element_size = sizeof(int32_t);
		break;
	case MX_CA_FLOAT:
		element_size = sizeof(float);
		break;
	case MX_CA_DOUBLE:
		element_size = sizeof(double);
		break;
	default:
		element_size = 0;
		break;
	}

	return element_size;
}

/*--------------------------------------------------------------------------*/

static void
mx_epics_subscription_callback_function( struct event_handler_args args )
{
#if MX_EPICS_DEBUG_HANDLERS
	static const char fname[] = "mx_epics_subscription_callback_function()";
#endif

	MX_EPICS_CALLBACK *callback;
	mx_status_type (*callback_function)( MX_EPICS_CALLBACK *, void * );

	callback = args.usr;

#if MX_EPICS_DEBUG_HANDLERS
	MX_DEBUG(-2,("%s invoked for PV '%s'", fname, callback->pv->pvname));
#endif

	callback->epics_type = args.type;
	callback->epics_count = args.count;
	callback->value_ptr = args.dbr;
	callback->epics_status = args.status;

	callback_function = callback->callback_function;

	if ( callback_function != NULL ) {
		(void) (*callback_function)( callback,
						callback->callback_argument );
	}

	return;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_add_callback( MX_EPICS_PV *pv,
		unsigned long requested_callback_mask,
		mx_status_type ( *callback_function )
				( MX_EPICS_CALLBACK *, void * ),
		void *callback_argument,
		MX_EPICS_CALLBACK **callback_object )
{
	static const char fname[] = "mx_epics_add_callback()";

	chid channel_id;
	evid event_id;
	int epics_status;
	mx_status_type mx_status;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

#if MX_EPICS_DEBUG_IO
	MX_DEBUG(-2,("%s invoked for PV '%s'", fname, pv->pvname));
#endif

	if ( pv->channel_id == NULL ) {
		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	channel_id = pv->channel_id;

	*callback_object = (MX_EPICS_CALLBACK *)
				malloc( sizeof(MX_EPICS_CALLBACK) );

	if ( (*callback_object) == (MX_EPICS_CALLBACK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate "
		"an MX_EPICS_CALLBACK structure." );
	}

	(*callback_object)->pv = pv;
	(*callback_object)->callback_function = callback_function;
	(*callback_object)->callback_argument = callback_argument;

#if MX_EPICS_DEBUG_IO
	MX_DEBUG(-2,("%s: About to call ca_create_subscription( "
		"%d, %lu, %p, %#lx, %p, %p, &event_id)", fname,
		ca_field_type(channel_id), ca_element_count(channel_id),
		channel_id, requested_callback_mask,
		mx_epics_subscription_callback_function,
		*callback_object));
#endif

	epics_status = ca_create_subscription(
					ca_field_type(channel_id),
					ca_element_count(channel_id),
					channel_id,
					requested_callback_mask,
					mx_epics_subscription_callback_function,
					*callback_object,
					&event_id );

	switch( epics_status ) {
	case ECA_NORMAL:
		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	case ECA_BADCHID:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The channel id %p for EPICS PV '%s' "
				"is invalid.", channel_id, pv->pvname );
		break;
	case ECA_BADTYPE:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The requested EPICS type %d for "
				"EPICS PV '%s' is invalid.",
					ca_field_type(channel_id),
					pv->pvname );
		break;
	case ECA_ALLOCMEM:
		mx_status = mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to set up "
				"an event subscription for EPICS PV '%s'.",
					pv->pvname );
		break;
	case ECA_ADDFAIL:
		mx_status = mx_error( MXE_FUNCTION_FAILED, fname,
				"A local database event add failed "
				"for EPICS PV '%s'.", pv->pvname );
		break;
	default:
		mx_status = mx_error( MXE_FUNCTION_FAILED, fname,
				"The attempt to create a callback for "
				"EPICS PV '%s' failed.  "
				"EPICS status = %d, EPICS error = '%s'.",
				pv->pvname,
				epics_status, ca_message( epics_status ) );
		break;
	}

#if MX_EPICS_DEBUG_IO
	MX_DEBUG(-2,("%s: ca_create_subscription(), status = %d",
		fname, epics_status));
#endif

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( *callback_object );
		return mx_status;
	}

	(*callback_object)->event_id = event_id;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_delete_callback( MX_EPICS_CALLBACK *callback )
{
	static const char fname[] = "mx_epics_delete_callback()";

	int epics_status;
	mx_status_type mx_status;

	epics_status = ca_clear_subscription( callback->event_id );

	switch( epics_status ) {
	case ECA_NORMAL:
		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	case ECA_BADCHID:
		mx_status =  mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The EPICS event id %p for EPICS callback %p "
				"is not a currently valid EPICS event id.",
				callback->event_id, callback );
		break;
	default:
		mx_status = mx_error( MXE_FUNCTION_FAILED, fname,
				"The deletion of EPICS callback %p with "
				"EPICS event id %p failed.  "
				"EPICS status = %d, EPICS error = '%s'.",
				callback, callback->event_id,
				epics_status, ca_message( epics_status ) );
		break;
	}

	mx_free( callback );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_start_group( MX_EPICS_GROUP *epics_group )
{
	static const char fname[] = "mx_epics_start_group()";

	CA_SYNC_GID group_id;
	int epics_status;
	mx_status_type mx_status;

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_TIMING measurement;
#endif

	if ( epics_group == (MX_EPICS_GROUP *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_GROUP pointer passed was NULL." );
	}

	if ( mx_epics_is_initialized == FALSE ) {
		mx_status = mx_epics_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_START( measurement );
#endif

	epics_status = ca_sg_create( &group_id );

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	switch( epics_status ) {
	case ECA_NORMAL:
		break;
	case ECA_ALLOCMEM:
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory trying to create an EPICS synchronous group." );
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Creation of an EPICS synchronous group failed.  "
		"EPICS status = %d, EPICS error = '%s'.",
		epics_status, ca_message( epics_status ) );
	}

	epics_group->group_id = group_id;

	epics_group->caget_pv_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

typedef struct {
	MX_EPICS_PV *pv;
	long epics_type;
	unsigned long num_elements;
	void *data_buffer;
} mxp_epics_group_caget_entry;


static mx_status_type
mx_epics_group_caget_traverse_fn( MX_LIST_ENTRY *list_entry,
				void *unused_ptr1,
				void **unused_ptr2 )
{
	static const char fname[] = "group_caget";

	mxp_epics_group_caget_entry *caget_entry;

	caget_entry = list_entry->list_entry_data;

	mx_epics_show_caget_value( caget_entry->pv,
				caget_entry->epics_type,
				caget_entry->num_elements,
				caget_entry->data_buffer,
				fname );

	mx_free( caget_entry );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_end_group( MX_EPICS_GROUP *epics_group )
{
	static const char fname[] = "mx_epics_end_group()";

	CA_SYNC_GID group_id;
	int epics_status;
	mx_status_type mx_status;

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_TIMING measurement;
#endif

	if ( epics_group == (MX_EPICS_GROUP *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_GROUP pointer passed was NULL." );
	}

	group_id = epics_group->group_id;

	/* Wait for the synchronous group to finish running. */

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_START( measurement );
#endif

	mx_status = mx_epics_poll();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_status = ca_sg_block( group_id, mx_epics_io_timeout_interval );

	switch( epics_status ) {
	case ECA_NORMAL:
		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	case ECA_TIMEOUT:
		mx_status = mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %g seconds for "
			"synchronous group %u to finish running.",
				mx_epics_io_timeout_interval,
				(unsigned int) group_id );
		break;
	case ECA_EVDISALLOW:
		mx_status = mx_error( MXE_UNSUPPORTED, fname,
			"Cannot use synchronous group requests inside "
			"an EPICS event handler." );
		break;
	case ECA_BADSYNCGRP:
		mx_status = mx_error( MXE_NOT_FOUND, fname,
			"An attempt to use EPICS synchronous group %u failed "
			"since no such synchronous group currently exists.",
				(unsigned int) group_id );
		break;
	default:
		mx_status = mx_error( MXE_FUNCTION_FAILED, fname,
			"A failure occurred while waiting for EPICS "
			"synchronous group %u to complete execution.  "
			"EPICS status = %d, EPICS error = '%s'.",
				group_id, epics_status,
				ca_message( epics_status ) );
		break;
	}

	/* Delete the synchronous group, unless we got a ECA_BADSYNCGRP
	 * error code from ca_sg_block() above.
	 */

	if ( epics_status != ECA_BADSYNCGRP ) {

		epics_status = ca_sg_delete( group_id );

		if ( epics_status != ECA_NORMAL ) {
			mx_status = mx_error( MXE_FUNCTION_FAILED, fname,
			"A failure occurred while trying to delete EPICS "
			"synchronous group %u.  "
			"EPICS status = %d, EPICS error = '%s'.",
				group_id, epics_status,
				ca_message( epics_status ) );
		}
	}

	if ( mx_epics_debug_flag ) {
		if ( epics_group->caget_pv_list != NULL ) {

			MX_DEBUG(-2,("%s: group caget start.", fname));

			mx_status = mx_list_traverse(
					epics_group->caget_pv_list,
					mx_epics_group_caget_traverse_fn,
					NULL, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			MX_DEBUG(-2,("%s: group caget end.", fname));
		}
	}

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_delete_group( MX_EPICS_GROUP *epics_group )
{
	static const char fname[] = "mx_epics_delete_group()";

	CA_SYNC_GID group_id;
	int epics_status;
	mx_status_type mx_status;

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_TIMING measurement;
#endif

	if ( epics_group == (MX_EPICS_GROUP *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_GROUP pointer passed was NULL." );
	}

	group_id = epics_group->group_id;

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_START( measurement );
#endif

	epics_status = ca_sg_delete( group_id );

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif
	if ( epics_group->caget_pv_list != NULL ) {
		(void) mx_list_destroy( epics_group->caget_pv_list );

		epics_group->caget_pv_list = NULL;
	}

	switch( epics_status ) {
	case ECA_NORMAL:
		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	case ECA_BADSYNCGRP:
		mx_status = mx_error( MXE_NOT_FOUND, fname,
		"An attempt to delete EPICS synchronous group %u failed "
		"since no such synchronous group currently exists.",
			(unsigned int) group_id );
		break;
	default:
		mx_status = mx_error( MXE_FUNCTION_FAILED, fname,
		"A failure occurred while trying to delete EPICS "
		"synchronous group %u.  EPICS status = %d, EPICS error = '%s'.",
			group_id, epics_status, ca_message( epics_status ) );
		break;
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mx_epics_internal_group_caget( CA_SYNC_GID group_id,
				MX_EPICS_PV *pv,
				long epics_type,
				unsigned long num_elements,
				void *data_buffer )
{
	static const char fname[] = "mx_epics_internal_group_caget()";

	int epics_status;
	mx_status_type mx_status;

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_TIMING measurement;
#endif

	if ( pv == ( MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}
	if ( data_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_buffer pointer passed was NULL." );
	}
	if ( pv->channel_id == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The channel_id passed for EPICS PV '%s' was NULL.",
			pv->pvname );
	}

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_START( measurement );
#endif

	epics_status = ca_sg_array_get( group_id, epics_type,
					num_elements, pv->channel_id,
					data_buffer );

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, pv->pvname );
#endif

#if MX_EPICS_DEBUG_IO
	MX_DEBUG(-2,("%s: PV '%s', epics_status = %d",
			fname, pv->pvname, epics_status));
#endif

	switch( epics_status ) {
	case ECA_NORMAL:
		break;
	case ECA_BADCHID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"ca_sg_array_get() was called with an "
			"illegal EPICS channel id" );
		break;
	case ECA_BADCOUNT:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of elements %lu for "
			"EPICS PV '%s' is larger than the maximum allowed "
			"(%lu) for this process variable.",
				num_elements, pv->pvname,
			(unsigned long) ca_element_count( pv->channel_id ) );
		break;
	case ECA_BADTYPE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"An illegal EPICS datatype was requested for "
			"EPICS PV '%s'.", pv->pvname );
		break;
	case ECA_BADSYNCGRP:
		return mx_error( MXE_NOT_FOUND, fname,
			"An attempt to use EPICS synchronous group %u failed "
			"since no such synchronous group currently exists.",
				(unsigned int) group_id );
		break;
	case ECA_GETFAIL:
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"An attempt to read the value of local EPICS "
			"database variable '%s' failed.",
				pv->pvname );
		break;
	case ECA_DISCONN:
	case ECA_DISCONNCHID:
		return mx_error( MXE_NETWORK_CONNECTION_LOST, fname,
			"The connection to EPICS PV '%s' "
			"has been disconnected.",
			pv->pvname );
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"A ca_sg_array_get() call failed for EPICS PV '%s'.  "
			"EPICS status = %d, EPICS error = '%s'.",
				pv->pvname,
				epics_status,
				ca_message( epics_status ) );
		break;
	}

	/* Allow background events to execute. */

	mx_status = mx_epics_poll();
		
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_group_caget( MX_EPICS_GROUP *epics_group,
		MX_EPICS_PV *pv,
		long epics_type,
		unsigned long num_elements,
		void *data_buffer )
{
	static const char fname[] = "mx_group_caget()";

	MX_LIST_ENTRY *list_entry;
	mx_status_type mx_status;

	if ( epics_group == (MX_EPICS_GROUP *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_GROUP pointer passed was NULL." );
	}

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

	if ( pv->channel_id == NULL ) {
		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_epics_internal_group_caget( epics_group->group_id,
						pv, epics_type,
						num_elements, data_buffer );

	if ( mx_status.code == MXE_NETWORK_CONNECTION_LOST ) {
		(void) ca_clear_channel( pv->channel_id );

		pv->channel_id = NULL;
	}

	if ( mx_epics_debug_flag ) {
		mxp_epics_group_caget_entry *caget_entry;

		if ( epics_group->caget_pv_list == NULL ) {
			mx_status = mx_list_create(
					&(epics_group->caget_pv_list) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		caget_entry = malloc( sizeof(mxp_epics_group_caget_entry) );

		if ( caget_entry == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"mxp_epics_group_caget_entry structure." );
		}

		caget_entry->pv = pv;
		caget_entry->epics_type = epics_type;
		caget_entry->num_elements = num_elements;
		caget_entry->data_buffer = data_buffer;

		mx_status = mx_list_entry_create( &list_entry,
						caget_entry, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_list_add_entry( epics_group->caget_pv_list,
						list_entry );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mx_epics_internal_group_caput( CA_SYNC_GID group_id,
				MX_EPICS_PV *pv,
				long epics_type,
				unsigned long num_elements,
				void *data_buffer )
{
	static const char fname[] = "mx_epics_internal_group_caput()";

	int epics_status;
	mx_status_type mx_status;

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_TIMING measurement;
#endif

	if ( pv == ( MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}
	if ( data_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_buffer pointer passed was NULL." );
	}
	if ( pv->channel_id == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The channel_id passed for EPICS PV '%s' was NULL.",
			pv->pvname );
	}

	if ( mx_epics_debug_flag ) {
		if ( num_elements > 1 ) {
			if ( epics_type == MX_CA_STRING ) {
				MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
					fname, (char *) data_buffer,
					pv->pvname ));
			} else {
				MX_DEBUG(-2,
				("%s: sending multielement array to '%s'.",
					fname, pv->pvname ));
			}
		} else {
			char byte_value;
			short short_value;
			int32_t int32_value;
			float float_value;
			double double_value;
	
			switch( epics_type ) {
			case MX_CA_CHAR:
				byte_value = *((char *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %d to '%s'.",
					fname, byte_value,
					pv->pvname ));
				break;
			case MX_CA_STRING:
				MX_DEBUG(-2,("%s: sending '%s' to '%s'",
					fname, (char *) data_buffer,
					pv->pvname ));
				break;
			case MX_CA_SHORT:
			case MX_CA_ENUM:
				short_value = *((short *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %hd to '%s'.",
					fname, short_value,
					pv->pvname ));
				break;
			case MX_CA_LONG:
				int32_value = *((int32_t *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %ld to '%s'.",
					fname, (long) int32_value,
					pv->pvname ));
				break;
			case MX_CA_FLOAT:
				float_value = *((float *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %g to '%s'.",
					fname, float_value,
					pv->pvname ));
				break;
			case MX_CA_DOUBLE:
				double_value = *((double *) data_buffer);
	
				MX_DEBUG(-2,("%s: sending %g to '%s'.",
					fname, double_value,
					pv->pvname ));
				break;
			default:
				MX_DEBUG(-2,
				("%s: '%s' has unsupported epics_type %ld",
					fname, pv->pvname,
					epics_type));
				break;
			}
		}
	}

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_START( measurement );
#endif

	epics_status = ca_sg_array_put( group_id, epics_type, num_elements,
					pv->channel_id, data_buffer );

#if MX_EPICS_DEBUG_PERFORMANCE
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, pv->pvname );
#endif

#if MX_EPICS_DEBUG_IO
	MX_DEBUG(-2,("%s: PV '%s', epics_status = %d",
			fname, pv->pvname, epics_status));
#endif

	switch( epics_status ) {
	case ECA_NORMAL:
		break;
	case ECA_BADCHID:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"ca_sg_array_put() was called with an "
			"illegal EPICS channel id" );
		break;
	case ECA_BADCOUNT:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of elements %lu for "
			"EPICS PV '%s' is larger than the maximum allowed "
			"(%lu) for this process variable.",
				num_elements, pv->pvname,
			(unsigned long) ca_element_count( pv->channel_id ) );
		break;
	case ECA_BADTYPE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"An illegal EPICS datatype was requested for "
			"EPICS PV '%s'.", pv->pvname );
		break;
	case ECA_BADSYNCGRP:
		return mx_error( MXE_NOT_FOUND, fname,
			"An attempt to use EPICS synchronous group %u failed "
			"since no such synchronous group currently exists.",
				(unsigned int) group_id );
		break;
	case ECA_STRTOBIG:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"An unusually long string was supplied for a call "
			"to ca_sg_array_put() for EPICS PV '%s'.",
				pv->pvname );
		break;
	case ECA_PUTFAIL:
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"An attempt to write to the value of local EPICS "
			"database variable '%s' failed.",
				pv->pvname );
		break;
	case ECA_DISCONN:
	case ECA_DISCONNCHID:
		return mx_error( MXE_NETWORK_CONNECTION_LOST, fname,
			"The connection to EPICS PV '%s' "
			"has been disconnected.",
			pv->pvname );
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"A ca_sg_array_put() call failed for EPICS PV '%s'.  "
			"EPICS status = %d, EPICS error = '%s'.",
				pv->pvname,
				epics_status,
				ca_message( epics_status ) );
		break;
	}

	/* Allow background events to execute. */

	mx_status = mx_epics_poll();
		
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_group_caput( MX_EPICS_GROUP *epics_group,
		MX_EPICS_PV *pv,
		long epics_type,
		unsigned long num_elements,
		void *data_buffer )
{
	static const char fname[] = "mx_group_caput()";

	mx_status_type mx_status;

	if ( pv == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PV pointer passed was NULL." );
	}

	if ( pv->channel_id == NULL ) {
		mx_status = mx_epics_pv_connect( pv,
					MXF_EPVC_WAIT_FOR_CONNECTION );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_epics_internal_group_caput( epics_group->group_id,
						pv, epics_type,
						num_elements, data_buffer );

	if ( mx_status.code == MXE_NETWORK_CONNECTION_LOST ) {
		(void) ca_clear_channel( pv->channel_id );

		pv->channel_id = NULL;
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_get_pv_type( char *pvname,
			long *epics_type,
			long *epics_array_length )
{
	static const char fname[] = "mx_epics_get_pv_type()";

	MX_EPICS_PV pv;
	chid channel_id;
	mx_status_type mx_status;

	if ( pvname == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pvname pointer passed was NULL." );
	}
	if ( epics_type == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The epics_type pointer passed was NULL." );
	}
	if ( epics_array_length == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The epics_array_length pointer passed was NULL." );
	}

	/* Connect to the EPICS PV. */

	mx_epics_pvname_init( &pv, pvname );

	mx_status = mx_epics_pv_connect( &pv, MXF_EPVC_WAIT_FOR_CONNECTION );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the type and length. */

	channel_id = pv.channel_id;

	*epics_type = ca_field_type( channel_id );

	*epics_array_length = ca_element_count( channel_id );

	/* We are done, so disconnect. */

	mx_status = mx_epics_pv_disconnect( &pv );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_epics_convert_mx_type_to_epics_type( long mx_type,
					chtype *epics_type )
{
	static const char fname[] = "mx_epics_convert_mx_type_to_epics_type()";

	switch( mx_type ) {
	case MXFT_STRING:
		*epics_type = DBR_STRING;
		break;
	case MXFT_CHAR:
	case MXFT_UCHAR:
		*epics_type = DBR_CHAR;
		break;
	case MXFT_SHORT:
	case MXFT_USHORT:
		*epics_type = DBR_SHORT;
		break;
	case MXFT_BOOL:
	case MXFT_LONG:
	case MXFT_ULONG:
	case MXFT_HEX:
		*epics_type = DBR_LONG;
		break;
	case MXFT_FLOAT:
		*epics_type = DBR_FLOAT;
		break;
	case MXFT_DOUBLE:
		*epics_type = DBR_DOUBLE;
		break;
	case MXFT_INT64:
		return mx_error( MXE_UNSUPPORTED, fname,
			"MX record field type MXFT_INT64 cannot be converted "
			"to an EPICS type." );
	case MXFT_UINT64:
		return mx_error( MXE_UNSUPPORTED, fname,
			"MX record field type MXFT_UINT64 cannot be converted "
			"to an EPICS type." );
	default:
		*epics_type = -1;
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"MX record field type of %ld cannot be converted to an EPICS type.",
			mx_type );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_epics_convert_epics_type_to_mx_type( chtype epics_type,
					long *mx_type )
{
	static const char fname[] = "mx_epics_convert_epics_type_to_mx_type()";

	switch( epics_type ) {
	case DBR_STRING:
		*mx_type = MXFT_STRING;
		break;
	case DBR_CHAR:
		*mx_type = MXFT_CHAR;
		break;
	case DBR_SHORT:
		*mx_type = MXFT_SHORT;
		break;
	case DBR_LONG:
		*mx_type = MXFT_LONG;
		break;
	case DBR_FLOAT:
		*mx_type = MXFT_FLOAT;
		break;
	case DBR_DOUBLE:
		*mx_type = MXFT_DOUBLE;
		break;
	default:
		*mx_type = -1;
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"EPICS type of %ld cannot be converted to an MX record field type.",
			*mx_type );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

