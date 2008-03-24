/*
 * Name:    d_bluice_timer.c
 *
 * Purpose: Support for Blu-Ice controlled timers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2006, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_TIMER_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_timer.h"

#include "mx_bluice.h"
#include "d_bluice_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_bluice_timer_record_function_list = {
	NULL,
	mxd_bluice_timer_create_record_structures,
	mxd_bluice_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_bluice_timer_timer_function_list = {
	mxd_bluice_timer_is_busy,
	mxd_bluice_timer_start,
	mxd_bluice_timer_stop
};

/* MX bluice timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_bluice_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_BLUICE_TIMER_STANDARD_FIELDS
};

long mxd_bluice_timer_num_record_fields
		= sizeof( mxd_bluice_timer_record_field_defaults )
		  / sizeof( mxd_bluice_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_timer_rfield_def_ptr
			= &mxd_bluice_timer_record_field_defaults[0];

/*=======================================================================*/

/* The following function must run after all of the 'bluice_ion_chamber'
 * records have been initialized.
 */

static mx_status_type
mxd_bluice_timer_setup_ion_chambers( MX_TIMER *timer,
					MX_BLUICE_TIMER *bluice_timer,
					MX_BLUICE_SERVER *bluice_server )
{
	static const char fname[] = "mxd_bluice_timer_setup_ion_chambers()";

	MX_RECORD *current_record, *timer_record;
	MX_BLUICE_FOREIGN_DEVICE **foreign_ion_chamber_array;
	MX_BLUICE_FOREIGN_DEVICE *foreign_ion_chamber;
	long i, n, num_ion_chambers;
	long mx_status_code;

#if BLUICE_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for timer '%s'", fname, timer->record->name));
#endif

	/* Invoke mx_analog_input_read() on all of the bluice_ion_chamber
	 * records in the database.  Doing this ensures that the Blu-Ice
	 * foreign data structures for all of the bluice_ion_chamber
	 * records have been initialized before we proceed on to the code
	 * that follows.
	 */

	timer_record = timer->record;

	if ( timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_TIMER %p passed was NULL.",
			timer );
	}

	current_record = timer_record->next_record;

	/* Go through all of the records in the circular list. */

	while ( current_record != timer_record ) {

		/* If the current record is a bluice_ion_chamber record,
		 * attempt to read from it.
		 */

		if ( current_record->mx_type == MXT_AIN_BLUICE_ION_CHAMBER ) {
#if BLUICE_TIMER_DEBUG
			MX_DEBUG(-2,("%s: Initializing ion chamber '%s'",
				fname, current_record->name ));
#endif
			(void) mx_analog_input_read( current_record, NULL );
		}

		current_record = current_record->next_record;
	}

	/* Go through the Blu-Ice ion chamber array to find out how many
	 * of the Blu-Ice controlled ion chambers use this timer.
	 */

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	num_ion_chambers = 0;

	foreign_ion_chamber_array = bluice_server->ion_chamber_array;

	if ( foreign_ion_chamber_array == (MX_BLUICE_FOREIGN_DEVICE **) NULL )
	{
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The foreign_ion_chamber array for Blu-Ice server '%s' "
		"has not been initialized.", bluice_server->record->name );
	}

	for ( i = 0; i < bluice_server->num_ion_chambers; i++ ) {
		foreign_ion_chamber = foreign_ion_chamber_array[i];

		if ( foreign_ion_chamber == (MX_BLUICE_FOREIGN_DEVICE *) NULL )
		{
			mx_warning("%s: Skipping NULL ion chamber %ld",
				fname, i );

			continue;    /* Go back to the top of the for() loop. */
		}

		if ( strcmp( foreign_ion_chamber->u.ion_chamber.timer_name,
				bluice_timer->bluice_name ) == 0 )
		{
			if ( strcmp( foreign_ion_chamber->dhs_server_name,
					bluice_timer->dhs_server_name ) == 0 )
			{
				num_ion_chambers++;
			}
		}
	}

#if BLUICE_TIMER_DEBUG
	MX_DEBUG(-2,("%s: timer '%s' num_ion_chambers = %ld",
		fname, timer_record->name, num_ion_chambers));
