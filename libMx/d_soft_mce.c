/*
 * Name:    d_soft_mce.c
 *
 * Purpose: MX MCE driver to support MX network multichannel encoders.
 *
 * Author:  William Lavender
 *
 * FIXME:   We need to implement a poll callback for this function
 *          in order to fill in the values of mce->value_array.
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2016, 2018, 2020-2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_MCE_DEBUG			FALSE

#define MXD_SOFT_MCE_DEBUG_READ_MEASUREMENT	FALSE

#define MXD_SOFT_MCE_DEBUG_MONITOR_THREAD	FALSE

#define MXD_SOFT_MCE_DEBUG_INTERVAL_TIMER	FALSE

#define MXD_SOFT_MCE_DEBUG_OBJECT_CREATION	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_thread.h"
#include "mx_mutex.h"
#include "mx_condition_variable.h"
#include "mx_atomic.h"
#include "mx_interval_timer.h"

#include "mx_motor.h"
#include "mx_dead_reckoning.h"
#include "d_soft_motor.h"

#include "mx_mce.h"
#include "d_soft_mce.h"

/* Initialize the MCE driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_soft_mce_record_function_list = {
	mxd_soft_mce_initialize_driver,
	mxd_soft_mce_create_record_structures,
	mxd_soft_mce_finish_record_initialization,
	mxd_soft_mce_delete_record,
	NULL,
	mxd_soft_mce_open
};

MX_MCE_FUNCTION_LIST mxd_soft_mce_mce_function_list = {
	NULL,
	NULL,
	mxd_soft_mce_read,
	mxd_soft_mce_get_current_num_values,
	mxd_soft_mce_get_last_measurement_number,
	mxd_soft_mce_get_status,
	mxd_soft_mce_start,
	mxd_soft_mce_stop,
	mxd_soft_mce_clear,
	mxd_soft_mce_read_measurement,
	NULL,
	NULL,
	mxd_soft_mce_get_parameter,
	mxd_soft_mce_set_parameter
};

/* soft mce data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_soft_mce_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCE_STANDARD_FIELDS,
	MXD_SOFT_MCE_STANDARD_FIELDS
};

long mxd_soft_mce_num_record_fields
		= sizeof( mxd_soft_mce_record_field_defaults )
		  / sizeof( mxd_soft_mce_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_mce_rfield_def_ptr
			= &mxd_soft_mce_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_soft_mce_get_pointers( MX_MCE *mce,
			MX_SOFT_MCE **soft_mce,
			MX_MOTOR **motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_mce_get_pointers()";

	MX_RECORD *record = NULL;
	MX_SOFT_MCE *soft_mce_ptr = NULL;
	MX_RECORD *motor_record = NULL;

	if ( mce == (MX_MCE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = mce->record;

	soft_mce_ptr = (MX_SOFT_MCE *) record->record_type_struct;

	if ( soft_mce_ptr == (MX_SOFT_MCE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SOFT_MCE pointer for record '%s' is NULL.",
			mce->record->name );
	}

	if ( soft_mce != (MX_SOFT_MCE **) NULL ) {
		*soft_mce = soft_mce_ptr;
	}

	if ( motor != (MX_MOTOR **) NULL ) {
		motor_record = soft_mce_ptr->motor_record;

		if ( motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The motor record pointer for record '%s' is NULL.",
				mce->record->name );
		}

		if ( motor_record->mx_class != MXC_MOTOR ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"'Motor' record '%s' specified for MCE '%s' "
			"is _NOT_ a motor.",
				mce->record->name,
				motor_record->name );
		}

		*motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( (*motor) == (MX_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MOTOR pointer for motor record '%s' "
			"used by soft MCE '%s' is NULL.",
				motor_record->name,
				mce->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

static mx_status_type
mxd_soft_mce_send_command_to_monitor_thread( MX_SOFT_MCE *soft_mce,
					int32_t monitor_command )
{
	static const char fname[] =
		"mxd_soft_mce_send_command_to_monitor_thread()";

	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( soft_mce == (MX_SOFT_MCE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOFT_MCE pointer passed was NULL." );
	}

#if MXD_SOFT_MCE_DEBUG_MONITOR_THREAD
	MX_DEBUG(-2,("%s: sending (%d) command to monitor thread.",
			fname, (int) monitor_command));
#endif

	/* Prepare to tell the "monitor" thread to start a command. */

	mx_status_code = mx_mutex_lock(
				soft_mce->monitor_thread_command_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the monitor thread command mutex for "
		"soft MCE '%s' failed.",
			soft_mce->record->name );
	}

	soft_mce->monitor_command = monitor_command;

	/* Notify the monitor thread. */

	mx_status = mx_condition_variable_signal(
			soft_mce->monitor_thread_command_cv );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We are done sending our notification. */

	mx_status_code = mx_mutex_unlock(
				soft_mce->monitor_thread_command_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock the monitor thread command mutex for "
		"soft MCE '%s' failed.",
			soft_mce->record->name );
	}


	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

