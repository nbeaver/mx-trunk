/*
 * Name:    i_epics_gpib.c
 *
 * Purpose: MX driver for the EPICS GPIB record written by Mark Rivers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2006, 2008-2011, 2014-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define EPICS_GPIB_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_epics.h"
#include "mx_gpib.h"
#include "mx_driver.h"
#include "i_epics_gpib.h"

MX_RECORD_FUNCTION_LIST mxi_epics_gpib_record_function_list = {
	NULL,
	mxi_epics_gpib_create_record_structures,
	mxi_epics_gpib_finish_record_initialization,
	NULL,
	mxi_epics_gpib_print_interface_structure,
	mxi_epics_gpib_open
};

MX_GPIB_FUNCTION_LIST mxi_epics_gpib_gpib_function_list = {
	mxi_epics_gpib_open_device,
	mxi_epics_gpib_close_device,
	mxi_epics_gpib_read,
	mxi_epics_gpib_write,
	mxi_epics_gpib_interface_clear,
	mxi_epics_gpib_device_clear,
	mxi_epics_gpib_selective_device_clear,
	mxi_epics_gpib_local_lockout,
	mxi_epics_gpib_remote_enable,
	mxi_epics_gpib_go_to_local,
	mxi_epics_gpib_trigger,
	mxi_epics_gpib_wait_for_service_request,
	mxi_epics_gpib_serial_poll,
	mxi_epics_gpib_serial_poll_disable
};

MX_RECORD_FIELD_DEFAULTS mxi_epics_gpib_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_GPIB_STANDARD_FIELDS,
	MXI_EPICS_GPIB_STANDARD_FIELDS
};

long mxi_epics_gpib_num_record_fields
		= sizeof( mxi_epics_gpib_record_field_defaults )
			/ sizeof( mxi_epics_gpib_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_epics_gpib_rfield_def_ptr
			= &mxi_epics_gpib_record_field_defaults[0];

/* ---- */

static mx_status_type
mxi_epics_gpib_set_address( MX_EPICS_GPIB *epics_gpib,
				MX_GPIB *gpib, int address );
static mx_status_type
mxi_epics_gpib_set_transaction_mode( MX_EPICS_GPIB *epics_gpib,
					MX_GPIB *gpib,
					long mode );

static mx_status_type
mxi_epics_gpib_universal_command( MX_EPICS_GPIB *epics_gpib,
					MX_GPIB *gpib,
					long command );

static mx_status_type
mxi_epics_gpib_addressed_command( MX_EPICS_GPIB *epics_gpib,
					MX_GPIB *gpib,
					int address,
					long command );

/* ==== Private function for the driver's use only. ==== */

