/*
 * Name:    i_fossil.c
 *
 * Purpose: MX driver for MSDOS COM RS-232 devices.
 *
 *          This driver uses the FOSSIL interface.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2005-2006 Illinois Institute of Technology
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
#include <dos.h>

#include "mx_util.h"
#include "mx_rs232.h"
#include "mx_record.h"
#include "i_fossil.h"

/* Do we show chars using MX_DEBUG() in the getchar and putchar functions? */

#define MXI_FOSSIL_DEBUG	FALSE

/* Define private functions to handle MSDOS COM ports. */

static mx_status_type msdos_fossil_write_parms( MX_RS232 *rs232 );

MX_RECORD_FUNCTION_LIST mxi_fossil_record_function_list = {
	NULL,
	mxi_fossil_create_record_structures,
	mxi_fossil_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_fossil_open,
	mxi_fossil_close
};

MX_RS232_FUNCTION_LIST mxi_fossil_rs232_function_list = {
	mxi_fossil_getchar,
	mxi_fossil_putchar,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_fossil_num_input_bytes_available,
	mxi_fossil_discard_unread_input,
	mxi_fossil_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_fossil_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_FOSSIL_STANDARD_FIELDS
};

long mxi_fossil_num_record_fields
		= sizeof( mxi_fossil_record_field_defaults )
			/ sizeof( mxi_fossil_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_fossil_rfield_def_ptr
			= &mxi_fossil_record_field_defaults[0];

/* ---- */

MX_EXPORT mx_status_type
mxi_fossil_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_fossil_create_record_structures()";

	MX_RS232 *rs232;
	MX_FOSSIL *fossil;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	fossil = (MX_FOSSIL *) malloc( sizeof(MX_FOSSIL) );

	if ( fossil == (MX_FOSSIL *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_FOSSIL structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = fossil;
	record->class_specific_function_list
				= &mxi_fossil_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_fossil_finish_record_initialization( MX_RECORD *record )
{
	MX_FOSSIL *fossil;
	mx_status_type status;

	fossil = (MX_FOSSIL *) record->record_type_struct;

	/* Check to see if the RS-232 parameters are valid. */

	status = mx_rs232_check_port_parameters( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Mark the FOSSIL device as being closed. */

	fossil->port_number = -1;

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_fossil_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_fossil_open()";

	MX_RS232 *rs232;
	MX_FOSSIL *fossil;
	union REGS regs;
	char *record_name;
	char *fossil_name;
	int port_number;
	unsigned short fossil_signature;
	mx_status_type status;

	MX_DEBUG(2, ("mxi_fossil_open() invoked."));

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

	fossil = (MX_FOSSIL *) (record->record_type_struct);

	if ( fossil == (MX_FOSSIL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_FOSSIL structure pointer for '%s' is NULL.", record_name );
	}

	/* Parse the serial port name. */

	fossil_name = fossil->filename;

	if ( strnicmp( fossil_name, "com", 3 ) != 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The serial device name '%s' is illegal.", fossil_name );
	}

	port_number = atoi( &fossil_name[3] ) - 1;

	if ( port_number < 0 || port_number > 7 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Only FOSSIL ports COM1 through COM8 are currently supported.");
	}

	fossil->port_number = port_number;

	/* Initialize the FOSSIL driver on this port. */

#ifdef OS_DOS_EXT_WATCOM
	regs.w.dx = port_number;
	regs.w.bx = 0;
	regs.h.ah = 0x04;
	int386( 0x14, &regs, &regs );
	fossil_signature = regs.w.ax;
#else
	regs.x.dx = port_number;
	regs.x.bx = 0;
	regs.h.ah = 0x04;
	int86( 0x14, &regs, &regs );
	fossil_signature = regs.x.ax;
#endif

	if ( fossil_signature != 0x1954 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"A FOSSIL driver is not installed for port '%s'.",
			fossil_name );
	}

	status = msdos_fossil_write_parms( rs232 );

	return status;
}

MX_EXPORT mx_status_type
mxi_fossil_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_fossil_close()";

	MX_RS232 *rs232;
	MX_FOSSIL *fossil;
	union REGS regs;
	char *name;

	MX_DEBUG(2, ("mxi_fossil_close() invoked."));

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

	fossil = (MX_FOSSIL *) (record->record_type_struct);

	if ( fossil == (MX_FOSSIL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_FOSSIL structure pointer is NULL." );
	}

	/* Deinitialize the FOSSIL driver on this port. */

#ifdef OS_DOS_EXT_WATCOM
	regs.w.dx = fossil->port_number;
	regs.h.ah = 0x05;
	int386( 0x14, &regs, &regs );
#else
	regs.x.dx = fossil->port_number;
	regs.h.ah = 0x05;
	int86( 0x14, &regs, &regs );
#endif

	/* Mark the port as closed. */

	fossil->port_number = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_fossil_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_fossil_getchar()";

	MX_FOSSIL *fossil;
	union REGS regs;
	char error_message[120];
	unsigned int fossil_status, dummy;
	unsigned short ax_register;
	long error_code, retries;

#if MXI_FOSSIL_DEBUG
	MX_DEBUG(-2, ("mxi_fossil_getchar() invoked."));
#endif

	fossil = (MX_FOSSIL*) (rs232->record->record_type_struct);

	dummy = 0;

	retries = 0;

	while ( 1 ) {
		/* If requested, check to see if there is a character ready
		 * before trying to read one.
		 */

		if ( rs232->transfer_flags & MXF_232_NOWAIT ) {
#ifdef OS_DOS_EXT_WATCOM
			regs.w.dx = fossil->port_number;
			regs.h.ah = 0x0c;
			int386( 0x14, &regs, &regs );
			ax_register = regs.w.ax;
#else
			regs.x.dx = fossil->port_number;
			regs.h.ah = 0x0c;
			int86( 0x14, &regs, &regs );
			ax_register = regs.x.ax;
#endif

			if ( ax_register == 0xffff ) {
				return mx_error_quiet( MXE_NOT_READY, fname,
				"No characters in input buffer for port '%s'",
				rs232->record->name );
			}
		}

		fossil_status = _bios_serialcom( _COM_RECEIVE,
						fossil->port_number, dummy );

#if MXI_FOSSIL_DEBUG
		MX_DEBUG(-2,
			("mxi_fossil_getchar(): fossil_status = 0x%x", fossil_status));
#endif

		/* We currently don't know what to do with BREAK conditions. */

		if ( fossil_status & 0x1000 ) {
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Handling a BREAK condition is not currently implemented.");
		}

		if ( fossil_status & MXF_FOSSIL_RECEIVE_ERROR ) {

			/* If character hasn't arrived yet and we
			 * are in MXF_232_WAIT mode, then try again
			 * if we haven't exceeded the maximum retry
			 * count, fossil->max_read_retries.
			 *
			 * If fossil->max_read_retries < 0, then we
			 * retry forever.
			 */

			if (fossil_status & MXF_FOSSIL_TIMED_OUT_ERROR) {


			    /* mxi_fossil_getchar() is often used to test
			     * whether there is input ready on the FOSSIL
			     * port.  Normally, it is not desirable to 
			     * broadcast a message to the world when this 
			     * fails, so we use mx_error_quiet() rather 
			     * than mx_error().
			     */

			    if ( rs232->transfer_flags & MXF_232_NOWAIT ) {

				return mx_error_quiet( MXE_NOT_READY, fname,
				"No characters in input buffer for port '%s'",
				rs232->record->name );

			    } else {

				if ( fossil->max_read_retries >= 0 ) {
				    if (retries < fossil->max_read_retries) {

						/* Do nothing. */
				    } else {
					return mx_error_quiet(
						MXE_NOT_READY, fname,
	"Read attempt from port '%s' exceeded maximum retry count = %ld",
						rs232->record->name,
						fossil->max_read_retries );
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
				rs232->record->name, fossil_status,
				fossil_status & MXF_FOSSIL_DATA );

			    error_code = MXE_INTERFACE_IO_ERROR;

			    if ( fossil_status & 0x0800 ) {
				strcat(error_message,"FRAMING_ERROR ");
				error_code = MXE_INTERFACE_IO_ERROR;
			    }
			    if ( fossil_status & 0x0400 ) {
				strcat(error_message,"PARITY_ERROR ");
				error_code = MXE_INTERFACE_IO_ERROR;
			    }
			    if ( fossil_status & 0x0200 ) {
				strcat(error_message,"BUFFER_OVERRUN_ERROR ");
				error_code = MXE_LIMIT_WAS_EXCEEDED;
			    }

			    return mx_error(error_code, fname, error_message);
			}
		} else {	/* no receive error. */

			*c = (char) ( fossil_status & MXF_FOSSIL_DATA );

#if MXI_FOSSIL_DEBUG
			MX_DEBUG(-2,
			("mxi_fossil_getchar(): c = 0x%x, '%c'", *c, *c));
#endif

			return MX_SUCCESSFUL_RESULT;
		}
	}
}

MX_EXPORT mx_status_type
mxi_fossil_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_fossil_putchar()";

	MX_FOSSIL *fossil;
	unsigned fossil_status, data;

#if MXI_FOSSIL_DEBUG
	MX_DEBUG(-2, ("mxi_fossil_putchar() invoked.  c = 0x%x, '%c'", c, c));
#endif

	fossil = (MX_FOSSIL*) (rs232->record->record_type_struct);

	data = ( unsigned ) c;

	fossil_status = _bios_serialcom( _COM_SEND, fossil->port_number, data);

#if MXI_FOSSIL_DEBUG
	MX_DEBUG(-2, ("mxi_fossil_putchar(): fossil_status = 0x%x",
		fossil_status));
#endif

	/* Currently, we don't test for MXF_232_WAIT since we haven't
	 * decided yet what to do with it.
	 */

	if ( fossil_status & MXF_FOSSIL_SEND_ERROR ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Attempt to send a character to port '%s' failed.  character = '%c'",
			rs232->record->name, c );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxi_fossil_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_fossil_num_input_bytes_available()";

	MX_FOSSIL *fossil;
	union REGS regs;
	unsigned short ax_register;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 structure pointer is NULL." );
	}

	fossil = (MX_FOSSIL *) (rs232->record->record_type_struct);

	if ( fossil == (MX_FOSSIL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_FOSSIL structure pointer is NULL.");
	}

#ifdef OS_DOS_EXT_WATCOM
	regs.w.dx = fossil->port_number;
	regs.h.ah = 0x0c;
	int386( 0x14, &regs, &regs );
	ax_register = regs.w.ax;
#else
	regs.x.dx = fossil->port_number;
	regs.h.ah = 0x0c;
	int86( 0x14, &regs, &regs );
	ax_register = regs.x.ax;
#endif

	if ( ax_register == 0xffff ) {
		rs232->num_input_bytes_available = 0;
	} else {
		rs232->num_input_bytes_available = 1;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_fossil_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_fossil_discard_unread_input()";

	MX_FOSSIL *fossil;
	union REGS regs;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 structure pointer is NULL." );
	}

	fossil = (MX_FOSSIL *) (rs232->record->record_type_struct);

	if ( fossil == (MX_FOSSIL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_FOSSIL structure pointer is NULL.");
	}

#ifdef OS_DOS_EXT_WATCOM
	regs.w.dx = fossil->port_number;
	regs.h.ah = 0x0a;
	int386( 0x14, &regs, &regs );
#else
	regs.x.dx = fossil->port_number;
	regs.h.ah = 0x0a;
	int86( 0x14, &regs, &regs );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_fossil_discard_unwritten_output( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_fossil_discard_unwritten_output()";

	MX_FOSSIL *fossil;
	union REGS regs;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 structure pointer is NULL." );
	}

	fossil = (MX_FOSSIL *) (rs232->record->record_type_struct);

	if ( fossil == (MX_FOSSIL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_FOSSIL structure pointer is NULL.");
	}

#ifdef OS_DOS_EXT_WATCOM
	regs.w.dx = fossil->port_number;
	regs.h.ah = 0x09;
	int386( 0x14, &regs, &regs );
#else
	regs.x.dx = fossil->port_number;
	regs.h.ah = 0x09;
	int86( 0x14, &regs, &regs );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/* ==== MSDOS I/O functions ==== */

static mx_status_type
msdos_fossil_write_parms( MX_RS232 *rs232 )
{
	static const char fname[] = "msdos_fossil_write_parms()";

	MX_RECORD *record;
	MX_FOSSIL *fossil;
	unsigned port_number, fossil_params, fossil_status;
	char *record_name;

	MX_DEBUG(2, ("msdos_fossil_write_parms() invoked."));

	/* Get the name of the port. */

	record = rs232->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD ptr for MX_RS232 ptr = 0x%p is NULL.", rs232);
	}

	record_name = &(record->name[0]);

	fossil = (MX_FOSSIL *) (rs232->record->record_type_struct);

	if ( fossil == (MX_FOSSIL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_FOSSIL structure for FOSSIL port '%s' is NULL.", record_name);
	}

	if ( fossil->port_number < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"FOSSIL port device '%s' is not open.  FOSSIL fd = %d",
		record_name, fossil->port_number);
	}

	port_number = (unsigned) (fossil->port_number);

	/* Set the port speed. */

	switch( rs232->speed ) {
	case 9600:  fossil_params = _COM_9600; break;
	case 4800:  fossil_params = _COM_4800; break;
	case 2400:  fossil_params = _COM_2400; break;
	case 1200:  fossil_params = _COM_1200; break;
	case 600:   fossil_params = _COM_600;  break;
	case 300:   fossil_params = _COM_300;  break;
	case 150:   fossil_params = _COM_150;  break;
	case 110:   fossil_params = _COM_110;  break;

	case 38400:
	case 19200:
	case 1800:
	case 200:
	case 75:
	case 50:
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported RS-232 speed = %ld for FOSSIL port '%s'.", 
		rs232->speed, record_name);

		break;
	}

	/* Set the word size. */

	switch( rs232->word_size ) {
	case 8:  fossil_params |= _COM_CHR8; break;
	case 7:  fossil_params |= _COM_CHR7; break;

	case 6:
	case 5:
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported RS-232 word size = %ld for FOSSIL port '%s'.", 
		rs232->word_size, record_name);

		break;
	}

	/* Set the parity. */

	switch( rs232->parity ) {
	case MXF_232_NO_PARITY:    fossil_params |= _COM_NOPARITY;   break;
	case MXF_232_EVEN_PARITY:  fossil_params |= _COM_EVENPARITY; break;
	case MXF_232_ODD_PARITY:   fossil_params |= _COM_ODDPARITY;  break;

	case MXF_232_MARK_PARITY:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"FOSSIL = '%s'.  Mark parity is not supported for MSDOS FOSSIL interfaces.",
			record_name);
		break;

	case MXF_232_SPACE_PARITY:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"FOSSIL = '%s'.  Space parity is not supported for MSDOS FOSSIL interfaces.",
			record_name);
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported RS-232 parity = '%c' for FOSSIL port '%s'.", 
		rs232->parity, record_name);

		break;
	}

	/* Set the stop bits. */

	switch( rs232->stop_bits ) {
	case 1:  fossil_params |= _COM_STOP1; break;
	case 2:  fossil_params |= _COM_STOP2; break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported RS-232 stop bits = %ld for FOSSIL port '%s'.", 
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
		"Unsupported RS-232 flow control = %d for FOSSIL port '%s'.", 
		rs232->flow_control, record_name);

		break;
	}

	/* Everything is set, so change the configuration of the FOSSIL port.*/

	MX_DEBUG(2, ("port_number = %d, fossil_params = 0x%x",
		port_number, fossil_params) );

	fossil_status = _bios_serialcom( _COM_INIT, port_number, fossil_params );

	MX_DEBUG(2, ("fossil_status = 0x%x", fossil_status));

	if ( fossil_status & MXF_FOSSIL_STATUS_ERROR ) {

		MX_DEBUG(2, ("Trying to open FOSSIL port a second time."));

		/* Sometimes trying one more time helps. */

		fossil_status = _bios_serialcom( _COM_INIT,
					port_number, fossil_params );

		if ( fossil_status & MXF_FOSSIL_STATUS_ERROR ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unable to initialize FOSSIL port '%s'.",
				record_name);
		}
	}
		
	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_MSDOS */
