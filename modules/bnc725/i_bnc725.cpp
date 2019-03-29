/*
 * Name:    i_bnc725.cpp
 *
 * Purpose: MX driver for the vendor-provided Win32 C++ library for the
 *          BNC725 digital delay generator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010-2011, 2016, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_BNC725_DEBUG		TRUE

#include <stdio.h>
#include <ctype.h>
#include <windows.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_process.h"
#include "mx_pulse_generator.h"
#include "i_bnc725.h"
#include "d_bnc725.h"

MX_RECORD_FUNCTION_LIST mxi_bnc725_record_function_list = {
	NULL,
	mxi_bnc725_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_bnc725_open,
	mxi_bnc725_close,
	NULL,
	NULL,
	mxi_bnc725_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_bnc725_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_BNC725_STANDARD_FIELDS
};

long mxi_bnc725_num_record_fields
		= sizeof( mxi_bnc725_record_field_defaults )
			/ sizeof( mxi_bnc725_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_bnc725_rfield_def_ptr
			= &mxi_bnc725_record_field_defaults[0];

static mx_status_type
mxi_bnc725_get_pointers( MX_RECORD *record,
				MX_BNC725 **bnc725,
				const char *calling_fname )
{
	static const char fname[] = "mxi_bnc725_get_pointers()";

	MX_BNC725 *bnc725_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	bnc725_ptr = (MX_BNC725 *) record->record_type_struct;

	if ( bnc725_ptr == (MX_BNC725 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BNC725 pointer for record '%s' is NULL.",
			record->name );
	}

	if ( bnc725 != (MX_BNC725 **) NULL ) {
		*bnc725 = bnc725_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

static mx_status_type mxip_bnc725_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/*------*/

MX_EXPORT mx_status_type
mxi_bnc725_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_bnc725_create_record_structures()";

	MX_BNC725 *bnc725;

	/* Allocate memory for the necessary structures. */

	bnc725 = (MX_BNC725 *) malloc( sizeof(MX_BNC725) );

	if ( bnc725 == (MX_BNC725 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_BNC725 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = bnc725;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	bnc725->record = record;

	bnc725->port = new CLC880;

	if ( bnc725->port == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to create a CLC880 port object." );
	}

	bnc725->enabled = FALSE;

	memset( bnc725->channel_record_array, 0,
		MXU_BNC725_NUM_CHANNELS * sizeof(MX_RECORD *) );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_bnc725_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_bnc725_open()";

	MX_BNC725 *bnc725;
	int i, c, num_items, com_number, com_speed, bnc_status;
	mx_status_type mx_status;

#if MXI_BNC725_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	mx_status = mxi_bnc725_get_pointers( record, &bnc725, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the COM port number. */

	if ( mx_strncasecmp( "COM", bnc725->port_name, 3 ) != 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The port name '%s' specified for BNC725 '%s' is not "
		"a COM port name.", record->name );
	}

	/* Force the COM port name to upper case. */

	for ( i = 0; i < 3; i++ ) {
		c = bnc725->port_name[i];

		if ( islower(c) ) {
			bnc725->port_name[i] = (char) toupper(c);
		}
	}

	/* Parse out the COM port number. */

	num_items = sscanf( bnc725->port_name, "COM%d:", &com_number );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Could not get the COM port number from port name '%s' "
		"for BNC725 '%s'.", bnc725->port_name, record->name );
	}

	/* Convert the port speed into the form 
	 * expected by the vendor library.
	 */

	switch( bnc725->port_speed ) {
	case 2400:
		com_speed = COMBAUD_2400;
		break;
	case 4800:
		com_speed = COMBAUD_4800;
		break;
	case 9600:
		com_speed = COMBAUD_9600;
		break;
	case 19200:
		com_speed = COMBAUD_19200;
		break;
	case 38400:
		com_speed = COMBAUD_38400;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported port speed %lu requested for BNC725 '%s'.  "
		"The supported port speeds are 2400, 4800, 9600, 19200, "
		"and 38400.", bnc725->port_speed, record->name );
		break;
	}

#if MXI_BNC725_DEBUG
	MX_DEBUG(-2,("%s: BNC725 '%s', com_number = %d, com_speed = %d.",
		fname, record->name, com_number, com_speed));
#endif

	/* Open a connection to the BNC 725 controller. */

	bnc_status = bnc725->port->InitConnection( com_number, com_speed );

	if ( bnc_status != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to open a connection on '%s' "
		"to BNC725 '%s' failed.  status = %d",
			bnc725->port_name,
			record->name, bnc_status );
	}

	/* Program all settings to defaults. */

	bnc_status = bnc725->port->CmdUpdateAll();

	if ( bnc_status == 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to program all settings of BNC725 '%s' failed.  "
		"status = %d", record->name, bnc_status );
	}

