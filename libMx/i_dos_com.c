/*
 * Name:    i_dos_com.c
 *
 * Purpose: MX driver for MSDOS COM RS-232 devices.
 *
 *          This driver uses the INT 14 BIOS interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"

#ifdef OS_MSDOS		/* Don't include this on non-MSDOS-like systems. */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <bios.h>

#include "mx_util.h"
#include "mx_rs232.h"
#include "mx_record.h"
#include "i_dos_com.h"

/* Do we show chars using MX_DEBUG() in the getchar and putchar functions? */

#define MXI_COM_DEBUG	FALSE

/* Define private functions to handle MSDOS COM ports. */

#if 0
static mx_status_type msdos_com_read_parms( MX_RS232 *rs232 );
#endif

static mx_status_type msdos_com_write_parms( MX_RS232 *rs232 );

MX_RECORD_FUNCTION_LIST mxi_com_record_function_list = {
	NULL,
	mxi_com_create_record_structures,
	mxi_com_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_com_open,
	mxi_com_close
};

MX_RS232_FUNCTION_LIST mxi_com_rs232_function_list = {
	mxi_com_getchar,
	mxi_com_putchar,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_com_num_input_bytes_available,
	mxi_com_discard_unread_input,
	mxi_com_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_com_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_COM_STANDARD_FIELDS
};