static mx_status_type
mxd_soft_mce_send_status_to_main_thread( MX_SOFT_MCE *soft_mce,
					int32_t monitor_status )
{
	static const char fname[] =
		"mxd_soft_mce_send_status_to_main_thread()";

	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( soft_mce == (MX_SOFT_MCE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOFT_MCE pointer passed was NULL." );
	}

#if MXD_SOFT_MCE_DEBUG_MONITOR_THREAD
	MX_DEBUG(-2,("%s: sending (%d) status to main thread.",
			fname, (int) monitor_status));
#endif

	/* Prepare to send the monitor thread status to the main thread. */

	mx_status_code = mx_mutex_lock( soft_mce->monitor_thread_status_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the monitor thread status mutex for "
		"soft MCE '%s' failed.",
			soft_mce->record->name );
	}

	soft_mce->monitor_status = monitor_status;

	/* Notify the main thread. */

	mx_status = mx_condition_variable_signal(
			soft_mce->monitor_thread_status_cv );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We are done sending our notification. */

	mx_status_code = mx_mutex_unlock(
				soft_mce->monitor_thread_status_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock the monitor thread status mutex for "
		"soft MCE '%s' failed.",
			soft_mce->record->name );
	}


	return MX_SUCCESSFUL_RESULT;
}


/*------------------------------------------------------------------*/

static void
mxd_soft_mce_interval_timer_callback( MX_INTERVAL_TIMER *itimer,
					void *callback_args )
{
	static const char fname[] = "mxd_soft_mce_interval_timer_callback()";

	MX_SOFT_MCE *soft_mce;

	static unsigned long timer_callback_counter = 0;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	if ( callback_args == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The callback_args pointer passed was NULL." );
	}

	soft_mce = (MX_SOFT_MCE *) callback_args;

	timer_callback_counter++;

	/* Tell the "monitor" thread to read a motor position. */

#if MXD_SOFT_MCE_DEBUG_INTERVAL_TIMER
	MX_DEBUG(-2,
    ("%s Timer %p [%lu]: Signaling the monitor thread to take a measurement.",
		fname, mx_get_current_thread_pointer(),
		timer_callback_counter));
#endif

	(void) mxd_soft_mce_send_command_to_monitor_thread( soft_mce,
						MXS_SOFT_MCE_CMD_READ_MOTOR );

	return;
}

/*------------------------------------------------------------------*/

