#include <stdio.h>
#include <stdlib.h>

int
main( int argc, char *argv[] )
{
	size_t sizeof_char, sizeof_short, sizeof_int, sizeof_long;
	size_t sizeof_void_ptr, sizeof_float, sizeof_double;

	union {
		short short_value;
		char char_array[2];
	} u;

	u.char_array[0] = 0x12;
	u.char_array[1] = 0x34;

	printf( "\n" );

	if ( u.short_value == 0x1234 ) {
		printf( "Processor is BIG-endian.\n" );
	} else
	if ( u.short_value == 0x3412 ) {
		printf( "Processor is LITTLE-endian.\n" );
	} else {
		printf( "Error in computing byte order.  u.short_value = %hx\n",
			u.short_value );
	}

	sizeof_char = sizeof(char);
	sizeof_short = sizeof(short);
	sizeof_int = sizeof(int);
	sizeof_long = sizeof(long);
	sizeof_void_ptr = sizeof(void *);
	sizeof_float = sizeof(float);
	sizeof_double = sizeof(double);

	printf( "\n" );

	printf( "sizeof(char)   = %d\n", sizeof_char );
	printf( "sizeof(short)  = %d\n", sizeof_short );
	printf( "sizeof(int)    = %d\n", sizeof_int );
	printf( "sizeof(long)   = %d\n", sizeof_long );
	printf( "sizeof(void *) = %d\n", sizeof_void_ptr );
	printf( "sizeof(float)  = %d\n", sizeof_float );
	printf( "sizeof(double) = %d\n", sizeof_double );

	printf( "\n" );

	if ( sizeof_int == 2 ) {
	    if ( (sizeof_long == 4) && (sizeof_void_ptr == 4) ) {
		printf(
		"Programming model is LP32  (int=16, long=32, ptr=32)\n" );
	    } else {
		printf( "Programming model is (int=%d, long=%d, ptr=%d)\n",
		    8 * sizeof_int, 8 * sizeof_long, 8 * sizeof_void_ptr );
	    }
	} else
	if ( sizeof_int == 4 ) {
	    if ( sizeof_long == 4 ) {
		if ( sizeof_void_ptr == 4 ) {
		    printf(
		    "Programming model is ILP32  (int=32, long=32, ptr=32)\n" );
		} else
		if ( sizeof_void_ptr == 8 ) {
		    printf( "Programming model probably is LLP64  "
				"(int=32, long=32, ptr=64, long long = 64)\n" );
		    printf( "Check the size of 'long long' to be sure.\n" );
		} else {
		    printf( "Programming model is (int=32, long=32, ptr=%d)\n",
				8 * sizeof(void *) );
		}
	    } else
	    if ( sizeof_long == 8 ) {
		if ( sizeof_void_ptr == 8 ) {
		    printf(
		    "Programming model is LP64  (int=32, long=64, ptr=64)\n" );
		} else {
		    printf( "Programming model is (int=32, long=64, ptr=%d)\n",
				8 * sizeof(void *) );
		}
	    } else {
		printf( "Programming model is (int=32, long=%d, ptr=%d)\n",
				8 * sizeof(long), 8 * sizeof(void *) );
	    }
	} else
	if ( sizeof_int == 8 ) {
	    if ( ( sizeof(long) == 8 ) && ( sizeof(void *) == 8 ) ) {
		printf(
		"Programming model is ILP64  (int=64, long=64, ptr=64)\n" );
	    } else {
		printf( "Programming model is (int=64, long=%d, ptr=%d)\n",
				8 * sizeof(long), 8 * sizeof(void *) );
	    }
	} else {
	    printf( "Programming model is (int=%d, long=%d, ptr=%d)\n",
		8 * sizeof(int), 8 * sizeof(long), 8 * sizeof(void *) );
	}

	printf( "\n" );

	exit(0);
}

