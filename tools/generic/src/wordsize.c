/* 
 * This is a small program to aid in determining the size of chars,
 * shorts, and longs on computers with twos complement arithmetic.
 */

#include <stdio.h>

main()
{
	unsigned char uchar;
	unsigned short ushort;
	unsigned long ulong;

	uchar = ~0;
	ushort = ~0;
	ulong = ~0;

	printf( "%x %hx %lx\n", uchar, ushort, ulong );

	exit(0);
}