static mx_status_type
mxd_soft_mce_monitor_thread_fn( MX_THREAD *thread, void *record_ptr )
{
	static const char fname[] = "mxd_soft_mce_monitor_thread_fn()";

	MX_RECORD *record = NULL;
	MX_MCE *mce = NULL;
	MX_SOFT_MCE *soft_mce = NULL;
	MX_MOTOR *motor = NULL;
	MX_SOFT_MOTOR *soft_motor = NULL;

	long i, n;
	unsigned long monitor_loop_counter;
	unsigned long mx_status_code;
	mx_status_type mx_status;

#if 0
	MX_DEBUG(-2,("%s invoked.",fname));
#endif

	/* Initialize the variables to be used by this thread. */

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	if ( record_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record = (MX_RECORD *) record_ptr;

	mce = (MX_MCE *) record->record_class_struct;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, &motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* 'soft_motor' records are a special case, since we have to call
	 * that driver's dead reckoning function.
	 */

	if ( motor->record->mx_type != MXT_MTR_SOFTWARE ) {
		soft_motor = NULL;
	} else {
		soft_motor = (MX_SOFT_MOTOR *)
				motor->record->record_type_struct;
	}

#if MXD_SOFT_MCE_DEBUG_MONITOR_THREAD
	MX_DEBUG(-2,("%s: about to create interval timer.", fname));
#endif

	/* Create the interval timer that will be used to trigger
	 * measurements of the motor position.  Note that this
	 * does _not_ start the timer.
	 */

	mx_status = mx_interval_timer_create( &(soft_mce->measurement_timer),
					MXIT_PERIODIC_TIMER,
					mxd_soft_mce_interval_timer_callback,
					soft_mce );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG_OBJECT_CREATION
	MX_DEBUG(-2,("%s: soft_mce->measurement_timer = %p",
		fname, soft_mce->measurement_timer));
#endif

	/*------------------------------------------------------------------*/

	/* Tell the main thread that we have finished initializing ourself. */

	mx_status_code = mx_mutex_lock( soft_mce->monitor_thread_init_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the monitor thread initialization mutex "
		"for soft MCE '%s' failed.",
			soft_mce->record->name );
	}

	soft_mce->monitor_status = MXS_SOFT_MCE_STAT_IDLE;

	mx_status = mx_condition_variable_signal(
			soft_mce->monitor_thread_init_cv );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status_code = mx_mutex_unlock( soft_mce->monitor_thread_init_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock the monitor thread initialization mutex "
		"for soft MCE '%s' failed.",
			soft_mce->record->name );
	}

	/* NOTE: We should not attempt to use the initialization mutex and cv
	 * after this point, since the main thread may have deallocated them.
	 */

	/*------------------------------------------------------------------*/

	/* Take ownership of the command mutex. */

	mx_status_code = mx_mutex_lock( soft_mce->monitor_thread_command_mutex);

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the monitor thread command mutex "
		"for soft MCE '%s' failed.",
			soft_mce->record->name );
	}

	/*------------------------------------------------------------------*/

	monitor_loop_counter = 0;

#if MXD_SOFT_MCE_DEBUG_MONITOR_THREAD
	MX_DEBUG(-2,("%s %p [%lu]: MCE '%s' entering event loop.",
		fname, mx_get_current_thread_pointer(),
		monitor_loop_counter, record->name));
