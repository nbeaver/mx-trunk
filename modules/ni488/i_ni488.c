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
 * Copyright 1999, 2001-2002, 2004-2006, 2008, 2010, 2015, 2018, 2020
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NI488_DEBUG		TRUE

#define MXI_NI488_THREAD_SAFE	FALSE

#if HAVE_NI488 && HAVE_LINUX_GPIB
#error You must not set both HAVE_NI488 and HAVE_LINUX_GPIB to non-zero values.  They are incompatible since the National Instruments library and the Linux-GPIB library both export functions and variables with the same names.
#endif

#include <stdio.h>
#include <math.h>
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
		snprintf( dynamic_error_message,
			sizeof(dynamic_error_message),
			error_message_table[ num_error_messages - 1 ],
			iberr_value );

		return &dynamic_error_message[0];
	} else
	if ( iberr_value == EDVR ) {
		snprintf( dynamic_error_message,
			sizeof(dynamic_error_message),
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
	MX_NI488 *ni488 = NULL;
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
	fprintf(file, "  default EOS mode       = 0x%lx\n",
					gpib->default_eos_mode);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ni488_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_ni488_open()";

	MX_GPIB *gpib;
	MX_NI488 *ni488 = NULL;
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

	MX_NI488 *ni488 = NULL;
	int dev, ibsta_value, time_duration_code;
#if 1
	short device_present;
#endif
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %ld, address %ld.",
		fname, ni488->board_number, address));
