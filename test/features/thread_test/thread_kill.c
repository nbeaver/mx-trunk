#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_key.h"
#include "mx_thread.h"

static mx_status_type
child_function( MX_THREAD *thread, void *args )
{
	fprintf( stderr, "Child: Child thread starting.\n" );

	while(1) {
		fprintf( stderr, "Child: Waiting to die.\n" );

		mx_msleep(500);
	}

#if !defined(OS_SOLARIS)
	/* We should never get here. */

	return MX_SUCCESSFUL_RESULT;
#endif
}

int
main( int argc, char *argv[] )
{
	MX_THREAD *child_thread;
	long thread_exit_status;
	mx_status_type mx_status;

	fprintf( stderr, "Parent: Creating child thread.\n" );

	mx_status = mx_thread_create( &child_thread, child_function, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "Parent: Hit any key to kill the child thread.\n" );

	while(1) {
		if ( mx_kbhit() ) {
			break;		/* Exit the while() loop. */
		}

		mx_msleep(100);
	}

	fprintf( stderr, "Parent: Killing the child thread.\n" );

	mx_status = mx_thread_kill( child_thread );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "Parent: Waiting for the child thread to die.\n" );

	mx_status = mx_thread_wait( child_thread,
					&thread_exit_status,
					MX_THREAD_INFINITE_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "Parent: Child thread exit status = %ld.\n",
						thread_exit_status );

	fprintf( stderr, "Parent: Freeing child data structures.\n" );

	mx_status = mx_thread_free_data_structures( child_thread );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Announce to the user that the test is over. */

	fprintf( stderr, "Parent: Thread test completed successfully.\n" );

	exit(0);
}

