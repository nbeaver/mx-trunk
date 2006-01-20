/*
 * Name:    i_vme58.c
 *
 * Purpose: MX driver for Oregon Microsystems VME58 motor controllers.
 *
 *          This driver can access the controller in either of two ways:
 *
 *          1.  An MX port I/O record.
 *                This is supported on any platform that implements
 *                MX port I/O records.
 *
 *          2.  An m68k Linux device driver written at the European
 *              Synchrotron Radiation Facility (ESRF).
 *                This is supported only on Linux 2.2.x.  Make sure to
 *                define HAVE_VME58_ESRF in "libMx/mxconfig.h" if you
 *                are planning to use this.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_VME58_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mxconfig.h"

#if HAVE_VME58_ESRF
#  if !defined( OS_LINUX )
#     error "The ESRF VME58 driver can only be used under Linux."
#  else
#     include <sys/ioctl.h>
#     include "omslib.h"
#  endif
#endif

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_array.h"
#include "mx_stdint.h"
#include "mx_vme.h"
#include "i_vme58.h"

MX_RECORD_FUNCTION_LIST mxi_vme58_record_function_list = {
	NULL,
	mxi_vme58_create_record_structures,
	mxi_vme58_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_vme58_open,
	mxi_vme58_close,
	NULL,
	mxi_vme58_resynchronize,
};

/*----*/

MX_RECORD_FIELD_DEFAULTS mxi_vme58_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_VME58_COMMON_STANDARD_FIELDS,
	MXI_VME58_PORTIO_STANDARD_FIELDS
};

long mxi_vme58_num_record_fields
		= sizeof( mxi_vme58_record_field_defaults )
			/ sizeof( mxi_vme58_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_vme58_rfield_def_ptr
			= &mxi_vme58_record_field_defaults[0];

static mx_status_type mxi_vme58_portio_open( MX_VME58 *vme58 );

static mx_status_type mxi_vme58_portio_command( MX_VME58 *vme58, char *command,
				char *response, int response_buffer_length,
				int debug_flag );

static mx_status_type mxi_vme58_portio_putline( MX_VME58 *vme58, char *command,
				int debug_flag );

static mx_status_type mxi_vme58_portio_getline( MX_VME58 *vme58,
				char *response, int response_buffer_length,
				int debug_flag );

/*----*/

#if HAVE_VME58_ESRF

MX_RECORD_FIELD_DEFAULTS mxi_vme58_esrf_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_VME58_COMMON_STANDARD_FIELDS,
	MXI_VME58_ESRF_STANDARD_FIELDS
};

