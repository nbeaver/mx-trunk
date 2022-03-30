/*
 * Name:    i_ks3344.c
 *
 * Purpose: MX driver for CAMAC-based Kinetic Systems
 *          Model 3344 RS-232 devices.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2005-2007, 2010-2011, 2015, 2022
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_KS3344_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_rs232.h"
#include "mx_camac.h"
#include "mx_record.h"
#include "i_ks3344.h"

/* Define private functions to handle CAMAC KS3344 ports. */

static mx_status_type mxi_ks3344_reset_channel( MX_RS232 *rs232 );
static mx_status_type mxi_ks3344_write_parms( MX_RS232 *rs232 );


MX_RECORD_FUNCTION_LIST mxi_ks3344_record_function_list = {
	NULL,
	mxi_ks3344_create_record_structures,
	mxi_ks3344_finish_record_initialization,
	NULL,
	NULL,
	mxi_ks3344_open,
	mxi_ks3344_close
};

MX_RS232_FUNCTION_LIST mxi_ks3344_rs232_function_list = {
	mxi_ks3344_getchar,
	mxi_ks3344_putchar,
	mxi_ks3344_read,
	mxi_ks3344_write,
	NULL,
	NULL,
	mxi_ks3344_num_input_bytes_available,
	mxi_ks3344_discard_unread_input,
	mxi_ks3344_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_ks3344_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_KS3344_STANDARD_FIELDS
};

