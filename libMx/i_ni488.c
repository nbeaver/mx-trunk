/*
 * Name:    i_ni488.c
 *
 * Purpose: MX driver for National Instruments GPIB interfaces.
 *
 *          If HAVE_NI488 is set, this driver compiles for use with the
 *          National Instruments GPIB driver.  If HAVE_LINUX_GPIB is set,
 *          this driver compiles for use with the driver from
 *          http://linux-gpib.sourceforge.net/.
 *
 *          Please note that you must _not_ simultaneously give non-zero
 *          values to both HAVE_NI488 and HAVE_LINUX_GPIB.  The Linux Lab
 *          Project GPIB driver uses many of the same function names as
 *          the National Instruments driver.  This makes it impossible to
 *          simultaneously link with both the National Instruments library
 *          and the Linux Lab Project GPIB library since you would have
 *          many function name conflicts.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NI488_DEBUG		FALSE

#define MXI_NI488_THREAD_SAFE	FALSE

#include <stdio.h>
#include <math.h>

#include "mxconfig.h"

#if HAVE_NI488 && HAVE_LINUX_GPIB
#error You must not set both HAVE_NI488 and HAVE_LINUX_GPIB to non-zero values.  They are incompatible since the National Instruments library and the Linux-GPIB library both export functions and variables with the same names.
#endif

#if HAVE_NI488 || HAVE_LINUX_GPIB

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include "mx_util.h"
#include "mx_gpib.h"
#include "mx_record.h"
#include "i_ni488.h"

/* Include the National Instruments driver include file. */

#if HAVE_LINUX_GPIB
#include "gpib/ib.h"
#else
#include "sys/ugpib.h"
#endif

#if MXI_NI488_THREAD_SAFE
#  define mx_ibcntl()	ThreadIbcntl()
#  define mx_iberr()	ThreadIberr()
#  define mx_ibsta()	ThreadIbsta()
#else
#  define mx_ibcntl()	ibcntl
#  define mx_iberr()	iberr
#  define mx_ibsta()	ibsta
#endif

MX_RECORD_FUNCTION_LIST mxi_ni488_record_function_list = {
	NULL,
	mxi_ni488_create_record_structures,
	mxi_ni488_finish_record_initialization,
	NULL,
	mxi_ni488_print_interface_structure,
	NULL,
	NULL,
	mxi_ni488_open
};

MX_GPIB_FUNCTION_LIST mxi_ni488_gpib_function_list = {
	mxi_ni488_open_device,
	mxi_ni488_close_device,
	mxi_ni488_read,
	mxi_ni488_write,
	mxi_ni488_interface_clear,
	mxi_ni488_device_clear,
	mxi_ni488_selective_device_clear,
	mxi_ni488_local_lockout,
	mxi_ni488_remote_enable,
	mxi_ni488_go_to_local,
	mxi_ni488_trigger,
	mxi_ni488_wait_for_service_request,
	mxi_ni488_serial_poll,
	mxi_ni488_serial_poll_disable
};

MX_RECORD_FIELD_DEFAULTS mxi_ni488_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_GPIB_STANDARD_FIELDS,
	MXI_NI488_STANDARD_FIELDS
};

