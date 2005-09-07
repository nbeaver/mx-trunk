#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_thread.h"
#include "mx_semaphore.h"

#define SEMAPHORE_INITIAL_VALUE		2
#define NUM_CHILD_THREADS		4
#define PARENT_SLEEP_SECONDS		2.0
#define CHILD_SLEEP_SECONDS		10.0

MX_SEMAPHORE *semaphore;

static mx_status_type
child_function( MX_THREAD *thread, void *args )
{
	char buffer[40];
	int child_number, exit_loop;
	long status;

	if ( args == NULL ) {
		fprintf(stderr, "Child: args pointer was NULL.  Exiting...\n");
		exit(1);
	}

	child_number = *((int *) args);

	fprintf( stderr, "Child %d: Child thread starting.\n", child_number );

	sprintf( buffer, "Child %d thread:", child_number );

	if ( args == NULL ) {
		fprintf(stderr, "Child: args pointer was NULL.  Exiting...\n");
		exit(1);
	}

	/* Busy wait the semaphore. */

	fprintf( stderr,
	"Child %d: Waiting to lock the semaphore.\n", child_number );

	exit_loop = FALSE;

	while( exit_loop == FALSE ) {
		status = mx_semaphore_trylock( semaphore );

		switch( status ) {
		case MXE_SUCCESS:
			exit_loop = TRUE;
			break;
		case MXE_NOT_AVAILABLE:
			fprintf(stderr, "Child %d: Semaphore not available.\n",
				child_number );
			break;
		default:
			fprintf( stderr,
"Child: Attempt to lock the semaphore failed with MX status code = %ld.\n",
				status );
			exit( status );
			break;
		}
		mx_msleep(200);
	}

	/* Sleep for a while. */

	fprintf( stderr, "Child %d: Acquired lock. Sleeping for %g seconds.\n",
		child_number, CHILD_SLEEP_SECONDS );

	mx_msleep( mx_round( 1000.0 * CHILD_SLEEP_SECONDS) );

	/* Unlock the semaphore. */

	fprintf( stderr, "Child %d: Unlocking the semaphore.\n", child_number );

	status = mx_semaphore_unlock( semaphore );

	if ( status != MXE_SUCCESS ) {
		fprintf( stderr,
		"Child %d: Attempt to unlock the child semaphore failed "
		"with MX status code = %ld.\n",
			child_number, status );
		exit( status );
	}

	fprintf( stderr,
	"Child %d: Child thread will now terminate with exit status = %d.\n",
		child_number, child_number );

	(void) mx_thread_exit( thread, (long) child_number );

	/* We should never get here. */

	return MX_SUCCESSFUL_RESULT;
}

int
main( int argc, char *argv[] )
{
	MX_THREAD *parent_thread, *child_thread;
	MX_THREAD *child_thread_array[NUM_CHILD_THREADS];
	int i, num_child_threads, child_thread_number[NUM_CHILD_THREADS];
	long thread_exit_status;
	unsigned long initial_value;
	mx_status_type mx_status;

	mx_set_debug_level(0);

	initial_value = SEMAPHORE_INITIAL_VALUE;
	num_child_threads = NUM_CHILD_THREADS;

	fprintf(stderr, "Parent: child_function = %p\n", child_function);

	mx_status = mx_get_current_thread( &parent_thread );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf(stderr, "Parent: Creating semaphore with value = %lu.\n",
				initial_value );

	mx_status = mx_semaphore_create( &semaphore, initial_value, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Create all of the child threads. */

	for ( i = 1; i <= num_child_threads; i++ ) {

		fprintf(stderr, "Parent: Creating child thread %d.\n", i );

		child_thread_number[i-1] = i;

		mx_status = mx_thread_create( &child_thread,
					child_function,
					&(child_thread_number[i-1]) );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		child_thread_array[i-1] = child_thread;

		/* Sleep for a while. */

		if ( i < num_child_threads ) {
			fprintf(stderr, "Parent: sleeping for %g seconds.\n",
				PARENT_SLEEP_SECONDS );
			mx_msleep( mx_round( 1000.0  * PARENT_SLEEP_SECONDS ) );
		}
	}

	/* Wait for the children to finish executing. */

	for ( i = 1; i <= num_child_threads; i++ ) {
		fprintf(stderr, "Parent: Waiting for child %d to exit.\n", i );

		child_thread = child_thread_array[i-1];

		mx_status = mx_thread_wait( child_thread,
					&thread_exit_status,
					MX_THREAD_INFINITE_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		fprintf(stderr, "Parent: Child thread %d exit status = %ld.\n",
					i, thread_exit_status );

		/* Free the data structures used by the child thread. */

		fprintf( stderr,
			"Parent: Freeing child %d data structures.\n", i );

		mx_status = mx_thread_free_data_structures( child_thread );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );
	}
	
	/* Destroy the semaphore. */

	fprintf( stderr, "Parent: Destroying semaphore.\n" );

	mx_status = mx_semaphore_destroy( semaphore );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );
	
	/* Announce to the user that the test is over. */

	fprintf(stderr, "Parent: Semaphore test completed successfully.\n");

	exit(0);
}