#endif

	if ( num_ion_chambers == 0 ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		mx_warning( "Blu-Ice timer '%s' does not seem to control "
		"any Blu-Ice ion chambers.", timer_record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	/* Allocate an array to store a local copy of
	 * the ion chamber pointers.
	 */

	bluice_timer->foreign_ion_chamber_array =
	    (MX_BLUICE_FOREIGN_DEVICE **) malloc(
		num_ion_chambers * sizeof(MX_BLUICE_FOREIGN_DEVICE *));

	if ( bluice_timer->foreign_ion_chamber_array ==
			(MX_BLUICE_FOREIGN_DEVICE **) NULL ) 
	{
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Failed to allocate a %ld element array of "
			"MX_BLUICE_FOREIGN_DEVICE pointers.",
				num_ion_chambers );
	}

	/* Go through the ion chamber array a second time so that we
	 * can store the ion chamber pointers.
	 */

	n = 0;

	for ( i = 0; i < bluice_server->num_ion_chambers; i++ ) {

		if ( n >= num_ion_chambers ) {
			mx_mutex_unlock( bluice_server->foreign_data_mutex );
		
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Somehow the foreign_ion_chamber_array for "
			"Blu-Ice server '%s' had more ion chambers "
			"connected to timer '%s' on the second pass "
			"through than the %ld it found the first time "
			"through.  Since we have the data structures "
			"locked, this should never happen.",
				bluice_server->record->name,
				timer_record->name,
				num_ion_chambers );
		}

		foreign_ion_chamber = foreign_ion_chamber_array[i];

		if ( foreign_ion_chamber == (MX_BLUICE_FOREIGN_DEVICE *) NULL )
		{
			mx_warning("%s: Skipping NULL ion chamber %ld",
				fname, i );

			continue;    /* Go back to the top of the for() loop. */
		}

		if ( strcmp( foreign_ion_chamber->u.ion_chamber.timer_name,
				bluice_timer->bluice_name ) == 0 )
		{
			if ( strcmp( foreign_ion_chamber->dhs_server_name,
					bluice_timer->dhs_server_name ) == 0 )
			{
				/* SUCCESS!  Save the necessary pointers. */

				foreign_ion_chamber->u.ion_chamber.mx_timer
									= timer;

				bluice_timer->foreign_ion_chamber_array[n]
					= foreign_ion_chamber;

				n++;
			}
		}
	}

	bluice_timer->num_ion_chambers = n;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	/* Optionally display all of the ion chambers we have found. */

#if BLUICE_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Ion chambers for Blu-Ice timer '%s' are ",
				fname, timer_record->name));

	for ( i = 0; i < bluice_timer->num_ion_chambers; i++ ) {
		foreign_ion_chamber =
				bluice_timer->foreign_ion_chamber_array[i];

		MX_DEBUG(-2,("%s:   Ion chamber %ld = '%s'",
			fname, i, foreign_ion_chamber->name ));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

/* WARNING: There is no guarantee that the ion chamber data structures
 * will already be initialized when mxd_bluice_timer_get_pointers() is
 * invoked, so we have to test for this.
 */

static mx_status_type
mxd_bluice_timer_get_pointers( MX_TIMER *timer,
			MX_BLUICE_TIMER **bluice_timer,
			MX_BLUICE_SERVER **bluice_server,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bluice_timer_get_pointers()";

	MX_RECORD *bluice_timer_record;
	MX_BLUICE_TIMER *bluice_timer_ptr;
	MX_RECORD *bluice_server_record;
	MX_BLUICE_SERVER *bluice_server_ptr;
	mx_status_type mx_status;

	/* In this section, we do standard MX ..._get_pointers() logic. */

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bluice_timer_record = timer->record;

	if ( bluice_timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_TIMER pointer %p "
		"passed was NULL.", timer );
	}

	bluice_timer_ptr = bluice_timer_record->record_type_struct;

	if ( bluice_timer_ptr == (MX_BLUICE_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_TIMER pointer for record '%s' is NULL.",
			bluice_timer_record->name );
	}

	if ( bluice_timer != (MX_BLUICE_TIMER **) NULL ) {
		*bluice_timer = bluice_timer_ptr;
	}

	bluice_server_record = bluice_timer_ptr->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' "
		"is NULL.", bluice_timer_record->name );
	}

	bluice_server_ptr = bluice_server_record->record_class_struct;

	if ( bluice_server_ptr == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for Blu-Ice server "
		"record '%s' used by record '%s' is NULL.",
			bluice_server_record->name,
			bluice_timer_record->name );
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = bluice_server_ptr;
	}

	/* In this section, we check to see if the pointer to the Blu-Ice
	 * ion chamber array has been set up yet.
	 */