long mxi_ni488_num_record_fields
		= sizeof( mxi_ni488_record_field_defaults )
			/ sizeof( mxi_ni488_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_ni488_rfield_def_ptr
			= &mxi_ni488_record_field_defaults[0];

/* ==== Private function for the driver's use only. ==== */

static char *
mxi_ni488_gpib_error_text( int ibsta_value )
{
	static char ibsta_not_set[] = 
	"Error flag in 'ibsta' is not set.  Are you sure there was an error?";

	static char error_message_table[][80] = {

"EDVR - Operating system error: errno = %d, OS message = '%s'", /*  0 */
"ECIC - Function requires GPIB board to be CIC",		/*  1 */
"ENOL - No listeners on the GPIB bus",				/*  2 */
"EADR - GPIB board not addressed correctly",			/*  3 */
"EARG - Bad argument to function call",				/*  4 */
"ESAC - GPIB board not system controller as required",		/*  5 */
"EABO - I/O operation aborted (timeout)",			/*  6 */
"ENEB - Nonexistent GPIB board",				/*  7 */
"EDMA - DMA hardware error detected",				/*  8 */
"EBTO - DMA hardware microprocessor bus timeout",		/*  9 (NI488) */
"EOIP - New I/O attempted with old I/O in progress",		/* 10 */
"ECAP - No capability for intended operation",			/* 11 */
"EFSO - File system operation error",				/* 12 */
"EOWN - Shareable board exclusively owned",			/* 13 (NI488) */
"EBUS - GPIB bus error",					/* 14 */
"ESTB - Serial poll queue overflow",				/* 15 */
"ESRQ - SRQ line stuck on",					/* 16 */
"E\?\?\? - Unknown error code 17",				/* 17 */
"E\?\?\? - Unknown error code 18",				/* 18 */
"E\?\?\? - Unknown error code 19",				/* 19 */
"ETAB - The return buffer is full",				/* 20 */
"ELCK - Board or address is locked",				/* 21 */
"EARM - ibnotify callback failed to rearm",			/* 22 (NI488) */
"EHDL - Input handle is invalid",				/* 23 (NI488) */
"E\?\?\? - Unknown error code 24",				/* 24 */
"E\?\?\? - Unknown error code 25",				/* 25 */
"EWIP - Wait in progress on specified input handle",		/* 26 (NI488) */
"ERST - The event notification was cancelled due to interface reset",
								/* 27 (NI488) */
"EPWR - The interface has lost power",				/* 28 (NI488) */
"E\?\?\? - Unknown GPIB error code %d"

	};

	static char dynamic_error_message[120];

	static long num_error_messages = sizeof( error_message_table )
					/ sizeof( error_message_table[0] );
	int iberr_value;

	if ( (ibsta_value & ERR) == 0 ) {
		return &ibsta_not_set[0];
	}

	iberr_value = mx_iberr();

	if ( iberr_value < 0 || iberr_value >= num_error_messages ) {
		sprintf( dynamic_error_message,
			error_message_table[ num_error_messages - 1 ],
			iberr_value );

		return &dynamic_error_message[0];
	} else
	if ( iberr_value == EDVR ) {
		sprintf( dynamic_error_message,
			error_message_table[ EDVR ],
			mx_ibcntl(), strerror( mx_ibcntl() ) );

		return &dynamic_error_message[0];
	} else {
		return &error_message_table[ iberr_value ][0];
	}
}

static mx_status_type
mxi_ni488_get_pointers( MX_GPIB *gpib,
			MX_NI488 **ni488,
			const char *calling_fname )
{
	static const char fname[] = "mxi_ni488_get_pointers()";

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GPIB pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ni488 == (MX_NI488 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_NI488 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*ni488 = (MX_NI488 *) gpib->record->record_type_struct;

	if ( *ni488 == (MX_NI488 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NI488 pointer for record '%s' is NULL.",
			gpib->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ==== Public functions ==== */

MX_EXPORT mx_status_type
mxi_ni488_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_ni488_create_record_structures()";

	MX_GPIB *gpib;
	MX_NI488 *ni488;

	/* Allocate memory for the necessary structures. */

	gpib = (MX_GPIB *) malloc( sizeof(MX_GPIB) );

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_GPIB structure." );
	}

	ni488 = (MX_NI488 *) malloc( sizeof(MX_NI488) );

	if ( ni488 == (MX_NI488 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_NI488 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = gpib;
	record->record_type_struct = ni488;
	record->record_function_list = &mxi_ni488_record_function_list;
	record->class_specific_function_list = &mxi_ni488_gpib_function_list;

	gpib->record = record;
	ni488->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_finish_record_initialization( MX_RECORD *record )
{
	return mx_gpib_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxi_ni488_print_interface_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxi_ni488_print_interface_structure()";

	MX_GPIB *gpib;
	MX_NI488 *ni488;
	char read_eos_char, write_eos_char;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	gpib = (MX_GPIB *) record->record_class_struct;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "GPIB parameters for interface '%s':\n", record->name);
	fprintf(file, "  GPIB type              = NI488.\n\n");

	fprintf(file, "  name                   = %s\n", record->name);
	fprintf(file, "  board number           = %ld\n", ni488->board_number);
	fprintf(file, "  default I/O timeout    = %g\n",
					gpib->default_io_timeout);
	fprintf(file, "  default EOI mode       = %ld\n",
					gpib->default_eoi_mode);

	read_eos_char = gpib->default_read_terminator;

	if ( isprint(read_eos_char) ) {
		fprintf(file,
		      "  default read EOS char  = 0x%02x (%c)\n",
			read_eos_char, read_eos_char);
	} else {
		fprintf(file,
		      "  default read EOS char  = 0x%02x\n", read_eos_char);
	}

	write_eos_char = gpib->default_write_terminator;

	if ( isprint(write_eos_char) ) {
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
mxi_ni488_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_ni488_open()";

	MX_GPIB *gpib;
	MX_NI488 *ni488;
	long i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	gpib = (MX_GPIB *) record->record_class_struct;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME - Are we sure that the following is true?  It seems to 
	 * work and the Linux-GPIB package asserts it to be true.
	 */

	ni488->board_descriptor = ni488->board_number;

	/* Initialize all of the device descriptors to the illegal value -1. */

	for ( i = 0; i < MX_NUM_GPIB_ADDRESSES; i++ ) {
		ni488->device_descriptor[i] = -1;
	}

	gpib->read_buffer_length = 0;
	gpib->write_buffer_length = 0;

	mx_status = mx_gpib_setup_communication_buffers( record );

	return mx_status;
}

/* ========== Internal only functions ========== */

static mx_status_type
mxi_ni488_compute_eos_value( MX_GPIB *gpib,
				long address,
				unsigned long *eos_value )
{
	static const char fname[] = "mxi_ni488_compute_eos_value()";

	unsigned long read_value, write_value;

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_GPIB pointer passed was NULL." );
	}
	if ( eos_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The eos_value pointer passed was NULL." );
	}

	read_value = gpib->read_terminator[address];
	write_value = gpib->write_terminator[address];

	if ( read_value > 0xff ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"EOS terminators for the NI-488 driver must be only one byte long." );
	}

	if ( read_value != 0 ) {
		if ( (write_value != read_value) && (write_value != 0) ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"If the read terminator (currently %#lx) and "
		"the write terminator (currently %#lx) for GPIB interface '%s' "
		"are both not equal to zero, then they must be equal.  "
		"However, the current values of the line terminators "
		"are not equal.",  read_value, write_value,
				gpib->record->name );
		}
	}

#ifndef REOS
#define REOS 	0x400
#endif

#ifndef XEOS
#define XEOS	0x800
#endif

	if ( read_value ) {
		*eos_value = REOS | read_value;
	} else {
		*eos_value = write_value;
	}

	if ( write_value ) {
		*eos_value |= XEOS;
	}

	return MX_SUCCESSFUL_RESULT;
}

static double
mxi_ni488_compute_io_timeout( int time_duration_code )
{
	static const char fname[] = "mxi_ni488_compute_io_timeout()";

	double io_timeout;

	switch( time_duration_code ) {
	case 0:  io_timeout = -1.0;     break;
	case 1:  io_timeout =  10.0e-6; break;
	case 2:  io_timeout =  30.0e-6; break;
	case 3:  io_timeout = 100.0e-6; break;
	case 4:  io_timeout = 300.0e-6; break;
	case 5:  io_timeout =   1.0e-3; break;
	case 6:  io_timeout =   3.0e-3; break;
	case 7:  io_timeout =  10.0e-3; break;
	case 8:  io_timeout =  30.0e-3; break;
	case 9:  io_timeout = 100.0e-3; break;
	case 10: io_timeout = 300.0e-3; break;
	case 11: io_timeout =   1.0;    break;
	case 12: io_timeout =   3.0;    break;
	case 13: io_timeout =  10.0;    break;
	case 14: io_timeout =  30.0;    break;
	case 15: io_timeout = 100.0;    break;
	case 16: io_timeout = 300.0;    break;
	case 17: io_timeout =   1.0e3;  break;
	default:
		(void) mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The reported National Instruments IEEE-488 time duration "
		"code %d is outside the allowed range of 0 to 17.  "
		"This should not be able to happen.", time_duration_code );

		io_timeout = 0.0;
		break;
	}

	return io_timeout;
}

static int
mxi_ni488_compute_time_duration_code( double io_timeout )
{
	int time_duration_code, exponent;
	double log_io_timeout, log_mantissa;

	if ( io_timeout < 1.0e-12 ) {
		/* Negative or zero timeout time intervals mean
		 * do not time out at all.
		 */

		time_duration_code = 0;

		MX_DEBUG( 2,("xxx: io_timeout = %g means no timeout.",
			io_timeout));

		return time_duration_code;
	}

	log_io_timeout = log10( io_timeout );

	exponent = mx_round( floor( log_io_timeout ) );

	log_mantissa = log_io_timeout - (double) exponent;

	MX_DEBUG( 2,
	("xxx: log_io_timeout = %g, exponent = %d, log_mantissa = %g",
				log_io_timeout, exponent, log_mantissa));

	time_duration_code = 11 + 2 * exponent;

	if ( log_mantissa <= log10(sqrt(3.0)) ) {
		/* Do nothing. */

	} else
	if ( log_mantissa <= log10(sqrt(30.0)) ) {
		time_duration_code += 1;
	} else {
		time_duration_code += 2;
	}

	if ( time_duration_code < 0 ) {
		time_duration_code = 0;
	} else
	if ( time_duration_code > 17 ) {
		time_duration_code = 17;
	}

#if 0
	{
		double computed_io_timeout;

		computed_io_timeout = 
			mxi_ni488_compute_io_timeout( time_duration_code );

		MX_DEBUG(-2,
  ("xxx: io_timeout = %g, computed_io_timeout = %g, time_duration_code = %d\n",
			io_timeout, computed_io_timeout, time_duration_code));
	}
#endif

	return time_duration_code;
}

static mx_status_type
mxi_ni488_get_device_descriptor( MX_GPIB *gpib,
				MX_NI488 *ni488,
				long address,
				int *device_descriptor )
{
	int dev;
	mx_status_type mx_status;

	dev = ni488->device_descriptor[address];

	if ( dev == -1 ) {
		mx_status = mxi_ni488_open_device( gpib, address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dev = ni488->device_descriptor[address];
	}

	if ( device_descriptor != NULL ) {
		*device_descriptor = dev;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ========== Device specific functions ========== */

MX_EXPORT mx_status_type
mxi_ni488_open_device( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_ni488_open_device()";

	MX_NI488 *ni488;
	int dev, ibsta_value, time_duration_code;
	unsigned long eos_value;
	short device_present;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %d, address %d.",
		fname, ni488->board_number, address));
#endif

	time_duration_code = mxi_ni488_compute_time_duration_code(
						gpib->io_timeout[address] );

	mx_status = mxi_ni488_compute_eos_value( gpib, address, &eos_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get a device descriptor for this address. */

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,
	("%s: ibdev( board_number = %d, address = %d, secondary_address=0, "
	"io_timeout_value=%d, eoi_mode=%d, eos_value=%#x )",
				fname, ni488->board_number, address,
				time_duration_code,
				gpib->eoi_mode[address],
				(int) eos_value ));
#endif

	dev = ibdev( ni488->board_number, address, 0,
				time_duration_code,
				gpib->eoi_mode[address],
				(int) eos_value );

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s: dev = %d", fname, dev));
#endif

	ni488->device_descriptor[address] = dev;

	ibsta_value = mx_ibsta();

	if ( ( dev == -1 ) || ( ibsta_value & ERR ) ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot open GPIB address %ld on interface '%s'.  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	/* Is there an actual device located at this address? */

	ibsta_value = ibln( ni488->board_descriptor,
				address, NO_SAD, &device_present );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"ibln() failed while attempting to check for the presence "
		"of a device at GPIB address %ld for interface '%s'.  "
		"GPIB error = '%s'", address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	if ( device_present == 0 ) {
		/* Close down the nonexistent device? */

		ibsta_value = ibonl( ni488->device_descriptor[address], 0 );

		if ( ibsta_value & ERR ) {
			(void) mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot close GPIB address %ld on interface '%s'.  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
		}

		ni488->device_descriptor[address] = -1;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"There is no device present at GPIB address %ld for interface '%s'.",
			address, ni488->record->name );
	}

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s: device found at GPIB address %d for interface '%s'.",
		fname, address, ni488->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxi_ni488_close_device( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_ni488_close_device()";

	MX_NI488 *ni488;
	int dev, ibsta_value, value, time_duration_code;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %d, address %d.",
		fname, ni488->board_number, address));
#endif

	/* Read all of the configuration parameters using ibask(). */

	dev = ni488->device_descriptor[address];

	/* Get secondary GPIB address. */

	ibsta_value = ibask( dev, IbaSAD, &value );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot read secondary address for primary GPIB address %ld on interface '%s'."
"  GPIB error = '%s'", address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	gpib->secondary_address[address] = value;

	/* Get I/O timeout (in_seconds). */

	ibsta_value = ibask( dev, IbaTMO, &time_duration_code );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot read I/O timeout for GPIB address %ld on interface '%s'.  "
"GPIB error = '%s'", address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	gpib->io_timeout[address] =
		mxi_ni488_compute_io_timeout( time_duration_code );

	/* Get EOI mode. */

	ibsta_value = ibask( dev, IbaEOT, &value );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot read EOI handling mode for GPIB address %ld on interface '%s'.  "
"GPIB error = '%s'", address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	gpib->eoi_mode[address] = value;

	/* Get read EOS character. */

	ibsta_value = ibask( dev, IbaEOSrd, &value );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot get read EOS character for GPIB address %ld on interface '%s'.  "
"GPIB error = '%s'", address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	gpib->read_terminator[address] = (char) value;

	/* Get write EOS character. */

	ibsta_value = ibask( dev, IbaEOSwrt, &value );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot get write EOS character for GPIB address %ld on interface '%s'.  "
"GPIB error = '%s'", address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	gpib->write_terminator[address] = (char) value;

	ibsta_value = ibonl( ni488->device_descriptor[address], 0 );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot close GPIB address %ld on interface '%s'.  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	ni488->device_descriptor[address] = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_read( MX_GPIB *gpib,
		long address,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		unsigned long flags )
{
	static const char fname[] = "mxi_ni488_read()";

	MX_NI488 *ni488;
	char *ptr;
	int dev, ibsta_value;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );
	}

	mx_status = mxi_ni488_get_device_descriptor( gpib,
						ni488, address, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( flags & MXF_GPIB_NOWAIT ) { 	/* Asynchronous read. */
		ibsta_value = ibrda( dev, buffer, max_bytes_to_read );

	} else {				/* Synchronous read. */
		ibsta_value = ibrd( dev, buffer, max_bytes_to_read );
	}

	if ( bytes_read != NULL ) {
		*bytes_read = (size_t) mx_ibcntl();
	}

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Read for GPIB address %ld on interface '%s' "
		"was unsuccessful.  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	/* If it is present, strip off a line terminator at the end
	 * of the buffer.
	 */

	ptr = &buffer[ *bytes_read - 1 ];

	if ( *ptr == (char) gpib->read_terminator[ address ] ) {
		*ptr = '\0';

		if ( bytes_read != NULL ) {
			(*bytes_read)--;
		}
	}

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s: read '%s' from GPIB board %d, address %d",
		fname, buffer, ni488->board_number, address));
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_write( MX_GPIB *gpib,
		long address,
		char *buffer,
		size_t bytes_to_write,
		size_t *bytes_written,
		unsigned long flags )
{
	static const char fname[] = "mxi_ni488_write()";

	MX_NI488 *ni488;
	int dev, ibsta_value;
	size_t local_bytes_to_write;
	char stack_buffer[100];
	char *heap_buffer_ptr, *buffer_ptr;
	unsigned long write_terminator;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );
	}

	MX_DEBUG( 2,
	("%s: gpib = '%s', address = %ld, buffer = '%s', bytes_to_write = %ld, "
	"bytes_written = %p, flags = %lu",
		fname, gpib->record->name, address, buffer,
		(long) bytes_to_write, bytes_written, flags ));

	/* If the write terminator is not '\0', then the buffer must
	 * have a line terminator character added at the end, since
	 * the National Instruments library does not do that for us.
	 *
	 * Needless to say, this is a nuisance, but computer programming
	 * is full of such things.
	 */

	write_terminator = gpib->write_terminator[address];

	MX_DEBUG( 2,("%s: write_terminator = %#lx",
		fname, write_terminator));

	buffer_ptr = NULL;
	heap_buffer_ptr = NULL;

	if ( write_terminator == '\0' ) {
		/* We can use the buffer passed to us as is. */

		buffer_ptr = buffer;
		local_bytes_to_write = bytes_to_write;
	} else {
		/* We must copy the buffer to a local buffer to add the
		 * terminator.  It is _not_ safe to write to the caller's
		 * buffer since we have no way of being sure that the
		 * extra terminator byte will fit.
		 */

		local_bytes_to_write = bytes_to_write + 1;

		/* Is the caller's buffer short enough to copy to the
		 * 'stack_buffer' array declared above?
		 */

		if ( bytes_to_write < (sizeof( stack_buffer ) - 1) ) {
			/* If so, just copy the caller's buffer
			 * to 'stack_buffer'.
			 */

			buffer_ptr = stack_buffer;
		} else {
			/* Otherwise, we must allocate a new buffer on
			 * the heap.  Hopefully this will not happen
			 * too often.
			 */

			heap_buffer_ptr = (char *)
				malloc( local_bytes_to_write * sizeof(char) );

			if ( heap_buffer_ptr == NULL ) {
				return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Could not allocate a temporary %d byte heap buffer "
			"for the internal write buffer.",
					(int) local_bytes_to_write );
			}

			buffer_ptr = heap_buffer_ptr;
		}

		/* It is now safe to copy to the local buffer. */
			
		memcpy( buffer_ptr, buffer, bytes_to_write );

		/* Now add the 'secret sauce', that is, the write terminator. */

		buffer_ptr[bytes_to_write] = write_terminator;

		/* For convenience, add a null terminator to make the string
		 * easier to print for debugging.
		 */

		buffer_ptr[bytes_to_write + 1] = '\0';
	}

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s: writing '%s' to GPIB board %d, address %d",
		fname, buffer, ni488->board_number, address));