long mxi_ks3344_num_record_fields
		= sizeof( mxi_ks3344_record_field_defaults )
			/ sizeof( mxi_ks3344_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_ks3344_rfield_def_ptr
			= &mxi_ks3344_record_field_defaults[0];

/* ---- */

MX_EXPORT mx_status_type
mxi_ks3344_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_ks3344_create_record_structures()";

	MX_RS232 *rs232;
	MX_KS3344 *ks3344;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	ks3344 = (MX_KS3344 *) malloc( sizeof(MX_KS3344) );

	if ( ks3344 == (MX_KS3344 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KS3344 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = ks3344;
	record->class_specific_function_list
				= &mxi_ks3344_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ks3344_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_ks3344_finish_record_initialization()";

	MX_KS3344 *ks3344;
	mx_status_type mx_status;

	ks3344 = (MX_KS3344 *) record->record_type_struct;

	/* CAMAC slot number. */

	if ( ks3344->slot < 1 || ks3344->slot > 23 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"CAMAC slot number %ld is out of allowed range 1-23.",
			ks3344->slot);
	}

	/* CAMAC subaddress. */

	if ( ks3344->subaddress < 0 || ks3344->subaddress > 3 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Subaddress number %ld is out of allowed range 0-3.",
			ks3344->subaddress);
	}

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the KS3344 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ks3344_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_ks3344_open()";

	MX_RS232 *rs232;
	mx_status_type mx_status;

	MX_DEBUG(-2, ("mxi_ks3344_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	rs232 = (MX_RS232 *) (record->record_class_struct);

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 structure pointer is NULL." );
	}

	mx_status = mxi_ks3344_reset_channel( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_ks3344_write_parms( rs232 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_ks3344_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_ks3344_close()";

	MX_RS232 *rs232;
	mx_status_type mx_status;

	MX_DEBUG(-2, ("mxi_ks3344_close() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) (record->record_class_struct);

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 structure pointer is NULL." );
	}

	mx_status = mxi_ks3344_reset_channel( rs232 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_ks3344_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_ks3344_getchar()";

	MX_KS3344 *ks3344;
	int32_t data, error_status_register, error_bits, temp;
	char error_message[120];
	int camac_Q, camac_X;
	long error_code, retries;

	retries = 0;

	ks3344 = (MX_KS3344*) (rs232->record->record_type_struct);

	/* Loop until we see a character or abort for some reason. */

	for (;;) {

		/* Try to read a character. */

		mx_camac( (ks3344->camac_record),
			(ks3344->slot), (ks3344->subaddress), 0,
			&data, &camac_Q, &camac_X );

		/* Q == 0 if no characters were available in the buffer
		 * or if we got an end-of-block character.
		 */

		if ( camac_Q == 0 ) {

			/* Did we somehow get an end-of-block character?
			 * Normally, this driver turns off the end-of-block
			 * feature, but we test here just in case it 
			 * somehow got turned on.
			 */

			if ( data & 0x4000 ) {

			    data &= 0xFF;	/* Mask to 8 bits. */

			    *c = (char) data;

			    return mx_error( MXE_NOT_READY, fname,
		"Unexpected end-of-block character '%c' seen on port '%s'",
				*c, rs232->record->name );
			}

			/* mxi_ks3344_getchar() is often used to test
			 * whether there is input ready on the RS-232
			 * port.  Normally, it is not desirable to 
			 * broadcast a message to the world when this
			 * fails, so we add the MXE_QUIET flag to the
			 * error code.
			 */

			if ( rs232->transfer_flags & MXF_232_NOWAIT ) {
			    return mx_error( (MXE_NOT_READY | MXE_QUIET), fname,
			      "Failed to read a character from port '%s'",
			      rs232->record->name );

			} else {

			    if ( ks3344->max_read_retries >= 0 ) {
				if ( retries < ks3344->max_read_retries ) {

						/* Do nothing. */
				} else {
				    return mx_error(
					(MXE_NOT_READY | MXE_QUIET), fname,
	"Read attempt from port '%s' exceeded maximum retry count = %ld",
					rs232->record->name,
					ks3344->max_read_retries );
				}
			    }

			    retries++;

#if 1
			    MX_DEBUG(-2,
			("mxi_ks3344_getchar(): retry %ld", retries));
#endif
			}

		} else {	/* Q == 1 */

			/* A character was read, so extract it. */

			temp = data & 0xFF;	/* Mask to 8 bits. */

			*c = (char) temp;

#if MXI_KS3344_DEBUG
			MX_DEBUG( -2, ("mxi_ks3344_getchar():  c = 0x%x, '%c'",
				*c, *c) );
#endif

			/* If bit 16 is set, there was some sort of error
			 * during the read.
			 */

			if ( data & 0x8000 ) {
			    /* Read the Error Status Register to find
			     * out what happened.
			     */

			    mx_camac( (ks3344->camac_record),
				(ks3344->slot), 1, 10,
				&error_status_register, &camac_Q, &camac_X );

			    if ( camac_Q == 0 || camac_X == 0 ) {
				return mx_error(MXE_INTERFACE_IO_ERROR, fname,
	"Error while reading KS3344 Error Status Register: Q = %d, X = %d",
					camac_Q, camac_X );
			    }

			    /* Extract the code for this channel. */

			    error_bits
		= ( error_status_register >> (4 * (ks3344->subaddress)) );

			    error_bits &= 0x07;

			    snprintf( error_message, sizeof(error_message),
				"Port '%s' receive error 0x%lx on char '%c': ",
				rs232->record->name,
				(unsigned long) error_bits, *c );

			    error_code = MXE_INTERFACE_IO_ERROR;

			    if ( error_bits & 0x04 ) {
				strlcat( error_message, "PARITY_ERROR ",
						sizeof(error_message) );

				error_code = MXE_INTERFACE_IO_ERROR;
			    }
			    if ( error_bits & 0x02 ) {
				strlcat( error_message, "FRAMING_ERROR ",
						sizeof(error_message) );

				error_code = MXE_INTERFACE_IO_ERROR;
			    }
			    if ( error_bits & 0x01 ) {
				strlcat( error_message,"BUFFER_OVERRUN_ERROR ",
						sizeof(error_message) );

				error_code = MXE_LIMIT_WAS_EXCEEDED;
			    }

			    /* Clear the error indicators for this channel. */

			    (void) mxi_ks3344_reset_channel( rs232 );

			    return mx_error( error_code, fname,
						"%s", error_message );

			} else {
			    /* If we get here, we have read a character
			     * without any errors, so we can leave.
			     */

			    return MX_SUCCESSFUL_RESULT;
			}
		}
	}
}

MX_EXPORT mx_status_type
mxi_ks3344_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_ks3344_putchar()";

	MX_KS3344 *ks3344;
	int32_t data;
	int camac_Q, camac_X;

#if MXI_KS3344_DEBUG
	MX_DEBUG(-2, ("mxi_ks3344_putchar() invoked.  c = 0x%x, '%c'", c, c));
#endif

	ks3344 = (MX_KS3344*) (rs232->record->record_type_struct);

	data = (int32_t) c;

	/* Mask to 8 bits just in case a char isn't 8 bits. */

	data &= 0xFF;

	/* Send the character. */

	mx_camac( (ks3344->camac_record), (ks3344->slot), (ks3344->subaddress),
		16, &data, &camac_Q, &camac_X );

	if ( camac_X == 0 || camac_Q == 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Error writing 0x%lX to RS-232 port: CAMAC Q = %d, X = %d\n",
			(long)data, camac_Q, camac_X );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxi_ks3344_read( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	mx_status_type mx_status;
	long i;

	MX_DEBUG(-2, ("mxi_ks3344_read() invoked."));

	/* This is slow, but simple.  It currently waits forever until
	 * 'max_chars' are read.  Think about doing something better later.
	 */

	rs232->transfer_flags = MXF_232_WAIT;

	for ( i = 0; i < max_bytes_to_read; i++ ) {
		mx_status = mxi_ks3344_getchar( rs232, &buffer[i] );

		if ( mx_status.code != MXE_SUCCESS ) {
			i--;
			continue;
		}
	}

	*bytes_read = max_bytes_to_read;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ks3344_write( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_write,
			size_t *bytes_written )
{
	mx_status_type mx_status;
	long i;

	MX_DEBUG(-2, ("mxi_ks3344_write() invoked."));

	/* This is slow, but simple. */

	rs232->transfer_flags = MXF_232_WAIT;

	for ( i = 0; i < max_bytes_to_write; i++ ) {
		mx_status = mxi_ks3344_putchar( rs232, buffer[i] );

		if ( mx_status.code != MXE_SUCCESS ) {
			return mx_status;
		}
	}

	*bytes_written = max_bytes_to_write;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ks3344_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_ks3344_num_input_bytes_available()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
"I haven't looked up whether a Kinetic Systems 3344 can do this or not yet.");
}

/* It is not possible to discard the read buffers and write buffers
 * separately.  They must both be discarded together for this interface.
 */

MX_EXPORT mx_status_type
mxi_ks3344_discard_unread_input( MX_RS232 *rs232 )
{
	return mxi_ks3344_reset_channel( rs232 );
}

MX_EXPORT mx_status_type
mxi_ks3344_discard_unwritten_output( MX_RS232 *rs232 )
{
	return mxi_ks3344_reset_channel( rs232 );
}

/* ==== Private I/O functions ==== */

/* mxi_ks3344_reset_channel() clears both the read and write buffers,
 * the Error Status register and resets the XON, XOFF state.  It is not
 * possible to do these things separately.
 */

static mx_status_type
mxi_ks3344_reset_channel( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_ks3344_reset_channel()";

	MX_KS3344 *ks3344;
	int32_t data;
	int camac_Q, camac_X;

	ks3344 = (MX_KS3344 *) (rs232->record->record_type_struct);

	if ( ks3344 == (MX_KS3344 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"KS3344 pointer is NULL.");
	}

	mx_camac( (ks3344->camac_record),
		(ks3344->slot), (ks3344->subaddress), 9,
		&data, &camac_Q, &camac_X );

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"CAMAC error: Q = %d, X = %d", camac_Q, camac_X );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

static mx_status_type
mxi_ks3344_write_parms( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_ks3344_write_parms()";

	MX_RECORD *record;
	MX_KS3344 *ks3344;
	int32_t config_word_1, config_word_2;
	int camac_Q, camac_X;

	MX_DEBUG(-2, ("mxi_ks3344_write_parms() invoked."));

	record = rs232->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for the MX_RS232 structure passed is NULL.");
	}

	ks3344 = (MX_KS3344 *) record->record_type_struct;

	if ( ks3344 == (MX_KS3344 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_KS3344 structure for KS3344 port '%s' is NULL.",
			record->name);
	}

	/* Set up configuration word #1.  This involves setting up 
	 * the flow control parameters.  Note that hardware flow control
	 * is always on, because it cannot be turned off.  We also disable 
	 * the end-of-block character.
	 */

	switch( rs232->flow_control ) {
	case MXF_232_NO_FLOW_CONTROL:
	case MXF_232_HARDWARE_FLOW_CONTROL:
		config_word_1 = 0;
		break;

	case MXF_232_SOFTWARE_FLOW_CONTROL:
	case MXF_232_BOTH_FLOW_CONTROL:
		config_word_1 = 0x4000;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported KS3344 flow control = %d for KS3344 '%s'.", 
		rs232->flow_control, record->name);
	}

	/* Write configuration word #1. */

	mx_camac( (ks3344->camac_record), (ks3344->slot),
		(ks3344->subaddress), 17, &config_word_1, &camac_Q, &camac_X );

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"CAMAC error writing configuration word 1: Q = %d, X = %d",
			camac_Q, camac_X );
	}

	/* Everything else is handled by configuration word #2. */

	config_word_2 = 0;

	MXW_UNUSED( config_word_2 );

	/* Set the port speed. */

	switch( rs232->speed ) {
	case 19200:	config_word_2 |= 0xF;  break;
	case 9600:	config_word_2 |= 0xE;  break;
	case 4800:	config_word_2 |= 0xC;  break;
	case 2400:	config_word_2 |= 0xA;  break;
	case 1800:	config_word_2 |= 0x8;  break;
	case 1200:	config_word_2 |= 0x7;  break;
	case 600:	config_word_2 |= 0x6;  break;
	case 300:	config_word_2 |= 0x5;  break;
	case 150:	config_word_2 |= 0x4;  break;
	case 110:	config_word_2 |= 0x2;  break;
	case 75:	config_word_2 |= 0x1;  break;
	case 50:	break;

	case 38400:
	case 200:
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported KS3344 speed = %ld for KS3344 '%s'.", 
		rs232->speed, record->name);
	}

	/* Set the word size. */

	switch( rs232->word_size ) {
	case 8:		config_word_2 |= 0x30;  break;
	case 7:		config_word_2 |= 0x20;  break;
	case 6:		config_word_2 |= 0x10;  break;
	case 5:		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported KS3344 word size = %ld for KS3344 '%s'.", 
		rs232->word_size, record->name);
	}

	/* Set the parity. */

	switch( rs232->parity ) {
	case MXF_232_NO_PARITY:    config_word_2 |= 0x080;  break;
	case MXF_232_EVEN_PARITY:  config_word_2 |= 0x100;  break;
	case MXF_232_ODD_PARITY:   break;

	case MXF_232_MARK_PARITY:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"KS3344 = '%s'.  Mark parity is not supported for CAMAC KS3344 interfaces.",
			record->name);

	case MXF_232_SPACE_PARITY:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"KS3344 = '%s'.  Space parity is not supported for CAMAC KS3344 interfaces.",
			record->name);

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported KS3344 parity = '%c' for KS3344 '%s'.", 
		rs232->parity, record->name);
	}

	/* Set the stop bits. */

	switch( rs232->stop_bits ) {
	case 2:		config_word_2 |= 0x40;  break;
	case 1:		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported KS3344 stop bits = %ld for KS3344 '%s'.", 
		rs232->stop_bits, record->name);
	}

	/* Write configuration word #2. */

	mx_camac( (ks3344->camac_record), (ks3344->slot),
		(ks3344->subaddress) + 4, 17,
		&config_word_1, &camac_Q, &camac_X );

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"CAMAC error writing configuration word 1: Q = %d, X = %d",
			camac_Q, camac_X );
	}

	return MX_SUCCESSFUL_RESULT;
}