long mxi_com_num_record_fields
		= sizeof( mxi_com_record_field_defaults )
			/ sizeof( mxi_com_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_com_rfield_def_ptr
			= &mxi_com_record_field_defaults[0];

/* ---- */

MX_EXPORT mx_status_type
mxi_com_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_com_create_record_structures()";

	MX_RS232 *rs232;
	MX_COM *com;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	com = (MX_COM *) malloc( sizeof(MX_COM) );

	if ( com == (MX_COM *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_COM structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = com;
	record->class_specific_function_list
				= &mxi_com_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_com_finish_record_initialization( MX_RECORD *record )
{
	MX_COM *com;
	mx_status_type status;

	/* Check to see if the RS-232 parameters are valid. */

	status = mx_rs232_check_port_parameters( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Mark the COM device as being closed. */

	com = (MX_COM *) record->record_type_struct;

	com->port_number = -1;

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_com_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_com_open()";

	MX_RS232 *rs232;
	MX_COM *com;
	char *record_name;
	char *com_name;
	int port_number;
	mx_status_type status;

	MX_DEBUG(-2, ("mxi_com_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	record_name = &(record->name[0]);    /* Get the name of the port. */

	rs232 = (MX_RS232 *) (record->record_class_struct);

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 structure pointer is NULL." );
	}

	com = (MX_COM *) (record->record_type_struct);

	if ( com == (MX_COM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_COM structure pointer for '%s' is NULL.", record_name );
	}

	/* Since we will use the BIOS to talk to the serial port,
	 * all we have to do here is verify that the COM port name
	 * is in the range COM1..4.
	 */

	com_name = com->filename;

	if ( stricmp( com_name, "com1" ) == 0 ) {
		port_number = 0;
	} else
	if ( stricmp( com_name, "com2" ) == 0 ) {
		port_number = 1;
	} else
	if ( stricmp( com_name, "com3" ) == 0 ) {
		port_number = 2;
	} else
	if ( stricmp( com_name, "com4" ) == 0 ) {
		port_number = 3;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
	"Only COM ports COM1, COM2, COM3, and COM4 are currently supported.");
	}

	com->port_number = port_number;

	MX_DEBUG( 2,
("mxi_com_open(): We need to check whether the COM port is present or not."));

	status = msdos_com_write_parms( rs232 );

	return status;
}

MX_EXPORT mx_status_type
mxi_com_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_com_close()";

	MX_RS232 *rs232;
	MX_COM *com;
	char *name;

	MX_DEBUG(-2, ("mxi_com_close() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed is NULL." );
	}

	name = &(record->name[0]);	 /* Get the name of the port. */

	rs232 = (MX_RS232 *) (record->record_class_struct);

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 structure pointer is NULL." );
	}

	com = (MX_COM *) (record->record_type_struct);

	if ( com == (MX_COM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_COM structure pointer is NULL." );
	}

	/* Mark the port as closed. */

	com->port_number = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_com_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_com_getchar()";

	MX_COM *com;
	char error_message[120];
	unsigned com_status, dummy;
	long error_code, retries;

#if MXI_COM_DEBUG
	MX_DEBUG(-2, ("mxi_com_getchar() invoked."));
#endif

	com = (MX_COM*) (rs232->record->record_type_struct);

	dummy = 0;

	retries = 0;

	while ( 1 ) {
		com_status = _bios_serialcom( _COM_RECEIVE,
						com->port_number, dummy );

#if MXI_COM_DEBUG
		MX_DEBUG(-2,
			("mxi_com_getchar(): com_status = 0x%x", com_status));
#endif

		/* We currently don't know what to do with BREAK conditions. */

		if ( com_status & 0x1000 ) {
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Handling a BREAK condition is not currently implemented.");
		}

		if ( com_status & MXF_COM_RECEIVE_ERROR ) {

			/* If character hasn't arrived yet and we
			 * are in MXF_232_WAIT mode, then try again
			 * if we haven't exceeded the maximum retry
			 * count, com->max_read_retries.
			 *
			 * If com->max_read_retries < 0, then we
			 * retry forever.
			 */

			if (com_status & MXF_COM_TIMED_OUT_ERROR) {


			    /* mxi_com_getchar() is often used to test whether
			     * there is input ready on the COM port.  Normally,
			     * it is not desirable to broadcast a message to 
			     * the world when this fails, so we use 
			     * mx_error_quiet() rather than mx_error().
			     */

			    if ( rs232->transfer_flags & MXF_232_NOWAIT ) {

				return mx_error_quiet( MXE_NOT_READY, fname,
				  "Failed to read a character from port '%s'",
				  rs232->record->name );

			    } else {

				if ( com->max_read_retries >= 0 ) {
				    if (retries < com->max_read_retries) {

						/* Do nothing. */
				    } else {
					return mx_error_quiet(
						MXE_NOT_READY, fname,
	"Read attempt from port '%s' exceeded maximum retry count = %ld",
						rs232->record->name,
						com->max_read_retries );
				    }
				}

				retries++;

#if 0
				MX_DEBUG(-2, ("Retry %ld", retries));
#endif
			    }
					
			} else {

			    sprintf( error_message,
				"Port '%s' receive error 0x%x on char '%c': ",
				rs232->record->name, com_status,
				com_status & MXF_COM_DATA );

			    error_code = MXE_INTERFACE_IO_ERROR;

			    if ( com_status & 0x0800 ) {
				strcat(error_message,"FRAMING_ERROR ");
				error_code = MXE_INTERFACE_IO_ERROR;
			    }
			    if ( com_status & 0x0400 ) {
				strcat(error_message,"PARITY_ERROR ");
				error_code = MXE_INTERFACE_IO_ERROR;
			    }
			    if ( com_status & 0x0200 ) {
				strcat(error_message,"BUFFER_OVERRUN_ERROR ");
				error_code = MXE_LIMIT_WAS_EXCEEDED;
			    }

			    return mx_error(error_code, fname, error_message);
			}
		} else {	/* no receive error. */

			*c = (char) ( com_status & MXF_COM_DATA );

#if MXI_COM_DEBUG
			MX_DEBUG(-2,
			("mxi_com_getchar(): c = 0x%x, '%c'", *c, *c));
#endif

			return MX_SUCCESSFUL_RESULT;
		}
	}
}

MX_EXPORT mx_status_type
mxi_com_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_com_putchar()";

	MX_COM *com;
	unsigned com_status, data;

#if MXI_COM_DEBUG
	MX_DEBUG(-2, ("mxi_com_putchar() invoked.  c = 0x%x, '%c'", c, c));
#endif

	com = (MX_COM*) (rs232->record->record_type_struct);

	data = ( unsigned ) c;

	com_status = _bios_serialcom( _COM_SEND, com->port_number, data );

#if MXI_COM_DEBUG
	MX_DEBUG(-2, ("mxi_com_putchar(): com_status = 0x%x", com_status));
#endif

	/* Currently, we don't test for MXF_232_WAIT since we haven't
	 * decided yet what to do with it.
	 */

	if ( com_status & MXF_COM_SEND_ERROR ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Attempt to send a character to port '%s' failed.  character = '%c'",
			rs232->record->name, c );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxi_com_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_com_num_input_bytes_available()";

	MX_COM *com;
	unsigned com_status, dummy;

	dummy = 0;
	
	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 structure pointer is NULL." );
	}

	com = (MX_COM *) (rs232->record->record_type_struct);

	if ( com == (MX_COM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_COM structure pointer is NULL.");
	}

	com_status = _bios_serialcom( _COM_SEND, com->port_number,dummy);

	if ( com_status & 0x01 ) {
		rs232->num_input_bytes_available = 1;
	} else {
		rs232->num_input_bytes_available = 0;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_com_discard_unread_input( MX_RS232 *rs232 )
{
	unsigned long saved_transfer_flags;
	char c;
	mx_status_type status;

	saved_transfer_flags = rs232->transfer_flags;

	rs232->transfer_flags |= MXF_232_NOWAIT;

	/* The first attempt to read a character may come back with
	 * an MXE_INTERFACE_IO_ERROR due to a buffer overrun that
	 * may have occurred before we invoked this function.  Thus,
	 * getting an MXE_INTERFACE_IO_ERROR on the first character
	 * read is not regarded as a reason to abort.
	 */

	status = mxi_com_getchar( rs232, &c );

	if ( (status.code != MXE_SUCCESS)
	  && (status.code != MXE_INTERFACE_IO_ERROR) ) {

		rs232->transfer_flags = saved_transfer_flags;

		if ( status.code == MXE_NOT_READY )
			status = MX_SUCCESSFUL_RESULT;

		return status;
	}

	/* Now read in any more characters that may be there until
	 * we get an MXE_NOT_READY status code.  If we get any other
	 * status code besides MXE_SUCCESS, immediately abort and
	 * return the corresponding status structure.
	 */

	status = MX_SUCCESSFUL_RESULT;

	while ( status.code == MXE_SUCCESS ) {

		status = mxi_com_getchar( rs232, &c );
	}

	rs232->transfer_flags = saved_transfer_flags;

	if ( status.code == MXE_NOT_READY )
		status = MX_SUCCESSFUL_RESULT;

	return status;
}

MX_EXPORT mx_status_type
mxi_com_discard_unwritten_output( MX_RS232 *rs232 )
{
	/* Not possible for a DOS COM port. */

	return MX_SUCCESSFUL_RESULT;
}

/* ==== MSDOS I/O functions ==== */

static mx_status_type
msdos_com_write_parms( MX_RS232 *rs232 )
{
	static const char fname[] = "msdos_com_write_parms()";

	MX_RECORD *record;
	MX_COM *com;
	unsigned port_number, com_params, com_status;
	char *record_name;

	MX_DEBUG(-2, ("msdos_com_write_parms() invoked."));

	/* Get the name of the port. */

	record = rs232->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD ptr for MX_RS232 ptr = 0x%p is NULL.", rs232);
	}

	record_name = &(record->name[0]);

	com = (MX_COM *) (record->record_type_struct);

	if ( com == (MX_COM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_COM structure for COM port '%s' is NULL.", record_name);
	}

	if ( com->port_number < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"COM port device '%s' is not open.  COM fd = %d",
		record_name, com->port_number);
	}

	port_number = (unsigned) (com->port_number);

	/* Set the port speed. */

	switch( rs232->speed ) {
	case 9600:  com_params = _COM_9600; break;
	case 4800:  com_params = _COM_4800; break;
	case 2400:  com_params = _COM_2400; break;
	case 1200:  com_params = _COM_1200; break;
	case 600:   com_params = _COM_600;  break;
	case 300:   com_params = _COM_300;  break;
	case 150:   com_params = _COM_150;  break;
	case 110:   com_params = _COM_110;  break;

	case 38400:
	case 19200:
	case 1800:
	case 200:
	case 75:
	case 50:
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported COM speed = %ld for COM '%s'.", 
		rs232->speed, record_name);

		break;
	}

	/* Set the word size. */

	switch( rs232->word_size ) {
	case 8:  com_params |= _COM_CHR8; break;
	case 7:  com_params |= _COM_CHR7; break;

	case 6:
	case 5:
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported COM word size = %d for COM '%s'.", 
		rs232->word_size, record_name);

		break;
	}

	/* Set the parity. */

	switch( rs232->parity ) {
	case MXF_232_NO_PARITY:    com_params |= _COM_NOPARITY;   break;
	case MXF_232_EVEN_PARITY:  com_params |= _COM_EVENPARITY; break;
	case MXF_232_ODD_PARITY:   com_params |= _COM_ODDPARITY;  break;

	case MXF_232_MARK_PARITY:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"COM = '%s'.  Mark parity is not supported for MSDOS COM interfaces.",
			record_name);
		break;

	case MXF_232_SPACE_PARITY:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"COM = '%s'.  Space parity is not supported for MSDOS COM interfaces.",
			record_name);
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported COM parity = '%c' for COM '%s'.", 
		rs232->parity, record_name);

		break;
	}

	/* Set the stop bits. */

	switch( rs232->stop_bits ) {
	case 1:  com_params |= _COM_STOP1; break;
	case 2:  com_params |= _COM_STOP2; break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported COM stop bits = %d for COM '%s'.", 
		rs232->stop_bits, record_name);

		break;
	}

	/* Check the flow control parameters but otherwise ignore them. */

	switch( rs232->flow_control ) {
	case MXF_232_NO_FLOW_CONTROL:
	case MXF_232_HARDWARE_FLOW_CONTROL:
	case MXF_232_SOFTWARE_FLOW_CONTROL:
	case MXF_232_BOTH_FLOW_CONTROL:
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported COM flow control = %d for COM '%s'.", 
		rs232->flow_control, record_name);

		break;
	}

	/* Everything is set, so change the configuration of the COM port. */

	MX_DEBUG(-2, ("port_number = %d, com_params = 0x%x",
		port_number, com_params) );

	com_status = _bios_serialcom( _COM_INIT, port_number, com_params );

	MX_DEBUG(-2, ("com_status = 0x%x", com_status));

	if ( com_status & MXF_COM_STATUS_ERROR ) {

		MX_DEBUG(-2, ("Trying to open COM port a second time."));

		/* Sometimes trying one more time helps. */

		com_status = _bios_serialcom( _COM_INIT,
					port_number, com_params );

		if ( com_status & MXF_COM_STATUS_ERROR ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unable to initialize COM port '%s'.",
				record_name);
		}
	}
		
	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_MSDOS */
