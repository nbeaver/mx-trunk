/*
 * Name:    d_remote_marccd.c
 *
 * Purpose: MX CCD driver for MarCCD when run in "Remote" data collection mode.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_REMOTE_MARCCD_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <errno.h>

#include "mx_osdef.h"

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_select.h"
#include "mx_net.h"
#include "mx_ccd.h"

#include "d_remote_marccd.h"

#define MX_CR_LF	"\015\012"

MX_RECORD_FUNCTION_LIST mxd_remote_marccd_record_function_list = {
	NULL,
	mxd_remote_marccd_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_remote_marccd_open,
	mxd_remote_marccd_close
};

MX_CCD_FUNCTION_LIST mxd_remote_marccd_ccd_function_list = {
	mxd_remote_marccd_start,
	mxd_remote_marccd_stop,
	mxd_remote_marccd_get_status,
	mxd_remote_marccd_readout,
	mxd_remote_marccd_dezinger,
	mxd_remote_marccd_correct,
	mxd_remote_marccd_writefile,
	mxd_remote_marccd_get_parameter,
	mxd_remote_marccd_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_remote_marccd_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_CCD_STANDARD_FIELDS,
	MXD_REMOTE_MARCCD_STANDARD_FIELDS
};

mx_length_type mxd_remote_marccd_num_record_fields
		= sizeof( mxd_remote_marccd_record_field_defaults )
			/ sizeof( mxd_remote_marccd_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_remote_marccd_rfield_def_ptr
			= &mxd_remote_marccd_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_remote_marccd_get_pointers( MX_CCD *ccd,
			MX_REMOTE_MARCCD **remote_marccd,
			const char *calling_fname )
{
	static const char fname[] = "mxd_remote_marccd_get_pointers()";

	MX_RECORD *remote_marccd_record;

	if ( ccd == (MX_CCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CCD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	remote_marccd_record = ccd->record;

	if ( remote_marccd_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_CCD pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	if ( remote_marccd == (MX_REMOTE_MARCCD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_REMOTE_MARCCD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*remote_marccd = (MX_REMOTE_MARCCD *)
				remote_marccd_record->record_type_struct;

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_remote_marccd_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_remote_marccd_create_record_structures()";

	MX_CCD *ccd;
	MX_REMOTE_MARCCD *remote_marccd;

	ccd = (MX_CCD *) malloc( sizeof(MX_CCD) );

	if ( ccd == (MX_CCD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for a MX_CCD structure "
		"for record '%s'.", record->name );
	}

	remote_marccd = (MX_REMOTE_MARCCD *) malloc( sizeof(MX_REMOTE_MARCCD) );

	if ( remote_marccd == (MX_REMOTE_MARCCD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for a MX_REMOTE_MARCCD structure"
		"for record '%s'.", record->name );
	}

	record->record_class_struct = ccd;
	record->record_type_struct = remote_marccd;

	record->class_specific_function_list =
			&mxd_remote_marccd_ccd_function_list;

	ccd->record = record;
	remote_marccd->record = record;

	remote_marccd->marccd_socket = NULL;

	remote_marccd->fd_from_marccd = -1;
	remote_marccd->fd_to_marccd = -1;

	remote_marccd->current_command = MXF_REMOTE_MARCCD_CMD_UNKNOWN;
	remote_marccd->current_state = MXF_REMOTE_MARCCD_STATE_UNKNOWN;
	remote_marccd->next_state = MXF_REMOTE_MARCCD_STATE_UNKNOWN;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_remote_marccd_find_file_descriptors( MX_REMOTE_MARCCD *remote_marccd )
{
	static const char fname[] = "mxd_remote_marccd_find_file_descriptors()";

	char *out_fd_string;
	char *in_fd_string;
	char *endptr;

	/* If this function is called, it means that we are running on
	 * the same computer as MarCCD and that the current process was
	 * actually created by MarCCD via the "Remote" control mode.
	 *
	 * If so, then the MarCCD process will have created two pipes for us
	 * to use for communicating with MarCCD.  MarCCD will have stored
	 * the numerical values of the associated file descriptors into
	 * two environment variable:
	 *
	 *   IN_FD  - Used to read responses from MarCCD.
	 *   OUT_FD - Used to write commands to MarCCD.
	 *
	 * Thus, we must find these two environment variables and treat them as
	 * file descriptors.
	 */

	in_fd_string = getenv( "IN_FD" );

	if ( in_fd_string == (char *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
			"MarCCD environment variable 'IN_FD' was not found.  "
			"That probably means that this process was not created "
			"by MarCCD." );
	}

	out_fd_string = getenv( "OUT_FD" );

	if ( out_fd_string == (char *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
			"MarCCD environment variable 'OUT_FD' was not found.  "
			"That probably means that this process was not created "
			"by MarCCD." );
	}

	remote_marccd->fd_from_marccd =
			(int) strtoul( in_fd_string, &endptr, 10 );

	if ( *endptr != '\0' ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The MarCCD created environment variable 'IN_FD' "
		"does not contain an integer.  Instead it is '%s'",
			in_fd_string );
	}

	remote_marccd->fd_to_marccd =
			(int) strtoul( out_fd_string, &endptr, 10 );

	if ( *endptr != '\0' ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The MarCCD created environment variable 'OUT_FD' "
		"does not contain an integer.  Instead it is '%s'",
			out_fd_string );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_remote_marccd_connect_to_remote_server( MX_REMOTE_MARCCD *remote_marccd )
{
	MX_SOCKET *client_socket;
	mx_status_type mx_status;

	/* If this function is called, it means that we are supposed to 
	 * connect via TCP/IP to 'marccd_server_socket' or some workalike
	 * running on the remote MarCCD computer.
	 */

	mx_status = mx_tcp_socket_open_as_client( &client_socket,
					remote_marccd->marccd_host,
					remote_marccd->marccd_port, 0,
					MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	remote_marccd->marccd_socket = client_socket;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_remote_marccd_open()";

	MX_CCD *ccd;
	MX_REMOTE_MARCCD *remote_marccd;
	int flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ccd = (MX_CCD *) record->record_class_struct;

	mx_status = mxd_remote_marccd_get_pointers(ccd, &remote_marccd, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.", fname, record->name));

	if ( strlen( remote_marccd->marccd_host )  == 0 ) {

		/* If the MarCCD host name is of zero length, then we assume
		 * that the current process was started by MarCCD itself.
		 */

		mx_status =
		    mxd_remote_marccd_find_file_descriptors( remote_marccd );
	} else {
		/* Otherwise, we assume that we need to connect via TCP/IP
		 * to a server spawned by MarCCD on the MarCCD host.
		 */

		mx_status =
		    mxd_remote_marccd_connect_to_remote_server( remote_marccd );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	remote_marccd->use_finish_time = FALSE;

	/* MarCCD sends an 'is_state' response message at startup
	 * followed by 'is_size' and 'is_bin' messages.
	 */

	flags = MXF_REMOTE_MARCCD_FORCE_READ | MXD_REMOTE_MARCCD_DEBUG;

	mx_status = mxd_remote_marccd_check_for_responses( ccd,
							remote_marccd, flags );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_remote_marccd_close()";

	MX_CCD *ccd;
	MX_REMOTE_MARCCD *remote_marccd;
	int saved_errno, status_from_marccd, status_to_marccd;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ccd = (MX_CCD *) record->record_class_struct;

	mx_status = mxd_remote_marccd_get_pointers(ccd, &remote_marccd, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.", fname, record->name));

	/* Tell MarCCD that it is time to exit remote mode. */

	remote_marccd->current_command = MXF_REMOTE_MARCCD_CMD_END_AUTOMATION;

	mx_status = mxd_remote_marccd_command( ccd, remote_marccd,
				"end_automation",
				MXD_REMOTE_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait a moment for the remote end to respond to what we have done. */

	mx_msleep(100);

	/* Shutdown the connection. */

	if ( remote_marccd->marccd_socket != NULL ) {

		mx_status = mx_socket_close( remote_marccd->marccd_socket );

	} else {
		mx_status = MX_SUCCESSFUL_RESULT;

		status_to_marccd = close( remote_marccd->fd_to_marccd );

		if ( status_to_marccd != 0 ) {
			saved_errno = errno;

			mx_status = mx_error( MXE_IPC_IO_ERROR, fname,
		"An error occurred while closing the pipe to MarCCD.  "
		"Error number = %d, error message = '%s'",
				saved_errno, strerror( saved_errno ) );
		}

		status_from_marccd = close( remote_marccd->fd_from_marccd );

		if ( status_from_marccd != 0 ) {
			saved_errno = errno;

			mx_status = mx_error( MXE_IPC_IO_ERROR, fname,
		"An error occurred while closing the pipe from MarCCD.  "
		"Error number = %d, error message = '%s'",
				saved_errno, strerror( saved_errno ) );
		}
	}

	remote_marccd->marccd_socket = NULL;

	remote_marccd->fd_from_marccd = -1;
	remote_marccd->fd_to_marccd = -1;

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_remote_marccd_start( MX_CCD *ccd )
{
	static const char fname[] = "mxd_remote_marccd_start()";

	MX_REMOTE_MARCCD *remote_marccd;
	MX_CLOCK_TICK clock_ticks_to_wait, start_time;
	mx_status_type mx_status;

	mx_status = mxd_remote_marccd_get_pointers(ccd, &remote_marccd, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	/* If needed, compute the finish time. */

	if ( ccd->preset_time < 0.0 ) {
		remote_marccd->use_finish_time = FALSE;
	} else {
		remote_marccd->use_finish_time = TRUE;

		clock_ticks_to_wait = 
			mx_convert_seconds_to_clock_ticks( ccd->preset_time );

		start_time = mx_current_clock_tick();

		remote_marccd->finish_time = mx_add_clock_ticks( start_time,
							clock_ticks_to_wait );
	}

	/* Send the start command. */

	remote_marccd->current_command = MXF_REMOTE_MARCCD_CMD_START;

	mx_status = mxd_remote_marccd_command( ccd, remote_marccd, "start",
						MXD_REMOTE_MARCCD_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_stop( MX_CCD *ccd )
{
	static const char fname[] = "mxd_remote_marccd_stop()";

	MX_REMOTE_MARCCD *remote_marccd;
	mx_status_type mx_status;

	mx_status = mxd_remote_marccd_get_pointers(ccd, &remote_marccd, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	remote_marccd->finish_time = mx_current_clock_tick();

	remote_marccd->current_command = MXF_REMOTE_MARCCD_CMD_ABORT;

	mx_status = mxd_remote_marccd_command( ccd, remote_marccd, "abort",
						MXD_REMOTE_MARCCD_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_get_status( MX_CCD *ccd )
{
	static const char fname[] = "mxd_remote_marccd_get_status()";

	MX_REMOTE_MARCCD *remote_marccd;
	MX_CLOCK_TICK current_time, finish_time;
	mx_status_type mx_status;

	mx_status = mxd_remote_marccd_get_pointers(ccd, &remote_marccd, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	mx_status = mxd_remote_marccd_check_for_responses( ccd, remote_marccd,
						MXD_REMOTE_MARCCD_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	current_time = mx_current_clock_tick();

	finish_time = remote_marccd->finish_time;

	if ( mx_compare_clock_ticks( current_time, finish_time ) < 0 ) {
		ccd->status |= MXSF_CCD_WAITING_FOR_FINISH_TIME;
	} else {
		ccd->status &= ~MXSF_CCD_WAITING_FOR_FINISH_TIME;

		remote_marccd->use_finish_time = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_readout( MX_CCD *ccd )
{
	static const char fname[] = "mxd_remote_marccd_readout()";

	MX_REMOTE_MARCCD *remote_marccd;
	char command[40];
	int flag;
	mx_hex_type cmd_flags;
	mx_status_type mx_status;

	mx_status = mxd_remote_marccd_get_pointers(ccd, &remote_marccd, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.  ccd->ccd_flags = %#lx",
		fname, ccd->record->name, (unsigned long) ccd->ccd_flags));

#if 0
	if ( ( ccd->ccd_flags & MXF_CCD_TO_RAW_DATA_FRAME ) == 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Readout from MarCCD '%s' can only take place if the "
		"raw data frame can be overwritten.", ccd->record->name );
	}
#endif

	if ( ccd->ccd_flags & MXF_CCD_TO_BACKGROUND_FRAME ) {
		flag = 1;
	} else
	if ( ccd->ccd_flags & MXF_CCD_TO_SCRATCH_FRAME ) {
		flag = 2;
	} else {
		flag = 0;
	}

	sprintf( command, "readout,%d", flag );

	remote_marccd->current_command = MXF_REMOTE_MARCCD_CMD_READOUT;

	cmd_flags = MXF_REMOTE_MARCCD_FORCE_READ | MXD_REMOTE_MARCCD_DEBUG;

	mx_status = mxd_remote_marccd_command( ccd, remote_marccd, command,
							cmd_flags );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_dezinger( MX_CCD *ccd )
{
	static const char fname[] = "mxd_remote_marccd_dezinger()";

	MX_REMOTE_MARCCD *remote_marccd;
	char command[40];
	int flag;
	mx_status_type mx_status;

	mx_status = mxd_remote_marccd_get_pointers(ccd, &remote_marccd, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	if ( ccd->ccd_flags & MXF_CCD_TO_RAW_DATA_FRAME ) {
		flag = 0;
	} else
	if ( ccd->ccd_flags & MXF_CCD_TO_BACKGROUND_FRAME ) {
		flag = 1;
	} else
	if ( ccd->ccd_flags & MXF_CCD_TO_SCRATCH_FRAME ) {
		flag = 2;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported destination requested for "
		"dezingering of MarCCD '%s' data frame.",
			ccd->record->name );
	}

	sprintf( command, "dezinger,%d", flag );

	remote_marccd->current_command = MXF_REMOTE_MARCCD_CMD_DEZINGER;

	mx_status = mxd_remote_marccd_command( ccd, remote_marccd, command,
						MXD_REMOTE_MARCCD_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_correct( MX_CCD *ccd )
{
	static const char fname[] = "mxd_remote_marccd_correct()";

	MX_REMOTE_MARCCD *remote_marccd;
	mx_status_type mx_status;

	mx_status = mxd_remote_marccd_get_pointers(ccd, &remote_marccd, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	remote_marccd->current_command = MXF_REMOTE_MARCCD_CMD_CORRECT;
	remote_marccd->next_state = MXF_REMOTE_MARCCD_STATE_CORRECT;

	mx_status = mxd_remote_marccd_command( ccd, remote_marccd, "correct",
						MXD_REMOTE_MARCCD_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_writefile( MX_CCD *ccd )
{
	static const char fname[] = "mxd_remote_marccd_writefile()";

	MX_REMOTE_MARCCD *remote_marccd;
	char command[MXU_FILENAME_LENGTH + 40];
	int flag;
	mx_status_type mx_status;

	mx_status = mxd_remote_marccd_get_pointers(ccd, &remote_marccd, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	if ( ccd->ccd_flags & MXF_CCD_FROM_RAW_DATA_FRAME ) {
		flag = 0;
	} else
	if ( ccd->ccd_flags & MXF_CCD_FROM_CORRECTED_FRAME ) {
		flag = 1;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
	"Unsupported frame file destination requested for MarCCD '%s'.",
			ccd->record->name );
	}

	sprintf( command, "writefile,%s,%d", ccd->writefile_name, flag );

	remote_marccd->current_command = MXF_REMOTE_MARCCD_CMD_WRITEFILE;
	remote_marccd->next_state = MXF_REMOTE_MARCCD_STATE_WRITING;

	mx_status = mxd_remote_marccd_command( ccd, remote_marccd, command,
						MXD_REMOTE_MARCCD_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_get_parameter( MX_CCD *ccd )
{
	static const char fname[] = "mxd_remote_marccd_get_parameter()";

	MX_REMOTE_MARCCD *remote_marccd;
	mx_status_type mx_status;

	mx_status = mxd_remote_marccd_get_pointers( ccd,
						&remote_marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for CCD '%s', parameter type '%s' (%d)",
		fname, ccd->record->name,
		mx_get_field_label_string( ccd->record,
					ccd->parameter_type ),
		ccd->parameter_type));

	switch( ccd->parameter_type ) {
	case MXLV_CCD_DATA_FRAME_SIZE:
		mx_status = mxd_remote_marccd_command( ccd, remote_marccd,
					"get_size", MXD_REMOTE_MARCCD_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXLV_CCD_BIN_SIZE:
		mx_status = mxd_remote_marccd_command( ccd, remote_marccd,
					"get_bin", MXD_REMOTE_MARCCD_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	default:
		return mx_ccd_default_get_parameter_handler( ccd );
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_set_parameter( MX_CCD *ccd )
{
	static const char fname[] = "mxd_remote_marccd_set_parameter()";

	MX_REMOTE_MARCCD *remote_marccd;
	char command[ MXU_CCD_HEADER_NAME_LENGTH + MXU_BUFFER_LENGTH + 40 ];
	mx_status_type mx_status;

	mx_status = mxd_remote_marccd_get_pointers( ccd,
						&remote_marccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for CCD '%s', parameter type '%s' (%d)",
		fname, ccd->record->name,
		mx_get_field_label_string( ccd->record,
					ccd->parameter_type ),
		ccd->parameter_type));

	switch( ccd->parameter_type ) {
	case MXLV_CCD_DATA_FRAME_SIZE:
		sprintf( command, "set_bin,%d,%d", ccd->bin_size[0],
						ccd->bin_size[1] );

		mx_status = mxd_remote_marccd_command( ccd, remote_marccd,
					command, MXD_REMOTE_MARCCD_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXLV_CCD_HEADER_VARIABLE_NAME:
		/* Nothing needs to be done beyond storing the name
		 * for later use by a write to 'header_variable_contents'.
		 */

		break;
	case MXLV_CCD_HEADER_VARIABLE_CONTENTS:
		sprintf( command, "header,%s=%s", ccd->header_variable_name,
					ccd->header_variable_contents );

		mx_status = mxd_remote_marccd_command( ccd, remote_marccd,
					command, MXD_REMOTE_MARCCD_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	default:
		return mx_ccd_default_set_parameter_handler( ccd );
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

/* --- */

MX_EXPORT mx_status_type
mxd_remote_marccd_command( MX_CCD *ccd,
				MX_REMOTE_MARCCD *remote_marccd,
				char *command,
				mx_hex_type flags )
{
	static const char fname[] = "mxd_remote_marccd_command()";

	size_t num_bytes_left;
	char *ptr;
	char newline = '\n';
	int saved_errno, num_bytes_written;
	mx_status_type mx_status;

	if ( remote_marccd == (MX_REMOTE_MARCCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_REMOTE_MARCCD pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command string pointer passed was NULL." );
	}

	if ( remote_marccd->fd_to_marccd < 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
	"The connection to MarCCD has not been correctly initialized." );
	}

	if ( flags ) {
		MX_DEBUG(-2,("%s: sent '%s' to '%s'",
			fname, command, remote_marccd->record->name));
	}

	if ( remote_marccd->marccd_socket != NULL ) {

		mx_status = mx_socket_putline( remote_marccd->marccd_socket,
						command, MX_CR_LF );
	} else {
		ptr            = command;
		num_bytes_left = strlen( command );

		while ( num_bytes_left > 0 ) {
			MX_DEBUG( 2,("%s: num_bytes_left = %ld",
				fname, (long) num_bytes_left));

			num_bytes_written = write( remote_marccd->fd_to_marccd,
						ptr, num_bytes_left );

			if ( num_bytes_written < 0 ) {
				saved_errno = errno;

				return mx_error( MXE_IPC_IO_ERROR, fname,
			"Error sending command '%s' to MarCCD '%s'.  "
			"Error message = '%s'.",
					command, remote_marccd->record->name,
					strerror( saved_errno ) );
			}

			MX_DEBUG( 2,("%s: num_bytes_written = %ld",
				fname, (long) num_bytes_written));

			ptr            += num_bytes_written;
			num_bytes_left -= num_bytes_written;
		}

		/* Write a trailing newline. */

		MX_DEBUG( 2,("%s: about to write a trailing newline.",fname));

		num_bytes_written = 0;

		while ( num_bytes_written == 0 ) {
			num_bytes_written = write( remote_marccd->fd_to_marccd,
						&newline, 1 );

			if ( num_bytes_written < 0 ) {
				saved_errno = errno;

				return mx_error( MXE_IPC_IO_ERROR, fname,
			"Error writing the trailing newline for command '%s' "
			"to MarCCD '%s'.  Error message = '%s'.",
					command, remote_marccd->record->name,
					strerror( saved_errno ) );
			}
		}
	}

	MX_DEBUG( 2,("%s: finished sending command '%s'.",
			fname, command));

	mx_status = mxd_remote_marccd_check_for_responses(
					ccd, remote_marccd, flags );

	return mx_status;
}

static const char *
mxd_remote_marccd_get_command_name( int marccd_command )
{
	static const char command_name[][20] = {
		"none",
		"start",
		"readout",
		"dezinger",
		"correct",
		"writefile",
		"abort",
		"header",
		"shutter",
		"get state",
		"set state",
		"get bin",
		"set bin",
		"get size",
		"phi",
		"distance",
		"end_automation" };

	static int num_command_names = sizeof( command_name )
					/ sizeof( command_name[0] );

	static const char illegal[20] = "illegal";

	if (( marccd_command < 0 ) || ( marccd_command > num_command_names )) {
		return &illegal[0];
	}

	return &command_name[ marccd_command ][0];
}

static const char *
mxd_remote_marccd_get_state_name( int marccd_state )
{
	static const char state_name[][20] = {
		"idle",
		"acquire",
		"readout",
		"correct",
		"writing",
		"aborting",
		"unavailable",
		"error",
		"busy" };

	static int num_state_names = sizeof( state_name )
					/ sizeof( state_name[0] );

	static const char illegal[20] = "illegal";

	if ( ( marccd_state < 0 ) || ( marccd_state > num_state_names ) ) {
		return &illegal[0];
	}

	return &state_name[ marccd_state ][0];
}

#define MX_REMOTE_MARCCD_UNHANDLED_COMMAND_MESSAGE \
	mx_warning( "UNHANDLED MarCCD command %d for MarCCD state = %d", \
		remote_marccd->current_command, current_state )


static mx_status_type
mxd_remote_marccd_handle_state_value( MX_CCD *ccd,
					MX_REMOTE_MARCCD *remote_marccd,
					int current_state )
{
	static const char fname[] = "mxd_remote_marccd_handle_state_value()";

	int old_state, next_state, current_command;

	old_state = remote_marccd->current_state;
	next_state = remote_marccd->next_state;

	remote_marccd->current_state = current_state;

	current_command = remote_marccd->current_command;

	if ( old_state != current_state ) {
		MX_DEBUG(-2,("%s: MarCCD NEW STATE = %d '%s', OLD STATE = '%s'",
			fname, current_state,
			mxd_remote_marccd_get_state_name(current_state),
			mxd_remote_marccd_get_state_name(old_state) ));
	}

	MX_DEBUG(-2,("%s: MarCCD command = %d '%s', state = %d '%s'", fname,
	  current_command, mxd_remote_marccd_get_command_name(current_command),
	  current_state, mxd_remote_marccd_get_state_name(current_state) ));

	/* The interpretation of the state flag depends on the command that
	 * is currently supposed to be executing.
	 */

	ccd->status = 0;

	switch( current_command ) {
	case MXF_REMOTE_MARCCD_CMD_NONE:
		switch( current_state ) {
		case MXF_REMOTE_MARCCD_STATE_IDLE:
			MX_DEBUG(-2,("%s: MarCCD is idle.", fname));

			ccd->status = 0;
			break;
		default:
			MX_REMOTE_MARCCD_UNHANDLED_COMMAND_MESSAGE;
			break;
		}
		break;
	case MXF_REMOTE_MARCCD_CMD_START:
		switch( current_state ) {
		case MXF_REMOTE_MARCCD_STATE_IDLE:
		case MXF_REMOTE_MARCCD_STATE_BUSY:
		case MXF_REMOTE_MARCCD_STATE_ACQUIRE:
			MX_DEBUG(-2,("%s: Waiting for start command to finish.",
				fname ));

			ccd->status = MXSF_CCD_ACQUIRING_FRAME;
			break;
		default:
			MX_REMOTE_MARCCD_UNHANDLED_COMMAND_MESSAGE;
			break;
		}
		break;
	case MXF_REMOTE_MARCCD_CMD_READOUT:
		switch( current_state ) {
		case MXF_REMOTE_MARCCD_STATE_READOUT:
			MX_DEBUG(-2,("%s: Readout in progress.", fname));

			ccd->status = MXSF_CCD_READING_FRAME;
			break;
		case MXF_REMOTE_MARCCD_STATE_IDLE:
			MX_DEBUG(-2,("%s: Readout is complete.", fname));

			ccd->status = 0;
			remote_marccd->current_command
					= MXF_REMOTE_MARCCD_CMD_NONE;
			break;
		default:
			MX_REMOTE_MARCCD_UNHANDLED_COMMAND_MESSAGE;
			break;
		}
		break;
	case MXF_REMOTE_MARCCD_CMD_CORRECT:
		switch( current_state ) {
		case MXF_REMOTE_MARCCD_STATE_IDLE:
			if ( next_state == MXF_REMOTE_MARCCD_STATE_CORRECT ) {
				MX_DEBUG(-2,
				("%s: Waiting for correct command to finish.",
					fname));

				ccd->status = MXSF_CCD_CORRECTING_FRAME;
			} else {
				MX_DEBUG(-2,
				("%s: Frame correction has completed.", fname));

				ccd->status = 0;
			}
			break;
		case MXF_REMOTE_MARCCD_STATE_CORRECT:
			MX_DEBUG(-2,
			("%s: Waiting for correct command to finish.", fname));

			ccd->status = MXSF_CCD_CORRECTING_FRAME;

			remote_marccd->next_state =
					MXF_REMOTE_MARCCD_STATE_IDLE;
			break;
		case MXF_REMOTE_MARCCD_STATE_ERROR:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
"An error occurred while trying to correct a frame for MarCCD device '%s'.",
				ccd->record->name );
		default:
			MX_REMOTE_MARCCD_UNHANDLED_COMMAND_MESSAGE;
			break;
		}
		break;
	case MXF_REMOTE_MARCCD_CMD_WRITEFILE:
		switch( current_state ) {
		case MXF_REMOTE_MARCCD_STATE_IDLE:
			if ( next_state == MXF_REMOTE_MARCCD_STATE_WRITING ) {
				MX_DEBUG(-2,
				("%s: Waiting for frame write to complete.",
					fname));

				ccd->status = MXSF_CCD_WRITING_FRAME;
			} else {
				MX_DEBUG(-2,
				("%s: Frame write has completed.", fname));

				ccd->status = 0;
			}
			break;
		case MXF_REMOTE_MARCCD_STATE_WRITING:
			MX_DEBUG(-2,
			("%s: Waiting for frame write to complete.", fname));

			ccd->status = MXSF_CCD_WRITING_FRAME;

			remote_marccd->next_state =
					MXF_REMOTE_MARCCD_STATE_IDLE;
			break;
		case MXF_REMOTE_MARCCD_STATE_ERROR:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
"An error occurred while trying to write a frame for MarCCD device '%s'.",
				ccd->record->name );
		default:
			MX_REMOTE_MARCCD_UNHANDLED_COMMAND_MESSAGE;
			break;
		}
		break;
	default:
		MX_REMOTE_MARCCD_UNHANDLED_COMMAND_MESSAGE;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_remote_marccd_handle_response( MX_CCD *ccd,
					MX_REMOTE_MARCCD *remote_marccd,
					mx_hex_type flags )
{
	static const char fname[] = "mxd_remote_marccd_handle_response()";

	char response[100];
	size_t length;
	int i, num_bytes_read, num_items, saved_errno, marccd_state;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s: invoked.",fname));

	if ( remote_marccd == (MX_REMOTE_MARCCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_REMOTE_MARCCD pointer passed was NULL." );
	}
	if ( remote_marccd->fd_from_marccd < 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The connection from MarCCD has not been fully initialized." );
	}

	/* Read the response line from MarCCD. */

	if ( remote_marccd->marccd_socket != NULL ) {

		mx_status = mx_socket_getline( remote_marccd->marccd_socket,
						response, sizeof(response),
						MX_CR_LF );
	} else {
		/* Read until we get a newline. */

		for ( i = 0; i < sizeof(response); i++ ) {

			num_bytes_read = read( remote_marccd->fd_from_marccd,
						&(response[i]), 1 );

			if ( num_bytes_read < 0 ) {
				saved_errno = errno;

				return mx_error( MXE_IPC_IO_ERROR, fname,
				"Error reading from MarCCD '%s'.  "
				"Error message = '%s'.",
					remote_marccd->record->name,
					strerror( saved_errno ) );
			}
			if ( num_bytes_read == 0 ) {
				response[i] = '\0';
				break;		/* Exit the for() loop. */
			}
			if ( response[i] == '\n' ) {
				/* Found the end of the line. */

				response[i] = '\0';
				break;		/* Exit the for() loop. */
			}
		}

		/* If the response was too long, print an error message. */

		if ( i >= sizeof(response) ) {
			response[ sizeof(response) - 1 ] = '\0';

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The response from MarCCD '%s' is too long to fit into "
			"our input buffer.  The response begins with '%s'.",
				remote_marccd->record->name, response );
		}
	}

	/* Delete any trailing newline if present. */

	length = strlen( response );

	if ( response[length-1] == '\n' )
		response[length-1] = '\0';

	if ( flags ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, remote_marccd->record->name ));
	}

	if ( strncmp( response, "is_size", 7 ) == 0 ) {

		num_items = sscanf( response, "is_size,%d,%d",
				&(ccd->data_frame_size[0]),
				&(ccd->data_frame_size[1]) );

		if ( num_items != 2 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Cannot find the MarCCD data frame size in the "
				"response '%s'.", response );
		}

		MX_DEBUG( 2,
			("%s: data_frame_size[0] = %d, data_frame_size[1] = %d",
			fname, ccd->data_frame_size[0],
			ccd->data_frame_size[0]));

	} else
	if ( strncmp( response, "is_bin", 6 ) == 0 ) {

		num_items = sscanf( response, "is_bin,%d,%d",
				&(ccd->bin_size[0]),
				&(ccd->bin_size[1]) );

		if ( num_items != 2 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Cannot find the MarCCD bin size in the "
				"response '%s'.", response );
		}

		MX_DEBUG( 2,("%s: bin_size[0] = %d, bin_size[1] = %d",
			fname, ccd->bin_size[0], ccd->bin_size[1]));

	} else
	if ( strncmp( response, "is_preset", 9 ) == 0 ) {

		MX_DEBUG(-2,("%s: preset '%s' seen. WHAT DO WE DO WITH IT?",
			fname, response));

	} else
	if ( strncmp( response, "is_state", 8 ) == 0 ) {
	
		num_items = sscanf( response, "is_state,%d", &marccd_state );
	
		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Cannot find the MarCCD status in the "
				"response '%s'.", response );
		}

		mx_status = mxd_remote_marccd_handle_state_value( ccd,
						remote_marccd, marccd_state );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable response '%s' from MarCCD '%s'.",
			response, ccd->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static int
mxd_remote_marccd_input_is_available( MX_REMOTE_MARCCD *remote_marccd )
{
	static const char fname[] = "mxd_remote_marccd_input_is_available()";

	int fd, select_status;
	struct timeval timeout;
#if HAVE_FD_SET
	fd_set mask;
#else
	long mask;
#endif

	fd = remote_marccd->fd_from_marccd;

#if HAVE_FD_SET
	FD_ZERO( &mask );
	FD_SET( fd, &mask );
#else
	mask = 1 << fd;
#endif
	timeout.tv_usec = 0;
	timeout.tv_sec = 0;

	select_status = select( 1 + fd, &mask, NULL, NULL, &timeout );

	MX_DEBUG( 2,("%s: select_status = %d", fname, select_status));

	if ( select_status ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

MX_EXPORT mx_status_type
mxd_remote_marccd_check_for_responses( MX_CCD *ccd,
					MX_REMOTE_MARCCD *remote_marccd,
					mx_hex_type flags )
{
	static const char fname[] = "mxd_remote_marccd_check_for_responses()";

	int input_available;
	unsigned long i, num_times_to_check, sleep_us;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s *** invoked.", fname));

	num_times_to_check = 100;

	sleep_us = 1;

	for ( i = 0; i < num_times_to_check; i++ ) {

		input_available =
			mxd_remote_marccd_input_is_available( remote_marccd );

		MX_DEBUG(-2,("%s: input_available = %d",
			fname, input_available));

		if ( flags & MXF_REMOTE_MARCCD_FORCE_READ ) {
			input_available = 1;

			flags &= ( ~MXF_REMOTE_MARCCD_FORCE_READ );

			MX_DEBUG(-2,("%s: Read was FORCED.", fname));
		}

		if ( input_available == 0 ) {
			/* No input is available, so exit the while() loop. */

			break;
		}

		mx_status = mxd_remote_marccd_handle_response( ccd,
							remote_marccd, flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_usleep(sleep_us);
	}

	if ( i >= num_times_to_check ) {
		return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after %g seconds while waiting for input "
			"from MarCCD detector '%s' to stop.",
			1.0e-6 * (double)( num_times_to_check * sleep_us ),
				ccd->record->name );
	}

	MX_DEBUG(-2,("%s: *** returning after %lu checks.", fname, i));

	mx_status = mxd_remote_marccd_handle_state_value( ccd, remote_marccd,
						remote_marccd->current_state );

	return mx_status;
}

