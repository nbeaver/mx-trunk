/*
 * Name:    i_vms_terminal.c
 *
 * Purpose: MX RS-232 driver for OpenVMS terminal devices.
 *
 * Author:  William Lavender
 *
 * Copyright 2003, 2005, 2007, 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_VMS_TERMINAL_DEBUG	FALSE

#include <stdio.h>

#ifdef OS_VMS	/* Include this only on OpenVMS systems. */

#define USE_SINGLE_CHARACTER_IO  TRUE

/* My recollection of sys$qio() for terminal devices is somewhat hazy, so
 * we need to stick to single character I/O until I relearn how it works.
 */

#include <stdlib.h>
#include <errno.h>
#include <ssdef.h>
#include <iodef.h>
#include <descrip.h>
#include <starlet.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_vms_terminal.h"

MX_RECORD_FUNCTION_LIST mxi_vms_terminal_record_function_list = {
	NULL,
	mxi_vms_terminal_create_record_structures,
	mxi_vms_terminal_finish_record_initialization,
	NULL,
	NULL,
	mxi_vms_terminal_open,
	mxi_vms_terminal_close
};

MX_RS232_FUNCTION_LIST mxi_vms_terminal_rs232_function_list = {
	mxi_vms_terminal_getchar,
	mxi_vms_terminal_putchar,
#if USE_SINGLE_CHARACTER_IO
	NULL,
	NULL,
	NULL,
	NULL,
#else
	mxi_vms_terminal_read,
	mxi_vms_terminal_write,
	mxi_vms_terminal_getline,
	mxi_vms_terminal_putline,
#endif
	mxi_vms_terminal_num_input_bytes_available,
	mxi_vms_terminal_discard_unread_input,
	mxi_vms_terminal_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_vms_terminal_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_VMS_TERMINAL_STANDARD_FIELDS
};

