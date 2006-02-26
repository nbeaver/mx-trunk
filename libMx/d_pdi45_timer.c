/*
 * Name:    d_pdi45_timer.c
 *
 * Purpose: MX timer driver to control a Prairie Digital Model 45 
 *          digital I/O line as an MX timer.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "i_pdi45.h"
#include "d_pdi45_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_pdi45_timer_record_function_list = {
	NULL,
	mxd_pdi45_timer_create_record_structures,
	mxd_pdi45_timer_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_pdi45_timer_open
};

MX_TIMER_FUNCTION_LIST mxd_pdi45_timer_timer_function_list = {
	mxd_pdi45_timer_is_busy,
	mxd_pdi45_timer_start,
	mxd_pdi45_timer_stop,
	mxd_pdi45_timer_clear,
	mxd_pdi45_timer_read,
	mxd_pdi45_timer_get_mode,
	mxd_pdi45_timer_set_mode
};

MX_RECORD_FIELD_DEFAULTS mxd_pdi45_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_PDI45_TIMER_STANDARD_FIELDS
};

long mxd_pdi45_timer_num_record_fields
		= sizeof( mxd_pdi45_timer_record_field_defaults )
		  / sizeof( mxd_pdi45_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_timer_rfield_def_ptr
			= &mxd_pdi45_timer_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_pdi45_timer_get_pointers( MX_TIMER *timer,
			MX_PDI45_TIMER **pdi45_timer,
			MX_PDI45 **pdi45,
			const char *calling_fname )
{
	const char fname[] = "mxd_pdi45_timer_get_pointers()";

	MX_PDI45_TIMER *pdi45_timer_ptr;
	MX_RECORD *pdi45_record;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The timer pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( timer->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for timer pointer passed by '%s' is NULL.",
			calling_fname );
	}

	pdi45_timer_ptr = (MX_PDI45_TIMER *) timer->record->record_type_struct;

	if ( pdi45_timer_ptr == (MX_PDI45_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_PDI45_TIMER pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
	}

	if ( pdi45_timer != (MX_PDI45_TIMER **) NULL ) {
		*pdi45_timer = pdi45_timer_ptr;
	}

	if ( pdi45 != (MX_PDI45 **) NULL ) {
		pdi45_record = pdi45_timer_ptr->pdi45_record;

		if ( pdi45_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The pdi45_record pointer for PDI45 timer '%s' is NULL.",
				timer->record->name );
		}

		*pdi45 = (MX_PDI45 *) pdi45_record->record_type_struct;

		if ( *pdi45 == (MX_PDI45 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_PDI45 pointer for PDI45 record '%s' used by timer record '%s' is NULL.",
				pdi45_record->name, timer->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_pdi45_timer_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_pdi45_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_PDI45_TIMER *pdi45_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	pdi45_timer = (MX_PDI45_TIMER *)
				malloc( sizeof(MX_PDI45_TIMER) );

	if ( pdi45_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PDI45_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = pdi45_timer;
	record->class_specific_function_list
			= &mxd_pdi45_timer_timer_function_list;

	timer->record = record;
	pdi45_timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_timer_finish_record_initialization( MX_RECORD *record )
{
	MX_TIMER *timer;
	mx_status_type mx_status;

	mx_status = mx_timer_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS ) {
		timer = (MX_TIMER *) record->record_class_struct;

		timer->mode = MXCM_PRESET_MODE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi45_timer_open( MX_RECORD *record )
{
	const char fname[] = "mxd_pdi45_timer_open()";

	MX_TIMER *timer;
	MX_PDI45_TIMER *pdi45_timer;
	MX_PDI45 *pdi45;
	unsigned long io_type;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	mx_status = mxd_pdi45_timer_get_pointers( timer,
					&pdi45_timer, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is the digital I/O line for this record configured as
	 * a time delay output?
	 */

	io_type = pdi45->io_type[ pdi45_timer->line_number ];

	if ( ( io_type < 0x81 )
	  || ( io_type > 0x84 ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The digital I/O line %ld of PDI45 controller '%s' used by "
	"PDI45 timer '%s' is not configured to be a time delay output.",
			pdi45_timer->line_number,
			pdi45->record->name,
			pdi45_timer->record->name );
	}

	/* Digital I/O channels that are configured as I/O types
	 * 0x07 and 0x08 are assumed to be gated by this timer.
	 */

	pdi45_timer->gated_counters_io_field = 1 << pdi45_timer->line_number;

	if ( ( pdi45->io_type[4] == 0x07 )
	  || ( pdi45->io_type[4] == 0x08 ) )
	{
		pdi45_timer->gated_counters_io_field |= 0x10;
	}

	if ( ( pdi45->io_type[5] == 0x07 )
	  || ( pdi45->io_type[5] == 0x08 ) )
	{
		pdi45_timer->gated_counters_io_field |= 0x20;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_timer_is_busy( MX_TIMER *timer )
{
	const char fname[] = "mxd_pdi45_timer_is_busy()";

	MX_PDI45_TIMER *pdi45_timer;
	MX_PDI45 *pdi45;
	char command[80];
	char response[80];
	int num_items, timer_on, hex_value;
	mx_status_type mx_status;

	mx_status = mxd_pdi45_timer_get_pointers( timer,
					&pdi45_timer, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "00*000%02X", 1 << ( pdi45_timer->line_number ) );

	mx_status = mxi_pdi45_command( pdi45, command,
					response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response + 1, "%1X%4X", &timer_on, &hex_value );

	if ( num_items != 2 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unparseable response '%s' to command '%s' for record '%s'.",
			response, command, timer->record->name );
	}

	MX_DEBUG(-2,("%s: timer_on = %d, hex_value = %#04x",
		fname, timer_on, hex_value ));

	if ( timer_on ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi45_timer_start( MX_TIMER *timer )
{
	const char fname[] = "mxd_pdi45_timer_start()";

	MX_PDI45_TIMER *pdi45_timer;
	MX_PDI45 *pdi45;
	char command[80];
	int hex_value;
	mx_status_type mx_status;

	mx_status = mxd_pdi45_timer_get_pointers( timer,
					&pdi45_timer, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for %g seconds.", fname, timer->value));

	/* Set the measurement time. */

	hex_value = mx_round( 100.0 * timer->value );

	sprintf( command, "00*100%02X%04X",
		1 << ( pdi45_timer->line_number ), hex_value );

	mx_status = mxi_pdi45_command( pdi45, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable gated counters. */

	sprintf( command, "00U%02lX", pdi45_timer->gated_counters_io_field );

	mx_status = mxi_pdi45_command( pdi45, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn on the timer. */

	sprintf( command, "00K%02X", 1 << ( pdi45_timer->line_number ) );

	mx_status = mxi_pdi45_command( pdi45, command, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi45_timer_stop( MX_TIMER *timer )
{
	const char fname[] = "mxd_pdi45_timer_stop()";

	MX_PDI45_TIMER *pdi45_timer;
	MX_PDI45 *pdi45;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_pdi45_timer_get_pointers( timer,
					&pdi45_timer, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off the timer. */

	sprintf( command, "00L%02X", 1 << ( pdi45_timer->line_number ) );

	mx_status = mxi_pdi45_command( pdi45, command, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi45_timer_clear( MX_TIMER *timer )
{
	mx_status_type mx_status;

	timer->value = 0.0;

	mx_status = mxd_pdi45_timer_stop( timer );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi45_timer_read( MX_TIMER *timer )
{
	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_timer_get_mode( MX_TIMER *timer )
{
	timer->mode = MXCM_PRESET_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_timer_set_mode( MX_TIMER *timer )
{
	const char fname[] = "mxd_pdi45_timer_set_mode()";

	if ( timer->mode != MXCM_PRESET_MODE ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Only preset time mode is supported for PDI45 timer '%s'.",
			timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