static mx_status_type
mxi_epics_gpib_get_pointers( MX_GPIB *gpib,
				MX_EPICS_GPIB **epics_gpib,
				const char *calling_fname )
{
	static const char fname[] = "mxi_epics_gpib_get_pointers()";

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GPIB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( epics_gpib == (MX_EPICS_GPIB **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_EPICS_GPIB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*epics_gpib = (MX_EPICS_GPIB *) (gpib->record->record_type_struct);

	if ( *epics_gpib == (MX_EPICS_GPIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_GPIB pointer for record '%s' is NULL.",
			gpib->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ==== Public functions ==== */

MX_EXPORT mx_status_type
mxi_epics_gpib_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_epics_gpib_create_record_structures()";

	MX_GPIB *gpib;
	MX_EPICS_GPIB *epics_gpib;

	/* Allocate memory for the necessary structures. */

	gpib = (MX_GPIB *) malloc( sizeof(MX_GPIB) );

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GPIB structure." );
	}

	epics_gpib = (MX_EPICS_GPIB *) malloc( sizeof(MX_EPICS_GPIB) );

	if ( epics_gpib == (MX_EPICS_GPIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPICS_GPIB structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = gpib;
	record->record_type_struct = epics_gpib;
	record->class_specific_function_list
				= &mxi_epics_gpib_gpib_function_list;

	gpib->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_epics_gpib_finish_record_initialization()";

	MX_GPIB *gpib;
	MX_EPICS_GPIB *epics_gpib = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mx_status = mx_gpib_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( record->network_type_name, "epics",
				MXU_NETWORK_TYPE_NAME_LENGTH );

	gpib = (MX_GPIB *) (record->record_class_struct);

	mx_status = mxi_epics_gpib_get_pointers( gpib, &epics_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_epics_pvname_init( &(epics_gpib->addr_pv),
				"%s.ADDR", epics_gpib->epics_record_name );

	mx_epics_pvname_init( &(epics_gpib->binp_pv),
				"%s.BINP", epics_gpib->epics_record_name );

	mx_epics_pvname_init( &(epics_gpib->bout_pv),
				"%s.BOUT", epics_gpib->epics_record_name );

	mx_epics_pvname_init( &(epics_gpib->eos_pv),
				"%s.EOS", epics_gpib->epics_record_name );

	mx_epics_pvname_init( &(epics_gpib->nord_pv),
				"%s.NORD", epics_gpib->epics_record_name );

	mx_epics_pvname_init( &(epics_gpib->nowt_pv),
				"%s.NOWT", epics_gpib->epics_record_name );

	mx_epics_pvname_init( &(epics_gpib->nrrd_pv),
				"%s.NRRD", epics_gpib->epics_record_name );

	mx_epics_pvname_init( &(epics_gpib->tmod_pv),
				"%s.TMOD", epics_gpib->epics_record_name );

	mx_epics_pvname_init( &(epics_gpib->tmot_pv),
				"%s.TMOT", epics_gpib->epics_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_print_interface_structure( FILE *file, MX_RECORD *record )
{
	MX_GPIB *gpib;
	MX_EPICS_GPIB *epics_gpib;
	char read_eos_char, write_eos_char;

	gpib = (MX_GPIB *) (record->record_class_struct);

	epics_gpib = (MX_EPICS_GPIB *) (record->record_type_struct);

	fprintf(file, "GPIB parameters for interface '%s':\n", record->name);
	fprintf(file, "  GPIB type              = EPICS_GPIB.\n\n");

	fprintf(file, "  name                   = %s\n", record->name);
	fprintf(file, "  EPICS record name      = %s\n",
					epics_gpib->epics_record_name);
	fprintf(file, "  default I/O timeout    = %g\n",
					gpib->default_io_timeout);
	fprintf(file, "  default EOI mode       = %ld\n",
					gpib->default_eoi_mode);

	read_eos_char = (char) ( gpib->default_read_terminator & 0xff );

	if ( isprint((int) read_eos_char) ) {
		fprintf(file,
		      "  default read EOS char  = 0x%02x (%c)\n",
			read_eos_char, read_eos_char);
	} else {
		fprintf(file,
		      "  default read EOS char  = 0x%02x\n", read_eos_char);
	}

	write_eos_char = (char) ( gpib->default_write_terminator & 0xff );

	if ( isprint((int) write_eos_char) ) {
		fprintf(file,
		      "  default write EOS char = 0x%02x (%c)\n",
			write_eos_char, write_eos_char);
	} else {
		fprintf(file,
		      "  default write EOS char = 0x%02x\n", write_eos_char);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_epics_gpib_open()";

	MX_GPIB *gpib;
	MX_EPICS_GPIB *epics_gpib = NULL;
	char pvname[MXU_EPICS_PVNAME_LENGTH+1];
	int32_t format, command;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	gpib = (MX_GPIB *) (record->record_class_struct);

	mx_status = mxi_epics_gpib_get_pointers( gpib, &epics_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if EPICS_GPIB_DEBUG
	mx_epics_set_debug_flag( TRUE );
#endif

	/* Send device clear to the bus. */

	snprintf( pvname, sizeof(pvname),
		"%s.UCMD", epics_gpib->epics_record_name );

	command = MXF_EPICS_GPIB_DEVICE_CLEAR;

	mx_status = mx_caput_by_name( pvname, MX_CA_LONG, 1, &command );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the input format to binary. */

	snprintf( pvname, sizeof(pvname),
		"%s.IFMT", epics_gpib->epics_record_name );

	format = MXF_EPICS_GPIB_BINARY_FORMAT;

	mx_status = mx_caput_by_name( pvname, MX_CA_LONG, 1, &format );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the output format to binary. */

	snprintf( pvname, sizeof(pvname),
		"%s.OFMT", epics_gpib->epics_record_name );

	format = MXF_EPICS_GPIB_BINARY_FORMAT;

	mx_status = mx_caput_by_name( pvname, MX_CA_LONG, 1, &format );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out how big the input buffer is for the EPICS record. */

	snprintf( pvname, sizeof(pvname),
		"%s.IMAX", epics_gpib->epics_record_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1,
					&(epics_gpib->max_input_length) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out how big the output buffer is for the EPICS record. */

	snprintf( pvname, sizeof(pvname),
		"%s.OMAX", epics_gpib->epics_record_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1,
					&(epics_gpib->max_output_length) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the EOS character. */

	if ( gpib->default_read_terminator > 0xff ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The read terminator %#lx is not allowed.  "
		"Only one character read terminators are allowed for "
		"the EPICS GPIB record.",
			gpib->default_read_terminator );
	}

	epics_gpib->current_eos_char = (long) (gpib->default_read_terminator);

	snprintf( pvname, sizeof(pvname),
		"%s.EOS", epics_gpib->epics_record_name );

	mx_status = mx_caput_by_name( pvname, MX_CA_LONG,
					1, &(epics_gpib->current_eos_char) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set num_chars_to_read to -1.  This lets other routines know
	 * that we have not yet done our first read command.
	 */

	epics_gpib->num_chars_to_read = -1L;

	/* Do the same for num_chars_to_write. */

	epics_gpib->num_chars_to_write = -1L;

	/* Set the default timeout to 500 milliseconds.  This matches
	 * the default for the EPICS record.
	 */

	epics_gpib->default_timeout = 500L;

	/* Initialize the other variables to known values. */

	epics_gpib->current_address = -1L;
	epics_gpib->transaction_mode = -1L;
	epics_gpib->timeout = -1L;

	gpib->read_buffer_length = epics_gpib->max_input_length;
	gpib->write_buffer_length = epics_gpib->max_output_length;

	mx_status = mx_gpib_setup_communication_buffers( record );

	return mx_status;
}

/* ========== Device specific calls ========== */

MX_EXPORT mx_status_type
mxi_epics_gpib_open_device( MX_GPIB *gpib, long address )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_close_device( MX_GPIB *gpib, long address )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_read( MX_GPIB *gpib,
		long address,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		unsigned long flags )
{
	static const char fname[] = "mxi_epics_gpib_read()";

	MX_EPICS_GPIB *epics_gpib = NULL;
	int32_t timeout, num_chars_read;
	int32_t int32_address, int32_eos, int32_max_bytes_to_read;
	mx_status_type mx_status;

	mx_status = mxi_epics_gpib_get_pointers( gpib, &epics_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("*** %s: about to read from '%s:%ld ***",
				fname, gpib->record->name, address));

	if ( max_bytes_to_read > epics_gpib->max_input_length ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested number of characters (%ld) for GPIB port '%s' is longer "
	"than the maximum input buffer length of %ld.",
			(long) max_bytes_to_read, gpib->record->name,
			(long) epics_gpib->max_input_length );
	}

	/* Change the transaction mode if needed. */

	if ( epics_gpib->transaction_mode != MXF_EPICS_GPIB_READ ) {

		mx_status = mxi_epics_gpib_set_transaction_mode( epics_gpib,
					gpib, MXF_EPICS_GPIB_READ );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Change the EPICS timeout value if required. */

	if ( flags & MXF_GPIB_NOWAIT ) {
		timeout = 500L;
	} else {
		timeout = epics_gpib->default_timeout;
	}

	if ( epics_gpib->timeout != timeout ) {

		mx_status = mx_caput( &(epics_gpib->tmot_pv),
					MX_CA_LONG, 1, &timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		epics_gpib->timeout = timeout;
	}

	/* Change the EOS character if required. */

	if ( epics_gpib->current_eos_char != gpib->read_terminator[ address ] )
	{
		int32_eos = gpib->read_terminator[address];

		mx_status = mx_caput( &(epics_gpib->eos_pv),
					MX_CA_LONG, 1, &int32_eos );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		epics_gpib->current_eos_char =
			(long) (gpib->read_terminator[address]);
	}

	/* Set the number of characters to read, if required. */

	if ( epics_gpib->num_chars_to_read != max_bytes_to_read ) {

		int32_max_bytes_to_read = max_bytes_to_read;

		mx_status = mx_caput( &(epics_gpib->nrrd_pv),
				MX_CA_LONG, 1, &int32_max_bytes_to_read);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		epics_gpib->num_chars_to_read = (long) max_bytes_to_read;
	}

	/* Change the GPIB address if required. */

	if ( epics_gpib->current_address != address ) {

		int32_address = (int32_t) address;

		mx_status = mx_caput( &(epics_gpib->addr_pv),
					MX_CA_LONG, 1, &int32_address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		epics_gpib->current_address = address;
	}

	/* Read characters into the buffer. */

	mx_status = mx_caget( &(epics_gpib->binp_pv),
				MX_CA_CHAR, max_bytes_to_read, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out how many characters were read. */

	mx_status = mx_caget( &(epics_gpib->nord_pv),
				MX_CA_LONG, 1, &num_chars_read );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_read != NULL ) {
		*bytes_read = num_chars_read;
	}

	MX_DEBUG( 2,("*** %s: read '%s' from '%s:%ld' ***",
			fname, buffer, gpib->record->name, address));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_write( MX_GPIB *gpib,
		long address,
		char *buffer,
		size_t bytes_to_write,
		size_t *bytes_written,
		unsigned long flags )
{
	static const char fname[] = "mxi_epics_gpib_write()";

	MX_EPICS_GPIB *epics_gpib = NULL;
	int32_t timeout, int32_bytes_to_write;
	mx_status_type mx_status;

	mx_status = mxi_epics_gpib_get_pointers( gpib, &epics_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("*** %s: sending '%s' to '%s:%ld' ***",
			fname, buffer, gpib->record->name, address));

	if ( bytes_to_write > epics_gpib->max_output_length ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested number of characters (%ld) for GPIB port '%s' is longer "
	"than the maximum output buffer length of %ld.",
			(long) bytes_to_write, gpib->record->name,
			(long) epics_gpib->max_output_length );
	}

	/* Change the transaction mode if needed. */

	if ( epics_gpib->transaction_mode != MXF_EPICS_GPIB_WRITE_READ ) {

		mx_status = mxi_epics_gpib_set_transaction_mode( epics_gpib,
					gpib, MXF_EPICS_GPIB_WRITE_READ );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Change the EPICS timeout value if required. */

	if ( flags & MXF_GPIB_NOWAIT ) {
		timeout = 500L;
	} else {
		timeout = epics_gpib->default_timeout;
	}

	if ( epics_gpib->timeout != timeout ) {

		mx_status = mx_caput( &(epics_gpib->tmot_pv),
					MX_CA_LONG, 1, &timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		epics_gpib->timeout = timeout;
	}

	/* Change the GPIB address if required. */

	if ( epics_gpib->current_address != address ) {

		mx_status = mxi_epics_gpib_set_address( epics_gpib,
							gpib, address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		epics_gpib->current_address = address;
	}

	/* Set the number of characters to write if required. */

	if ( epics_gpib->num_chars_to_write != bytes_to_write ) {

		int32_bytes_to_write = bytes_to_write;

		mx_status = mx_caput( &(epics_gpib->nowt_pv),
					MX_CA_LONG, 1, &int32_bytes_to_write );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		epics_gpib->num_chars_to_write = (long) bytes_to_write;
	}

	/* Write characters from the buffer. */

	mx_status = mx_caput( &(epics_gpib->bout_pv),
				MX_CA_CHAR, bytes_to_write, buffer );

	/* There seems to be no way to find out how many characters were
	 * actually written, so we set *bytes_written to bytes_to_write.
	 */

	if ( bytes_written != NULL ) {
		*bytes_written = bytes_to_write;
	}

	MX_DEBUG( 2,("*** %s: write to '%s:%ld' complete ***",
				fname, gpib->record->name, address));

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_interface_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_epics_gpib_interface_clear()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Interface clear is not yet implemented for the EPICS GPIB record");
}

MX_EXPORT mx_status_type
mxi_epics_gpib_device_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_epics_gpib_device_clear()";

	MX_EPICS_GPIB *epics_gpib = NULL;
	mx_status_type mx_status;

	mx_status = mxi_epics_gpib_get_pointers( gpib, &epics_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epics_gpib_universal_command(epics_gpib, gpib,
					MXF_EPICS_GPIB_DEVICE_CLEAR);

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_selective_device_clear( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_epics_gpib_selective_device_clear()";

	MX_EPICS_GPIB *epics_gpib = NULL;
	mx_status_type mx_status;

	mx_status = mxi_epics_gpib_get_pointers( gpib, &epics_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epics_gpib_addressed_command(epics_gpib, gpib, address, 
					MXF_EPICS_GPIB_SELECTED_DEVICE_CLEAR);

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_local_lockout( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_epics_gpib_local_lockout()";

	MX_EPICS_GPIB *epics_gpib = NULL;
	mx_status_type mx_status;

	mx_status = mxi_epics_gpib_get_pointers( gpib, &epics_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epics_gpib_universal_command(epics_gpib, gpib,
					MXF_EPICS_GPIB_LOCAL_LOCKOUT);

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_remote_enable( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_epics_gpib_remote_enable()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Remote enable is not yet implemented for the EPICS GPIB record");
}

MX_EXPORT mx_status_type
mxi_epics_gpib_go_to_local( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_epics_gpib_go_to_local()";

	MX_EPICS_GPIB *epics_gpib = NULL;
	mx_status_type mx_status;

	mx_status = mxi_epics_gpib_get_pointers( gpib, &epics_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epics_gpib_addressed_command(epics_gpib, gpib, address, 
					MXF_EPICS_GPIB_GO_TO_LOCAL);

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_trigger( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_epics_gpib_trigger_device()";

	MX_EPICS_GPIB *epics_gpib = NULL;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	mx_status = mxi_epics_gpib_get_pointers( gpib, &epics_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epics_gpib_addressed_command(epics_gpib, gpib, address, 
					MXF_EPICS_GPIB_GROUP_EXECUTE_TRIGGER);

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_wait_for_service_request( MX_GPIB *gpib, double timeout )
{
	static const char fname[] = "mxi_epics_gpib_wait_for_service_request()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Wait for SRQ is not yet implemented." );
}

MX_EXPORT mx_status_type
mxi_epics_gpib_serial_poll( MX_GPIB *gpib, long address,
				unsigned char *serial_poll_byte)
{
	static const char fname[] = "mxi_epics_gpib_serial_poll()";

	MX_EPICS_GPIB *epics_gpib = NULL;
	char buffer[40];
	size_t bytes_read;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	mx_status = mxi_epics_gpib_get_pointers( gpib, &epics_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epics_gpib_universal_command(epics_gpib, gpib,
					MXF_EPICS_GPIB_SERIAL_POLL_ENABLE);

	/* Get the serial poll byte. */

	mx_status = mxi_epics_gpib_read( gpib, address,
					buffer, sizeof(buffer)-1,
					&bytes_read, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*serial_poll_byte = (unsigned char) buffer[0];

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_gpib_serial_poll_disable( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_epics_gpib_serial_poll_disable()";

	MX_EPICS_GPIB *epics_gpib = NULL;
	mx_status_type mx_status;

	mx_status = mxi_epics_gpib_get_pointers( gpib, &epics_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epics_gpib_universal_command(epics_gpib, gpib,
					MXF_EPICS_GPIB_SERIAL_POLL_DISABLE);

	return mx_status;
}

/* ==== private functions ==== */

static mx_status_type
mxi_epics_gpib_set_address( MX_EPICS_GPIB *epics_gpib,
				MX_GPIB *gpib, int address )
{
	int32_t int32_address;
	mx_status_type mx_status;

	int32_address = (int32_t) address;

	mx_status = mx_caput( &(epics_gpib->addr_pv),
				MX_CA_LONG, 1, &int32_address );

	return mx_status;
}

static mx_status_type
mxi_epics_gpib_set_transaction_mode( MX_EPICS_GPIB *epics_gpib,
					MX_GPIB *gpib,
					long mode )
{
	int32_t int32_mode;
	mx_status_type mx_status;

	int32_mode = mode;

	mx_status = mx_caput( &(epics_gpib->tmod_pv),
				MX_CA_LONG, 1, &int32_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_gpib->transaction_mode = mode;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_epics_gpib_universal_command( MX_EPICS_GPIB *epics_gpib,
					MX_GPIB *gpib,
					long command )
{
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_epics_gpib_addressed_command( MX_EPICS_GPIB *epics_gpib,
					MX_GPIB *gpib,
					int address,
					long command )
{
	mx_status_type mx_status;

	mx_status = mxi_epics_gpib_set_address( epics_gpib, gpib, address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

