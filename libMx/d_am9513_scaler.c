/*
 * Name:    d_am9513_scaler.c
 *
 * Purpose: MX scaler driver to use Am9513 counter channels as scalers.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "i_am9513.h"
#include "d_am9513_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_am9513_scaler_record_function_list = {
	mxd_am9513_scaler_initialize_type,
	mxd_am9513_scaler_create_record_structures,
	mxd_am9513_scaler_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_am9513_scaler_open,
	mxd_am9513_scaler_close
};

MX_SCALER_FUNCTION_LIST mxd_am9513_scaler_scaler_function_list = {
	mxd_am9513_scaler_clear,
	mxd_am9513_scaler_overflow_set,
	mxd_am9513_scaler_read,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_am9513_scaler_get_parameter,
	mxd_am9513_scaler_set_parameter
};

/* Am9513 scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_am9513_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_AM9513_SCALER_STANDARD_FIELDS
};

long mxd_am9513_scaler_num_record_fields
		= sizeof( mxd_am9513_scaler_record_field_defaults )
		  / sizeof( mxd_am9513_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_am9513_scaler_rfield_def_ptr
			= &mxd_am9513_scaler_record_field_defaults[0];

static MX_AM9513 *debug_am9513_ptr = &mxi_am9513_debug_struct;

/* A private function for the use of the driver. */

