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
 * Copyright 2017-2018 Illinois Institute of Technology
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
						int operation );

static mx_status_type mxi_dg645_interpret_trigger_source( MX_DG645 *dg645 );

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
	char command[80];
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
							MX_PROCESS_PUT );
	}

	/* If requested, turn burst mode on.  Otherwise, turn it off. */

	if ( dg645->instrument_settings < 0 ) {
		if ( flags & MXF_DG645_BURST_MODE ) {
			strlcpy( command, "BURM 1", sizeof(command) );
		} else {
			strlcpy( command, "BURM 0", sizeof(command) );
		}

		mx_status = mxi_dg645_command( dg645, command,
						NULL, 0, MXI_DG645_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

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

	MX_DEBUG(-2,
	("%s complete for record '%s'.", fname, dg645->record->name));

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

#if 1
	MX_DEBUG(-2,("%s: ===========================================", fname));
#endif

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
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Command '%s' sent to DG645 controller '%s' "
			"failed with an LERR error code of %d.",
				command, dg645->record->name, lerr_code );
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

	/* If the INSR summary bit is set, then ask for the
	 * instrument status register.
	 */

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

	/* If the ESR summary bit is set, then ask for the
	 * event status register.
	 */

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

MX_EXPORT mx_status_type
mxi_dg645_setup_pulser_trigger_mode( MX_DG645 *dg645, long new_trigger_mode )
{
	static const char fname[] = "mxi_dg645_setup_pulser_trigger_mode()";

	switch( new_trigger_mode ) {
	case MXF_PGN_INTERNAL_TRIGGER:
		dg645->trigger_type = MXF_DG645_INTERNAL_TRIGGER;
		dg645->trigger_direction = 0;

		if ( dg645->single_shot == FALSE ) {
			dg645->trigger_source = 0;
		} else {
			dg645->trigger_source = 5;
		}
		break;

	case MXF_PGN_EXTERNAL_TRIGGER:
		dg645->trigger_type = MXF_DG645_EXTERNAL_TRIGGER;
		
		if ( dg645->trigger_direction >= 0 ) {
			if ( dg645->single_shot == FALSE ) {
				dg645->trigger_source = 1;
			} else {
				dg645->trigger_source = 3;
			}
		} else {
			if ( dg645->single_shot == FALSE ) {
				dg645->trigger_source = 2;
			} else {
				dg645->trigger_source = 4;
			}
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Trigger source %ld is not supported for pulse generator '%s'.",
			new_trigger_mode, dg645->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==================================================================*/

static mx_status_type
mxi_dg645_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
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

			mx_status = mxi_dg645_interpret_trigger_source( dg645 );
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

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxi_dg645_interpret_trigger_source( dg645 );
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

static mx_status_type
mxi_dg645_interpret_trigger_source( MX_DG645 *dg645 )
{
	static const char fname[] = "mxi_dg645_interpret_trigger_source()";

	switch( dg645->trigger_source ) {
	case 0:
		dg645->trigger_type = MXF_DG645_INTERNAL_TRIGGER;
		dg645->trigger_direction = 0;
		dg645->single_shot = FALSE;
		break;
	case 1:
		dg645->trigger_type = MXF_DG645_EXTERNAL_TRIGGER;
		dg645->trigger_direction = 1;
		dg645->single_shot = FALSE;
		break;
	case 2:
		dg645->trigger_type = MXF_DG645_EXTERNAL_TRIGGER;
		dg645->trigger_direction = -1;
		dg645->single_shot = FALSE;
		break;
	case 3:
		dg645->trigger_type = MXF_DG645_EXTERNAL_TRIGGER;
		dg645->trigger_direction = 1;
		dg645->single_shot = TRUE;
		break;
	case 4:
		dg645->trigger_type = MXF_DG645_EXTERNAL_TRIGGER;
		dg645->trigger_direction = -1;
		dg645->single_shot = TRUE;
		break;
	case 5:
		dg645->trigger_type = MXF_DG645_INTERNAL_TRIGGER;
		dg645->trigger_direction = 0;
		dg645->single_shot = TRUE;
		break;
	case 6:
		dg645->trigger_type = MXF_DG645_LINE_TRIGGER;
		dg645->trigger_direction = 0;
		dg645->single_shot = FALSE;
		break;
	default:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The trigger source for DG645 controller '%s' has "
		"an illegal value of %lu.  The allowed values are "
		"from 0 to 6.",
			dg645->record->name,
			dg645->trigger_source );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

