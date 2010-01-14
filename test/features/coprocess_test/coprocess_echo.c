#include <stdio.h>
#include <ctype.h>

int
main( int argc, char *argv[] )
{
	int c;

	setvbuf( stdin, NULL, _IONBF, 0 );
	setvbuf( stdout, NULL, _IONBF, 0 );

	while (1) {
		c = fgetc( stdin );

		if ( c == EOF ) {
			fprintf(stderr,"\ncoprocess_echo exiting.\n");
			exit(0);
		}

		if ( isupper(c) ) {
			c = tolower(c);
		} else
		if ( islower(c) ) {
			c = toupper(c);
		}

		fputc( c, stdout );
	}
}