long mxi_vms_terminal_num_record_fields
		= sizeof( mxi_vms_terminal_record_field_defaults )
			/ sizeof( mxi_vms_terminal_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_vms_terminal_rfield_def_ptr
			= &mxi_vms_terminal_record_field_defaults[0];

/* ---- */

/* Private functions for the use of the driver. */

static mx_status_type
mxi_vms_terminal_get_pointers( MX_RS232 *rs232,
			MX_VMS_TERMINAL **vms_terminal,
			const char *calling_fname )
{
	const char fname[] = "mxi_vms_terminal_get_pointers()";

	MX_RECORD *vms_terminal_record;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vms_terminal == (MX_VMS_TERMINAL **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VMS_TERMINAL pointer passed by '%s' was NULL.",
			calling_fname );
	}

	vms_terminal_record = rs232->record;

	if ( vms_terminal_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The vms_terminal_record pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*vms_terminal = (MX_VMS_TERMINAL *)
			vms_terminal_record->record_type_struct;

	if ( *vms_terminal == (MX_VMS_TERMINAL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VMS_TERMINAL pointer for record '%s' is NULL.",
			vms_terminal_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static char *
mxi_vms_error_message( int vms_status ) {
	static char buffer[250];

	snprintf( buffer, sizeof(buffer),
 		"VMS error number %d, error message = '%s'",
		vms_status, strerror( EVMSERR, vms_status ) );

	return &buffer[0];
}


/*==========================*/

MX_EXPORT mx_status_type
mxi_vms_terminal_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxi_vms_terminal_create_record_structures()";

	MX_RS232 *rs232;
	MX_VMS_TERMINAL *vms_terminal;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	vms_terminal = (MX_VMS_TERMINAL *) malloc( sizeof(MX_VMS_TERMINAL) );

	if ( vms_terminal == (MX_VMS_TERMINAL *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VMS_TERMINAL structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = vms_terminal;
	record->class_specific_function_list
				= &mxi_vms_terminal_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vms_terminal_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the VMS_TERMINAL device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vms_terminal_open( MX_RECORD *record )
{
	const char fname[] = "mxi_vms_terminal_open()";

	MX_RS232 *rs232;
	MX_VMS_TERMINAL *vms_terminal;
	int i, vms_status, mask_address, byte_number, bit_offset;
	mx_status_type mx_status;
	char device_name[ MXU_FILENAME_LENGTH + 1 ];

	$DESCRIPTOR( devname_desc, device_name );

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_vms_terminal_get_pointers(rs232, &vms_terminal, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( rs232->read_terminators > 0xff ) {
		mx_warning(
	"VMS terminal device '%s' has been configured to use "
	"a multibyte read terminator of %#x.  However, "
	"the VMS operating system only supports single byte read terminators.  "
	"Only the first byte, namely %#x will be used.",
			record->name, rs232->read_terminators,
			rs232->read_terminator_array[0] );
	}

	for ( i = 0; i < 32; i++ ) {
		vms_terminal->read_terminator_mask[0] = 0;
	}

	vms_terminal->read_terminator_quadword[0] = 32;
	vms_terminal->read_terminator_quadword[1] = 0;

	if ( rs232->read_terminators == 0 ) {
		vms_terminal->read_terminator_quadword[2] = 0;
		vms_terminal->read_terminator_quadword[3] = 0;
	} else {
		mask_address = (int) &(vms_terminal->read_terminator_mask[0]);

		vms_terminal->read_terminator_quadword[2] 
			= mask_address & 0xffff;

		vms_terminal->read_terminator_quadword[3]
			= ( mask_address >> 16 ) & 0xffff;

		byte_number = rs232->read_terminator_array[0] / 32;
		bit_offset = rs232->read_terminator_array[0] % 32;

		vms_terminal->read_terminator_mask[ byte_number ] = bit_offset;
	}

#if 0
	fprintf(stderr, "%s: read_terminator_quadword = ", fname);

	for ( i = 0; i < 4; i++ ) {
		fprintf( stderr, "%#x ",
			vms_terminal->read_terminator_quadword[i] );
	}

	fprintf(stderr, "\n%s: read_terminator_mask = ", fname);

	for ( i = 0; i < 32; i++ ) {
		fprintf( stderr, "%#x ",
			vms_terminal->read_terminator_mask[i] );
	}

	fprintf(stderr, "\n");
#endif

	strlcpy(device_name, vms_terminal->device_name, MXU_FILENAME_LENGTH);

	MX_DEBUG( 2,("%s: device_name = '%s'", fname, device_name));

	/* Create a VMS channel to be used to communicate with the device. */

	vms_status = sys$assign( &devname_desc,
				&(vms_terminal->vms_channel), 0, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot assign a channel for VMS terminal device '%s'.  %s",
			vms_terminal->device_name,
			mxi_vms_error_message( vms_status ) );
	}

	/* For now, we assume that the terminal parameters have been
	 * set up in advance with a SET TERMINAL... command.
	 */

	MX_DEBUG( 2,("%s complete.",fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vms_terminal_close( MX_RECORD *record )
{
	const char fname[] = "mxi_vms_terminal_close()";

	MX_RS232 *rs232;
	MX_VMS_TERMINAL *vms_terminal;
	int vms_status;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) (record->record_class_struct);

	mx_status = mxi_vms_terminal_get_pointers(rs232, &vms_terminal, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Deassign the VMS channel for the device. */

	vms_status = sys$dassgn( vms_terminal->vms_channel );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot deassign channel for VMS terminal device '%s'.  %s",
			vms_terminal->device_name,
			mxi_vms_error_message( vms_status ) );
	}

	MX_DEBUG( 2,("%s complete.",fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vms_terminal_getchar( MX_RS232 *rs232, char *c )
{
	const char fname[] = "mxi_vms_terminal_getchar()";

	MX_VMS_TERMINAL *vms_terminal;
	int vms_status;
	short iosb[4];
	mx_status_type mx_status;

	mx_status = mxi_vms_terminal_get_pointers(rs232, &vms_terminal, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {

		mx_status = mxi_vms_terminal_num_input_bytes_available( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( rs232->num_input_bytes_available == 0 ) {
			return mx_error( (MXE_NOT_READY | MXE_QUIET), fname,
				"Failed to read a character from port '%s'.",
				rs232->record->name );
		}
	}

	vms_status = sys$qiow( 0, vms_terminal->vms_channel,
				IO$_READVBLK | IO$M_NOECHO ,
				iosb, 0, 0, &c, 1, 0, 0, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot read a character from VMS terminal device '%s'.  %s",
			vms_terminal->device_name,
			mxi_vms_error_message( vms_status ) );
	}

#if MXI_VMS_TERMINAL_DEBUG
	MX_DEBUG(-2,("%s: received c = 0x%x, '%c'", fname, *c, *c));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vms_terminal_putchar( MX_RS232 *rs232, char c )
{
	const char fname[] = "mxi_vms_terminal_putchar()";

	MX_VMS_TERMINAL *vms_terminal;
	int input_available, vms_status;
	short iosb[4];
	mx_status_type mx_status;

#if MXI_VMS_TERMINAL_DEBUG
	MX_DEBUG(-2,("%s: sending c = 0x%x, '%c'", fname, c, c));
#endif

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		  "Non-blocking VMS terminal I/O not yet implemented.");
	}

	mx_status = mxi_vms_terminal_get_pointers(rs232, &vms_terminal, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vms_status = sys$qiow( 0, vms_terminal->vms_channel,
				IO$_WRITEVBLK | IO$M_NOFORMAT,
				iosb, 0, 0, &c, 1, 0, 0, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot write a character to VMS terminal device '%s'.  %s",
			vms_terminal->device_name,
			mxi_vms_error_message( vms_status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#if ( USE_SINGLE_CHARACTER_IO == FALSE )

MX_EXPORT mx_status_type
mxi_vms_terminal_read( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	const char fname[] = "mxi_vms_terminal_read()";

	MX_VMS_TERMINAL *vms_terminal;
	int vms_status;
	short iosb[4];
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	mx_status = mxi_vms_terminal_get_pointers(rs232, &vms_terminal, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vms_status = sys$qiow( 0, vms_terminal->vms_channel,
				IO$_READVBLK | IO$M_NOECHO ,
				iosb, 0, 0, buffer, max_bytes_to_read,
				0, 0, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot read a buffer from VMS terminal device '%s'.  %s",
			vms_terminal->device_name,
			mxi_vms_error_message( vms_status ) );
	}

	*bytes_read = max_bytes_to_read;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vms_terminal_write( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_write,
			size_t *bytes_written )
{
	const char fname[] = "mxi_vms_terminal_write()";

	MX_VMS_TERMINAL *vms_terminal;
	int vms_status;
	short iosb[4];
	mx_status_type mx_status;

	mx_status = mxi_vms_terminal_get_pointers(rs232, &vms_terminal, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vms_status = sys$qiow( 0, vms_terminal->vms_channel,
				IO$_WRITEVBLK | IO$M_NOFORMAT,
				iosb, 0, 0, buffer, max_bytes_to_write,
				0, 0, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot write a buffer to VMS terminal device '%s'.  %s",
			vms_terminal->device_name,
			mxi_vms_error_message( vms_status ) );
	}

	*bytes_written = max_bytes_to_write;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vms_terminal_getline( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	const char fname[] = "mxi_vms_terminal_getline()";

	MX_VMS_TERMINAL *vms_terminal;
	int vms_status;
	unsigned short iosb[4];
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	mx_status = mxi_vms_terminal_get_pointers(rs232, &vms_terminal, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( rs232->read_terminators == 0 ) {
		vms_status = sys$qiow( 0, vms_terminal->vms_channel,
		    IO$_READVBLK | IO$M_NOECHO | IO$M_NOFILTR | IO$M_TRMNOECHO,
				iosb, 0, 0, buffer, max_bytes_to_read,
				0, 0, 0, 0 );
	} else {
		vms_status = sys$qiow( 0, vms_terminal->vms_channel,
		    IO$_READVBLK | IO$M_NOECHO | IO$M_NOFILTR | IO$M_TRMNOECHO,
				iosb, 0, 0, buffer, max_bytes_to_read,
				0, &(vms_terminal->read_terminator_quadword),
				0, 0 );
	}

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot read a buffer from VMS terminal device '%s'.  %s",
			vms_terminal->device_name,
			mxi_vms_error_message( vms_status ) );
	}

	MX_DEBUG( 2,("%s: transfer count = %h", fname, iosb[1]));

	*bytes_read = iosb[1];

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vms_terminal_putline( MX_RS232 *rs232,
			char *buffer,
			size_t *bytes_written )
{
	const char fname[] = "mxi_vms_terminal_putline()";

	MX_VMS_TERMINAL *vms_terminal;
	int vms_status;
	short iosb[4];
	long max_bytes_to_write;
	mx_status_type mx_status;

	mx_status = mxi_vms_terminal_get_pointers(rs232, &vms_terminal, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	max_bytes_to_write = strlen( buffer );

	MX_DEBUG( 2,("%s: buffer = '%s', max_bytes_to_write = %ld",
		fname, buffer, max_bytes_to_write));

	vms_status = sys$qiow( 0, vms_terminal->vms_channel,
				IO$_WRITEVBLK | IO$M_NOFORMAT,
				iosb, 0, 0, buffer, max_bytes_to_write,
				0, 0, 0, 0 );

	MX_DEBUG( 2,("%s: vms_status = %d", fname, vms_status));

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot write a buffer to VMS terminal device '%s'.  %s",
			vms_terminal->device_name,
			mxi_vms_error_message( vms_status ) );
	}

	MX_DEBUG( 2,("%s: iosb = %p", fname, iosb));

#if 0
	{
		int i;
		for ( i = 0; i < 4; i++ ) {
			MX_DEBUG(-2,("%s: iosb[%d] = %h", fname, i, iosb[i]));
		}
	}
#endif

	*bytes_written = iosb[1];

	return MX_SUCCESSFUL_RESULT;
}

#endif /* USE_SINGLE_CHARACTER_IO */

MX_EXPORT mx_status_type
mxi_vms_terminal_num_input_bytes_available( MX_RS232 *rs232 )
{
	const char fname[] = "mxi_vms_terminal_num_input_bytes_available()";

	struct {
		unsigned short typeahead_count;
		char first_character;
		char reserved[5];
	} typeahead_struct;

	MX_VMS_TERMINAL *vms_terminal;
	short iosb[4];
	int vms_status;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.",fname));

	mx_status = mxi_vms_terminal_get_pointers(rs232, &vms_terminal, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vms_status = sys$qiow( 0, vms_terminal->vms_channel,
				IO$_SENSEMODE | IO$M_TYPEAHDCNT,
				iosb, 0, 0, &typeahead_struct,
				1, 0, 0, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Cannot determine whether or not input is available "
		"for VMS terminal device '%s'.  %s",
			vms_terminal->device_name,
			mxi_vms_error_message( vms_status ) );
	}

	rs232->num_input_bytes_available = typeahead_struct.typeahead_count;

	MX_DEBUG(-2,("%s: num_input_bytes_available = %d",
		fname, rs232->num_input_bytes_available));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vms_terminal_discard_unread_input( MX_RS232 *rs232 )
{
	const char fname[] = "mxi_vms_terminal_discard_unread_input()";

	MX_VMS_TERMINAL *vms_terminal;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.",fname));

	mx_status = mxi_vms_terminal_get_pointers(rs232, &vms_terminal, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s complete.",fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vms_terminal_discard_unwritten_output( MX_RS232 *rs232 )
{
	const char fname[] = "mxi_vms_terminal_discard_unwritten_output()";

	MX_VMS_TERMINAL *vms_terminal;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.",fname));

	mx_status = mxi_vms_terminal_get_pointers(rs232, &vms_terminal, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s complete.",fname));

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_VMS */

