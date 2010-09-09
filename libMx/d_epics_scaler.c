/*
 * Name:    d_epics_scaler.c
 *
 * Purpose: MX scaler driver for the EPICS APS-XFD developed scaler record.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006, 2008-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "d_epics_timer.h"
#include "d_epics_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_epics_scaler_record_function_list = {
	NULL,
	mxd_epics_scaler_create_record_structures,
	mxd_epics_scaler_finish_record_initialization,
	NULL,
	mxd_epics_scaler_print_structure,
	mxd_epics_scaler_open,
	NULL
};

MX_SCALER_FUNCTION_LIST mxd_epics_scaler_scaler_function_list = {
	NULL,			/* EPICS scalers cannot be cleared. */
	NULL,			/* EPICS scalers do not set an overflow flag. */
	mxd_epics_scaler_read,
	mxd_epics_scaler_read_raw,
	mxd_epics_scaler_is_busy,
	mxd_epics_scaler_start,
	mxd_epics_scaler_stop,
	mxd_epics_scaler_get_parameter,
	mxd_epics_scaler_set_parameter,
	mxd_epics_scaler_set_modes_of_associated_counters
};

/* EPICS scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_epics_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_EPICS_SCALER_STANDARD_FIELDS
};

long mxd_epics_scaler_num_record_fields
		= sizeof( mxd_epics_scaler_record_field_defaults )
		  / sizeof( mxd_epics_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_scaler_rfield_def_ptr
			= &mxd_epics_scaler_record_field_defaults[0];

static mx_status_type mxd_epics_scaler_get_mode( MX_SCALER * );
static mx_status_type mxd_epics_scaler_set_mode( MX_SCALER * );

/* A private function for the use of the driver. */