#endif

	while (TRUE) {

		monitor_loop_counter++;

		/* Wait on the command condition variable. */

		while ( soft_mce->monitor_command == MXS_SOFT_MCE_CMD_NONE ) {
			mx_status = mx_condition_variable_wait(
					soft_mce->monitor_thread_command_cv,
					soft_mce->monitor_thread_command_mutex);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

#if MXD_SOFT_MCE_DEBUG_MONITOR_THREAD
		MX_DEBUG(-2,("%s %p [%lu]: '%s' command = %ld",
			fname, mx_get_current_thread_pointer(),
			monitor_loop_counter, record->name,
			(long) soft_mce->monitor_command));
#endif

		switch( soft_mce->monitor_command ) {
		case MXS_SOFT_MCE_CMD_NONE:
			/* No command was requested, so do not do anything. */
			break;
		case MXS_SOFT_MCE_CMD_START:
			mx_atomic_write32(
			    &(soft_mce->monitor_last_measurement_number), -1 );

			mx_status = mxd_soft_mce_send_status_to_main_thread(
					soft_mce, MXS_SOFT_MCE_STAT_ACQUIRING );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_interval_timer_start(
					soft_mce->measurement_timer,
					mce->measurement_time );
			break;
		case MXS_SOFT_MCE_CMD_STOP:
			mx_status = mx_interval_timer_stop(
					soft_mce->measurement_timer, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxd_soft_mce_send_status_to_main_thread(
					soft_mce, MXS_SOFT_MCE_STAT_IDLE );
			break;
		case MXS_SOFT_MCE_CMD_CLEAR:
			mx_status = mx_interval_timer_stop(
					soft_mce->measurement_timer, NULL );

			for ( i = 0; i < mce->maximum_num_values; i++ ) {
				mce->value_array[i] = 0.0;
			}

			mx_status = mxd_soft_mce_send_status_to_main_thread(
					soft_mce, MXS_SOFT_MCE_STAT_IDLE );
			break;
		case MXS_SOFT_MCE_CMD_READ_MOTOR:
			/* First check to see if we have reached the
			 * end of the array and stop if we have.
			 */

			n = mx_atomic_read32(
				&(soft_mce->monitor_last_measurement_number) );

			if ( (n+1) >= mce->current_num_values ) {
				mx_status = mx_interval_timer_stop(
					soft_mce->measurement_timer, NULL );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				mx_status =
				    mxd_soft_mce_send_status_to_main_thread(
					soft_mce, MXS_SOFT_MCE_STAT_IDLE );
				break;
			}

			/* If we are not yet at the end of the array,
			 * increment the array index and write a new
			 * value there.
			 */

			i = mx_atomic_increment32(
				&(soft_mce->monitor_last_measurement_number) );

			/* WARNING:
			 * We cannot call mx_motor_get_position() here
			 * because we are not in the main thread.
			 * Instead, we just read the value most recently
			 * written to motor->position.  This read is
			 * _NOT_ guaranteed to be atomic, but probably
			 * is anyway on x86 and amd64, however probably
			 * not on arm.  Because of this, the 'soft_mce'
			 * driver is not really suitable for use in
			 * _real_ experiments.
			 */

			if ( soft_motor != NULL ) {
				/* 'soft_motor' records are a special case,
				 * since we have to call that driver's
				 * dead reckoning function.
				 */

				mx_status = mx_dead_reckoning_predict_motion(
						&(soft_motor->dead_reckoning),
						NULL, NULL );
			}

			mce->value_array[i] = motor->position;

#if MXD_SOFT_MCE_DEBUG_MONITOR_THREAD
			MX_DEBUG(-2,("%s %p [%lu]: '%s' value_array[%lu] = %f",
				fname, mx_get_current_thread_pointer(),
				monitor_loop_counter, record->name,
				i, mce->value_array[i]));
#endif
			break;
		default:
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Unsupported command %ld requested "
				"for soft MCE '%s' monitor thread.",
					(long) soft_mce->monitor_command,
					record->name );
			break;
		}

		soft_mce->monitor_command = MXS_SOFT_MCE_CMD_NONE;

#if MXD_SOFT_MCE_DEBUG_MONITOR_THREAD
		MX_DEBUG(-2,("%s %p [%lu]: '%s' status = %ld",
			fname, mx_get_current_thread_pointer(),
			monitor_loop_counter, record->name,
			(long) soft_mce->monitor_status));
#endif
	}
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_soft_mce_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_values_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mce_initialize_driver( driver,
					&maximum_num_values_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_mce_create_record_structures()";

	MX_MCE *mce;
	MX_SOFT_MCE *soft_mce;

	/* Allocate memory for the necessary structures. */

	mce = (MX_MCE *) malloc( sizeof(MX_MCE) );

	if ( mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCE structure." );
	}

	soft_mce = (MX_SOFT_MCE *) malloc( sizeof(MX_SOFT_MCE) );

	if ( soft_mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SOFT_MCE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mce;
	record->record_type_struct = soft_mce;
	record->class_specific_function_list = &mxd_soft_mce_mce_function_list;

	mce->record = record;
	soft_mce->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_soft_mce_finish_record_initialization()";

	MX_MCE *mce = NULL;
	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	mce = (MX_MCE *) record->record_class_struct;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mce->window_is_available = FALSE;
	mce->use_window = FALSE;

	memset( mce->window,
		0, MXU_MTR_NUM_WINDOW_PARAMETERS * sizeof(double) );

	/*----*/

	mce->encoder_type = MXT_MCE_ABSOLUTE_ENCODER;

	mce->current_num_values = mce->maximum_num_values;

	strlcpy( mce->selected_motor_name,
			soft_mce->motor_record->name,
			MXU_RECORD_NAME_LENGTH );

	mce->num_motors = 1;

	mce->motor_record_array = &(soft_mce->motor_record);

	mx_status = mx_mce_fixup_motor_record_array_field( mce );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_delete_record( MX_RECORD *record )
{
	MX_MCE *mce;

	if ( record == (MX_RECORD *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mce = (MX_MCE *) record->record_class_struct;

	if ( mce != (MX_MCE *) NULL ) {
		if ( mce->motor_record_array != (MX_RECORD **) NULL ) {
			mx_free( mce->motor_record_array );
		}
		mx_free( record->record_class_struct );
	}

	if ( record->record_type_struct != NULL ) {
		mx_free( record->record_type_struct );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_mce_open()";

	MX_MCE *mce = NULL;
	MX_SOFT_MCE *soft_mce = NULL;
	MX_MOTOR *motor = NULL;
	long mx_status_code;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	mce = (MX_MCE *) record->record_class_struct;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, &motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*-----------------------------------------------------------------*/

	soft_mce->monitor_command = MXS_SOFT_MCE_CMD_NONE;
	soft_mce->monitor_status  = MXS_SOFT_MCE_STAT_NOT_INITIALIZED;

	soft_mce->monitor_last_measurement_number = -1;

	/* Create some synchronization objects to be used by the
	 * monitor thread.
	 */

	/* monitor_thread_init_mutex and monitor_thread_init_cv are used by
	 * the main thread to wait until the monitor thread initializes itself.
	 */

	mx_status = mx_mutex_create( &(soft_mce->monitor_thread_init_mutex));

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_condition_variable_create(
				&(soft_mce->monitor_thread_init_cv) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG_OBJECT_CREATION
	MX_DEBUG(-2,("%s: init: mutex = %p, cv = %p",
		fname, soft_mce->monitor_thread_init_mutex,
		soft_mce->monitor_thread_init_cv));
#endif

	/* monitor_thread_command_mutex and monitor_thread_command_cv are used
	 * to send commands to the monitor thread.
	 */

	mx_status = mx_mutex_create( &(soft_mce->monitor_thread_command_mutex));

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_condition_variable_create(
				&(soft_mce->monitor_thread_command_cv) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG_OBJECT_CREATION
	MX_DEBUG(-2,("%s: command: mutex = %p, cv = %p",
		fname, soft_mce->monitor_thread_command_mutex,
		soft_mce->monitor_thread_command_cv));
#endif

	/* monitor_thread_status_mutex and monitor_thread_status_cv are used
	 * to check the status of the monitor thread.
	 */

	mx_status = mx_mutex_create( &(soft_mce->monitor_thread_status_mutex));

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_condition_variable_create(
				&(soft_mce->monitor_thread_status_cv) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG_OBJECT_CREATION
	MX_DEBUG(-2,("%s: status: mutex = %p, cv = %p",
		fname, soft_mce->monitor_thread_status_mutex,
		soft_mce->monitor_thread_status_cv));
#endif

	/*-----------------------------------------------------------------*/

	/* Prepare to wait for the notification that the monitor thread
	 * is initialized.
	 */

	mx_status_code = mx_mutex_lock( soft_mce->monitor_thread_init_mutex );

	MXW_UNUSED( mx_status_code );

	/* Create the monitor thread to handle updates to 'mce->value_array'. */

	mx_status = mx_thread_create( &(soft_mce->monitor_thread),
					mxd_soft_mce_monitor_thread_fn,
					record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the monitor thread to get itself initialized. */

	while ( soft_mce->monitor_status == MXS_SOFT_MCE_STAT_NOT_INITIALIZED )
	{
		mx_status = mx_condition_variable_wait(
				soft_mce->monitor_thread_init_cv,
				soft_mce->monitor_thread_init_mutex );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} 

	/* We now know that the monitor thread has finished initializing
	 * itself, so we unlock the initialization mutex.
	 */

	mx_status_code =
		mx_mutex_unlock( soft_mce->monitor_thread_init_mutex );

	/*-----------------------------------------------------------------*/

	/* Clear the contents of the MCE. */

	mx_status = mxd_soft_mce_clear( mce );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the measurement time to an illegal value. */

	mx_status = mx_mce_set_measurement_time( record, -1.0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_read( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_read()";

	MX_SOFT_MCE *soft_mce;
	mx_status_type mx_status;

	soft_mce = NULL;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	/* The contents of value_array have already been set by this driver's
	 * poll callback, so we do not need to do anything further here.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_get_current_num_values( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_get_current_num_values()";

	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	/* We use the already existing value of mce->current_num_values. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_get_last_measurement_number( MX_MCE *mce )
{
	static const char fname[] =
		"mxd_soft_mce_get_last_measurement_number()";

	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	/* We use the already existing value of mce->current_num_values. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_get_status( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_get_status()";

	MX_SOFT_MCE *soft_mce = NULL;
	int32_t monitor_thread_status;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	monitor_thread_status = mx_atomic_read32( &(soft_mce->monitor_status) );

	if ( monitor_thread_status == MXS_SOFT_MCE_STAT_IDLE ) {
		mce->status = 0;
	} else {
		mce->status = 0x1;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_start( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_start()";

	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	mx_status = mxd_soft_mce_send_command_to_monitor_thread(
					soft_mce, MXS_SOFT_MCE_CMD_START );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_stop( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_stop()";

	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	mx_status = mxd_soft_mce_send_command_to_monitor_thread(
					soft_mce, MXS_SOFT_MCE_CMD_STOP );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_clear( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_clear()";

	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	mx_status = mxd_soft_mce_send_command_to_monitor_thread(
					soft_mce, MXS_SOFT_MCE_CMD_CLEAR );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_read_measurement( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_read_measurement()";

	MX_SOFT_MCE *soft_mce = NULL;
	MX_MOTOR *motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, &motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mce->value = mce->value_array[ mce->measurement_index ];

#if MXD_SOFT_MCE_DEBUG_READ_MEASUREMENT
	MX_DEBUG(-2,("%s: MCE '%s', index = %ld, value = %f",
		fname, mce->record->name, mce->measurement_index, mce->value));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_get_parameter( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_get_parameter()";

	MX_SOFT_MCE *soft_mce;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, mce->record->name,
		mx_get_field_label_string( mce->record, mce->parameter_type ),
		mce->parameter_type ));
#endif

	switch( mce->parameter_type ) {
	case MXLV_MCE_MEASUREMENT_TIME:
		/* Just report the value already in the variable.  Including
		 * this case prevents the default handler from being called,
		 * since the default handler overwrites the value with -1.
		 */

		break;

	case MXLV_MCE_MEASUREMENT_WINDOW_OFFSET:
		mce->measurement_window_offset = 0;
		break;
	default:
		return mx_mce_default_get_parameter_handler( mce );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_set_parameter( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_set_parameter()";

	MX_SOFT_MCE *soft_mce;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, mce->record->name,
		mx_get_field_label_string( mce->record, mce->parameter_type ),
		mce->parameter_type ));
#endif

	switch( mce->parameter_type ) {
	default:
		return mx_mce_default_set_parameter_handler( mce );
		break;
	}

	MXW_NOT_REACHED( return mx_status; )
}

