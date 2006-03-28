#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_virtual_timer.h"

static void
callback_fn( MX_VIRTUAL_TIMER *vtimer, void *args )
{
	int *value_ptr;

	value_ptr = (int *) args;

	MX_DEBUG(-2,("Callback: vtimer = %p, args = %p, *value_ptr = %d",
		 	vtimer, args, *value_ptr));
}

int
main( int argc, char *argv[] ) {

	MX_VIRTUAL_TIMER *vtimer;
	MX_INTERVAL_TIMER *master_timer;
	int busy, value;
	mx_status_type mx_status;

	mx_set_debug_level(0);

	value = 7;

	mx_status = mx_virtual_timer_create_master( &master_timer, 1.0 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	mx_status = mx_virtual_timer_create( &vtimer, master_timer,
				MXIT_ONE_SHOT_TIMER, callback_fn, &value );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	MX_DEBUG(-2,("Starting test."));

	mx_status = mx_virtual_timer_start( vtimer, 2.5 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	while (1) {
		mx_status = mx_virtual_timer_is_busy( vtimer, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		MX_DEBUG(-2,("busy = %d", busy));

		if ( busy == 0 )
			break;

		mx_msleep(100);
	}

	MX_DEBUG(-2,("Test is complete."));

	mx_status = mx_virtual_timer_destroy( vtimer );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	mx_status = mx_virtual_timer_destroy_master( master_timer );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	return(0);
}

