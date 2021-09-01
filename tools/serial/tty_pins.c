/*
 * Name:    tty_pins.c
 *
 * Purpose: A standalone program for viewing and controlling individual pins
 *          of a TTY serial port on Linux.  Support will be added for other
 *          platforms that use TTY-style serial ports as time permits.
 *
 * Note:    This program is indeed standaline and does not depend on MX
 *          in any way.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#if !defined(TIOCMGET)
#  error This function depends on the use of the TIOCMGET ioctl call.
#endif

int
main( int argc, char *argv[] )
{
	char *filename = NULL;
	int fd, ioctl_status, status_bits, saved_errno;
	int rts, cts, dsr, dtr, cd, ri;

	if ( argc < 2 ) {
		fprintf( stderr, "Usage:  tty_pins <name_of_serial_device>\n" );
		exit(1);
	}

	filename = argv[1];

	fd = open( filename, O_RDWR | O_NOCTTY );

	if ( fd < 0 ) {
		saved_errno = errno;

		fprintf( stderr,
    "The attempt to open '%s' failed with errno = %d, error message = '%s'\n",
		filename, saved_errno, strerror( saved_errno ) );
		exit(1);
	}

	ioctl_status = ioctl( fd, TIOCMGET, &status_bits );

	if ( ioctl_status != 0 ) {
		saved_errno = errno;

		fprintf( stderr,
    "TIOCMGET ioctl() for '%s' failed with errno = %d, error message = '%s'\n",
		filename, saved_errno, strerror( saved_errno ) );
		exit(1);
	}

	if ( status_bits & TIOCM_RTS ) {
		rts = 1;
	} else {
		rts = 0;
	}

	if ( status_bits & TIOCM_CTS ) {
		cts = 1;
	} else {
		cts = 0;
	}

	if ( status_bits & TIOCM_LE ) {
		dsr = 1;
	} else {
		dsr = 0;
	}

	if ( status_bits & TIOCM_DTR ) {
		dtr = 1;
	} else {
		dtr = 0;
	}

	if ( status_bits & TIOCM_CAR ) {
		cd = 1;
	} else {
		cd = 0;
	}

	if ( status_bits & TIOCM_RNG ) {
		ri = 1;
	} else {
		ri = 0;
	}

	printf(
    "Device '%s': RTS = %d, CTS = %d, DSR = %d, DTR = %d, CD = %d, RI = %d\n",
		filename, rts, cts, dsr, dtr, cd, ri );

	exit(0);
}

