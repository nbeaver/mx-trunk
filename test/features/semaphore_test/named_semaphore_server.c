#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_semaphore.h"

int
main( int argc, char *argv[] )
{
	MX_SEMAPHORE *semaphore;
	long initial_value, current_value;
	unsigned long status_code;
	char *semaphore_name;
	mx_status_type mx_status;

	if ( argc != 3 ) {
		fprintf(stderr, "\nUsage: %s initial_value semaphore_name\n\n",
					argv[0] );
		exit(1);
	}

	mx_set_debug_level(0);

	initial_value = atol( argv[1] );;
	semaphore_name = argv[2];

	if ( initial_value < 1 ) {
		fprintf(stderr,
			"Server: The initial semaphore value must be >= 1.\n" );
		exit(1);
	}

	fprintf(stderr,
		"Server: Creating semaphore '%s' with value = %ld.\n",
				semaphore_name, initial_value );

	mx_status = mx_semaphore_create( &semaphore,
					(unsigned long) initial_value,
					semaphore_name );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( stderr,
		"mx_semaphore_create() returned with a status of %ld.\n"
		"Exiting...\n", mx_status.code );

		exit( mx_status.code );
	}

	fprintf(stderr, "Server: Locking the semaphore.\n" );

	status_code = mx_semaphore_lock( semaphore );

	if ( status_code != MXE_SUCCESS ) {
		fprintf( stderr,
		"mx_semaphore_lock() returned with a status of %ld.\n"
		"Exiting...\n", status_code );

		exit( status_code );
	}

	while (1) {
		mx_status = mx_semaphore_get_value( semaphore, &current_value );

		if ( mx_status.code != MXE_SUCCESS ) {
			fprintf( stderr,
		"mx_semaphore_get_value() returned with a status of %ld.\n"
			"Exiting...\n", mx_status.code );

			exit( mx_status.code );
		}

		fprintf(stderr, "Server: Semaphore value = %ld\n",
						current_value );

		if ( current_value >= initial_value ) {
			break;		/* Exit the while loop. */
		}

		mx_msleep(500);
	}

	/* Destroy the semaphore. */

	fprintf( stderr, "Server: Destroying semaphore.\n" );

	mx_status = mx_semaphore_destroy( semaphore );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( stderr,
		"mx_semaphore_destroy() returned with a status of %ld.\n"
		"Exiting...\n", mx_status.code );

		exit( mx_status.code );
	}
	
	/* Announce to the user that the test is over. */

	fprintf(stderr, "Server: Semaphore test completed successfully.\n");

	exit(0);
}