static mx_status_type
mxd_epics_scaler_get_pointers( MX_SCALER *scaler,
			MX_EPICS_SCALER **epics_scaler,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_scaler_get_pointers()";

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( epics_scaler == (MX_EPICS_SCALER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scaler->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_SCALER pointer "
		"passed by '%s' was NULL.", calling_fname );
	}

	*epics_scaler = (MX_EPICS_SCALER *) scaler->record->record_type_struct;

	if ( *epics_scaler == (MX_EPICS_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_SCALER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*======*/

MX_EXPORT mx_status_type
mxd_epics_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_epics_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_EPICS_SCALER *epics_scaler = NULL;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	epics_scaler = (MX_EPICS_SCALER *)
				malloc( sizeof(MX_EPICS_SCALER) );

	if ( epics_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPICS_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = epics_scaler;
	record->class_specific_function_list
			= &mxd_epics_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_finish_record_initialization( MX_RECORD *record )
{
	return mx_scaler_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_epics_scaler_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_scaler_print_structure()";

	MX_SCALER *scaler;
	MX_EPICS_SCALER *epics_scaler = NULL;
	long current_value;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	scaler = (MX_SCALER *) (record->record_class_struct);

	mx_status = mxd_epics_scaler_get_pointers( scaler,
						&epics_scaler, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "SCALER parameters for scaler '%s':\n", record->name);

	fprintf(file, "  Scaler type           = EPICS_SCALER.\n\n");
	fprintf(file, "  EPICS record          = %s\n",
					epics_scaler->epics_record_name);
	fprintf(file, "  scaler number         = %ld\n",
					epics_scaler->scaler_number);

	mx_status = mx_scaler_read( record, &current_value );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read value of scaler '%s'",
			record->name );
	}

	fprintf(file, "  present value         = %ld\n", current_value);

	mx_status = mx_scaler_get_dark_current( record, NULL );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read the dark current for scaler '%s'",
			record->name );
	}

	fprintf(file, "  dark current          = %g counts per second.\n",
						scaler->dark_current);
	fprintf(file, "  scaler flags          = %#lx\n",scaler->scaler_flags);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_scaler_open()";

	MX_SCALER *scaler;
	MX_EPICS_SCALER *epics_scaler = NULL;
	double version_number;
	long i, scaler_number;
	char pvname[80];
	char driver_name[80];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	scaler = (MX_SCALER *) (record->record_class_struct);

	mx_status = mxd_epics_scaler_get_pointers( scaler,
						&epics_scaler, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out if this EPICS record really exists in an IOC's database
	 * by asking for the record's version number.
	 */

	snprintf( pvname, sizeof(pvname),
		"%s.VERS", epics_scaler->epics_record_name );

	mx_status = mx_caget_by_name(pvname, MX_CA_DOUBLE, 1, &version_number);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_scaler->epics_record_version = version_number;

	MX_DEBUG( 2, ("%s: EPICS scaler '%s' version number = %g",
		fname, epics_scaler->epics_record_name,
		epics_scaler->epics_record_version));

	/* Initialize MX EPICS data structures. */

	mx_epics_pvname_init( &(epics_scaler->cnt_pv),
				"%s.CNT", epics_scaler->epics_record_name );

	if ( epics_scaler->epics_record_version < 3.0 ) {
		mx_epics_pvname_init( &(epics_scaler->cont_pv),
			"%s_mode.VAL", epics_scaler->epics_record_name );
	} else {
		mx_epics_pvname_init( &(epics_scaler->cont_pv),
			"%s.CONT", epics_scaler->epics_record_name );
	}

	mx_epics_pvname_init( &(epics_scaler->dark_pv),
				"%s_Dark%d.VAL",
					epics_scaler->epics_record_name,
					epics_scaler->scaler_number );

	mx_epics_pvname_init( &(epics_scaler->nch_pv),
				"%s.NCH", epics_scaler->epics_record_name );

	mx_epics_pvname_init( &(epics_scaler->pr_pv),
				"%s.PR%d", epics_scaler->epics_record_name,
					epics_scaler->scaler_number );

	mx_epics_pvname_init( &(epics_scaler->s_pv),
				"%s.S%d", epics_scaler->epics_record_name,
					epics_scaler->scaler_number );

	mx_epics_pvname_init( &(epics_scaler->sd_pv),
				"%s_SD%d.VAL", epics_scaler->epics_record_name,
					epics_scaler->scaler_number );

	/* Find out what type of EPICS scaler record this is. */

	snprintf( pvname, sizeof(pvname),
		"%s.DTYP", epics_scaler->epics_record_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_STRING, 1, driver_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: record '%s' driver_name = '%s'",
		fname, record->name, driver_name));

	if ( strcmp( driver_name, "MxScaler" ) == 0 ) {
		epics_scaler->driver_type = MXT_EPICS_SCALER_MX;
	} else {
		epics_scaler->driver_type = MXT_EPICS_SCALER_BCDA;
	}

	/* Find out how many counter channels the EPICS scaler record has. */

	mx_status = mx_caget( &(epics_scaler->nch_pv),
			MX_CA_SHORT, 1, &(epics_scaler->num_epics_counters) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: Record '%s' num_epics_counters = %hd",
		fname, record->name, epics_scaler->num_epics_counters));

	epics_scaler->gate_control_pv_array = (MX_EPICS_PV *)
		malloc( epics_scaler->num_epics_counters * sizeof(MX_EPICS_PV));

	if ( epics_scaler->gate_control_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %hd elements for gate_control_pv_array "
	"used by scaler record '%s'.",
			epics_scaler->num_epics_counters, record->name );
	}

	for ( i = 0; i < epics_scaler->num_epics_counters; i++ ) {
		mx_epics_pvname_init( &(epics_scaler->gate_control_pv_array[i]),
			"%s.G%ld", epics_scaler->epics_record_name, i+1 );
	}

	/* Check that the scaler number is valid. */

	scaler_number = epics_scaler->scaler_number;

	if ( ( scaler_number < 1 )
	  || ( scaler_number > epics_scaler->num_epics_counters ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"EPICS scaler channel number %ld is out of allowed range 2-%hd",
			scaler_number, epics_scaler->num_epics_counters );
	}

	if ( scaler_number == 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"EPICS scaler channel 1 is reserved for timer use "
		"and cannot be used as a normal scaler.  Use channels 2-%hd",
			epics_scaler->num_epics_counters );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_epics_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_epics_scaler_read()";

	MX_EPICS_SCALER *epics_scaler = NULL;
	int32_t value;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_get_pointers( scaler,
						&epics_scaler, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scaler->scaler_flags & MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT ) {

		mx_status = mx_caget( &(epics_scaler->sd_pv),
					MX_CA_LONG, 1, &value );
	} else {
		mx_status = mx_caget( &(epics_scaler->s_pv),
					MX_CA_LONG, 1, &value );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler->raw_value = value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_read_raw( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_epics_scaler_read_raw()";

	MX_EPICS_SCALER *epics_scaler = NULL;
	int32_t value;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_get_pointers( scaler,
						&epics_scaler, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_scaler->s_pv), MX_CA_LONG, 1, &value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler->raw_value = value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_is_busy( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_epics_scaler_is_busy()";

	MX_EPICS_SCALER *epics_scaler = NULL;
	int32_t count_field;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_get_pointers( scaler,
						&epics_scaler, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_scaler->cnt_pv),
				MX_CA_LONG, 1, &count_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch ( count_field ) {
	case 0:
		scaler->busy = FALSE;
		break;
	case 1:
		scaler->busy = TRUE;
		break;
	default:
		scaler->busy = FALSE;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Count field (.CNT) for scaler '%s' had an unexpected value of %ld",
			epics_scaler->epics_record_name, (long) count_field );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_start( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_epics_scaler_start()";

	MX_EPICS_SCALER *epics_scaler = NULL;
	int32_t value, count_field;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_get_pointers( scaler,
						&epics_scaler, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the scaler preset. */

	value = scaler->raw_value;

	mx_status = mx_caput( &(epics_scaler->pr_pv), MX_CA_LONG, 1, &value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the scaler counting. */

	count_field = 1;

	if ( epics_scaler->epics_record_version < 3.0 ) {
		mx_status = mx_caput( &(epics_scaler->cnt_pv),
					MX_CA_LONG, 1, &count_field );
	} else {
		mx_status = mx_caput_nowait( &(epics_scaler->cnt_pv),
					MX_CA_LONG, 1, &count_field );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_stop( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_epics_scaler_stop()";

	MX_EPICS_SCALER *epics_scaler = NULL;
	int32_t count_field;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_get_pointers( scaler,
						&epics_scaler, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the scaler from counting. */

	count_field = 0;

	mx_status = mx_caput( &(epics_scaler->cnt_pv),
				MX_CA_LONG, 1, &count_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler->raw_value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_get_parameter( MX_SCALER *scaler )
{
	MX_EPICS_SCALER *epics_scaler = NULL;
	double dark_current;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		mx_status = mxd_epics_scaler_get_mode( scaler );
		break;

	case MXLV_SCL_DARK_CURRENT:
		/* If the database is configured such that the server
		 * maintains the dark current rather than the client, then
		 * ask the server for the value.  Otherwise, just return
		 * our locally cached value.
		 */

		if ( scaler->scaler_flags
			& MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT )
		{
			epics_scaler = (MX_EPICS_SCALER *)
				( scaler->record->record_type_struct );

			mx_status = mx_caget( &(epics_scaler->dark_pv),
					MX_CA_DOUBLE, 1, &dark_current );

			scaler->dark_current = dark_current;
		}
		break;

	default:
		mx_status = mx_scaler_default_get_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_set_parameter( MX_SCALER *scaler )
{
	MX_EPICS_SCALER *epics_scaler = NULL;
	double dark_current;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		mx_status = mxd_epics_scaler_set_mode( scaler );
		break;

	case MXLV_SCL_DARK_CURRENT:
		/* If the database is configured such that the server
		 * maintains the dark current rather than the client, then
		 * send the value to the server.  Otherwise, just cache
		 * it locally.
		 */

		if ( scaler->scaler_flags
			& MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT )
		{
			epics_scaler = (MX_EPICS_SCALER *)
				( scaler->record->record_type_struct );

			dark_current = scaler->dark_current;

			mx_status = mx_caput( &(epics_scaler->dark_pv),
					MX_CA_DOUBLE, 1, &dark_current );
		}
		break;

	default:
		mx_status = mx_scaler_default_set_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

static mx_status_type
mxd_epics_scaler_get_mode( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_epics_scaler_get_mode()";

	MX_EPICS_SCALER *epics_scaler = NULL;
	long offset;
	int32_t gate_control;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_get_pointers( scaler,
						&epics_scaler, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the gate control field Gn. */

	offset = epics_scaler->scaler_number - 1;

	mx_status = mx_caget( &(epics_scaler->gate_control_pv_array[ offset ] ),
				MX_CA_LONG, 1, &gate_control );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( gate_control ) {
	case 0:
		scaler->mode = MXCM_COUNTER_MODE;
		break;

	case 1:
		scaler->mode = MXCM_PRESET_MODE;
		break;

	default:
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"The gate control field for EPICS scaler '%s' (%s) "
		"has the unexpected value %ld.",
			scaler->record->name,
			epics_scaler->epics_record_name,
			(long) gate_control );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_epics_scaler_set_mode( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_epics_scaler_set_mode()";

	MX_EPICS_SCALER *epics_scaler = NULL;
	long offset;
	int32_t gate_control, counter_mode;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_get_pointers( scaler,
						&epics_scaler, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Choose the correct gate control mode. */

	switch( scaler->mode ) {
	case MXCM_PRESET_MODE:
		gate_control = 1;
		break;

	case MXCM_COUNTER_MODE:
		gate_control = 0;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The counter mode %ld requested for scaler '%s' is not supported.",
			scaler->mode, scaler->record->name );
		break;
	}

	/* Make sure the counter is _not_ in autocount mode. */

	counter_mode = 0;	/* One-shot mode. */

	mx_status = mx_caput( &(epics_scaler->cont_pv),
				MX_CA_LONG, 1, &counter_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the scaler to stop counting. */

	mx_status = mxd_epics_scaler_stop( scaler );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the gate control field. */

	offset = epics_scaler->scaler_number - 1;

	mx_status = mx_caput( &(epics_scaler->gate_control_pv_array[ offset ] ),
				MX_CA_LONG, 1, &gate_control );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_set_modes_of_associated_counters( MX_SCALER *scaler )
{
	static const char fname[]
		= "mxd_epics_scaler_set_modes_of_associated_counters()";

	MX_EPICS_SCALER *epics_scaler = NULL;
	MX_EPICS_GROUP epics_group;
	int i;
	int32_t gate_control;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_get_pointers( scaler,
						&epics_scaler, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->mode ) {
	case MXCM_PRESET_MODE:
	case MXCM_COUNTER_MODE:
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"The counter mode %ld is not supported by this function.",
			scaler->mode );
		break;
	}

	/* FIXME - If the EPICS scaler is of DTYP 'MxScaler', then
	 * skip this part, since 'mxca_server' does not yet support
	 * synchronous groups.
	 */

	if ( epics_scaler->driver_type == MXT_EPICS_SCALER_MX )
		return MX_SUCCESSFUL_RESULT;

	/* Start an EPICS synchronous group. */

	mx_epics_start_group( &epics_group );

	/* Set the gate control fields for all channels but this one. */

	for ( i = 1; i <= epics_scaler->num_epics_counters; i++ ) {

		/* Skip the channel that corresponds to this scaler. */

		if ( i == epics_scaler->scaler_number ) {
			continue;
		}

		if ( i > 1 ) {
			/* The other scaler channels are always put into
			 * counter mode.
			 */

			gate_control = 0;

		} else {
			/* Channel 1 is the timer.  The timer's gate mode
			 * is always set to the opposite of the counter mode
			 * for the scaler_record variable.
			 */

			if ( scaler->mode == MXCM_PRESET_MODE ) {
				gate_control = 0;	/* counter mode */
			} else {
				gate_control = 1;	/* preset mode */
			}
		}

		mx_status = mx_group_caput( &epics_group,
			&(epics_scaler->gate_control_pv_array[ i-1 ]),
				MX_CA_LONG, 1, &gate_control );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}
	}

	/* Send the synchronous group to EPICS. */

	mx_epics_end_group( &epics_group );

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_EPICS */

