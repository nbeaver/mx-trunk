/*
 * Name:    i_linux_iopl.c
 *
 * Purpose: MX port I/O driver that uses the Linux iopl() or ioperm() functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_LINUX_IOPL_DEBUG_TIMING	FALSE

#include <stdio.h>

#include "mxconfig.h"

#if defined(OS_LINUX) && defined(__i386__) && defined(__GNUC__)

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/io.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_portio.h"

#include "i_linux_iopl.h"

#if MXI_LINUX_IOPL_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_linux_iopl_record_function_list = {
	NULL,
	mxi_linux_iopl_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_linux_iopl_open,
	mxi_linux_iopl_close
};

MX_PORTIO_FUNCTION_LIST mxi_linux_iopl_portio_function_list = {
	mxi_linux_iopl_inp8,
	mxi_linux_iopl_inp16,
	mxi_linux_iopl_outp8,
	mxi_linux_iopl_outp16,
	mxi_linux_iopl_request_region,
	mxi_linux_iopl_release_region
};

MX_RECORD_FIELD_DEFAULTS mxi_linux_iopl_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_LINUX_IOPL_STANDARD_FIELDS
};

long mxi_linux_iopl_num_record_fields
		= sizeof( mxi_linux_iopl_record_field_defaults )
			/ sizeof( mxi_linux_iopl_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_linux_iopl_rfield_def_ptr
			= &mxi_linux_iopl_record_field_defaults[0];

/* ---- */

MX_EXPORT mx_status_type
mxi_linux_iopl_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxi_linux_iopl_create_record_structures()";

	MX_LINUX_IOPL *linux_iopl;

	/* Allocate memory for the necessary structures. */

	linux_iopl = (MX_LINUX_IOPL *) malloc( sizeof(MX_LINUX_IOPL) );

	if ( linux_iopl == (MX_LINUX_IOPL *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_LINUX_IOPL structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = linux_iopl;
	record->class_specific_function_list
				= &mxi_linux_iopl_portio_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_iopl_open( MX_RECORD *record )
{
	const char fname[] = "mxi_linux_iopl_open()";

	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	if ( record->mx_type == MXI_PIO_IOPL ) {
		status = iopl(3);

		if ( status != 0 ) {
			saved_errno = errno;

			if ( saved_errno == EPERM ) {
				return mx_error( MXE_PERMISSION_DENIED, fname,
		"The Linux iopl() driver can only be used by a program "
		"running as run or which is setuid root.  Instead, you are "
		"running under effective user id %d.", (int) geteuid() );
			} else {
				return mx_error( MXE_FUNCTION_FAILED, fname,
		"The iopl(3) call failed.  Errno = %d, error text = '%s'",
				saved_errno, strerror( saved_errno ) );
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_iopl_close( MX_RECORD *record )
{
	const char fname[] = "mxi_linux_iopl_close()";

	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	if ( record->mx_type == MXI_PIO_IOPL ) {
		/* If our effective user id is not 0, then don't try to do
		 * anything.  This could happen in a setuid program that
		 * has dropped its privileges.
		 */

		if ( geteuid() == 0 ) {
			status = iopl(0);

			if ( status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_FUNCTION_FAILED, fname,
		"The iopl(0) call failed.  Errno = %d, error text = '%s'",
				saved_errno, strerror( saved_errno ) );
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT uint8_t
mxi_linux_iopl_inp8( MX_RECORD *record, unsigned long port_number )
{
	uint8_t value;

#if MXI_LINUX_IOPL_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	value = (uint8_t) inb( port_number );

#if MXI_LINUX_IOPL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, "mxi_linux_iopl_inp8",
		"read %#x from port %#lx", value, port_number );
#endif

	return value;
}

MX_EXPORT uint16_t
mxi_linux_iopl_inp16( MX_RECORD *record, unsigned long port_number )
{
	uint16_t value;

#if MXI_LINUX_IOPL_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	value = (uint16_t) inw( port_number );

#if MXI_LINUX_IOPL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, "mxi_linux_iopl_inp16",
		"read %#x from port %#lx", value, port_number );
#endif

	return value;
}

MX_EXPORT void
mxi_linux_iopl_outp8( MX_RECORD *record,
			unsigned long port_number,
			uint8_t byte_value )
{
#if MXI_LINUX_IOPL_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	outb( (char) byte_value, port_number );

#if MXI_LINUX_IOPL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, "mxi_linux_iopl_outp8",
		"wrote %#x to port %#lx", byte_value, port_number );
#endif

	return;
}

MX_EXPORT void
mxi_linux_iopl_outp16( MX_RECORD *record,
			unsigned long port_number,
			uint16_t word_value )
{
#if MXI_LINUX_IOPL_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	outw( (short) word_value, port_number );

#if MXI_LINUX_IOPL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, "mxi_linux_iopl_outp16",
		"wrote %#x to port %#lx", word_value, port_number );
#endif

	return;
}

MX_EXPORT mx_status_type
mxi_linux_iopl_request_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length )
{
	const char fname[] = "mxi_linux_iopl_request_region()";

	int status, saved_errno;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	if ( record->mx_type == MXI_PIO_IOPERM ) {

		status = ioperm( port_number, length, 1 );

		if ( status != 0 ) {
			saved_errno = errno;

			if ( saved_errno == EPERM ) {
				return mx_error( MXE_PERMISSION_DENIED, fname,
		"The Linux ioperm() driver can only be used by a program "
		"running as run or which is setuid root.  Instead, you are "
		"running under effective user id %d.", (int) geteuid() );
			} else {
				return mx_error( MXE_FUNCTION_FAILED, fname,
		"The ioperm() call failed.  Errno = %d, error text = '%s'",
				saved_errno, strerror( saved_errno ) );
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_iopl_release_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length )
{
	const char fname[] = "mxi_linux_iopl_release_region()";

	int status, saved_errno;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	if ( record->mx_type == MXI_PIO_IOPERM ) {
		/* If our effective user id is not 0, then don't try to do
		 * anything.  This could happen in a setuid program that
		 * has dropped its privileges.
		 */

		if ( geteuid() == 0 ) {
			status = ioperm( port_number, length, 0 );

			if ( status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_FUNCTION_FAILED, fname,
		"The ioperm() call failed.  Errno = %d, error text = '%s'",
				saved_errno, strerror( saved_errno ) );
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_LINUX && __i386__ */