#if 0
	MX_DEBUG(-2,("%s: bluice_timer_ptr->foreign_ion_chamber_array = %p",
		fname, bluice_timer_ptr->foreign_ion_chamber_array));
#endif

	if ( bluice_timer_ptr->foreign_ion_chamber_array == NULL ) {

		mx_status = mxd_bluice_timer_setup_ion_chambers( timer,
							bluice_timer_ptr,
							bluice_server_ptr );

		return mx_status;
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_bluice_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_bluice_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_BLUICE_TIMER *bluice_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	bluice_timer = (MX_BLUICE_TIMER *) malloc( sizeof(MX_BLUICE_TIMER) );

	if ( bluice_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = bluice_timer;
	record->class_specific_function_list
			= &mxd_bluice_timer_timer_function_list;

	timer->record = record;

	bluice_timer->record = record;
	bluice_timer->num_ion_chambers = 0;
	bluice_timer->foreign_ion_chamber_array = NULL;

	bluice_timer->measurement_in_progress = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_bluice_timer_finish_record_initialization()";

	MX_TIMER *timer;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TIMER pointer for record '%s' is NULL.",
			record->name );
	}

	timer->mode = MXCM_UNKNOWN_MODE;

	mx_status = mx_timer_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_bluice_timer_is_busy()";

	MX_BLUICE_TIMER *bluice_timer;
	MX_BLUICE_SERVER *bluice_server;
	mx_status_type mx_status;

	mx_status = mxd_bluice_timer_get_pointers( timer,
					&bluice_timer, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bluice_timer->measurement_in_progress ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

#if BLUICE_TIMER_DEBUG
	MX_DEBUG(-2,("%s: timer '%s' busy = %d",
		fname, timer->record->name, (int) timer->busy));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_bluice_timer_start()";

	MX_BLUICE_TIMER *bluice_timer;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_ion_chamber;
	char chamber_name_string[MXU_BLUICE_NAME_LENGTH+5];
	char command[200];
	long i;
	mx_status_type mx_status;
	long mx_status_code;

	mx_status = mxd_bluice_timer_get_pointers( timer,
					&bluice_timer, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		snprintf( command, sizeof(command),
			"gtos_read_ion_chambers %g 0", timer->value );
		break;
	case MXN_BLUICE_DHS_SERVER:
		snprintf( command, sizeof(command),
			"stoh_read_ion_chambers %g 0", timer->value );
		break;
	}

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	for ( i = 0; i < bluice_timer->num_ion_chambers; i++ ) {
		foreign_ion_chamber =
			bluice_timer->foreign_ion_chamber_array[i];

		snprintf( chamber_name_string, sizeof(chamber_name_string),
			" %s", foreign_ion_chamber->name );

		strlcat( command, chamber_name_string, sizeof(command) );
	}

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

#if BLUICE_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s' start command = '%s'",
		fname, timer->record->name, command ));
#endif

	mx_status = mx_bluice_check_for_master( bluice_server );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bluice_timer->measurement_in_progress = TRUE;

	mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_bluice_timer_stop()";

	MX_BLUICE_TIMER *bluice_timer;
	MX_BLUICE_SERVER *bluice_server;
	mx_status_type mx_status;

	mx_status = mxd_bluice_timer_get_pointers( timer,
					&bluice_timer, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bluice_timer->measurement_in_progress = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

