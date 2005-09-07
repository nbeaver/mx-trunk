#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_mutex.h"

int
main( int argc, char *argv[] )
{
	MX_MUTEX *mutex;
	int i;
	long status;
	mx_status_type mx_status;

	fprintf(stderr, "Recursive mutex test started.\n");

	fprintf(stderr, "Creating the mutex.\n");

	mx_status = mx_mutex_create( &mutex );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Try to lock the mutex 3 times in a row. */

	for ( i = 0; i < 3; i++ ) {
		fprintf(stderr, "Lock attempt %d\n", i+1);

		status = mx_mutex_lock( mutex );

		if ( status != MXE_SUCCESS ) {
			fprintf(stderr,
		"Attempt %d to lock the mutex failed with MX status = %ld\n",
				i+1, status );
			exit( status );
		}
	}

	fprintf(stderr, "All lock attempts succeeded.\n");

	/* Try to unlock the mutex 3 times in a row. */

	for ( i = 0; i < 3; i++ ) {
		fprintf(stderr, "Unlock attempt %d\n", i+1);

		status = mx_mutex_unlock( mutex );

		if ( status != MXE_SUCCESS ) {
			fprintf(stderr,
		"Attempt %d to unlock the mutex failed with MX status = %ld\n",
				i+1, status );
			exit( status );
		}
	}

	fprintf(stderr, "All unlock attempts succeeded.\n");

	/* Try to unlock one more time, just to see what happens. */

	fprintf(stderr, "About to attempt to unlock one time too many.\n");
	fprintf(stderr,
		"This _should_ result in an error code of some sort.\n");

	status = mx_mutex_unlock( mutex );

	if ( status == MXE_SUCCESS ) {
		fprintf(stderr,
	"Attempt to unlock the mutex succeeded when it should have failed.\n" );
		exit( MXE_SUCCESS );
	}

	if ( status != MXE_SUCCESS ) {
		fprintf(stderr,
		"Attempt %d to unlock the mutex failed with MX status = %ld\n",
				i+1, status );
		exit( status );
	}

	/* Destroy the mutex. */

	fprintf(stderr, "Destroying the mutex.\n");

	mx_status = mx_mutex_destroy( mutex );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	exit(0);
}

