#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_time.h"
#include "mx_hrt.h"

int
main( int argc, char *argv[] )
{
	static const char fname[] = "boot_test";

	char buffer[80];
	struct timespec system_boot_timespec;
	mx_status_type mx_status;

	mx_status = mx_get_system_boot_time( &system_boot_timespec );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "\n%s: system_boot_timespec = (%lu,%ld)\n\n",
		fname, (unsigned long) system_boot_timespec.tv_sec,
			system_boot_timespec.tv_nsec );

	mx_os_time_string( system_boot_timespec, buffer, sizeof(buffer) );

	fprintf( stderr, "%s: This computer booted at time = '%s'\n\n",
		fname, buffer );

	return 0;
}

