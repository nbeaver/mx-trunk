#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_stdint.h"
#include "mx_util.h"
#include "mx_key.h"
#include "mx_interval_timer.h"

static void
callback_fn( MX_INTERVAL_TIMER *itimer, void *args )
{
	int *value_ptr;

	value_ptr = (int *) args;

	MX_DEBUG(-2,("Callback: itimer = %p, args = %p, *value_ptr = %d",
		 	itimer, args, *value_ptr));
}

int
main( int argc, char *argv[] ) {

	MX_INTERVAL_TIMER *itimer;
	int busy, value;
	mx_status_type mx_status;

	mx_set_debug_level(0);

	value = 7;

	mx_status = mx_interval_timer_create( &itimer,
				MXIT_PERIODIC_TIMER, callback_fn, &value );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	MX_DEBUG(-2,("Starting test."));

	mx_status = mx_interval_timer_start( itimer, 0.5 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	MX_DEBUG(-2,("Hit any key to stop the timer."));

	while (1) {
		if ( mx_kbhit() ) {
			MX_DEBUG(-2,("Invoking mx_interval_timer_stop()." ));

			mx_status = mx_interval_timer_stop( itimer, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				exit( mx_status.code );
		}

		mx_status = mx_interval_timer_is_busy( itimer, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		MX_DEBUG(-2,("busy = %d", busy));

		if ( busy == 0 )
			break;

		mx_msleep(100);
	}

	/* Should never get here. */

	MX_DEBUG(-2,("Test is complete."));

	mx_status = mx_interval_timer_destroy( itimer );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	return(0);
}

