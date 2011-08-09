/*
 * Name:    i_epics_rs232.c
 *
 * Purpose: MX driver for the EPICS RS-232 record written by Mark Rivers.
 *
 * Author:  William Lavender
 *
 *          The SCAN field for the EPICS record must be set to 'Passive'.
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define EPIC_RS232_DEBUG	FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_epics.h"
#include "mx_rs232.h"
#include "i_epics_rs232.h"

MX_RECORD_FUNCTION_LIST mxi_epics_rs232_record_function_list = {
	NULL,
	mxi_epics_rs232_create_record_structures,
	mxi_epics_rs232_finish_record_initialization,
	mxi_epics_rs232_delete_record,
	NULL,
	mxi_epics_rs232_open
};

MX_RS232_FUNCTION_LIST mxi_epics_rs232_rs232_function_list = {
	mxi_epics_rs232_getchar,
	mxi_epics_rs232_putchar,
	mxi_epics_rs232_read,
	mxi_epics_rs232_write,
	mxi_epics_rs232_getline,
	mxi_epics_rs232_putline,
	mxi_epics_rs232_num_input_bytes_available,
	mxi_epics_rs232_discard_unread_input,
	mxi_epics_rs232_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_epics_rs232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_EPICS_RS232_STANDARD_FIELDS
};

long mxi_epics_rs232_num_record_fields
		= sizeof( mxi_epics_rs232_record_field_defaults )
			/ sizeof( mxi_epics_rs232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_epics_rs232_rfield_def_ptr
			= &mxi_epics_rs232_record_field_defaults[0];

/* ---- */

static mx_status_type
mxi_epics_rs232_set_transaction_mode( MX_EPICS_RS232 *epics_rs232,
					MX_RS232 *rs232,
					long mode );

static mx_status_type
mxi_epics_rs232_read_buffer( MX_EPICS_RS232 *epics_rs232,
				MX_RS232 *rs232,
				char *buffer,
				long *num_chars,
				int enable_input_delimiter );

static mx_status_type
mxi_epics_rs232_write_buffer( MX_EPICS_RS232 *epics_rs232,
				MX_RS232 *rs232,
				char *buffer,
				long *num_chars,
				int add_output_delimiter );

/* A private function for the use of the driver. */