#endif

#if 0
	{
		int i;

		for ( i = 0; i < local_bytes_to_write; i++ ) {
			fprintf( stderr, "%#x ", buffer_ptr[i] );
		}
		fprintf( stderr, "\n" );
	}
#endif

	mx_status = mxi_ni488_get_device_descriptor( gpib,
						ni488, address, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( flags & MXF_GPIB_NOWAIT ) { 	/* Asynchronous write. */
		ibsta_value = ibwrta( dev, buffer_ptr, local_bytes_to_write );

	} else {				/* Synchronous write. */
		ibsta_value = ibwrt( dev, buffer_ptr, local_bytes_to_write );
	}

	if ( bytes_written != NULL ) {
		*bytes_written = (size_t) mx_ibcntl();
	}

	/* If we allocated a buffer on the heap, get rid of it now. */

	if ( heap_buffer_ptr != NULL ) {
		free( heap_buffer_ptr );
	}

	/* Was there an error with the write? */

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Write for GPIB address %ld on interface '%s' was unsuccessful."
		"  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_interface_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_ni488_interface_clear()";

	MX_NI488 *ni488;
	int ibsta_value;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %d.",
		fname, ni488->board_number));
#endif

	ibsta_value = ibsic( ni488->board_descriptor );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot do an interface clear for GPIB interface '%s'.  GPIB error = '%s'",
			ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_device_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_ni488_device_clear()";

	MX_NI488 *ni488;
	int ibsta_value;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %d.",
		fname, ni488->board_number));