long mxi_vme58_esrf_num_record_fields
		= sizeof( mxi_vme58_esrf_record_field_defaults )
			/ sizeof( mxi_vme58_esrf_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_vme58_esrf_rfield_def_ptr
			= &mxi_vme58_esrf_record_field_defaults[0];

static mx_status_type mxi_vme58_esrf_command( MX_VME58 *vme58, char *command,
				char *response, int response_buffer_length,
				int debug_flag );

#endif /* HAVE_VME58_ESRF */

/*----*/

/* A private function for the use of the driver. */

static mx_status_type
mxi_vme58_get_pointers( MX_RECORD *vme58_record,
			MX_VME58 **vme58,
			const char *calling_fname )
{
	static const char fname[] = "mxi_vme58_get_pointers()";

	if ( vme58_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vme58 == (MX_VME58 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VME58 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*vme58 = (MX_VME58 *) vme58_record->record_type_struct;

	if ( *vme58 == (MX_VME58 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VME58 pointer for record '%s' is NULL.",
			vme58_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_vme58_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_vme58_create_record_structures()";

	MX_VME58 *vme58;

	/* Allocate memory for the necessary structures. */

	vme58 = (MX_VME58 *) malloc( sizeof(MX_VME58) );

	if ( vme58 == (MX_VME58 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_VME58 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = vme58;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	vme58->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vme58_finish_record_initialization( MX_RECORD *record )
{
	MX_VME58 *vme58;
	double delay_seconds;
	int i;

	vme58 = (MX_VME58 *) record->record_type_struct;

	/* Set the server minimum delay time between commands. */

	delay_seconds = 0.0001;

	vme58->event_interval
		= mx_convert_seconds_to_clock_ticks( delay_seconds );

	/* Mark the Linux device file as being closed. */

	if ( record->mx_type == MXI_GEN_VME58_ESRF ) {
		vme58->u.esrf.device_fd = -1;
	}

	/* Initialize the motor array. */

	for ( i = 0; i < MX_MAX_VME58_AXES; i++ ) {
		vme58->motor_array[i] = NULL;
	}

#if MXI_VME58_COMMAND_DELAY
	vme58->next_command_must_be_after_this_tick = mx_current_clock_tick();
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vme58_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_vme58_open()";

	MX_VME58 *vme58;
	int saved_errno;
	mx_status_type mx_status;

	mx_status = mxi_vme58_get_pointers( record, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( record->mx_type == MXI_GEN_VME58_ESRF ) {

#if defined( OS_LINUX )

		/* Try to open the ESRF device file. */

		vme58->u.esrf.device_fd
			= open( vme58->u.esrf.device_name, O_RDWR );

		if ( vme58->u.esrf.device_fd == -1 ) {
			saved_errno = errno;

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot open the VME58 device file '%s'.  Reason = '%s'.",
			vme58->u.esrf.device_name, strerror( saved_errno ) );
		}

#else /* OS_LINUX */
		/* Prevent the compiler from complaining about
		 * an unused variable.
		 */

		saved_errno = 0;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The 'vme58_esrf' driver can only be used under Linux." );
#endif /* OS_LINUX */

	} else {
		/* Port I/O */

		mx_status = mxi_vme58_portio_open( vme58 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Synchronize with the controller. */

	mx_status = mxi_vme58_resynchronize( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vme58_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_vme58_close()";

	MX_VME58 *vme58;
	int close_status, saved_errno;
	mx_status_type mx_status;

	mx_status = mxi_vme58_get_pointers( record, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( record->mx_type == MXI_GEN_VME58_ESRF ) {

		if ( vme58->u.esrf.device_fd >= 0 ) {
			close_status = close( vme58->u.esrf.device_fd );

			if ( close_status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot close the VME58 device file '%s'.  Reason = '%s'.",
			vme58->u.esrf.device_name, strerror( saved_errno ) );
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vme58_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_vme58_resynchronize()";

	MX_VME58 *vme58;
	char response[80];
	char type_string[80];
	char *version_ptr, *ptr;
	int num_items;
	mx_status_type mx_status;

	mx_status = mxi_vme58_get_pointers( record, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the controller is there by asking it for its
	 * version number.
	 */

	mx_status = mxi_vme58_command( vme58, "WY", response, sizeof response,
						MXI_VME58_DEBUG );
	
	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_INTERFACE_IO_ERROR:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Attempt to communicate with the VME58 controller '%s' failed.",
			record->name );
		break;
	default:
		return mx_status;
		break;
	}

	/* Start with some conservative defaults. */

	vme58->controller_type = MXT_VME58_UNKNOWN;
	vme58->num_axes = 1;

	/* The first two characters in the response should an LF followed
	 * by a CR.
	 */

	if ( (response[0] != MX_LF) || (response[1] != MX_CR) ) {

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"VME58 interface '%s' did not return a response to the WY command.  "
"Instead we received '%s'.", record->name, response );
	}

	version_ptr = response + 2;

	if ( strncmp( version_ptr, "VME58", 5 ) != 0 )
	{
		/* Don't know what type of controller this is. */

		(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
"VME58 interface '%s' had an unrecognized response to the WY command.  "
"WY response = '%s'", record->name, version_ptr );

	} else {
		/* Find the hyphen in the response string.  This is just
		 * before the start of the controller type.
		 */

		ptr = strchr( version_ptr, '-' );

		if ( ptr == NULL ) {

			(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
"Cannot determine the particular controller type for VME interface '%s'.  "
"WY response = '%s'", record->name, version_ptr );

		} else {
			ptr++;		/* Skip over the hyphen. */

			num_items = sscanf( ptr, "%s", type_string );

			if ( num_items != 1 ) {

				(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
"Cannot find the particular controller type for VME interface '%s'.  "
"WY response = '%s'", record->name, version_ptr );

			} else {
				if ( strcmp( type_string, "8" ) == 0 ) {
					vme58->controller_type = MXT_VME58_8;
					vme58->num_axes = 8;
				} else
				if ( strcmp( type_string, "4E" ) == 0 ) {
					vme58->controller_type = MXT_VME58_4E;
					vme58->num_axes = 4;
				} else {
					(void) mx_error(MXE_UNPARSEABLE_STRING,
						fname,
"Unrecognized controller type '%s' in WY response for VME interface '%s'.  "
"WY response = '%s'", type_string, record->name, version_ptr );
				}
			}
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === Functions specific to this driver. === */

MX_EXPORT mx_status_type
mxi_vme58_command( MX_VME58 *vme58, char *command,
		char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_vme58_command()";

	mx_status_type mx_status;

	if ( vme58 == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_VME58 pointer passed was NULL." );
	}

#if MXI_VME58_COMMAND_DELAY
	while (1) {
		MX_CLOCK_TICK current_tick;
		int comparison;

		current_tick = mx_current_clock_tick();

		comparison = mx_compare_clock_ticks( current_tick,
				vme58->next_command_must_be_after_this_tick );

		if ( comparison >= 0 )
			break;		/* Exit the while() loop. */
	}
#endif

	if ( vme58->record->mx_type == MXI_GEN_VME58_ESRF ) {

#if HAVE_VME58_ESRF

		mx_status = mxi_vme58_esrf_command( vme58 , command,
				response, response_buffer_length, debug_flag );
#else /* HAVE_VME58_ESRF */

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"This copy of MX was not compiled with HAVE_VME58_ESRF "
		"set to 1, so the 'vme58_esrf' record type is unavailable." );

#endif /* HAVE_VME58_ESRF */

	} else {
		/* Port I/O */

		mx_status = mxi_vme58_portio_command( vme58 , command,
				response, response_buffer_length, debug_flag );
	}

#if MXI_VME58_COMMAND_DELAY
	vme58->next_command_must_be_after_this_tick = mx_add_clock_ticks(
		mx_current_clock_tick(), vme58->event_interval );
#endif

	return mx_status;
}

/*---------------------------------------------------------------------------*/

#if HAVE_VME58_ESRF

static char *OMS_Error_Strings[] = {
	"No error!",
	"Invalid function",
	"Multiple axes specified",
	"Axis is busy",
	"Non-implemented function",
	"Timeout",
	"No axes specified",
	"Bad axis specification",
	"DONE interrupt pending",
	"Response overload",
	"String too long",
	"Bad position data from board",
	"Insufficient output buffer space",
	"Interrupt queued",
	"Bad response to status request",
	"Slip detected",
	"Limit detected",
	"Command error",
};

static mx_status_type
mxi_vme58_esrf_command( MX_VME58 *vme58, char *command,
		char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_vme58_esrf_command()";

	int oms_status;
	OMS_req_t request;

	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}
	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s'", fname, command ));
	}

	/* Set up the request structure */

	memset( &request, 0, sizeof(request) );

	request.cmdstr = command;
	request.cmdlen = strlen( command );

	/* Send the command. */

	if ( response == NULL ) {

		/* No response is expected. */

		oms_status = ioctl( vme58->u.esrf.device_fd,
					OMS_CMD_RET, &request );

	} else {

		/* A response is expected. */

		request.rspstr = response;
		request.rsplen = response_buffer_length;

		oms_status = ioctl( vme58->u.esrf.device_fd,
					OMS_CMD_RESP, &request );
	}

	if ( oms_status != 0 ) {

		if ( oms_status >= OMS_ERR_BASE_VALUE &&
			oms_status <= OMS_ERR_BASE_VALUE + OMS_ERR_RANGE )
		{
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"An OMS error occurred for command '%s' to controller '%s'.  Error = '%s'.",
				command, vme58->record->name,
				OMS_Error_Strings[ oms_status
						- OMS_ERR_BASE_VALUE ] );
		} else {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"An operating system error occurred for command '%s' to controller '%s'.  "
"Error = '%s'.", command, vme58->record->name, strerror( oms_status ) );
		}
	}

	if ( response && debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s'", fname, response ));
	}
	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_VME58_ESRF */

/*---------------------------------------------------------------------------*/

static mx_status_type
mxi_vme58_portio_open( MX_VME58 *vme58 )
{
	static const char fname[] = "mxi_vme58_portio_open()";

	MX_RECORD *vme_record;
	unsigned long crate, base;
	unsigned long i, max_retries, sleep_ms;
	uint8_t status_register;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for VME58 controller '%s'.",
		fname, vme58->record->name ));

	vme_record = vme58->u.portio.vme_record;
	crate      = vme58->u.portio.crate_number;
	base       = vme58->u.portio.base_address;

	/* Wait for the board initialized flag to be set. */

	max_retries = 10;
	sleep_ms = 100;

	for ( i = 0; i < max_retries; i++ ) {
		mx_status = mx_vme_in8( vme_record, crate, MXF_VME_A16,
					base + MX_VME58_STATUS_REGISTER,
					&status_register );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,("%s: status_register = %#x",
					fname, status_register));

		if ( status_register & 0x2 ) {

			/* The board is initialized, so exit the for() loop. */

			break;
		}

		mx_msleep( sleep_ms );
	}

	if ( i >= max_retries ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"VME58 controller '%s' has not finished initializing, "
		"even after waiting for %lu milliseconds.",
			vme58->record->name,
			max_retries * sleep_ms );
	}

	/* This driver does not currently use interrupts, so turn them off. */

	mx_status = mx_vme_out8( vme_record, crate, MXF_VME_A16,
					base + MX_VME58_CONTROL_REGISTER,
					0 );

	return mx_status;
}

/*---------------------------------------------------------------------------*/

static mx_status_type
mxi_vme58_portio_command( MX_VME58 *vme58, char *command,
		char *response, int response_buffer_length,
		int debug_flag )
{
	mx_status_type mx_status;

	mx_status = mxi_vme58_portio_putline( vme58, command, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response != NULL ) {
		mx_status = mxi_vme58_portio_getline( vme58,
				response, response_buffer_length, debug_flag );
	}

	return mx_status;
}

/* The VME58 uses two circular buffers to manage serial communication I/O
 * with the host computer:
 *
 *   output_buffer - Characters being sent from the host computer to the VME58.
 *   input buffer  - Characters being read by the host computer from the VME58.
 *
 * Each buffer has two index pointers that indicate where in the buffer each
 * side is at.  This means that there are four index pointers in total:
 *
 *   output_get_index - The index of the next character the VME58 is about
 *                      to read.
 *
 *   output_put_index - The index of the next character the host computer
 *                      is about to write.
 *
 *   input_get_index  - The index of the next character the host computer
 *                      is about to read.
 *
 *   input_put_index  - The index of the next character the VME58 is about
 *                      to write.
 */

static mx_status_type
mxi_vme58_portio_putline( MX_VME58 *vme58, char *command, int debug_flag )
{
	static const char fname[] = "mxi_vme58_portio_putline()";

	MX_RECORD *vme_record;
	unsigned long crate, base;
	uint16_t output_get_index, output_put_index;
	uint16_t i, available_buffer_space;
	size_t command_length;
	char *ptr;
	mx_status_type mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, vme58->record->name));
	}

	vme_record = vme58->u.portio.vme_record;
	crate      = vme58->u.portio.crate_number;
	base       = vme58->u.portio.base_address;

	/* Read the output index pointers. */ 

	mx_status = mx_vme_in16( vme_record, crate, MXF_VME_A16,
					base + MX_VME58_OUTPUT_GET_INDEX,
					&output_get_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_vme_in16( vme_record, crate, MXF_VME_A16,
					base + MX_VME58_OUTPUT_PUT_INDEX,
					&output_put_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if there is enough space in the buffer to send the command. */

	available_buffer_space = ((uint16_t) output_get_index)
				- ((uint16_t) output_put_index)
				- ((uint16_t) 2);

	MX_DEBUG( 2,("%s: available_buffer_space = %d",
		fname, (int) available_buffer_space));

	command_length = strlen( command );

	if ( available_buffer_space < command_length ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The length (%lu) of command '%s' is too long to fit "
		"in the currently available buffer space (%lu) "
		"for OMS controller '%s'.",
			(unsigned long) command_length, command,
			(unsigned long) available_buffer_space,
			vme58->record->name );
	}

	/* Start sending the command. */

	i = (uint16_t) output_put_index;

	ptr = command;

	while ( *ptr != '\0' ) {

		/* Send a character. */

		mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A16,
					base + MX_VME58_OUTPUT_BUFFER + 2*i,
					(uint16_t) *ptr );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr++;

		i++;

		if ( i >= MX_VME58_BUFFER_SIZE ) {
			i = 0;	/* Go back to the start of the buffer. */
		}
	}

	/* Move the output put index pointer so that the VME58 knows that
	 * new characters have been sent.
	 */

	mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A16,
					base + MX_VME58_OUTPUT_PUT_INDEX,
					i );

	return mx_status;
}

#define HANDLE_BROKEN_TERMINATOR( s, t ) \
			do { \
				/* Temporarily terminate the response \
				 * string so that we can display it \
				 * in the warning message. \
				 */ \
				\
				response[n] = '\0'; \
				\
				mx_warning( \
	"There %s at the start of response '%s' " \
	"from VME58 controller '%s' but %s at the end.  " \
	"Instead, we saw %#x (%c).  Continuing anyway.", \
					(s), response, (t), \
					vme58->record->name, c, c ); \
				\
				/* Arrange for 'response' to be \
				 * terminated in a more permanent \
				 * fashion. \
				 */ \
				\
				c = '\0'; \
				\
			} while(0)

/* mxi_vme58_portio_getline() currently implements a blocking read
 * with a timeout.
 *
 * Responses to commands have one of two forms:
 *
 *     \n\rRESPONSE\n\r
 *
 *     \n\r\rRESPONSE\n\r\r
 *
 * where the prefix before the response always matches the postfix
 * after the response.  The only variability is that some responses
 * have one carriage return and some have two carriage returns.  So
 * we must keep track of how many carriage returns are in the prefix.
 */

static mx_status_type
mxi_vme58_portio_getline( MX_VME58 *vme58,
		char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_vme58_portio_getline()";

	MX_RECORD *vme_record;
	unsigned long crate, base;
	uint16_t i, input_get_index, input_put_index, c16;
	unsigned long r, max_retries, sleep_ms;
	int n;
	char c;
	int two_carriage_returns_at_start;
	int terminating_line_feed_seen;
	int first_terminating_carriage_return_seen;
	int end_of_message;
	mx_status_type mx_status;

	vme_record = vme58->u.portio.vme_record;
	crate      = vme58->u.portio.crate_number;
	base       = vme58->u.portio.base_address;

	/* Get the index of the next character to read. */

	mx_status = mx_vme_in16( vme_record, crate, MXF_VME_A16,
					base + MX_VME58_INPUT_GET_INDEX,
					&input_get_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	two_carriage_returns_at_start = FALSE;
	terminating_line_feed_seen = FALSE;
	first_terminating_carriage_return_seen = FALSE;
	end_of_message = FALSE;

	i = input_get_index;

	for ( n = 0; n < response_buffer_length - 1; n++ ) {

		/* Wait for the next character. */

		max_retries = 10;
		sleep_ms = 100;

		for ( r = 0; r < max_retries; r++ ) {

			mx_status = mx_vme_in16( vme_record, crate, MXF_VME_A16,
					base + MX_VME58_INPUT_PUT_INDEX,
					&input_put_index );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( i != input_put_index ) {
				break;		/* Exit the for(r...) loop. */
			}

			mx_msleep(sleep_ms);
		}

		if ( r >= max_retries ) {
			return mx_error( MXE_TIMED_OUT, fname,
		"Timed out reading characters from VME58 controller '%s' "
		"after waiting for %lu milliseconds.",
				vme58->record->name,
				max_retries * sleep_ms );
		}

		/* Read the next character. */

		mx_status = mx_vme_in16( vme_record, crate, MXF_VME_A16,
					base + MX_VME58_INPUT_BUFFER + 2*i,
					&c16 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 0
		MX_DEBUG(-2,("%s: c16 = %c %#x", fname, c16 & 0xff, c16 ));
#endif

		c = (uint8_t) ( c16 & 0xff );

		/* Handle special cases concerning the start and end of
		 * VME58 response messages.
		 */

		if ( n == 0 ) {
			/* The first character should _always_ be a line feed
			 * character.  If the first character is not a line
			 * feed, then throw this character away and read
			 * the next one.
			 */

			if ( c != MX_LF ) {
				mx_warning(
			"Expected a line feed character at the start of a "
			"response from VME58 controller '%s'.  "
			"Instead, saw %#x (%c).  Skipping...",
					vme58->record->name, c, c );

				/* Back up the loop index to -1 so that it
				 * automatically increment back to 0 at the
				 * top of the loop.
				 */

				n = -1;

				/* Go back to the top of the loop. */

				continue;
			}

		} else if ( n == 1 ) {
			/* The second character should always be a carriage
			 * return.  Something has gone wrong if it is not
			 * a carriage return.
			 */

			if ( c != MX_CR ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not see a carriage return after the line feed at the "
		"start of a response from VME58 controller '%s'.  "
		"Instead, saw %#x (%c).", vme58->record->name, c, c );
			}

		} else if ( n == 2 ) {
			/* If the third character is a carriage return, then
			 * there will also be two carriage returns at the end.
			 */

			if ( c == MX_CR ) {
				two_carriage_returns_at_start = TRUE;
			}

		} else if ( c == MX_LF ) {
			/* We have reached the end of the response and need
			 * only to read out one or two more carriage returns.
			 */

			terminating_line_feed_seen = TRUE;

		} else if ( terminating_line_feed_seen ) {

			if ( c != MX_CR ) {

				/* I'm in a maze of twisty braces, all alike. */

				if ( two_carriage_returns_at_start ) {
				    if (first_terminating_carriage_return_seen)
				    {
					HANDLE_BROKEN_TERMINATOR(
						"were two carriage returns",
						"only one" );
				    } else {
					HANDLE_BROKEN_TERMINATOR(
						"were two carriage returns",
						"none" );
				    }
				} else {
				    HANDLE_BROKEN_TERMINATOR(
						"was one carriage return",
						"none" );
				}

				end_of_message = TRUE;

			} else {
				/* I'm in a twisty maze of braces, all alike. */

				if ( two_carriage_returns_at_start ) {
				    if (first_terminating_carriage_return_seen)
				    {
					end_of_message = TRUE;
				    } else {
					first_terminating_carriage_return_seen
						= TRUE;
				    }
				} else {
				    end_of_message = TRUE;
				}
			}
		}

		/* Save the character in the response buffer. */

		response[n] = c;

		i++;

		if ( i >= MX_VME58_BUFFER_SIZE ) {
			i = 0;	/* Go back to the start of the buffer. */
		}

		/* Tell the VME58 that we have read another character. */

		mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A16,
					base + MX_VME58_INPUT_GET_INDEX,
					i );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( end_of_message ) {
			/* Null terminate the response and exit the loop. */

			response[ n+1 ] = '\0';

			break;
		}
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, vme58->record->name));
	}

	return MX_SUCCESSFUL_RESULT;
}