#endif

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	time_duration_code = mxi_ni488_compute_time_duration_code(
						gpib->io_timeout[address] );

	/* Get a device descriptor for this address. */

	dev = ibdev( ni488->board_number, address, 0,
				time_duration_code,
				gpib->eoi_mode[address],
				gpib->eos_mode[address] );

	ni488->device_descriptor[address] = dev;

	ibsta_value = mx_ibsta();

	if ( debug ) {
		MX_DEBUG(-2,
		("%s: *** ibdev( board_number = %ld, address = %ld, "
		"secondary_address=0, io_timeout_value=%d, eoi_mode=%ld, "
		"eos_mode=%#lx ) = %d, ibsta_value = %#x",
				fname, ni488->board_number, address,
				time_duration_code,
				gpib->eoi_mode[address],
				gpib->eos_mode[address],
				dev, ibsta_value ));
	}

	if ( ( dev == -1 ) || ( ibsta_value & ERR ) ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot open GPIB address %ld on interface '%s'.  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

#if 1
	/* Is there an actual device located at this address? */

	ibsta_value = ibln( ni488->board_descriptor,
				address, NO_SAD, &device_present );

	if ( debug ) {
		MX_DEBUG(-2,
		("%s: *** ibln( board_descriptor = %ld, address = %ld, NO_SAD, "
		"&device_present = %hd ) = %#x",
			fname, ni488->board_descriptor, address,
			device_present, ibsta_value ));
	}

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

		if ( debug ) {
			MX_DEBUG(-2,
			("%s: *** ibonl( device_descriptor = %ld, 0 ) = %#x",
				fname, ni488->device_descriptor[address],
				ibsta_value ));
		}

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

	if ( debug ) {
		MX_DEBUG(-2,
		("%s: device found at GPIB address %ld for interface '%s'.",
			fname, address, ni488->record->name));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxi_ni488_close_device( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_ni488_close_device()";

	MX_NI488 *ni488 = NULL;
	int dev, ibsta_value, time_duration_code;
	int secondary_address, eoi_mode;
	int read_eos_terminator, write_eos_terminator;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s invoked for GPIB board %ld, address %ld.",
		fname, ni488->board_number, address));
#endif

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	/* Read all of the configuration parameters using ibask(). */

	dev = ni488->device_descriptor[address];

	/* Get secondary GPIB address. */

	ibsta_value = ibask( dev, IbaSAD, &secondary_address );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibask( device_descriptor = %d, "
		"IbaSAD, &secondary_address = %d ) = %#x",
		fname, dev, secondary_address, ibsta_value));
	}

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot read secondary address for primary GPIB address %ld on interface '%s'."
"  GPIB error = '%s'", address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	gpib->secondary_address[address] = secondary_address;

	/* Get I/O timeout (in_seconds). */

	ibsta_value = ibask( dev, IbaTMO, &time_duration_code );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibask( device_descriptor = %d, "
		"IbaTMO, &time_duration_code = %d ) = %#x",
		fname, dev, time_duration_code, ibsta_value));
	}

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot read I/O timeout for GPIB address %ld on interface '%s'.  "
"GPIB error = '%s'", address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	gpib->io_timeout[address] =
		mxi_ni488_compute_io_timeout( time_duration_code );

	/* Get EOI mode. */

	ibsta_value = ibask( dev, IbaEOT, &eoi_mode );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibask( device_descriptor = %d, "
		"IbaEOT, &eoi_mode = %d ) = %#x",
		fname, dev, eoi_mode, ibsta_value));
	}

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot read EOI handling mode for GPIB address %ld on interface '%s'.  "
"GPIB error = '%s'", address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	gpib->eoi_mode[address] = eoi_mode;

	/* Get read EOS character. */

	ibsta_value = ibask( dev, IbaEOSrd, &read_eos_terminator );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibask( device_descriptor = %d, "
		"IbaEOSrd, &read_eos_terminator = %d ) = %#x",
		fname, dev, read_eos_terminator, ibsta_value));
	}

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot get read EOS character for GPIB address %ld on interface '%s'.  "
"GPIB error = '%s'", address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	gpib->read_terminator[address] = (char) read_eos_terminator;

	/* Get write EOS character. */

	ibsta_value = ibask( dev, IbaEOSwrt, &write_eos_terminator );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibask( device_descriptor = %d, "
		"IbaEOSwrt, &write_eos_terminator = %d ) = %#x",
		fname, dev, write_eos_terminator, ibsta_value));
	}

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot get write EOS character for GPIB address %ld on interface '%s'.  "
"GPIB error = '%s'", address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	gpib->write_terminator[address] = (char) write_eos_terminator;

	ibsta_value = ibonl( dev, 0 );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibonl( device_descriptor = %d, 0",
								fname, dev ));
	}

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

	MX_NI488 *ni488 = NULL;
	char *ptr;
	int dev, ibsta_value, ibcntl_value;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );
	}

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else
	if ( flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	mx_status = mxi_ni488_get_device_descriptor( gpib,
						ni488, address, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( flags & MXF_GPIB_NOWAIT ) { 	/* Asynchronous read. */
		ibsta_value = ibrda( dev, buffer, max_bytes_to_read );

		if ( debug ) {
			MX_DEBUG(-2,("%s: *** ibrda( device_descriptor = %d, "
			"buffer = '%s', max_bytes_to_read = %ld ) = %#x",
			fname, dev, buffer, max_bytes_to_read, ibsta_value ));
		}
	} else {				/* Synchronous read. */
		ibsta_value = ibrd( dev, buffer, max_bytes_to_read );

		if ( debug ) {
			MX_DEBUG(-2,("%s: *** ibrd( device_descriptor = %d, "
			"buffer = '%s', max_bytes_to_read = %ld ) = %#x",
			fname, dev, buffer, max_bytes_to_read, ibsta_value ));
		}
	}

	ibcntl_value = mx_ibcntl();

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibcntl() = %d", fname, ibcntl_value));
	}

	if ( bytes_read != NULL ) {
		*bytes_read = ibcntl_value;
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

	if ( debug ) {
		MX_DEBUG(-2,
		("%s: <<< read '%s' from GPIB '%s', board %ld, address %ld",
			fname, buffer, gpib->record->name,
			ni488->board_number, address));
	}

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

	MX_NI488 *ni488 = NULL;
	int dev, ibsta_value;
	size_t local_bytes_to_write;
	char stack_buffer[100];
	char *heap_buffer_ptr, *buffer_ptr;
	unsigned long write_terminator;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );
	}

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else
	if ( flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,
		("%s: writing '%s' to GPIB '%s', board %ld, address %ld",
			fname, buffer, gpib->record->name,
			ni488->board_number, address));
	}

