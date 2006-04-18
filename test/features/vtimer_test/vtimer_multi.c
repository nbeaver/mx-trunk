#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_key.h"
#include "mx_virtual_timer.h"

static void
oneshot_callback_1( MX_VIRTUAL_TIMER *vtimer, void *args )
{
	int *value_ptr;

	value_ptr = (int *) args;

	MX_DEBUG(-2,("ONE-SHOT CALLBACK 1: value = %d", *value_ptr));
}

static void
oneshot_callback_2( MX_VIRTUAL_TIMER *vtimer, void *args )
{
	int *value_ptr;

	value_ptr = (int *) args;

	MX_DEBUG(-2,("ONE-SHOT CALLBACK 2: value = %d", *value_ptr));
}

static void
periodic_callback_1( MX_VIRTUAL_TIMER *vtimer, void *args )
{
	int *value_ptr;

	value_ptr = (int *) args;

	MX_DEBUG(-2,("PERIODIC CALLBACK 3: value = %d", *value_ptr));
}

static void
periodic_callback_2( MX_VIRTUAL_TIMER *vtimer, void *args )
{
	int *value_ptr;

	value_ptr = (int *) args;

	MX_DEBUG(-2,("PERIODIC CALLBACK 4: value = %d", *value_ptr));
}

static void
shutdown_timers( MX_INTERVAL_TIMER *master_timer,
		MX_VIRTUAL_TIMER *vtimer_o1,
		MX_VIRTUAL_TIMER *vtimer_o2,
		MX_VIRTUAL_TIMER *vtimer_p3,
		MX_VIRTUAL_TIMER *vtimer_p4 )
{
	MX_DEBUG(-2,("Stopping one-shot timer 1."));

	(void) mx_virtual_timer_stop( vtimer_o1, NULL );

	MX_DEBUG(-2,("Stopping one-shot timer 2."));

	(void) mx_virtual_timer_stop( vtimer_o2, NULL );

	MX_DEBUG(-2,("Stopping periodic timer 1."));

	(void) mx_virtual_timer_stop( vtimer_p3, NULL );

	MX_DEBUG(-2,("Stopping periodic timer 2."));

	(void) mx_virtual_timer_stop( vtimer_p4, NULL );

	MX_DEBUG(-2,("Stopping master interval timer."));

	(void) mx_virtual_timer_destroy_master( master_timer );
}

int
main( int argc, char *argv[] )
{
	MX_INTERVAL_TIMER *master_timer;
	MX_VIRTUAL_TIMER *vtimer_o1, *vtimer_o2, *vtimer_p3, *vtimer_p4;
	int busy_o1, busy_o2, busy_p3, busy_p4;
	int value_o1, value_o2, value_p3, value_p4;
	unsigned long i;
	mx_status_type mx_status;

	mx_set_debug_level(0);

	/* Create the master interval timer that starts it all. */

	mx_status = mx_virtual_timer_create_master( &master_timer, 0.2 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Create the test virtual timers. */

	value_o1 = 11;

	mx_status = mx_virtual_timer_create( &vtimer_o1, master_timer,
			MXIT_ONE_SHOT_TIMER, oneshot_callback_1, &value_o1 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	value_o2 = 22;

	mx_status = mx_virtual_timer_create( &vtimer_o2, master_timer,
			MXIT_ONE_SHOT_TIMER, oneshot_callback_2, &value_o2 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	value_p3 = 45;

	mx_status = mx_virtual_timer_create( &vtimer_p3, master_timer,
			MXIT_PERIODIC_TIMER, periodic_callback_1, &value_p3 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	value_p4 = 67;

	mx_status = mx_virtual_timer_create( &vtimer_p4, master_timer,
			MXIT_PERIODIC_TIMER, periodic_callback_2, &value_p4 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	MX_DEBUG(-2,("Starting test."));

	/* Start the periodic timers. */

	mx_status = mx_virtual_timer_start( vtimer_p3, 0.5 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	mx_status = mx_virtual_timer_start( vtimer_p4, 2.0 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	MX_DEBUG(-2,("Hit any key to stop the test."));

	for ( i = 1; ; i++ ) {
		if ( mx_kbhit() ) {
			shutdown_timers( master_timer,
				vtimer_o1, vtimer_o2, vtimer_p3, vtimer_p4 );

			exit(0);
		}

		if ( (i % 40) == 0 ) {
			MX_DEBUG(-2,("Starting ONE-SHOT TIMER 1"));

			mx_status = mx_virtual_timer_start( vtimer_o1, 1.5 );
		}

		if ( (i % 70) == 0 ) {
			MX_DEBUG(-2,("Starting ONE-SHOT TIMER 2"));

			mx_status = mx_virtual_timer_start( vtimer_o2, 2.5 );
		}

		(void) mx_virtual_timer_is_busy( vtimer_o1, &busy_o1 );

		(void) mx_virtual_timer_is_busy( vtimer_o2, &busy_o2 );

		(void) mx_virtual_timer_is_busy( vtimer_p3, &busy_p3 );

		(void) mx_virtual_timer_is_busy( vtimer_p4, &busy_p4 );

		MX_DEBUG(-2,("TIMER STATUS: o1 = %d, o2 = %d, p3 = %d, p4 = %d",
			busy_o1, busy_o2, busy_p3, busy_p4 ));

		mx_msleep(100);
	}

	MX_DEBUG(-2,("Should never get here!"));

	shutdown_timers( master_timer,
		vtimer_o1, vtimer_o2, vtimer_p3, vtimer_p4 );

	return (0);
}

