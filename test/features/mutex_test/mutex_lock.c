#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_thread.h"
#include "mx_mutex.h"

typedef struct {
	MX_MUTEX *parent_mutex;
	MX_MUTEX *child_mutex;
} mutex_pair_t;

static mx_status_type
child_function( MX_THREAD *thread, void *args )
{
	mutex_pair_t *mutex_pair;
	long status, exit_status;

	fprintf(stderr, "Child: Child thread starting.\n");

#if 0
	mx_show_thread_info( thread, "Child thread:" );
#endif

	if ( args == NULL ) {
		fprintf(stderr, "Child: args pointer was NULL.  Exiting...\n");
		exit(1);
	}

	mutex_pair = (mutex_pair_t *) args;

	/* Lock the child mutex. */

	fprintf( stderr, "Child: Locking child mutex.\n" );

	status = mx_mutex_lock( mutex_pair->child_mutex );

	if ( status != MXE_SUCCESS ) {
		fprintf( stderr,
"Child: Attempt to lock the child mutex failed with MX status code = %ld.\n",
			status );
		exit( status );
	}

	/* Block waiting for the parent mutex. */

	fprintf( stderr, "Child: Waiting to lock the parent mutex.\n" );

	status = mx_mutex_lock( mutex_pair->parent_mutex );

	if ( status != MXE_SUCCESS ) {
		fprintf( stderr,
"Child: Attempt to lock the parent mutex failed with MX status code = %ld.\n",
			status );
		exit( status );
	}

	/* Once we have acquired the parent mutex, release it. */

	fprintf( stderr,
		"Child: Parent mutex locked.  It will now be unlocked.\n");

	status = mx_mutex_unlock( mutex_pair->parent_mutex );

	if ( status != MXE_SUCCESS ) {
		fprintf( stderr,
"Child: Attempt to unlock the parent mutex failed with MX status code = %ld.\n",
			status );
		exit( status );
	}

	/* Sleep for 5 seconds. */

	fprintf( stderr, "Child: Sleeping for 5 seconds.\n" );

	mx_msleep(5000);

	/* Unlock the child mutex. */

	fprintf( stderr, "Child: Unlocking the child mutex.\n" );

	status = mx_mutex_unlock( mutex_pair->child_mutex );

	if ( status != MXE_SUCCESS ) {
		fprintf( stderr,
"Child: Attempt to unlock the child mutex failed with MX status code = %ld.\n",
			status );
		exit( status );
	}

	exit_status = 123;

	fprintf( stderr,
	"Child: Child thread will now terminate with exit status = %ld.\n",
		exit_status );

	(void) mx_thread_exit( thread, exit_status );

	/* We should never get here. */

	return MX_SUCCESSFUL_RESULT;
}

int
main( int argc, char *argv[] )
{
	mutex_pair_t mutex_pair;
	MX_THREAD *parent_thread, *child_thread;
	long status, thread_exit_status;
	mx_status_type mx_status;

	mx_set_debug_level(0);

	fprintf(stderr, "Parent: child_function = %p\n", child_function);

	mx_status = mx_get_current_thread( &parent_thread );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

#if 0
	mx_show_thread_info( parent_thread, "Parent thread:" );
#endif

	fprintf(stderr, "Parent: Creating parent mutex.\n");

	mx_status = mx_mutex_create( &(mutex_pair.parent_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf(stderr, "Parent: Creating child mutex.\n");

	mx_status = mx_mutex_create( &(mutex_pair.child_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf(stderr, "Parent: Locking parent mutex.\n");

	status = mx_mutex_lock( mutex_pair.parent_mutex );

	if ( status != MXE_SUCCESS ) {
		fprintf(stderr,
"Parent: Attempt to lock the parent mutex failed with MX status code = %ld.\n",
			status );
		exit( status );
	}

	fprintf(stderr, "Parent: Creating child thread.\n" );

	mx_status = mx_thread_create( &child_thread,
					child_function,
					&mutex_pair );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Give the child long enough to lock the child mutex. */

	fprintf(stderr, "Parent: Sleeping for 5 seconds.\n");

	mx_msleep(5000);

	/* Unlock the parent mutex so that the child can proceed. */

	fprintf(stderr, "Parent: Unlocking parent mutex.\n");

	status = mx_mutex_unlock( mutex_pair.parent_mutex );

	if ( status != MXE_SUCCESS ) {
		fprintf( stderr,
		"Parent: Attempt to unlock the parent mutex failed "
			"with MX status code = %ld.\n", status );
		exit( status );
	}

	/* Wait for the child to unlock its mutex. */

	fprintf(stderr, "Parent: Waiting to lock the child mutex.\n");

	status = mx_mutex_lock( mutex_pair.child_mutex );

	if ( status != MXE_SUCCESS ) {
		fprintf( stderr,
"Parent: Attempt to lock the child mutex failed with MX status code = %ld.\n",
			status );
		exit( status );
	}

	fprintf(stderr,
		"Parent: Child mutex locked.  It will now be unlocked.\n" );

	status = mx_mutex_unlock( mutex_pair.child_mutex );

	if ( status != MXE_SUCCESS ) {
		fprintf( stderr,
"Parent: Attempt to unlock the child mutex failed with MX status code = %ld.\n",
			status );
		exit( status );
	}

	/* Wait for the child to finish executing. */

	fprintf(stderr, "Parent: Waiting for the child to exit.\n" );

	mx_status = mx_thread_wait( child_thread,
					&thread_exit_status,
					MX_THREAD_INFINITE_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf(stderr, "Parent: Child thread exit status = %ld.\n",
					thread_exit_status );

	/* Free the data structures used by the child thread. */

	fprintf( stderr, "Parent: Freeing child data structures.\n" );

	mx_status = mx_thread_free_data_structures( child_thread );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );
	
	/* Destroy the parent and child mutexes. */

	fprintf( stderr, "Parent: Destroying parent mutex.\n" );

	mx_status = mx_mutex_destroy( mutex_pair.parent_mutex );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );
	
	fprintf( stderr, "Parent: Destroying child mutex.\n" );

	mx_status = mx_mutex_destroy( mutex_pair.child_mutex );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Announce to the user that the test is over. */

	fprintf(stderr, "Parent: Mutex test completed successfully.\n");

	exit(0);
}