#if 0
	MX_DEBUG( 2,
	("%s: gpib = '%s', address = %ld, buffer = '%s', bytes_to_write = %ld, "
	"bytes_written = %p, flags = %lu",
		fname, gpib->record->name, address, buffer,
		(long) bytes_to_write, bytes_written, flags ));
#endif

	/* If the write terminator is not '\0', then the buffer must
	 * have a line terminator character added at the end, since
	 * the National Instruments library does not do that for us.
	 *
	 * Needless to say, this is a nuisance.
	 */

	write_terminator = gpib->write_terminator[address];

	MX_DEBUG(-2,("%s: write_terminator = %#lx",
		fname, write_terminator));

	buffer_ptr = NULL;
	heap_buffer_ptr = NULL;

	if ( write_terminator == '\0' ) {
		/* We can use the buffer passed to us as is. */

		buffer_ptr = buffer;
		local_bytes_to_write = bytes_to_write;
	} else {
		size_t test_bytes_to_write;

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

		test_bytes_to_write = strlen(buffer_ptr);

		/* Now add the 'secret sauce', that is, the write terminator. */

		if ( buffer_ptr[test_bytes_to_write] == '\0' ) {
			buffer_ptr[test_bytes_to_write] = write_terminator;

			/* For convenience, add a null terminator to make
			 * the string easier to print for debugging.
			 */

			buffer_ptr[test_bytes_to_write + 1] = '\0';
		}
	}

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,("%s: >>> writing '%s' to GPIB board %ld, address %ld",
		fname, buffer_ptr, ni488->board_number, address));
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

		if ( debug ) {
			MX_DEBUG(-2,("%s: *** ibwrta( device_descriptor = %d, "
			"buffer_ptr = '%s', local_bytes_to_write = %ld ) = %#x",
				fname, dev, buffer_ptr,
				local_bytes_to_write, ibsta_value ));
		}
	} else {				/* Synchronous write. */
		ibsta_value = ibwrt( dev, buffer_ptr, local_bytes_to_write );

		if ( debug ) {
			MX_DEBUG(-2,("%s: *** ibwrt( device_descriptor = %d, "
			"buffer_ptr = '%s', local_bytes_to_write = %ld ) = %#x",
				fname, dev, buffer_ptr,
				local_bytes_to_write, ibsta_value ));
		}
	}

	if ( bytes_written != NULL ) {
		*bytes_written = (size_t) mx_ibcntl();

		if ( debug ) {
			MX_DEBUG(-2,("%s: *** ibcntl() = %d",
			fname, (int) *bytes_written ));
		}
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

	MX_NI488 *ni488 = NULL;
	int ibsta_value;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		MX_DEBUG(-2,("%s invoked for GPIB '%s', board %ld.",
			fname, gpib->record->name, ni488->board_number));
	}

	ibsta_value = ibsic( ni488->board_descriptor );

	if ( debug ) {
		MX_DEBUG(-2,("%s: ibsic( board_descriptor = %ld ) = %#x",
			fname, ni488->board_descriptor, ibsta_value ));
	}

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

	MX_NI488 *ni488 = NULL;
	int ibsta_value;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,("%s invoked for GPIB '%s', board %ld.",
			fname, gpib->record->name, ni488->board_number));
	}

	DevClear( ni488->board_number, NOADDR );

	ibsta_value = mx_ibsta();

	if ( debug ) {
		MX_DEBUG(-2,
		("%s: *** DevClear( board_number = %ld, NOADDR ) = %#x",
			fname, ni488->board_number, ibsta_value ));
	}

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cannot clear GPIB interface '%s'.  GPIB error = '%s'",
			ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#if 1
MX_EXPORT mx_status_type
mxi_ni488_selective_device_clear( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_ni488_selective_device_clear()";

	MX_NI488 *ni488 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s is disabled for GPIB interface '%s'.",
		fname, gpib->record->name));

	return MX_SUCCESSFUL_RESULT;
}
#else
MX_EXPORT mx_status_type
mxi_ni488_selective_device_clear( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_ni488_selective_device_clear()";

	MX_NI488 *ni488 = NULL;
	int ibsta_value, dev;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,
		("%s invoked for GPIB '%s', board %ld, address %ld.",
			fname, gpib->record->name,
			ni488->board_number, address));
	}

	mx_status = mxi_ni488_get_device_descriptor( gpib,
						ni488, address, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ibsta_value = ibclr( dev );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibclr( device_descriptor = %d ) = %#x",
		fname, dev, ibsta_value ));
	}

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot clear GPIB address %ld on interface '%s'.  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

	return MX_SUCCESSFUL_RESULT;
}
#endif

