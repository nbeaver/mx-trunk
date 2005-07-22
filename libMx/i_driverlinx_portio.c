/*
 * Name:    i_driverlinx_portio.c
 *
 * Purpose: MX driver for the DriverLINX Port I/O driver for Windows NT/98/95
 *          written by Scientific Software Tools, Inc.  The DriverLINX package
 *          may be downloaded from http://www.sstnet.com/dnload/dnload.htm.
 *          This driver is primarily intended for use under Windows NT, since
 *          the 'dos_portio' driver already handles Windows 98/95, but it
 *          should work on all three operating systems.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2000-2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#ifdef OS_WIN32

#if HAVE_DRIVERLINX_PORTIO

#include <windows.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_types.h"
#include "mx_portio.h"

#include "i_driverlinx_portio.h"

#include "Dlportio.h"

MX_RECORD_FUNCTION_LIST mxi_driverlinx_portio_record_function_list = {
	mxi_driverlinx_portio_initialize_type,
	mxi_driverlinx_portio_create_record_structures,
	mxi_driverlinx_portio_finish_record_initialization,
	mxi_driverlinx_portio_delete_record,
	NULL,
	mxi_driverlinx_portio_read_parms_from_hardware,
	mxi_driverlinx_portio_write_parms_to_hardware,
	mxi_driverlinx_portio_open,
	mxi_driverlinx_portio_close
};

MX_PORTIO_FUNCTION_LIST mxi_driverlinx_portio_portio_function_list = {
	mxi_driverlinx_portio_inp8,
	mxi_driverlinx_portio_inp16,
	mxi_driverlinx_portio_outp8,
	mxi_driverlinx_portio_outp16,
	mxi_driverlinx_portio_request_region,
	mxi_driverlinx_portio_release_region
};

MX_RECORD_FIELD_DEFAULTS mxi_driverlinx_portio_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS
};

long mxi_driverlinx_portio_num_record_fields
		= sizeof( mxi_driverlinx_portio_record_field_defaults )
		    / sizeof( mxi_driverlinx_portio_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_driverlinx_portio_rfield_def_ptr
			= &mxi_driverlinx_portio_record_field_defaults[0];

/* ---- */

MX_EXPORT mx_status_type
mxi_driverlinx_portio_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_driverlinx_portio_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxi_driverlinx_portio_create_record_structures()";

	MX_DRIVERLINX_PORTIO *driverlinx_portio;

	/* Allocate memory for the necessary structures. */

	driverlinx_portio = (MX_DRIVERLINX_PORTIO *)
					malloc( sizeof(MX_DRIVERLINX_PORTIO) );

	if ( driverlinx_portio == (MX_DRIVERLINX_PORTIO *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DRIVERLINX_PORTIO structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = driverlinx_portio;
	record->class_specific_function_list
				= &mxi_driverlinx_portio_portio_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_driverlinx_portio_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_driverlinx_portio_delete_record( MX_RECORD *record )
{
        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }
        if ( record->record_type_struct != NULL ) {
                free( record->record_type_struct );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_driverlinx_portio_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_driverlinx_portio_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_driverlinx_portio_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_driverlinx_portio_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_uint8_type
mxi_driverlinx_portio_inp8( MX_RECORD *record, unsigned long port_number )
{
	mx_uint8_type value;

	value = (mx_uint8_type) DlPortReadPortUchar((IN ULONG) port_number );

	return value;
}

MX_EXPORT mx_uint16_type
mxi_driverlinx_portio_inp16( MX_RECORD *record, unsigned long port_number )
{
	mx_uint16_type value;

	value = (mx_uint16_type) DlPortReadPortUshort((IN ULONG) port_number );

	return value;
}

MX_EXPORT void
mxi_driverlinx_portio_outp8( MX_RECORD *record,
			unsigned long port_number,
			mx_uint8_type byte_value )
{
	DlPortWritePortUchar((IN ULONG) port_number, (IN UCHAR) byte_value );

	return;
}

MX_EXPORT void
mxi_driverlinx_portio_outp16( MX_RECORD *record,
			unsigned long port_number,
			mx_uint16_type word_value )
{
	DlPortWritePortUshort((IN ULONG) port_number, (IN USHORT) word_value );

	return;
}

MX_EXPORT mx_status_type
mxi_driverlinx_portio_request_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length )
{
	const char fname[] = "mxi_driverlinx_portio_request_region()";

	unsigned long last_port_requested;

	last_port_requested = port_number + length - 1L;

	if ( (port_number < 0x0100) || (last_port_requested > 0xFFFF) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The DriverLINX port I/O driver only supports access to "
		"I/O ports in the range 0x0100 to 0xFFFF.  Your request "
		"fell outside that range." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_driverlinx_portio_release_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length )
{
	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_DRIVERLINX_PORTIO */

#endif /* OS_WIN32 */
