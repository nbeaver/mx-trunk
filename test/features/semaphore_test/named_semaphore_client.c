#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_semaphore.h"

int
main( int argc, char *argv[] )
{
	MX_SEMAPHORE *semaphore;
	unsigned long process_id, status_code;
	long change_value;
	char *semaphore_name;
	mx_status_type mx_status;

	if ( argc != 3 ) {
		fprintf(stderr, "\nUsage: %s value_change semaphore_name\n\n",
					argv[0] );
		exit(1);
	}

	mx_set_debug_level(0);

	process_id = mx_process_id();

	change_value = atoi( argv[1] );;
	semaphore_name = argv[2];

	fprintf(stderr, "Client %lu: Connecting to semaphore '%s'\n",
				process_id, semaphore_name );

	mx_status = mx_semaphore_open( &semaphore, semaphore_name );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( stderr,
		"mx_semaphore_open() returned with a status of %ld.\n"
		"Exiting...\n", mx_status.code );

		exit( mx_status.code );
	}

	if ( change_value > 0 ) {
		fprintf(stderr, "Client %lu: Unlocking the semaphore.\n",
				process_id );

		status_code = mx_semaphore_unlock( semaphore );

		if ( status_code != MXE_SUCCESS ) {
			fprintf( stderr,
		"mx_semaphore_unlock() returned with a status of %ld.\n"
			"Exiting...\n", status_code );

			exit( status_code );
		}
	} else
	if ( change_value < 0 ) {
		fprintf(stderr, "Client %lu: Locking the semaphore.\n",
				process_id );

		status_code = mx_semaphore_lock( semaphore );

		if ( status_code != MXE_SUCCESS ) {
			fprintf( stderr,
		"mx_semaphore_lock() returned with a status of %ld.\n"
			"Exiting...\n", status_code );

			exit( status_code );
		}
	} else {
		fprintf(stderr, "Client %lu: No action requested.\n",
				process_id );
	}

	/* Destroy the semaphore. */

	fprintf( stderr, "Client %lu: Destroying semaphore.\n",
				process_id );

	mx_status = mx_semaphore_destroy( semaphore );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( stderr,
		"mx_semaphore_destroy() returned with a status of %ld.\n"
		"Exiting...\n", mx_status.code );

		exit( mx_status.code );
	}
	
	/* Announce to the user that the test is over. */

	fprintf(stderr, "Client %lu: Semaphore test completed successfully.\n",
				process_id );

	exit(0);
}

