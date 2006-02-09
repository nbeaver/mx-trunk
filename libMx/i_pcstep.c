/*
 * Name:    i_pcstep.c
 *
 * Purpose: MX interface driver for National Instruments PC-Step
 *          motor controllers (formerly made by nuLogic).
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_PCSTEP_DEBUG	FALSE

#include <stdio.h>

#include "mxconfig.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_generic.h"
#include "mx_stdint.h"
#include "mx_portio.h"
#include "mx_bit.h"
#include "mx_motor.h"

#include "i_pcstep.h"

MX_RECORD_FUNCTION_LIST mxi_pcstep_record_function_list = {
	mxi_pcstep_initialize_type,
	mxi_pcstep_create_record_structures,
	mxi_pcstep_finish_record_initialization,
	mxi_pcstep_delete_record,
	mxi_pcstep_print_structure,
	mxi_pcstep_read_parms_from_hardware,
	mxi_pcstep_write_parms_to_hardware,
	mxi_pcstep_open,
	mxi_pcstep_close,
	NULL,
	mxi_pcstep_resynchronize
};

MX_GENERIC_FUNCTION_LIST mxi_pcstep_generic_function_list = {
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxi_pcstep_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_PCSTEP_STANDARD_FIELDS
};

mx_length_type mxi_pcstep_num_record_fields
		= sizeof( mxi_pcstep_record_field_defaults )
			/ sizeof( mxi_pcstep_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_pcstep_rfield_def_ptr
			= &mxi_pcstep_record_field_defaults[0];

static mx_status_type
mxi_pcstep_get_pointers( MX_RECORD *record,
			MX_PCSTEP **pcstep,
			const char *calling_fname )
{
	const char fname[] = "mxi_pcstep_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( pcstep == (MX_PCSTEP **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_PCSTEP pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pcstep = (MX_PCSTEP *) (record->record_type_struct);

	if ( *pcstep == (MX_PCSTEP *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PCSTEP pointer for record '%s' is NULL.",
			record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxi_pcstep_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pcstep_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxi_pcstep_create_record_structures()";

	MX_GENERIC *generic;
	MX_PCSTEP *pcstep;
	int i;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	pcstep = (MX_PCSTEP *) malloc( sizeof(MX_PCSTEP) );

	if ( pcstep == (MX_PCSTEP *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_PCSTEP structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = pcstep;
	record->class_specific_function_list
				= &mxi_pcstep_generic_function_list;

	generic->record = record;
	pcstep->record = record;

	for ( i = 0; i < MX_PCSTEP_NUM_MOTORS; i++ ) {
		pcstep->motor_array[i] = NULL;
	}

	pcstep->controller_initialized = FALSE;
	pcstep->home_search_in_progress = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pcstep_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxi_pcstep_finish_record_initialization()";

	MX_PCSTEP *pcstep;

	pcstep = (MX_PCSTEP *) record->record_type_struct;

	if ( pcstep == (MX_PCSTEP *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PCSTEP pointer for record '%s' is NULL.", record->name );
	}

	pcstep->retries = 100;
	pcstep->max_discarded_words = 100;
	pcstep->delay_milliseconds = 10;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pcstep_delete_record( MX_RECORD *record )
{
        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }
        if ( record->record_type_struct != NULL ) {
                free( record->record_type_struct );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pcstep_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxi_pcstep_print_structure()";

	MX_PCSTEP *pcstep;
	MX_RECORD *this_record;
	int i;
	mx_status_type mx_status;

	mx_status = mxi_pcstep_get_pointers( record, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "Record '%s':\n\n", record->name);
	fprintf(file, "  name                       = \"%s\"\n",
						record->name);

	fprintf(file, "  superclass                 = interface\n");
	fprintf(file, "  class                      = generic\n");
	fprintf(file, "  type                       = PCSTEP\n");
	fprintf(file, "  label                      = \"%s\"\n",
						record->label);

	fprintf(file, "  portio record              = \"%s\"\n",
						pcstep->portio_record->name);

	fprintf(file, "  base address               = %#lx\n",
						pcstep->base_address);

	fprintf(file, "  motors in use              = (");

	for ( i = 0; i < MX_PCSTEP_NUM_MOTORS; i++ ) {

		this_record = pcstep->motor_array[i];

		if ( this_record == NULL ) {
			fprintf( file, "NULL" );
		} else {
			fprintf( file, "%s", this_record->name );
		}
		if ( i != (MX_PCSTEP_NUM_MOTORS - 1) ) {
			fprintf( file, ", " );
		}
	}

	fprintf(file, ")\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pcstep_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pcstep_write_parms_to_hardware( MX_RECORD *record )
{
	const char fname[] = "mxi_pcstep_write_parms_to_hardware()";

	MX_PCSTEP *pcstep;
	uint16_t limit_switch_polarity;
	uint16_t enable_limit_switches;
	mx_status_type mx_status;

	mx_status = mxi_pcstep_get_pointers( record, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the polarity of the limit switches and home switches. */

	limit_switch_polarity = (uint16_t) pcstep->limit_switch_polarity;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_SET_LIMIT_SWITCH_POLARITY,
				limit_switch_polarity, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable the limit switches. */

	enable_limit_switches = (uint16_t) pcstep->enable_limit_switches;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_ENABLE_LIMIT_SWITCH_INPUTS,
				enable_limit_switches, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_pcstep_open( MX_RECORD *record )
{
	const char fname[] = "mxi_pcstep_open()";

	MX_PCSTEP *pcstep;
	MX_RECORD *motor_record;
	int i;
	mx_status_type mx_status;

	mx_status = mxi_pcstep_get_pointers( record, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if we have permission to write to the PC-Step's ports.*/

	mx_status = mx_portio_request_region( pcstep->portio_record,
						pcstep->base_address, 7 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Synchronize with the controller. */

	mx_status = mxi_pcstep_resynchronize( record );

	if ( mx_status.code == MXE_SUCCESS ) {

		pcstep->controller_initialized = TRUE;

		return MX_SUCCESSFUL_RESULT;

	} else if ( mx_status.code == MXE_NOT_READY ) {

		/* We get here if mxi_pcstep_resynchronize() failed to
		 * complete due to a power on reset or watchdog reset
		 * condition.  Try to recover by setting all the motor
		 * positions to zero.
		 */

		for ( i = 0; i < MX_PCSTEP_NUM_MOTORS; i++ ) {

			motor_record = pcstep->motor_array[i];

			if ( motor_record != NULL ) {
				mx_status = mx_motor_set_position(
						motor_record, 0.0 );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
		}

		/* If we get here, our recovery attempt succeeded. */

		pcstep->controller_initialized = TRUE;

		return MX_SUCCESSFUL_RESULT;

	} else if ( mx_status.code == MXE_INTERFACE_ACTION_FAILED ) {

		/* If we get here, it is because some previous command to
		 * the controller failed.  However, since this driver is
		 * just starting up (we're in an open() routine, after all),
		 * the failure probably happened before this instance of
		 * the program was run.
		 *
		 * In that case, we do not really know what the controller
		 * is doing at this point, so we must take a best guess as
		 * to how to recover from this.  The guess chosen here is to
		 * unconditionally send stop motion commands to all the axes.
		 */

		mx_warning(
		"Warning: Attempting to recover from a previous command error "
		"for PC-Step controller '%s' by sending a 'soft abort' to all "
		"attached motors.", record->name );

		for ( i = 1; i <= MX_PCSTEP_NUM_MOTORS; i++ ) {
			mx_status = mxi_pcstep_command( pcstep,
					MX_PCSTEP_STOP_MOTION(i), 0, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* If we get here, our recovery attempt succeeded. */

		pcstep->controller_initialized = TRUE;

		return MX_SUCCESSFUL_RESULT;

	} else {

		/* Some other error occurred. */

		return mx_status;
	}
}

MX_EXPORT mx_status_type
mxi_pcstep_close( MX_RECORD *record )
{
	const char fname[] = "mxi_pcstep_close()";

	MX_PCSTEP *pcstep;
	mx_status_type mx_status;

	mx_status = mxi_pcstep_get_pointers( record, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_portio_release_region( pcstep->portio_record,
						pcstep->base_address, 7 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_pcstep_resynchronize( MX_RECORD *record )
{
	const char fname[] = "mxi_pcstep_resynchronize()";

	MX_PCSTEP *pcstep;
	uint32_t long_status_word;
	mx_status_type mx_status;

	mx_status = mxi_pcstep_get_pointers( record, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_pcstep_reset_errors( pcstep );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Try to verify that the PC-Step controller is there by using the
	 * read communications status command 0x2041.  If the controller
	 * successfully performs the handshake in mxi_pcstep_command(),
	 * then it must be present.
	 */

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_READ_COMMUNICATIONS_STATUS,
				0, &long_status_word );

	if ( mx_status.code == MXE_TIMED_OUT ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"PC-Step controller not found.  Are you sure that there is "
		"a controller at port address %#lx?",
			pcstep->base_address );
	}

	return mx_status;
}

/* === Driver specific functions === */

/* WARNING: For some reason, the values sent to and read from the PC-Step
 * controller have to be byte swapped to get the values described in the
 * old nuLogic manual.  National Instruments doesn't describe the port I/O
 * interface in the current manuals, so we can't check to see if this
 * was a documentation bug or not.
 */

MX_EXPORT uint16_t
mxi_pcstep_get_status_word( MX_PCSTEP *pcstep )
{
	const char fname[] = "mxi_pcstep_get_status_word()";

	uint16_t result;
	unsigned long port_address;

	if ( pcstep == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL pcstep pointer passed." );
		return (-1);
	}

	port_address = pcstep->base_address + 6;

	result = mx_portio_inp16( pcstep->portio_record, port_address );

	result = mx_16bit_byteswap( result );

	return result;
}

static mx_status_type
mxi_pcstep_transmit_word( MX_PCSTEP *pcstep, uint16_t transmitted_word )
{
	const char fname[] = "mxi_pcstep_transmit_word()";

	uint16_t status_word;
	unsigned long i;

	for ( i = 0; i <= pcstep->retries; i++ ) {

		status_word = mxi_pcstep_get_status_word( pcstep );

		if ( status_word & MXF_PCSTEP_READY_TO_RECEIVE )
			break;		/* Exit the for() loop. */

		mx_msleep( pcstep->delay_milliseconds );
	}

	if ( i > pcstep->retries ) {
		return mx_error( MXE_TIMED_OUT, fname,
"Failed to send word %#6.4x to PC-Step controller '%s' after %lu retries.",
			transmitted_word,
			pcstep->record->name,
			pcstep->retries );
	}

	transmitted_word = mx_16bit_byteswap( transmitted_word );

	mx_portio_outp16( pcstep->portio_record,
				pcstep->base_address,
				transmitted_word );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_pcstep_receive_word( MX_PCSTEP *pcstep, uint16_t *received_word )
{
	const char fname[] = "mxi_pcstep_receive_word()";

	uint16_t original_received_word, status_word;
	unsigned long i;

	for ( i = 0; i <= pcstep->retries; i++ ) {

		status_word = mxi_pcstep_get_status_word( pcstep );

		if ( status_word & MXF_PCSTEP_RETURN_DATA_PENDING )
			break;		/* Exit the for() loop. */

		mx_msleep( pcstep->delay_milliseconds );
	}

	if ( i > pcstep->retries ) {
		return mx_error( MXE_TIMED_OUT, fname,
"Failed to receive a word from the PC-Step controller '%s' after %lu retries.",
			pcstep->record->name,
			pcstep->retries );
	}

	original_received_word = mx_portio_inp16( pcstep->portio_record,
						pcstep->base_address );

	*received_word = mx_16bit_byteswap( original_received_word );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pcstep_reset_errors( MX_PCSTEP *pcstep )
{
	const char fname[] = "mxi_pcstep_reset_errors()";

	uint16_t status_word, received_word;
	unsigned long i;
	mx_status_type mx_status;

	if ( pcstep == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL pcstep pointer passed." );
	}

#if MXI_PCSTEP_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* Attempt to send the packet terminator word. */

	mx_status = mxi_pcstep_transmit_word( pcstep,
				MX_PCSTEP_COMMAND_PACKET_TERMINATOR_WORD );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot reset command error for PC-Step controller '%s'.",
			pcstep->record->name );
	}

	/* Read in and discard everything in the return data buffer. */

	for ( i = 0; i < pcstep->max_discarded_words; i++ ) {

		status_word = mxi_pcstep_get_status_word( pcstep );

		/* If there is nothing more to discard, then return
		 * to the caller.
		 */

		if ( ( status_word & MXF_PCSTEP_RETURN_DATA_PENDING ) == 0 ) {
			if ( i == 1 ) {
				MX_DEBUG( 2,("%s: 1 word discarded.", fname));
			} else if ( i > 1 ) {
				MX_DEBUG( 2,("%s: %lu words discarded.",
					fname, i));
			}
			return MX_SUCCESSFUL_RESULT;
		}

		/* Read in and throw away the data word. */

		mx_status = mxi_pcstep_receive_word( pcstep, &received_word );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep( pcstep->delay_milliseconds );
	}

	/* If we get here, there were too many words in the buffer
	 * to discard.
	 */

	return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Return data buffer not cleared after discarding %lu data words.  "
	"Perhaps something is wrong with the card?",
		pcstep->max_discarded_words );
}

MX_EXPORT mx_status_type
mxi_pcstep_command( MX_PCSTEP *pcstep, int command,
			uint32_t command_argument,
			uint32_t *command_response )
{
	const char fname[] = "mxi_pcstep_command()";

	uint16_t command_word, current_word;
	uint16_t received_word, return_packet_id;
	uint16_t status_word, test_var;
	uint8_t original_command_id, originating_command_id;
	uint8_t original_axis_id, originating_axis_id;
	unsigned long i;
	int num_transmitted_words, num_received_words;
	mx_status_type mx_status, mx_status2, mx_command_execution_status;

	if ( pcstep == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL pcstep pointer passed." );
	}

	command_word = (uint16_t) command;

#if MXI_PCSTEP_DEBUG
	MX_DEBUG(-2,("%s: sending %#6.4x, %#10.8x",
			fname, command_word, command_argument));
#endif

	num_transmitted_words = ( command_word & 0xF000 ) >> 12;

	if ( (num_transmitted_words < 2) || (num_transmitted_words > 4) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal number of words (%d) in command %#6.4x",
			num_transmitted_words, command_word );
	}

	/* Clear any command errors. */

	status_word = mxi_pcstep_get_status_word( pcstep );

#if 0
	MX_DEBUG(-2,("%s: preexisting status = %x", fname, status_word));
#endif

	if ( status_word & MXF_PCSTEP_COMMAND_ERROR ) {
		mx_status = mxi_pcstep_reset_errors( pcstep );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Transmit the first word. */

	mx_status = mxi_pcstep_transmit_word( pcstep, command_word );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If needed, transmit the high order word. */

	if ( num_transmitted_words == 4 ) {

		current_word = (uint16_t) ( (command_argument >> 16) & 0xFFFF );

		mx_status = mxi_pcstep_transmit_word( pcstep, current_word );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* If needed, transmit the low order word. */

	if ( ( num_transmitted_words == 3 )
	  || ( num_transmitted_words == 4 ) )
	{
		current_word = (uint16_t) ( command_argument & 0xFFFF );

		mx_status = mxi_pcstep_transmit_word( pcstep, current_word );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Transmit the command packet terminator. */

	mx_status = mxi_pcstep_transmit_word( pcstep,
				MX_PCSTEP_COMMAND_PACKET_TERMINATOR_WORD );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the command to finish executing. */

	for ( i = 0; i <= pcstep->retries; i++ ) {
		status_word = mxi_pcstep_get_status_word( pcstep );

		/* If it has finished, exit the for() loop. */

		if ( (status_word & MXF_PCSTEP_COMMAND_IN_PROCESS) == 0 )
			break;

		mx_msleep( pcstep->delay_milliseconds );
	}

	/* Did we exit the loop by timing out? */

	if ( i > pcstep->retries ) {
		return mx_error( MXE_TIMED_OUT, fname,
	"Timed out waiting for the PC-Step controller to finish executing "
	"the '%#6.4x' command.  Timeout period = %lu milliseconds",
			command_word,
			pcstep->retries * pcstep->delay_milliseconds );
	}

	/* Was there an error with the command? */

#if MXI_PCSTEP_DEBUG
	MX_DEBUG(-2,("%s: command status = %#06.4x", fname, status_word ));
#endif

	if ( status_word & MXF_PCSTEP_ERROR_FLAGS ) {

		/* Check to see what errors occurred.  We check the most
		 * severe errors first.
		 */

		MX_DEBUG( 2,("%s: PC-Step status word = %#4.2x",
				fname, status_word));

		if ( status_word & MXF_PCSTEP_HARDWARE_FAILURE ) {
			/* If we get this error, give up immediately. */

			return mx_error( MXE_CONTROLLER_INTERNAL_ERROR,
				fname,
	"The PC-Step controller has reported a hardware failure while trying "
	"to execute the command '%#6.4x'.  This error cannot be fixed by MX.",
				command_word );
		}

		/* For these other errors, try to recover. */

		if ( status_word & MXF_PCSTEP_WATCHDOG_RESET ) {

			/* Suppress the display of the error message if the
			 * controller has not yet been fully initialized.
			 */

			if ( pcstep->controller_initialized == FALSE ) {

				mx_status = mx_error_quiet(MXE_NOT_READY, fname,
	"The PC-Step command '%#6.4x' was not executed due to either a power "
	"on reset or a watchdog reset.", command_word );

			} else {

				mx_status = mx_error(MXE_NOT_READY, fname,
	"The PC-Step command '%#6.4x' was not executed due to either a power "
	"on reset or a watchdog reset.", command_word );

			}
		}
		if ( status_word & MXF_PCSTEP_COMMAND_ERROR ) {
			mx_status = mx_error( MXE_INTERFACE_ACTION_FAILED,
				fname,
	"The PC-Step command '%#6.4x' resulted in a command error.  "
	"We will attempt to recover from the error automatically.",
				command_word );
		}

		mx_status2 = mxi_pcstep_reset_errors( pcstep );

		if ( mx_status2.code != MXE_SUCCESS ) {
			MX_DEBUG(-2,
		("%s: The attempt to recover from the error failed.", fname));
		}

		return mx_status;
	}

	/* Arrange to return either MXE_SUCCESS or MXE_INTERFACE_ACTION_FAILED
	 * to the caller in order to let the caller know that the command
	 * execution status bit was set.
	 */

	if ( status_word & MXF_PCSTEP_COMMAND_EXECUTION ) {

		/* If the command was a start motion command, suppress the
		 * printing of the error message.  Otherwise, display it
		 * to the user.
		 */

		test_var = ( command_word & 0xff );

		if ( test_var == ( MX_PCSTEP_START_MOTION(0) & 0xff ) ) {
			mx_command_execution_status = mx_error_quiet(
				MXE_INTERFACE_ACTION_FAILED, fname,
"The PC-Step controller refused to execute a start motion command %#6.4x.",
				command_word );

		} else if ( test_var
			== ( MX_PCSTEP_READ_COMMUNICATIONS_STATUS & 0xff ) )
		{
			mx_command_execution_status = mx_error_quiet(
				MXE_INTERFACE_ACTION_FAILED, fname,
"The PC-Step controller refused to send the communication status word %#6.4x.",
				command_word );
		} else {
			mx_command_execution_status = mx_error(
				MXE_INTERFACE_ACTION_FAILED, fname,
	"The PC-Step controller refused to execute a '%#6.4x' command.",
				command_word );
		}
	} else {
		mx_command_execution_status = MX_SUCCESSFUL_RESULT;
	}

	/****************************************************************
	 * If command_response is NULL, we are not expecting a response *
	 * from the controller and return at this point.                *
	 ****************************************************************/

	if ( command_response == NULL )
		return mx_command_execution_status;

	/* To keep the compiler from complaining about possibly
	 * uninitialized variables.
	 */

	originating_command_id = originating_axis_id = 0;
	num_received_words = 0;

	/* Read the return packet ID word. */

	mx_status = mxi_pcstep_receive_word( pcstep, &return_packet_id );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_received_words = ( return_packet_id & 0xF000 ) >> 12;

	if ( ( num_received_words != 2 ) && ( num_received_words != 3 ) ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Number of received words (%d) in the return packet ID %#6.4x is "
	"outside the allowed range of 2 to 3.",
			num_received_words, return_packet_id );
	}

	/* Read the low order word. */

	mx_status = mxi_pcstep_receive_word( pcstep, &received_word );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*command_response = received_word;

	*command_response &= 0xFFFF;

	/* If expected, read the high order word. */

	if ( num_received_words == 3 ) {

		mx_status = mxi_pcstep_receive_word( pcstep, &received_word );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*command_response |= ( received_word << 16 );
	}

#if MXI_PCSTEP_DEBUG
	MX_DEBUG(-2,("%s: received %#06.4x, %#010.8x",
			fname, return_packet_id, *command_response ));
#endif

	/* Check to see if the response packet corresponds to the original
	 * command.
	 */

	/* Split out the command IDs. */

	original_command_id = ( command_word & 0xFF );
	originating_command_id = ( return_packet_id & 0xFF );

	if ( original_command_id != originating_command_id ) {
		(void) mxi_pcstep_reset_errors( pcstep );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"PC-Step synchronization error.  The originating command ID %#4.2x does not "
"match the original command ID %#4.2x.",
			originating_command_id,
			original_command_id );
	}

	/* Split out the axis IDs. */

	original_axis_id = ( ( command_word >> 8 ) & 0xF );
	originating_axis_id = ( ( return_packet_id >> 8 ) & 0xF );

	if ( original_axis_id != originating_axis_id ) {
		(void) mxi_pcstep_reset_errors( pcstep );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"PC-Step synchronization error.  The originating axis ID %#4.2x does not "
"match the original axis ID %#4.2x.",
			originating_axis_id,
			original_axis_id );
	}

	return mx_command_execution_status;
}