#endif

	DevClear( ni488->board_number, NOADDR );

	ibsta_value = mx_ibsta();

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cannot clear GPIB interface '%s'.  GPIB error = '%s'",
			ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_selective_device_clear( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_ni488_selective_device_clear()";

	MX_NI488 *ni488;
	int ibsta_value, dev;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %d, address %d.",
		fname, ni488->board_number, address));
#endif

	mx_status = mxi_ni488_get_device_descriptor( gpib,
						ni488, address, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ibsta_value = ibclr( dev );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot clear GPIB address %ld on interface '%s'.  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_local_lockout( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_ni488_local_lockout()";

	MX_NI488 *ni488;
	int ibsta_value;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %d.",
		fname, ni488->board_number));
#endif

	SendLLO( ni488->board_number );

	ibsta_value = mx_ibsta();

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot set Local Lockout GPIB interface '%s'.  GPIB error = '%s'",
			ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_remote_enable( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_ni488_remote_enable()";

	MX_NI488 *ni488;
	int ibsta_value;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %d, address %d.",
		fname, ni488->board_number, address));
#endif

	ibsta_value = ibsre( ni488->board_descriptor, 1 );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot perform remote enable for GPIB address %ld on interface '%s'.  "
	"GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_go_to_local( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_ni488_go_to_local()";

	MX_NI488 *ni488;
	int ibsta_value, dev;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %d, address %d.",
		fname, ni488->board_number, address));