MX_EXPORT mx_status_type
mxi_ni488_local_lockout( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_ni488_local_lockout()";

	MX_NI488 *ni488 = NULL;
	int ibsta_value;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,("%s invoked for GPIB '%s', board %ld.",
			fname, gpib->record->name, ni488->board_number));
	}

	SendLLO( ni488->board_number );

	ibsta_value = mx_ibsta();

	if ( debug ) {
		MX_DEBUG(-2,("%s *** SendLLO( board_number = %ld ) = %#x",
		fname, ni488->board_number, ibsta_value));
	}

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

	MX_NI488 *ni488 = NULL;
	int ibsta_value;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,
		("%s invoked for GPIB '%s', board %ld, address %ld.",
			fname, gpib->record->name,
			ni488->board_number, address));
	}

	ibsta_value = ibsre( ni488->board_descriptor, 1 );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibsre( board_descriptor = %ld, 1 ) = %#x",
			fname, ni488->board_descriptor, ibsta_value ));
	}

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

	MX_NI488 *ni488 = NULL;
	int ibsta_value, dev;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,
		("%s invoked for GPIB '%s', board %ld, address %ld.",
			fname, gpib->record->name,
			ni488->board_number, address));
	}

	mx_status = mxi_ni488_get_device_descriptor( gpib,
						ni488, address, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ibsta_value = ibloc( dev );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibloc( device_descriptor = %d ) = %#x",
			fname, dev, ibsta_value ));
	}

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

	MX_NI488 *ni488 = NULL;
	int ibsta_value, dev;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,
		("%s invoked for GPIB '%s', board %ld, address %ld.",
			fname, gpib->record->name,
			ni488->board_number, address));
	}

	mx_status = mxi_ni488_get_device_descriptor( gpib,
						ni488, address, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ibsta_value = ibtrg( dev );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibtrg( device_descriptor = %d ) = %#x",
			fname, dev, ibsta_value ));
	}

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

	MX_NI488 *ni488 = NULL;
	int ibsta_value, dev;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,
		("%s invoked for GPIB '%s', board %ld, timeout = %g.",
			fname, gpib->record->name,
			ni488->board_number, timeout));
	}

	/* FIXME - The board address is _assumed_ to be zero. */

	mx_status = mxi_ni488_get_device_descriptor( gpib, ni488, 0, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ibsta_value = ibwait( 0, 0 );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibwait( 0, 0 ) = %#x",
			fname, ibsta_value ));
	}

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

	MX_NI488 *ni488 = NULL;
	int ibsta_value, dev;
	mx_bool_type debug;
	mx_status_type mx_status;

	mx_status = mxi_ni488_get_pointers( gpib, &ni488, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( serial_poll_byte == (unsigned char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The serial_poll_byte pointer passed was NULL." );
	}

	if ( gpib->gpib_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,
		("%s invoked for GPIB '%s', board %ld, address %ld.",
			fname, gpib->record->name,
			ni488->board_number, address));
	}

	mx_status = mxi_ni488_get_device_descriptor( gpib,
						ni488, address, &dev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ibsta_value = ibrsp( dev, (char *) serial_poll_byte );

	if ( debug ) {
		MX_DEBUG(-2,("%s: *** ibrsp( device_descriptor = %d, "
		"&serial_poll_byte = %uc ) = %#x", fname, dev,
			*serial_poll_byte, ibsta_value ));
	}

	if ( ibsta_value & ERR ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Serial poll of address %ld for GPIB interface '%s' failed.  GPIB error = '%s'",
			address, ni488->record->name,
			mxi_ni488_gpib_error_text( ibsta_value ) );
	}

#if MXI_NI488_DEBUG
	MX_DEBUG(-2,
	("%s: serial poll byte for GPIB board %ld, address %ld is %#x",
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

