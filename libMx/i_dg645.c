/*
 * Name:    i_dg645.c
 *
 * Purpose: MX driver for Stanford Research Systems DG645 Digital
 *          Delay Generator
 *          
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017-2019, 2022-2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_DG645_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_pulse_generator.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_dg645.h"

MX_RECORD_FUNCTION_LIST mxi_dg645_record_function_list = {
	NULL,
	mxi_dg645_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_dg645_open,
	NULL,
	NULL,
	NULL,
	mxi_dg645_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_dg645_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_DG645_STANDARD_FIELDS
};

long mxi_dg645_num_record_fields
		= sizeof( mxi_dg645_record_field_defaults )
			/ sizeof( mxi_dg645_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_dg645_rfield_def_ptr
			= &mxi_dg645_record_field_defaults[0];

/*---*/

static mx_status_type mxi_dg645_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/*---*/

MX_EXPORT mx_status_type
mxi_dg645_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_dg645_create_record_structures()";

	MX_DG645 *dg645;

	/* Allocate memory for the necessary structures. */

	dg645 = (MX_DG645 *) malloc( sizeof(MX_DG645) );

	if ( dg645 == (MX_DG645 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DG645 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = dg645;

	record->record_function_list = &mxi_dg645_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	dg645->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_dg645_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_dg645_open()";

	MX_DG645 *dg645 = NULL;
	unsigned long flags;
	MX_RECORD_FIELD *rs_field = NULL;
	char response[80];
	unsigned long major, minor, update;
	int num_items;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

#if MXI_DG645_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	dg645 = (MX_DG645 *) record->record_type_struct;

	if ( dg645 == (MX_DG645 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DG645 pointer for record '%s' is NULL.", record->name);
	}

	flags = dg645->dg645_flags;

	MXW_UNUSED(flags);

	/* Verify that this is an SRS DG645 controller. */

	mx_status = mxi_dg645_command( dg645, "*IDN?",
					response, sizeof(response),
					MXI_DG645_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response,
		"Stanford Research Systems,DG645,s/n%lu,ver%lu.%lu.%lu",
		&(dg645->serial_number), &major, &minor, &update );

	if ( num_items != 4 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Could not find the serial number and firmware version "
		"in the response '%s' to a '*IDN?' command for "
		"DG645 controller '%s'.",
			response, record->name );
	}

	dg645->firmware_version = 1000000L * major + 1000L * minor + update;

	/* If requested, recall a specific set of instrument settings. */

	if ( dg645->instrument_settings >= 0 ) {
		rs_field = mx_get_record_field( record, "recall_settings" );

		if ( rs_field == (MX_RECORD_FIELD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Could not find the 'recall_settings' field "
			"DG645 controller '%s'.", record->name );
		}
		
		mx_status = mxi_dg645_process_function( record, rs_field,
							NULL, MX_PROCESS_PUT );
	}

	/* Unconditionally turn burst mode off. */

	mx_status = mxi_dg645_command( dg645, "BURM 0",
					NULL, 0, MXI_DG645_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable status registers for the DG645. */

	mx_status = mxi_dg645_command( dg645, "*ESE 255",
					NULL, 0, MXI_DG645_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_dg645_command( dg645, "INSE 255",
					NULL, 0, MXI_DG645_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	/* Set the voltages of the 5 outputs T0, AB, CD, EF, and GH. */

	mx_status = mx_process_record_field_by_name( record, "output_voltage",
							MX_PROCESS_PUT, NULL );
#endif

	/* Clear out any status bits left over from previous runs. */

	mx_status = mxi_dg645_get_status( dg645 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear some MX status bits. */

	dg645->armed = FALSE;
	dg645->triggered = FALSE;
	dg645->burst_mode_on = FALSE;

#if 0
	MX_DEBUG(-2,
	("%s complete for record '%s'.", fname, dg645->record->name));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_dg645_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_DG645_OUTPUT_VOLTAGE:
		case MXLV_DG645_RECALL_SETTINGS:
		case MXLV_DG645_SAVE_SETTINGS:
		case MXLV_DG645_TRIGGER_LEVEL:
		case MXLV_DG645_TRIGGER_RATE:
		case MXLV_DG645_TRIGGER_SOURCE:
		case MXLV_DG645_STATUS:
			record_field->process_function
					= mxi_dg645_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}


/*---*/

MX_EXPORT mx_status_type
mxi_dg645_command( MX_DG645 *dg645,
		char *command,
		char *response,
		size_t max_response_length,
		unsigned long dg645_flags )
{
	static const char fname[] = "mxi_dg645_command()";

	MX_RECORD *interface_record;
	long gpib_address = -1;
	mx_bool_type debug;

	char lerr_response[40];
	int num_items, lerr_code;
	mx_status_type mx_status;

	if ( dg645 == (MX_DG645 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DG645 pointer passed was NULL." );
	}

	if ( dg645_flags & MXF_DG645_DEBUG ) {
		debug = TRUE;
	} else
	if ( dg645->dg645_flags & MXF_DG645_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,
		("%s: ===========================================", fname));
	}

	interface_record = dg645->dg645_interface.record;

	if ( interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The interface record pointer for DG645 interface '%s' is NULL.",
			dg645->record->name );
	}

	/* Send the command and get the response. */

	if ( ( command != NULL ) && debug ) {
		MX_DEBUG(-2,("%s: sending command '%s' to '%s'.",
		    fname, command, dg645->record->name));
	}

	if ( interface_record->mx_class == MXI_RS232 ) {

		if ( command != NULL ) {
			mx_status = mx_rs232_putline( interface_record,
							command, NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		if ( response != NULL ) {
			mx_status = mx_rs232_getline( interface_record,
							response,
							max_response_length,
							NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

	} else {	/* GPIB */

		gpib_address = dg645->dg645_interface.address;

		if ( command != NULL ) {
			mx_status = mx_gpib_putline( interface_record,
							gpib_address,
							command, NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		if ( response != NULL ) {
			mx_status = mx_gpib_getline(
					interface_record, gpib_address,
					response, max_response_length, NULL, 0);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	if ( ( response != NULL ) && debug ) {
		MX_DEBUG(-2,("%s: received response '%s' from '%s'.",
			fname, response, dg645->record->name ));
	}

	/* See if there was an error in the previous command. */

	if ( TRUE ) {

		if ( interface_record->mx_class == MXI_RS232 ) {
			mx_status = mx_rs232_putline( interface_record,
						"LERR?", NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_rs232_getline( interface_record,
						lerr_response,
						sizeof(lerr_response),
						NULL, 0 );
		} else {	/* GPIB */
			mx_status = mx_gpib_putline( interface_record,
						gpib_address,
						"LERR?", NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_gpib_getline( interface_record,
						gpib_address,
						lerr_response,
						sizeof(lerr_response),
						NULL, 0 );
		}

		num_items = sscanf( lerr_response, "%d", &lerr_code );

		if ( num_items != 1 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Did not see an error code in the response '%s' "
			"to an 'LERR?' command for DG645 controller '%s'.",
				lerr_response, dg645->record->name );
		}

		dg645->last_error = lerr_code;

		if ( lerr_code != 0 ) {
			mx_status = mx_error( MXE_INTERFACE_ACTION_FAILED,fname,
			"Command '%s' sent to DG645 controller '%s' failed "
			"with an LERR error code of %d.  Error = '%s'.",
				command, dg645->record->name, lerr_code,
				mxi_dg645_get_lerr_message( lerr_code ) );

			return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_dg645_compute_delay_between_channels( MX_DG645 *dg645,
				unsigned long original_channel,
				unsigned long requested_channel,
				double *requested_delay,
				unsigned long *adjacent_channel,
				double *adjacent_delay )
{
	static const char fname[] =
		"mxi_dg645_compute_delay_between_channels()";

	unsigned long current_channel, next_channel;
	double channel_delay;
	int i, max_steps, num_items;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	/* We must loop through the various reported delays until we get
	 * to the requested channel.
	 */

	max_steps = 20;
	*requested_delay = 0.0;

	current_channel = original_channel;

	for ( i = 0; i < max_steps; i++ ) {
		snprintf( command, sizeof(command),
			"DLAY?%lu", current_channel );

		mx_status = mxi_dg645_command( dg645, command,
					response, sizeof(response),
					MXI_DG645_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lu,%lg",
					&next_channel, &channel_delay );

		if ( num_items != 2 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not find the expected channel delay information "
			"in the response '%s' to command '%s' sent to "
			"DG645 controller '%s'.",
				response, command, dg645->record->name );
		}

		*requested_delay += channel_delay;

		if ( adjacent_channel != NULL ) {
			*adjacent_channel = next_channel;
			*adjacent_delay = channel_delay;
		}

		if ( next_channel == requested_channel ) {
			/* We have reached the requested channel,
			 * so we are done.
			 */
			break;
		}
	}

	if ( i >= max_steps ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Somehow DG645 controller '%s' seems to have an infinite loop "
		"in its delay settings.  We did not reach the time for "
		"requestd channel %lu even after %d steps.",
			dg645->record->name, requested_channel, i );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_dg645_get_status( MX_DG645 *dg645 )
{
	static const char fname[] = "mxi_dg645_get_status()";

	char response[80];
	int num_items;
	mx_status_type mx_status;

	if ( dg645 == (MX_DG645 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DG645 pointer passed was NULL." );
	}

	dg645->status_update_succeeded = FALSE;

	dg645->event_status_register = 0;
	dg645->instrument_status_register = 0;
	dg645->operation_complete = 0;
	dg645->status_byte = 0;

	/* Get the operation complete bit first. */

	mx_status = mxi_dg645_command( dg645, "*OPC?",
					response, sizeof(response),
					MXI_DG645_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu", &(dg645->operation_complete) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The response '%s' by interface '%s' to the command '*STB?' "
		"did not seem to contain the numerical value of "
		"the operation complete flag.", response, dg645->record->name );
	}

	/* Get the status byte. */

	mx_status = mxi_dg645_command( dg645, "*STB?",
					response, sizeof(response),
					MXI_DG645_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu", &(dg645->status_byte) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The response '%s' by interface '%s' to the command '*STB?' "
		"did not seem to contain the numerical value of "
		"the status byte.", response, dg645->record->name );
	}

	/* Get the instrument status register. */

	mx_status = mxi_dg645_command( dg645, "INSR?",
					response, sizeof(response),
					MXI_DG645_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu",
			&(dg645->instrument_status_register) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The response '%s' by interface '%s' to the command '*STB?' "
		"did not seem to contain the numerical value of "
		"the instrument status register.",
			response, dg645->record->name );
	}

	/* Get the event status register. */

	mx_status = mxi_dg645_command( dg645, "*ESR?",
					response, sizeof(response),
					MXI_DG645_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu", &(dg645->event_status_register) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The response '%s' by interface '%s' to the command '*STB?' "
		"did not seem to contain the numerical value of "
		"the event status register.",
			response, dg645->record->name );
	}

	/* Indicate that we successfully read the status. */

	dg645->status_update_succeeded = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

/*==================================================================*/

static mx_status_type
mxi_dg645_process_function( void *record_ptr,
			void *record_field_ptr,
			void *socket_handler_ptr,
			int operation )
{
	static const char fname[] = "mxi_dg645_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_DG645 *dg645;
	char command[80];
	char response[80];
	int i, num_items, polarity;
	double voltage;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	dg645 = (MX_DG645 *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_DG645_TRIGGER_LEVEL:
			mx_status = mxi_dg645_command( dg645, "TLVL?",
						response, sizeof(response),
						MXI_DG645_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lg",
						&(dg645->trigger_level) );

			if ( num_items != 1 ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Did not find a trigger level value in "
				"the response '%s' to command 'TLVL?' for "
				"DG645 controller '%s'.",
					response, record->name );
			}
			break;
		case MXLV_DG645_TRIGGER_RATE:
			mx_status = mxi_dg645_command( dg645, "TRAT?",
						response, sizeof(response),
						MXI_DG645_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lg",
						&(dg645->trigger_rate) );

			if ( num_items != 1 ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Did not find a trigger rate value in "
				"the response '%s' to command 'TRAT?' for "
				"DG645 controller '%s'.",
					response, record->name );
			}
			break;
		case MXLV_DG645_TRIGGER_SOURCE:
			mx_status = mxi_dg645_command( dg645, "TSRC?",
						response, sizeof(response),
						MXI_DG645_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lu",
						&(dg645->trigger_source) );

			if ( num_items != 1 ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Did not find a trigger source value in "
				"the response '%s' to command 'TSRC?' for "
				"DG645 controller '%s'.",
					response, record->name );
			}
			break;
		case MXLV_DG645_STATUS:
			mx_status = mxi_dg645_get_status( dg645 );
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_DG645_OUTPUT_VOLTAGE:
			for ( i = 0; i < MXU_DG645_NUM_OUTPUTS; i++ ) {
				voltage = dg645->output_voltage[i];

				/* Skip a voltage that is out of range. */

				if ( (voltage < -5.0) && (voltage > 5.0) ) {
					continue;
				}

				/* The polarity and the magnitude must be
				 * handled separately.
				 */

				if ( voltage < 0.0 ) {
					polarity = 0;
				} else {
					polarity = 1;
				}

				snprintf( command, sizeof(command),
					"LPOL %d,%d", i, polarity );

				mx_status = mxi_dg645_command( dg645, command,
						NULL, 0, MXI_DG645_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				snprintf( command, sizeof(command),
					"LAMP %d,%g", i, fabs(voltage) );

				mx_status = mxi_dg645_command( dg645, command,
						NULL, 0, MXI_DG645_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
			break;

		case MXLV_DG645_RECALL_SETTINGS:
			if ( dg645->recall_settings == 0 ) {
				return mx_status;
			}
			dg645->recall_settings = 0;

			if ( dg645->instrument_settings < 0 ) {
				return mx_status;
			} else
			if ( dg645->instrument_settings >= 10 ) {
				mx_status =  mx_error(
				MXE_ILLEGAL_ARGUMENT, fname,
				"Instrument settings parameter %ld is "
				"outside the allowed range of 0 to 9 "
				"for DG645 controller '%s'",
					dg645->instrument_settings,
					record->name );

				dg645->instrument_settings = -1;
				return mx_status;
			}
			snprintf( command, sizeof(command),
				"*RCL %ld", dg645->instrument_settings );

			mx_status = mxi_dg645_command( dg645, command,
							NULL, 0,
							MXI_DG645_DEBUG );
			break;
		case MXLV_DG645_SAVE_SETTINGS:
			if ( dg645->save_settings == 0 ) {
				return mx_status;
			}
			dg645->save_settings = 0;

			if ( dg645->instrument_settings < 0 ) {
				return mx_status;
			} else
			if ( dg645->instrument_settings >= 10 ) {
				mx_status =  mx_error(
				MXE_ILLEGAL_ARGUMENT, fname,
				"Instrument settings parameter %ld is "
				"outside the allowed range of 0 to 9 "
				"for DG645 controller '%s'",
					dg645->instrument_settings,
					record->name );

				dg645->instrument_settings = -1;
				return mx_status;
			}
			snprintf( command, sizeof(command),
				"*SAV %ld", dg645->instrument_settings );

			mx_status = mxi_dg645_command( dg645, command,
							NULL, 0,
							MXI_DG645_DEBUG );
			break;
		case MXLV_DG645_TRIGGER_LEVEL:
			snprintf( command, sizeof(command),
				"TLVL %g", dg645->trigger_level );

			mx_status = mxi_dg645_command( dg645, command,
							NULL, 0,
							MXI_DG645_DEBUG );
			break;
		case MXLV_DG645_TRIGGER_RATE:
			snprintf( command, sizeof(command),
				"TRAT %g", dg645->trigger_rate );

			mx_status = mxi_dg645_command( dg645, command,
							NULL, 0,
							MXI_DG645_DEBUG );
			break;
		case MXLV_DG645_TRIGGER_SOURCE:
			snprintf( command, sizeof(command),
				"TSRC %lu", dg645->trigger_source );

			mx_status = mxi_dg645_command( dg645, command,
							NULL, 0,
							MXI_DG645_DEBUG );
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
	}

	return mx_status;
}

/*==================================================================*/

static struct lerr_message_type {
    unsigned long lerr_code;
    const char lerr_message[150];
} lerr_message_array[] = {

{ 0, "No Error --- No more errors left in the queue." },
{ 10, "Illegal Value --- A parameter was out of range." },
{ 11, "Illegal Mode --- The action is illegal in the current mode." },
{ 12, "Illegal Delay --- The requested delay is out of range." },
{ 13, "Illegal Link --- The requested delay linkage is illegal." },
{ 14, "Recall Failed --- The recall of instrument settings from nonvolatile "
        "storage failed. The instrument settings were invalid." },
{ 15, "Not Allowed --- The requested action is not allowed because the "
        "instrument is locked by another interface." },
{ 16, "Failed Self Test --- The DG645 self test failed." },
{ 17, "Failed Auto Calibration --- The DG645 auto calibration failed." },
{ 30, "Lost Data --- Data in the output buffer was lost." },
{ 32, "No Listener --- This is a communications error that occurs if the DG645 "
        "is addressed to talk on the GPIB bus, but there are no listeners." },
{ 40, "Failed ROM Check --- The ROM checksum failed. The firmware code "
        "is likely corrupted." },
{ 41, "Failed Offset T0 Test --- Self test of offset functionality "
        "for output T0 failed." },
{ 42, "Failed Offset AB Test --- Self test of offset functionality "
        "for output AB failed." },
{ 43, "Failed Offset CD Test --- Self test of offset functionality "
        "for output CD failed." },
{ 44, "Failed Offset EF Test --- Self test of offset functionality "
        "for output EF failed." },
{ 45, "Failed Offset GH Test --- Self test of offset functionality "
        "for output GH failed." },
{ 46, "Failed Amplitude T0 Test --- Self test of amplitude functionality "
        "for output T0 failed." },
{ 47, "Failed Amplitude AB Test --- Self test of amplitude functionality "
        "for output AB failed." },
{ 48, "Failed Amplitude CD Test --- Self test of amplitude functionality "
        "for output CD failed." },
{ 49, "Failed Amplitude EF Test --- Self test of amplitude functionality "
        "for output EF failed." },
{ 50, "Failed Amplitude GH Test --- Self test of amplitude functionality "
        "for output GH failed." },
{ 51, "Failed FPGA Communications Test --- Self test of FPGA communications "
        "failed." },
{ 52, "Failed GPIB Communications Test --- Self test of GPIB communications "
        "failed." },
{ 53, "Failed DDS Communications Test --- Self test of DDS communications "
        "failed." },
{ 54, "Failed Serial EEPROM Communications Test --- Self test of serial "
        "EEPROM communications failed." },
{ 55, "Failed Temperature Sensor Communications Test --- Self test of the "
        "temperature sensor communications failed." },
{ 56, "Failed PLL Communications Test --- Self test of PLL communications "
        "failed." },
{ 57, "Failed DAC 0 Communications Test --- Self test of DAC 0 communications "
        "failed." },
{ 58, "Failed DAC 1 Communications Test --- Self test of DAC 1 communications "
        "failed." },
{ 59, "Failed DAC 2 Communications Test --- Self test of DAC 2 communications "
        "failed." },
{ 60, "Failed Sample and Hold Operations Test --- Self test of sample and "
        "hold operations failed." },
{ 61, "Failed Vjitter Operations Test --- Self test of Vjitter operation "
        "failed." },
{ 62, "Failed Channel T0 Analog Delay Test --- Self test of channel T0 "
        "analog delay failed." },
{ 63, "Failed Channel T1 Analog Delay Test --- Self test of channel T1 "
        "analog delay failed." },
{ 64, "Failed Channel A Analog Delay Test --- Self test of channel A "
        "analog delay failed." },
{ 65, "Failed Channel B Analog Delay Test --- Self test of channel B "
        "analog delay failed." },
{ 66, "Failed Channel C Analog Delay Test --- Self test of channel C "
        "analog delay failed." },
{ 67, "Failed Channel D Analog Delay Test --- Self test of channel D "
        "analog delay failed." },
{ 68, "Failed Channel E Analog Delay Test --- Self test of channel E "
        "analog delay failed." },
{ 69, "Failed Channel F Analog Delay Test --- Self test of channel F "
        "analog delay failed." },
{ 70, "Failed Channel G Analog Delay Test --- Self test of channel G "
        "analog delay failed." },
{ 71, "Failed Channel H Analog Delay Test --- Self test of channel H "
        "analog delay failed." },
{ 80, "Failed Sample and Hold Calibration --- Auto calibration of sample "
        "and hold DAC failed." },
{ 81, "Failed T0 Calibration --- Auto calibration of channel T0 failed." },
{ 82, "Failed T1 Calibration --- Auto calibration of channel T1 failed." },
{ 83, "Failed A Calibration --- Auto calibration of channel A failed." },
{ 84, "Failed B Calibration --- Auto calibration of channel B failed." },
{ 85, "Failed C Calibration --- Auto calibration of channel C failed." },
{ 86, "Failed D Calibration --- Auto calibration of channel D failed." },
{ 87, "Failed E Calibration --- Auto calibration of channel E failed." },
{ 88, "Failed F Calibration --- Auto calibration of channel F failed." },
{ 89, "Failed G Calibration --- Auto calibration of channel G failed." },
{ 90, "Failed H Calibration --- Auto calibration of channel H failed." },
{ 91, "Failed Vjitter Calibration --- Auto calibration of Vjitter failed." },
{ 110, "Illegal Command --- The command syntax used was illegal." },
{ 111, "Undefined Command --- The specified command does not exist." },
{ 112, "Illegal Query --- The specified command does not permit queries." },
{ 113, "Illegal Set --- The specified command can only be queried." },
{ 114, "Null Parameter --- The parser detected an empty parameter." },
{ 115, "Extra Parameters --- The parser detected more parameters than "
        "allowed by the command." },
{ 116, "Missing Parameters --- The parser detected missing parameters "
        "required by the command." },
{ 117, "Parameter Overflow --- The buffer for storing parameter values "
        "overflowed. This probably indicates a syntax error." },
{ 118, "Invalid Floating Point Number --- The parser expected a floating "
        "point number, but was unable to parse it." },
{ 120, "Invalid Integer --- The parser expected an integer, but was unable "
        "to parse it." },
{ 121, "Integer Overflow --- A parsed integer was too large to "
        "store correctly." },
{ 122, "Invalid Hexadecimal --- The parser expected hexadecimal characters "
        "but was unable to parse them." },
{ 126, "Syntax Error --- The parser detected a syntax error in the command." },
{ 170, "Communication Error --- A communication error was detected. This is "
        "reported if the hardware detects a framing, or parity error in "
        "the data stream." },
{ 171, "Over run --- The input buffer of the remote interface overflowed. "
        "All data in both the input and output buffers will be flushed." },
{ 254, "Too Many Errors --- The error buffer is full. Subsequent errors "
        "have been dropped." },
};

static size_t num_lerr_messages = sizeof( lerr_message_array )
	                        / sizeof( lerr_message_array[0] );

MX_EXPORT const char *
mxi_dg645_get_lerr_message( unsigned long lerr_code )
{
	size_t i;
	const char *lerr_message_ptr = NULL;
	static const char unrecognized_lerr_message[] =
				"Unrecognized LERR message code.";

	for ( i = 0; i < num_lerr_messages; i++ ) {
		if ( lerr_code == lerr_message_array[i].lerr_code ) {
			lerr_message_ptr = lerr_message_array[i].lerr_message;
			break;
		}
	}

	if ( lerr_message_ptr != NULL ) {
		return lerr_message_ptr;
	}

	return unrecognized_lerr_message;
}