#if MXI_BNC725_DEBUG
	MX_DEBUG(-2,("%s: BNC725 '%s' successfully initialized.",
		fname, record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_bnc725_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_bnc725_close()";

	MX_BNC725 *bnc725;
	int bnc_status;
	mx_status_type mx_status;

	mx_status = mxi_bnc725_get_pointers( record, &bnc725, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bnc_status = bnc725->port->CloseConnection();

	if ( bnc_status != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to close the connection to BNC725 '%s' failed.  "
		"BNC status = %d",  record->name, bnc_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_bnc725_special_processing_setup( MX_RECORD *record )
{
#if MXI_BNC725_DEBUG
	static const char fname[] = "mxi_bnc725_special_processing_setup()";
#endif

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

#if MXI_BNC725_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_BNC725_START:
			record_field->process_function
					= mxip_bnc725_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

static mx_status_type
mxip_bnc725_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxip_bnc725_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_BNC725 *bnc725;
	mx_status_type mx_status;

#if MXI_BNC725_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	bnc725 = (MX_BNC725 *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_BNC725_START:
			mx_status = mxi_bnc725_start( bnc725 );
			break;
		case MXLV_BNC725_STOP:
			mx_status = mxi_bnc725_stop( bnc725 );
			break;
		case MXLV_BNC725_STATUS:
			mx_status = mxi_bnc725_get_status( bnc725 );
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
		break;
	}

	return mx_status;
}

/*------*/

MX_EXPORT mx_status_type
mxi_bnc725_start( MX_BNC725 *bnc725 )
{
#if MXI_BNC725_DEBUG
	static const char fname[] = "mxi_bnc725_start()";
#endif

	MX_RECORD *channel_record = NULL;
	MX_PULSE_GENERATOR *pulser = NULL;
	MX_BNC725_CHANNEL *bnc725_channel = NULL;
	unsigned long i;
	mx_status_type mx_status;

#if MXI_BNC725_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	for ( i = 0; i < MXU_BNC725_NUM_CHANNELS; i++ ) {
		channel_record = bnc725->channel_record_array[i];

		if ( channel_record != (MX_RECORD *) NULL ) {
			pulser = (MX_PULSE_GENERATOR *)
				channel_record->record_class_struct;
			bnc725_channel = (MX_BNC725_CHANNEL *)
				channel_record->record_type_struct;

			mx_status = mxdp_bnc725_setup_channel( pulser,
						bnc725_channel, bnc725 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	mx_status = mxip_bnc725_start_logic( bnc725 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_bnc725_stop( MX_BNC725 *bnc725 )
{
	static const char fname[] = "mxi_bnc725_stop()";

	int bnc_status;

#if MXI_BNC725_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	bnc_status = bnc725->port->CmdDisableOutputs();

	if ( bnc_status != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to stop the pulsers for BNC725 '%s' failed.  "
		"status = %d",
			bnc725->record->name, bnc_status );
	}

	bnc725->enabled = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_bnc725_get_status( MX_BNC725 *bnc725 )
{
#if MXI_BNC725_DEBUG
	static const char fname[] = "mxi_bnc725_get_status()";

	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* FIXME: Surely we can do better than this? */

	if ( bnc725->enabled ) {
		bnc725->status = MXF_BNC725_ENABLED;
	} else {
		bnc725->status = 0;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxip_bnc725_start_logic( MX_BNC725 *bnc725 )
{
	static const char fname[] = "mxip_bnc725_start_logic()";

	int bnc_status;

#if MXI_BNC725_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* Setting the global logic for the controller as a whole. */

	bnc_status = bnc725->port->SetGlobalLogic( bnc725->global_logic );

	if ( bnc_status != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to set the global logic to '%s' for "
		"BNC725 '%s' failed.  status = %d",
			bnc725->global_logic,
			bnc725->record->name, bnc_status );
	}

	/* Disable the inputs and outputs. */

	bnc_status = bnc725->port->CmdDisableOutputs();

	if ( bnc_status != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to disable the inputs and outputs for "
		"BNC725 '%s' failed.  status = %d",
			bnc725->global_logic,
			bnc725->record->name, bnc_status );
	}

	/* Send the new logic to the BNC725. */

	bnc_status = bnc725->port->CmdTransferAllLogic();

	if ( bnc_status != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to send the new logic to "
		"BNC725 '%s' failed.  status = %d",
			bnc725->global_logic,
			bnc725->record->name, bnc_status );
	}

	/* Setting the timing modes. */

	bnc_status = bnc725->port->CmdSetAllTimingModes();

	if ( bnc_status != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to set the timing modes for "
		"BNC725 '%s' failed.  status = %d",
			bnc725->global_logic,
			bnc725->record->name, bnc_status );
	}

	/* Setup the logic programming. */

	bnc_status = bnc725->port->CmdImplementLogic();

	if ( bnc_status != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to implement the logic for "
		"BNC725 '%s' failed.  status = %d",
			bnc725->global_logic,
			bnc725->record->name, bnc_status );
	}

	/* Enable inputs and outputs. */

	bnc_status = bnc725->port->CmdEnableOutputs();

	if ( bnc_status != 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to enable the inputs and outputs for "
		"BNC725 '%s' failed.  status = %d",
			bnc725->global_logic,
			bnc725->record->name, bnc_status );
	}

	bnc725->enabled = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