static mx_status_type
mxi_epics_rs232_get_pointers( MX_RS232 *rs232,
			MX_EPICS_RS232 **epics_rs232,
			const char *calling_fname )
{
	static const char fname[] = "mxi_epics_rs232_get_pointers()";

	MX_RECORD *epics_rs232_record;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( epics_rs232 == (MX_EPICS_RS232 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	epics_rs232_record = rs232->record;

	if ( epics_rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The epics_rs232_record pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*epics_rs232 = (MX_EPICS_RS232 *)
				epics_rs232_record->record_type_struct;

	if ( *epics_rs232 == (MX_EPICS_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_RS232 pointer for record '%s' is NULL.",
			epics_rs232_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_epics_rs232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_epics_rs232_create_record_structures()";

	MX_RS232 *rs232;
	MX_EPICS_RS232 *epics_rs232;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	epics_rs232 = (MX_EPICS_RS232 *) malloc( sizeof(MX_EPICS_RS232) );

	if ( epics_rs232 == (MX_EPICS_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPICS_RS232 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = epics_rs232;
	record->class_specific_function_list
				= &mxi_epics_rs232_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_rs232_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_epics_rs232_finish_record_initialization()";

	MX_EPICS_RS232 *epics_rs232;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.", fname));

	epics_rs232 = (MX_EPICS_RS232 *) (record->record_type_struct);

	if ( epics_rs232 == (MX_EPICS_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_EPICS_RS232 structure pointer for '%s' is NULL.",
			record->name );
	}

	epics_rs232->input_buffer = NULL;
	epics_rs232->next_unread_char_ptr = NULL;
	epics_rs232->last_read_length = 0;

	/* Check to see if the RS-232 parameters are valid. */

	status = mx_rs232_check_port_parameters( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Mark the EPICS_RS232 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	/* Initialize MX EPICS data structures. */

	mx_epics_pvname_init( &(epics_rs232->binp_pv),
				"%s.BINP", epics_rs232->epics_record_name );

	mx_epics_pvname_init( &(epics_rs232->bout_pv),
				"%s.BOUT", epics_rs232->epics_record_name );

	mx_epics_pvname_init( &(epics_rs232->idel_pv),
				"%s.IDEL", epics_rs232->epics_record_name );

	mx_epics_pvname_init( &(epics_rs232->nord_pv),
				"%s.NORD", epics_rs232->epics_record_name );

	mx_epics_pvname_init( &(epics_rs232->nowt_pv),
				"%s.NOWT", epics_rs232->epics_record_name );

	mx_epics_pvname_init( &(epics_rs232->nrrd_pv),
				"%s.NRRD", epics_rs232->epics_record_name );

	mx_epics_pvname_init( &(epics_rs232->tmod_pv),
				"%s.TMOD", epics_rs232->epics_record_name );

	mx_epics_pvname_init( &(epics_rs232->tmot_pv),
				"%s.TMOT", epics_rs232->epics_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_rs232_delete_record( MX_RECORD *record )
{
	MX_EPICS_RS232 *epics_rs232;

        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }

	epics_rs232 = (MX_EPICS_RS232 *) record->record_type_struct;

        if ( epics_rs232 != NULL ) {
		if ( epics_rs232->input_buffer != NULL ) {
			free( epics_rs232->input_buffer );
		}

                free( epics_rs232 );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_rs232_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_epics_rs232_open()";

	MX_RS232 *rs232;
	MX_EPICS_RS232 *epics_rs232 = NULL;
	char pvname[MXU_EPICS_PVNAME_LENGTH+1];
	char output_delimiter;
	int32_t format;
	int32_t speed, wordsize, parity, stop_bits, flow_control;
	int buffer_length;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	rs232 = (MX_RS232 *) (record->record_class_struct);

	mx_status = mxi_epics_rs232_get_pointers( rs232, &epics_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if EPIC_RS232_DEBUG
	mx_epics_set_debug_flag( TRUE );
#endif

	/* Set the input format to binary. */

	sprintf( pvname, "%s.IFMT", epics_rs232->epics_record_name );

	format = MXF_EPICS_RS232_BINARY_FORMAT;

	mx_status = mx_caput_nowait_by_name( pvname, MX_CA_LONG, 1, &format );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the output format to binary. */

	sprintf( pvname, "%s.OFMT", epics_rs232->epics_record_name );

	format = MXF_EPICS_RS232_BINARY_FORMAT;

	mx_status = mx_caput_nowait_by_name( pvname, MX_CA_LONG, 1, &format );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Unconditionally turn off output delimiters. */

	sprintf( pvname, "%s.OEOS", epics_rs232->epics_record_name );

	output_delimiter = '\0';

	mx_status = mx_caput_nowait_by_name( pvname,
					MX_CA_STRING, 1, &output_delimiter );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out how big the input buffer is for the EPICS record. */

	sprintf( pvname, "%s.IMAX", epics_rs232->epics_record_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1,
				&(epics_rs232->max_input_length) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate a local input buffer. */

	buffer_length =
		epics_rs232->max_input_length + MX_RS232_MAX_TERMINATORS + 1;

	epics_rs232->input_buffer = (char *)
				malloc( buffer_length * sizeof(char) );

	if ( epics_rs232->input_buffer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Unable to allocate %d byte input buffer for EPICS RS-232 record '%s'.",
			buffer_length, record->name );
	}

	/* Find out how big the output buffer is for the EPICS record. */

	sprintf( pvname, "%s.OMAX", epics_rs232->epics_record_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1,
				&(epics_rs232->max_output_length) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate a local output buffer. */

	buffer_length =
		epics_rs232->max_output_length + MX_RS232_MAX_TERMINATORS + 1;

	epics_rs232->output_buffer = (char *) 
				malloc( buffer_length * sizeof(char) );

	if ( epics_rs232->output_buffer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Unable to allocate %d byte output buffer for EPICS RS-232 record '%s'.",
		    (int) epics_rs232->max_output_length + 1, record->name );
	}

	/* Set num_chars_to_read to -1.  This lets other routines know
	 * that we have not yet done our first read command.
	 */

	epics_rs232->num_chars_to_read = -1L;

	/* Do the same for num_chars_to_write. */

	epics_rs232->num_chars_to_write = -1L;

	/* Set the default timeout to 500 milliseconds.  This matches
	 * the default for the EPICS record.
	 */

	epics_rs232->default_timeout = 500L;

	/* Initialize the other variables to known values. */

	epics_rs232->input_delimiter_on = FALSE;
	epics_rs232->transaction_mode = -1L;
	epics_rs232->timeout = -1L;

	/* Set the port speed. */

	switch( rs232->speed ) {
	case 38400: speed = MXF_EPICS_RS232_BPS_38400;	break;
	case 19200: speed = MXF_EPICS_RS232_BPS_19200;	break;
	case 9600:  speed = MXF_EPICS_RS232_BPS_9600;	break;
	case 4800:  speed = MXF_EPICS_RS232_BPS_4800;	break;
	case 2400:  speed = MXF_EPICS_RS232_BPS_2400;	break;
	case 1200:  speed = MXF_EPICS_RS232_BPS_1200;	break;
	case 600:   speed = MXF_EPICS_RS232_BPS_600;	break;
	case 300:   speed = MXF_EPICS_RS232_BPS_300;	break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported EPICS RS-232 speed = %ld for port '%s'.", 
		rs232->speed, record->name);

		break;
	}

	sprintf( pvname, "%s.BAUD", epics_rs232->epics_record_name );

	mx_status = mx_caput_nowait_by_name( pvname, MX_CA_LONG, 1, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the word size. */

	switch( rs232->word_size ) {
	case 8: wordsize = MXF_EPICS_RS232_WORDSIZE_8; break;
	case 7: wordsize = MXF_EPICS_RS232_WORDSIZE_7; break;
	case 6: wordsize = MXF_EPICS_RS232_WORDSIZE_6; break;
	case 5: wordsize = MXF_EPICS_RS232_WORDSIZE_5; break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported EPICS RS-232 word size = %ld for port '%s'.", 
		rs232->word_size, record->name);

		break;
	}

	sprintf( pvname, "%s.DBIT", epics_rs232->epics_record_name );

	mx_status = mx_caput_nowait_by_name( pvname,
						MX_CA_LONG, 1, &wordsize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the parity. */

	switch( rs232->parity ) {
	case MXF_232_NO_PARITY:
		parity = MXF_EPICS_RS232_NO_PARITY;
		break;

	case MXF_232_EVEN_PARITY:
		parity = MXF_EPICS_RS232_EVEN_PARITY;
		break;
		
	case MXF_232_ODD_PARITY:
		parity = MXF_EPICS_RS232_ODD_PARITY;
		break;

	case MXF_232_MARK_PARITY:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Mark parity is not supported for RS-232 port '%s'.",
			record->name);
		break;

	case MXF_232_SPACE_PARITY:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Space parity is not supported for RS-232 port '%s'.",
			record->name);
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported EPICS RS-232 parity = '%c' for port '%s'.", 
		rs232->parity, record->name);

		break;
	}


	sprintf( pvname, "%s.PRTY", epics_rs232->epics_record_name );

	mx_status = mx_caput_nowait_by_name( pvname, MX_CA_LONG, 1, &parity );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the stop bits. */

	switch( rs232->stop_bits ) {
	case 1: stop_bits = MXF_EPICS_RS232_STOPBITS_1; break;
	case 2: stop_bits = MXF_EPICS_RS232_STOPBITS_2; break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported EPICS RS-232 stop bits = %ld for port '%s'.", 
		rs232->stop_bits, record->name);

		break;
	}

	sprintf( pvname, "%s.SBIT", epics_rs232->epics_record_name );

	mx_status = mx_caput_nowait_by_name( pvname,
						MX_CA_LONG, 1, &stop_bits );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the flow control. */

	switch( rs232->flow_control ) {
	case MXF_232_NO_FLOW_CONTROL:
		flow_control = MXF_EPICS_RS232_NO_FLOW_CONTROL;
		break;

	case MXF_232_HARDWARE_FLOW_CONTROL:
		flow_control = MXF_EPICS_RS232_HW_FLOW_CONTROL;
		break;

	case MXF_232_SOFTWARE_FLOW_CONTROL:
	case MXF_232_BOTH_FLOW_CONTROL:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Software flow control is not supported for EPICS RS-232 port '%s'.  "
	"Only hardware flow control is supported.",
			record->name );
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported EPICS RS-232 flow control = %d for port '%s'.", 
		rs232->flow_control, record->name);

		break;
	}

	sprintf( pvname, "%s.FCTL", epics_rs232->epics_record_name );

	mx_status = mx_caput_nowait_by_name( pvname,
					MX_CA_LONG, 1, &flow_control );

	return mx_status;
}


MX_EXPORT mx_status_type
mxi_epics_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_epics_rs232_getchar()";

	MX_EPICS_RS232 *epics_rs232 = NULL;
	long num_chars;
	mx_status_type mx_status;

	mx_status = mxi_epics_rs232_get_pointers( rs232, &epics_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Try to read the character. */

	num_chars = 1L;

	mx_status = mxi_epics_rs232_read_buffer( epics_rs232, rs232,
						c, &num_chars, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* mxi_epics_rs232_getchar() is often used to test whether there is
	 * input ready on the EPICS_RS232 port.  Normally, it is not desirable
	 * to broadcast a message to the world when this fails, so we
	 * add the MXE_QUIET flag to the error code.
	 */

	if ( num_chars != 1L ) {
		return mx_error( (MXE_NOT_READY | MXE_QUIET), fname,
			"Failed to read a character from port '%s'.",
			rs232->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_rs232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_epics_rs232_putchar()";

	MX_EPICS_RS232 *epics_rs232 = NULL;
	long num_chars;
	mx_status_type mx_status;

	mx_status = mxi_epics_rs232_get_pointers( rs232, &epics_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_chars = 1L;

	mx_status = mxi_epics_rs232_write_buffer( epics_rs232, rs232,
						&c, &num_chars, FALSE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_rs232_read( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	static const char fname[] = "mxi_epics_rs232_read()";

	MX_EPICS_RS232 *epics_rs232 = NULL;
	long num_chars;
	mx_status_type mx_status;

	mx_status = mxi_epics_rs232_get_pointers( rs232, &epics_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_chars = (long) max_bytes_to_read;

	mx_status = mxi_epics_rs232_read_buffer( epics_rs232, rs232,
						buffer, &num_chars, FALSE );

	*bytes_read = num_chars;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_rs232_write( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_write,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_epics_rs232_write()";

	MX_EPICS_RS232 *epics_rs232 = NULL;
	long num_chars;
	mx_status_type mx_status;

	mx_status = mxi_epics_rs232_get_pointers( rs232, &epics_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_chars = (long) max_bytes_to_write;

	mx_status = mxi_epics_rs232_write_buffer( epics_rs232, rs232,
						buffer, &num_chars, FALSE );

	*bytes_written = num_chars;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_rs232_getline( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	static const char fname[] = "mxi_epics_rs232_getline()";

	MX_EPICS_RS232 *epics_rs232 = NULL;
	long num_chars;
	mx_status_type mx_status;

	mx_status = mxi_epics_rs232_get_pointers( rs232, &epics_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("\n%s: *** about to read from '%s' ***",
			fname, rs232->record->name));

	num_chars = (long) max_bytes_to_read;

	mx_status = mxi_epics_rs232_read_buffer( epics_rs232, rs232,
						buffer, &num_chars, TRUE );

	if ( bytes_read != NULL ) {
		*bytes_read = num_chars;
	}

	MX_DEBUG(-2,("%s: *** read '%s' from '%s' ***",
		fname, buffer, rs232->record->name ));

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_rs232_putline( MX_RS232 *rs232,
			char *buffer,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_epics_rs232_putline()";

	MX_EPICS_RS232 *epics_rs232 = NULL;
	long num_chars;
	mx_status_type mx_status;

	mx_status = mxi_epics_rs232_get_pointers( rs232, &epics_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("\n%s: *** writing '%s' to '%s' ***",
			fname, buffer, rs232->record->name ));

	num_chars = (long) strlen( buffer );

	mx_status = mxi_epics_rs232_write_buffer( epics_rs232, rs232,
						buffer, &num_chars, TRUE );

	if ( bytes_written != NULL ) {
		*bytes_written = num_chars;
	}

	MX_DEBUG(-2,("%s: *** write complete ***", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	/* At present, I don't see how to implement this function using
	 * the EPICS RS-232 record, so for now we claim that input
	 * is always available.
	 */

	rs232->num_input_bytes_available = 1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_rs232_discard_unread_input( MX_RS232 *rs232 )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_rs232_discard_unwritten_output( MX_RS232 *rs232 )
{
	return MX_SUCCESSFUL_RESULT;
}

/* ==== private functions ==== */

static mx_status_type
mxi_epics_rs232_set_transaction_mode( MX_EPICS_RS232 *epics_rs232,
					MX_RS232 *rs232,
					long mode )
{
	int32_t int32_mode;
	mx_status_type mx_status;

	int32_mode = mode;

	mx_status = mx_caput( &(epics_rs232->tmod_pv), MX_CA_LONG, 1, &int32_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_rs232->transaction_mode = mode;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_epics_rs232_read_buffer( MX_EPICS_RS232 *epics_rs232,
				MX_RS232 *rs232,
				char *buffer,
				long *num_chars,
				int enable_input_delimiter )
{
	static const char fname[] = "mxi_epics_rs232_read_buffer()";

	long i;
	int32_t input_delimiter, timeout, num_chars_read;
	char *ptr;
	mx_bool_type first_time;
	mx_status_type mx_status;

	if ( *num_chars < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Requested number of characters (%ld) for RS-232 port '%s' is "
	"a negative number.", *num_chars, rs232->record->name );
	}
	if ( *num_chars > epics_rs232->max_input_length ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested number of characters (%ld) for RS-232 port '%s' is longer "
	"than the maximum input buffer length of %d.",
			*num_chars, rs232->record->name,
			(int) epics_rs232->max_input_length );
	}

	/* Check to see if this is the first time we have tried to read
	 * from the EPICS RS-232 port.
	 */

	if ( epics_rs232->num_chars_to_read == -1L ) {
		first_time = TRUE;
	} else {
		first_time = FALSE;
	}

	/* Change the transaction mode if needed. */

#if 0
	if ( epics_rs232->transaction_mode != MXF_EPICS_RS232_READ ) {

		mx_status = mxi_epics_rs232_set_transaction_mode( epics_rs232,
						rs232, MXF_EPICS_RS232_READ );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
#endif

	/* Change the state of the input delimiter if needed. */

	if ( (epics_rs232->input_delimiter_on != enable_input_delimiter)
	  || (first_time == TRUE) )
	{
		if ( enable_input_delimiter == FALSE ) {
			input_delimiter = -1;
		} else {
			input_delimiter = (long)
	rs232->read_terminator_array[ rs232->num_read_terminator_chars - 1 ];

		}

		mx_status = mx_caput_nowait( &(epics_rs232->idel_pv),
					MX_CA_LONG, 1, &input_delimiter );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Change the EPICS timeout value if required. */

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {
		timeout = 0L;
	} else {
		timeout = epics_rs232->default_timeout;
	}

	if ( epics_rs232->timeout != timeout ) {

		mx_status = mx_caput_nowait( &(epics_rs232->tmot_pv),
					MX_CA_LONG, 1, &timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		epics_rs232->timeout = timeout;
	}

	/* Set the number of characters to read if required. */

	if ( epics_rs232->num_chars_to_read != *num_chars ) {

		int32_t temp;

#if 0
		temp = 0;
#else
		temp = *num_chars;
#endif

		mx_status = mx_caput_nowait( &(epics_rs232->nrrd_pv),
						MX_CA_LONG, 1, &temp );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		epics_rs232->num_chars_to_read = *num_chars;
	}

	/* Read characters into the buffer. */

	mx_status = mx_caget( &(epics_rs232->binp_pv),
			MX_CA_CHAR, *num_chars, epics_rs232->input_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out how many characters were read. */

	mx_status = mx_caget( &(epics_rs232->nord_pv),
			MX_CA_LONG, 1, &num_chars_read );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	for ( i = 0; i < num_chars_read; i++ ) {
		fprintf(stderr, "%#x ", epics_rs232->input_buffer[i] );
	}
	fprintf(stderr,"\n");
#endif

	/* Strip off additional input delimiter characters if needed. */

	if ( enable_input_delimiter ) {

		/* Only need to strip off extra delimiters if the number
		 * of read terminator characters is greater than one,
		 * since the usage of the .IDEL EPICS field by the EPICS
		 * record will strip off the last delimiter for us.
		 * Go from the end of the buffer back towards the beginning.
		 */

		if ( rs232->num_read_terminator_chars > 1 ) {

			ptr = buffer + num_chars_read - 1;

			for (i =  rs232->num_read_terminator_chars - 2;
				i >= 0; i-- )
			{
				if ( rs232->read_terminator_array[i] != *ptr )
				{
					/* The terminators do not match, so
					 * break out of the loop.
					 */
					break;
				}

				*ptr = '\0';

				num_chars_read--;
				ptr--;
			}
		}
	}

	/* Make sure we are null terminated. */

	(epics_rs232->input_buffer)[num_chars_read] = '\0';

	MX_DEBUG(-2,("%s: input_buffer = '%s'",
			fname, epics_rs232->input_buffer ));

	*num_chars = num_chars_read;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_epics_rs232_write_buffer( MX_EPICS_RS232 *epics_rs232,
				MX_RS232 *rs232,
				char *buffer,
				long *num_chars,
				int add_output_delimiters )
{
	static const char fname[] = "mxi_epics_rs232_write_buffer()";

	long i;
	int32_t timeout, real_num_chars_to_write;
	char *ptr, *buffer_ptr;
	mx_status_type mx_status;

	if ( *num_chars == 1 ) {
		MX_DEBUG(-2,("\n%s: *** sending char %#x to '%s' ***",
			fname, *buffer, rs232->record->name));
	} else {
		MX_DEBUG(-2,("%s: invoked for string.", fname));
	}

	if ( *num_chars < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Requested number of characters (%ld) for RS-232 port '%s' is "
	"a negative number.", *num_chars, rs232->record->name );
	}
	if ( *num_chars > epics_rs232->max_output_length ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested number of characters (%ld) for RS-232 port '%s' is longer "
	"than the maximum output buffer length of %d.",
			*num_chars, rs232->record->name,
			(int) epics_rs232->max_output_length );
	}

	/* Change the transaction mode if needed. */

	if ( epics_rs232->transaction_mode != MXF_EPICS_RS232_WRITE_READ ) {

		mx_status = mxi_epics_rs232_set_transaction_mode( epics_rs232,
					rs232, MXF_EPICS_RS232_WRITE_READ );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Change the EPICS timeout value if required. */

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {
		timeout = 0L;
	} else {
		timeout = epics_rs232->default_timeout;
	}

	if ( epics_rs232->timeout != timeout ) {

		mx_status = mx_caput_nowait( &(epics_rs232->tmot_pv),
						MX_CA_LONG, 1, &timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		epics_rs232->timeout = timeout;
	}

	/* Add output delimiters if requested. */

	real_num_chars_to_write = *num_chars;

	if ( add_output_delimiters == FALSE ) {
		buffer_ptr = buffer;
	} else {
		/* Would adding the delimiter characters to the buffer
		 * exceed the maximum number of characters that can
		 * be sent to the EPICS RS-232 record?  If so, we must
		 * return an error message.
		 */

		if ( *num_chars > epics_rs232->max_output_length ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The output string would require %ld characters which exceeds "
		"the maximum allowed value of %d characters.",
			*num_chars, (int) epics_rs232->max_output_length );
		}

		/* We do not know how long the caller's buffer is, so we
		 * cannot depend on being able to add delimiter characters
		 * after the end of the provided string.  Thus, we must
		 * copy the data in the caller's buffer into a local buffer
		 * of known length.
		 */

		memcpy( epics_rs232->output_buffer,
				buffer, (size_t) *num_chars );

		buffer_ptr = epics_rs232->output_buffer;

		/* Add the delimiters */

		ptr = buffer_ptr + *num_chars;

		for ( i = 0; i < rs232->num_write_terminator_chars; i++ ) {
			*ptr = rs232->write_terminator_array[i];

			ptr++;
		}

		/* Add a null character at the end. */

		*ptr = '\0';

		real_num_chars_to_write += rs232->num_write_terminator_chars;
	}

	/* Set the number of characters to write if required. */

	if ( epics_rs232->num_chars_to_write != real_num_chars_to_write ) {

		mx_status = mx_caput_nowait( &(epics_rs232->nowt_pv),
				MX_CA_LONG, 1, &real_num_chars_to_write);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		epics_rs232->num_chars_to_write = real_num_chars_to_write;
	}

	/* Write characters from the buffer. */

	mx_status = mx_caput_nowait( &(epics_rs232->bout_pv),
			MX_CA_CHAR, real_num_chars_to_write, buffer_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* There seems to be no way to find out how many characters were
	 * actually written, so we leave the value of *num_chars alone.
	 */

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_EPICS */