static mx_status_type
mxd_am9513_scaler_get_pointers( MX_SCALER *scaler,
				MX_AM9513_SCALER **am9513_scaler,
				long *num_counters,
				MX_INTERFACE **am9513_interface_array,
				const char *calling_fname )
{
	static const char fname[] = "mxd_am9513_get_pointers()";

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( am9513_scaler == (MX_AM9513_SCALER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AM9513_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( num_counters == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_counters pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( am9513_interface_array == (MX_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERFACE array pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*am9513_scaler = (MX_AM9513_SCALER *)
				(scaler->record->record_type_struct);

	if ( *am9513_scaler == (MX_AM9513_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AM9513_SCALER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	*num_counters = (*am9513_scaler)->num_counters;

	*am9513_interface_array = (*am9513_scaler)->am9513_interface_array;

	if ( *am9513_interface_array == (MX_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INTERFACE array pointer for scaler '%s' is NULL.",
			scaler->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_am9513_scaler_initialize_type( long type )
{
	static const char fname[] = "mxd_am9513_scaler_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults, *field;
	long num_record_fields;
	long num_counters_field_index;
	long num_counters_varargs_cookie;
	mx_status_type mx_status;

	driver = mx_get_driver_by_type( type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.", type );
	}

	record_field_defaults = *(driver->record_field_defaults_ptr);
	num_record_fields = *(driver->num_record_fields);

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults, num_record_fields,
			"num_counters", &num_counters_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
		num_counters_field_index, 0, &num_counters_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"am9513_interface_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_counters_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_am9513_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_AM9513_SCALER *am9513_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	am9513_scaler = (MX_AM9513_SCALER *)
				malloc( sizeof(MX_AM9513_SCALER) );

	if ( am9513_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AM9513_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = am9513_scaler;
	record->class_specific_function_list
			= &mxd_am9513_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_scaler_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_am9513_scaler_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_am9513_scaler_open()";

	MX_AM9513_SCALER *am9513_scaler;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	uint16_t counter_mode_register;
	int i, m, n;
	int same_chip, use_external;
	long num_counters;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s called.", fname));

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	mx_status = mxd_am9513_scaler_get_pointers( record->record_class_struct,
			&am9513_scaler, &num_counters,
			&am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_counters <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The number of counters (%ld) in record '%s' should be greater than zero.",
			num_counters, record->name );
	}

	mx_status = mxi_am9513_grab_counters( record, num_counters,
						am9513_interface_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	this_record = am9513_interface_array[0].record;

	this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

	n = am9513_interface_array[0].address - 1;

	/*
	 * Initialize the mode register for the lowest order counter.
	 *
	 * Settings:
	 *    ( get gate from database param )
	 *    Count on rising edge.
	 *    ( get source from database param )
	 *    Disable special gate.
	 *    Reload from load.
	 *    Count repetitively.
	 *    Binary count.
	 *    Count up.
	 *    ( output depends on configuration )
	 */

	counter_mode_register = (uint16_t)
				( ( am9513_scaler->gating_control ) << 13 );

	counter_mode_register |= (uint16_t)
				( ( am9513_scaler->count_source & 0xf ) << 8 );

	counter_mode_register |= 0x0020;	/* Count repetitively */
	counter_mode_register |= 0x0008;	/* Count up */

	if ( num_counters > 1 ) {
		/* active high terminal pulse count */

		counter_mode_register |= 0x0001;
	} else {
		/* TC toggled */

		counter_mode_register |= 0x0002;
	}

	this_am9513->counter_mode_register[n] = counter_mode_register;

	mx_status = mxi_am9513_set_counter_mode_register( this_am9513, n );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	/*** Initialize the rest of the counters. ***/

	for ( i = 1; i < num_counters; i++ ) {

		this_record = am9513_interface_array[i].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		n = am9513_interface_array[i].address - 1;

		/* Determine whether we use an internal or an external
		 * count source or gate.
		 */

		/* Is the previous 16 bit counter on the same Am9513 chip? */

		if ( this_record == am9513_interface_array[i-1].record ) {
			same_chip = TRUE;
		} else {
			same_chip = FALSE;
		}

		/* If it is on the same chip, is the counter number immediately
		 * before the current one?  If so, we use an internal count
		 * source.  Otherwise, we use the external source with the
		 * same number as the counter number.
		 */
		
		if ( same_chip == FALSE ) {
			use_external = TRUE;
		} else {
			m = am9513_interface_array[i-1].address - 1;

			if ( n == (m+1) ) {
				use_external = FALSE;
			} else {
				use_external = TRUE;
			}
		}

		counter_mode_register = 0;  /* No gating, count rising edge. */

		/* Count source selection.  Source is SRC(n+1) if the
		 * source is external.  Otherwise, the source is TCN-1.
		 */

		if ( use_external ) {
			counter_mode_register |= ( (n+1) << 8 );
		}

		/* No special gate, reload from load, binary count, count up. */

		counter_mode_register |= 0x0008;

		/* The highest order counter's output is handled differently */

		if ( i != (num_counters - 1) ) {

			/* Count repeat, active high terminal pulse count */

			counter_mode_register |= 0x0021;

		} else {
			/* This is the highest order counter. */

			/* Count once, TC toggled */

			counter_mode_register |= 0x0002;
		}

		this_am9513->counter_mode_register[n] = counter_mode_register;

		/* Configure the counter. */

		mx_status = mxi_am9513_set_counter_mode_register(
							this_am9513, n );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_scaler_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_am9513_scaler_close()";

	MX_AM9513_SCALER *am9513_scaler;
	MX_INTERFACE *am9513_interface_array;
	long num_counters;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s called.", fname));

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	mx_status = mxd_am9513_scaler_get_pointers( record->record_class_struct,
			&am9513_scaler, &num_counters,
			&am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_counters <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The number of counters (%ld) in record '%s' should be greater than zero.",
			num_counters, record->name );
	}

	mx_status = mxi_am9513_release_counters( record, num_counters,
						am9513_interface_array );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_am9513_scaler_clear( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_am9513_scaler_clear()";

	MX_AM9513_SCALER *am9513_scaler;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	long num_counters;
	int i, n;
	mx_status_type mx_status;

	mx_status = mxd_am9513_scaler_get_pointers( scaler, &am9513_scaler,
			&num_counters, &am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Prevent GCC from generating warnings about possibly
	 * uninitialized variables.
	 */

	this_record = NULL;
	this_am9513 = NULL;
	n = 0;

	/* Disarm the counters. */

	for ( i = 0; i < num_counters; i++ ) {
		
		this_record = am9513_interface_array[i].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		n = am9513_interface_array[i].address - 1;

		mx_status = mxi_am9513_disarm_counter( this_am9513, n );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );
	}

	/* Clear the counters. */

	for ( i = 0; i < num_counters; i++ ) {
		
		this_record = am9513_interface_array[i].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		n = am9513_interface_array[i].address - 1;

		this_am9513->load_register[n] = 0;

		mx_status = mxi_am9513_load_counter( this_am9513, n );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );
	}

	/* Set OUT(n) for the highest order counter to the low state. */

	if ( num_counters > 0 ) {

		mx_status = mxi_am9513_set_tc( this_am9513, n );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );
	}

	/* Re-arm the counters.  This is most safely done in reverse order. */

	for ( i = num_counters - 1; i >= 0; i-- ) {
		
		this_record = am9513_interface_array[i].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		n = am9513_interface_array[i].address - 1;

		mx_status = mxi_am9513_arm_counter( this_am9513, n );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );
	}

	scaler->raw_value = 0L;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_scaler_overflow_set( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_am9513_scaler_overflow_set()";

	MX_AM9513_SCALER *am9513_scaler;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	long num_counters;
	uint8_t am9513_status;
	int mask;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s called.", fname));

	mx_status = mxd_am9513_scaler_get_pointers( scaler, &am9513_scaler,
			&num_counters, &am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If OUT(n) for the highest order counter is set, the scaler
	 * has overflowed.
	 */

	this_record = am9513_interface_array[0].record;

	this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

	mx_status = mxi_am9513_get_status( this_am9513 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

	am9513_status = this_am9513->status_register;

	MX_DEBUG( 2,("%s: am9513_status = 0x%x", fname, am9513_status));

	mask = 1 << am9513_interface_array[ num_counters - 1 ].address;

	if ( am9513_status & mask ) {
		scaler->overflow_set = TRUE;
	} else {
		scaler->overflow_set = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_am9513_scaler_read()";

	MX_AM9513_SCALER *am9513_scaler;
	MX_INTERFACE *am9513_interface_array;
	MX_RECORD *this_record;
	MX_AM9513 *this_am9513;
	long num_counters;
	int i, n;
	uint16_t hold_register;
	unsigned long scaler_value;
	mx_status_type mx_status;

	mx_status = mxd_am9513_scaler_get_pointers( scaler, &am9513_scaler,
			&num_counters, &am9513_interface_array, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Disarm the counters. */

	for ( i = 0; i < num_counters; i++ ) {
		
		this_record = am9513_interface_array[i].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		n = am9513_interface_array[i].address - 1;

		mx_status = mxi_am9513_disarm_counter( this_am9513, n );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );
	}

	/* Read out the counters. */

	scaler_value = 0L;

	for ( i = num_counters - 1; i >= 0; i-- ) {
		
		this_record = am9513_interface_array[i].record;

		this_am9513 = (MX_AM9513 *) this_record->record_type_struct;

		n = am9513_interface_array[i].address - 1;

		mx_status = mxi_am9513_read_counter( this_am9513, n );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_AM9513_DEBUG( debug_am9513_ptr, 1 );

		hold_register = this_am9513->hold_register[n];

		MX_DEBUG( 2,("%s: counter[%d] hold_register = %hu",
				fname, n, hold_register ));

		scaler_value = 65536L * scaler_value + hold_register;
	}

	scaler->raw_value = (long) scaler_value;

	MX_DEBUG( 2,("%s: scaler->raw_value = %ld", fname, scaler->raw_value));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_scaler_get_parameter( MX_SCALER *scaler )
{
	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		scaler->mode = MXCM_COUNTER_MODE;
		break;
	case MXLV_SCL_DARK_CURRENT:
		break;
	default:
		return mx_scaler_default_get_parameter_handler( scaler );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_am9513_scaler_set_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_am9513_scaler_set_parameter()";

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		if ( scaler->mode != MXCM_COUNTER_MODE ) {
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
				"This operation is not yet implemented." );
		}
		break;
	case MXLV_SCL_DARK_CURRENT:
		break;
	default:
		return mx_scaler_default_set_parameter_handler( scaler );
	}

	return MX_SUCCESSFUL_RESULT;
}

