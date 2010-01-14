#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_coprocess.h"

/*-------------------------------------------------------------------------*/

int
main( int argc, char *argv[] )
{
#if 0
	static const char fname[] = "main()";
#endif

	MX_COPROCESS *coprocess;
	FILE *from_cp, *to_cp;
	char command_line[100];
	char msg[200];
	int i, c, status, saved_errno;
	size_t num_bytes_available;
	mx_status_type mx_status;

	if ( argc < 2 ) {
		fprintf( stderr,
	    "\nUsage: coprocess_test 'test_command' [ 'test_arguments ...]\n" );
		exit(1);
	}

	strlcpy( command_line, argv[1], sizeof(command_line) );

	for ( i = 2; i < argc; i++ ) {
		strlcat( command_line, " ", sizeof(command_line) );
		strlcat( command_line, argv[i], sizeof(command_line) );
	}

	fprintf(stderr, "command_line = '%s'\n", command_line);

	mx_status = mx_coprocess_open( &coprocess, command_line );

	if ( mx_status.code != MXE_SUCCESS )
		exit(mx_status.code);

	fprintf(stderr, "coprocess PID = %lu\n", coprocess->coprocess_pid);

	from_cp = coprocess->from_coprocess;
	to_cp   = coprocess->to_coprocess;

	setvbuf( from_cp, (char *) NULL, _IONBF, 0 );
	setvbuf( to_cp, (char *) NULL, _IONBF, 0 );

	fprintf(stderr, "from_cp = %p, to_cp = %p\n", from_cp, to_cp);

	while (1) {
		mx_msleep(500);

		while (1) {
			mx_status = mx_coprocess_num_bytes_available( coprocess,
							&num_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				exit(mx_status.code);

			if ( num_bytes_available == 0 )
				break;

			for ( i = 0; i < num_bytes_available; i++ ) {
				c = fgetc(from_cp);

				if ( c == EOF ) {
					fprintf( stderr,
				"coprocess has closed its transmit pipe.\n" );
					exit(1);
				}

				fputc( c, stdout );
				fflush( stdout );
			}

			mx_msleep(100);
		}

		printf( "msg> " );
		fflush( stdout );

		fgets( msg, sizeof(msg), stdin );

		if ( feof(stdin) || ferror(stdin) ) {
			break;
		}

		status = fprintf( to_cp, msg );

		if ( status == EOF ) {
			saved_errno = errno;

			fprintf(stderr,
		"Error writing to coprocess pipe.  Errno = %d, reason = '%s'\n",
				saved_errno, strerror(saved_errno) );
			exit(1);
		}
	}

	fprintf(stderr, "coprocess_test complete.\n");

	(void) mx_coprocess_close( coprocess, 5.0 );

	exit(0);
}