#endif

	mx_status = mxi_ni488_get_device_descriptor( gpib,
						ni488, address, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ibsta_value = ibloc( dev );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot go to local for GPIB address %ld on interface '%s'.  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_trigger( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_ni488_trigger()";

	MX_NI488 *ni488;
	int ibsta_value, dev;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %d, address %d.",
		fname, ni488->board_number, address));
#endif

	mx_status = mxi_ni488_get_device_descriptor( gpib,
						ni488, address, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ibsta_value = ibtrg( dev );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot trigger GPIB address %ld on interface '%s'.  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_wait_for_service_request( MX_GPIB *gpib, double timeout )
{
	static const char fname[] = "mxi_ni488_wait_for_service_request()";

	MX_NI488 *ni488;
	int ibsta_value, dev;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %d, timeout = %g.",
		fname, ni488->board_number, timeout));
#endif

	/* FIXME - The board address is _assumed_ to be zero. */

	mx_status = mxi_ni488_get_device_descriptor( gpib, ni488, 0, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ibsta_value = ibwait( 0, 0 );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Wait for service request for GPIB interface '%s' failed.  "
		"GPIB error = '%s'", ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_serial_poll( MX_GPIB *gpib, long address,
				unsigned char *serial_poll_byte)
{
	static const char fname[] = "mxi_ni488_serial_poll()";

	MX_NI488 *ni488;
	int ibsta_value, dev;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( serial_poll_byte == (unsigned char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The serial_poll_byte pointer passed was NULL." );
	}

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %d, address %d.",
		fname, ni488->board_number, address));
#endif

	mx_status = mxi_ni488_get_device_descriptor( gpib,
						ni488, address, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ibsta_value = ibrsp( dev, serial_poll_byte );

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Serial poll of address %ld for GPIB interface '%s' failed.  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,
	("%s: serial poll byte for GPIB board %d, address %d is %#x",
	 	fname, ni488->board_number, address, *serial_poll_byte));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_serial_poll_disable( MX_GPIB *gpib )
{
	/* Apparently, this function is not needed for this interface. */

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_NI488 || HAVE_LINUX_GPIB */
