/*
 * Name:    i_dos_portio.c
 *
 * Purpose: MX driver for DOS style port I/O under MSDOS, Windows 3.1, and
 *          Windows 95/98.  This driver does _NOT_ work under Windows NT.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"

#if defined(OS_MSDOS) \
    || ( defined(OS_WIN32) && !defined(__BORLANDC__) && !defined(__GNUC__) )

#include <conio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_portio.h"

#include "i_dos_portio.h"

#ifdef OS_DJGPP
#include <pc.h>
#endif

#ifdef OS_WIN32

#define inp(m)      _inp(m)
#define inpw(m)     _inpw(m)
#define outp(m,n)   _outp(m,n)
#define outpw(m,n)  _outpw(m,n)

/* The following pragma is to work around the omission of the port I/O
 * functions from MSVCRT.LIB in Microsoft C version 4.0.  See the 
 * Microsoft Knowledge Base article Q152030 for more information.
 * Leaving it in for other versions of Microsoft C should not cause
 * a problem since the pragma should make port I/O faster under any
 * Win32 version of Microsoft C.  
 */

#pragma intrinsic(_inp,_inpw,_outp,_outpw)

#endif /* OS_WIN32 */

MX_RECORD_FUNCTION_LIST mxi_dos_portio_record_function_list = {
	NULL,
	mxi_dos_portio_create_record_structures
};

MX_PORTIO_FUNCTION_LIST mxi_dos_portio_portio_function_list = {
	mxi_dos_portio_inp8,
	mxi_dos_portio_inp16,
	mxi_dos_portio_outp8,
	mxi_dos_portio_outp16,
	mxi_dos_portio_request_region,
	mxi_dos_portio_release_region
};

MX_RECORD_FIELD_DEFAULTS mxi_dos_portio_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS
};

long mxi_dos_portio_num_record_fields
		= sizeof( mxi_dos_portio_record_field_defaults )
			/ sizeof( mxi_dos_portio_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_dos_portio_rfield_def_ptr
			= &mxi_dos_portio_record_field_defaults[0];

/* ---- */

MX_EXPORT mx_status_type
mxi_dos_portio_create_record_structures( MX_RECORD *record )
{
	/* Set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = NULL;
	record->class_specific_function_list
				= &mxi_dos_portio_portio_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT uint8_t
mxi_dos_portio_inp8( MX_RECORD *record, unsigned long port_number )
{
#ifdef OS_DJGPP
	return ( uint8_t ) inportb( (unsigned short) port_number );
#else
	return ( uint8_t ) inp( (unsigned short) port_number );
#endif
}

MX_EXPORT uint16_t
mxi_dos_portio_inp16( MX_RECORD *record, unsigned long port_number )
{
#ifdef OS_DJGPP
	return ( uint16_t) inportw( (unsigned short) port_number );
#else
	return ( uint16_t ) inpw( (unsigned short) port_number );
#endif
}

MX_EXPORT void
mxi_dos_portio_outp8( MX_RECORD *record,
			unsigned long port_number,
			uint8_t byte_value )
{
#ifdef OS_DJGPP
	outportb( (unsigned short) port_number, (unsigned char) byte_value );
#else
	(void) outp( (unsigned short) port_number, (int) byte_value );
#endif
}

MX_EXPORT void
mxi_dos_portio_outp16( MX_RECORD *record,
			unsigned long port_number,
			uint16_t word_value )
{
#ifdef OS_DJGPP
	outportw( (unsigned short) port_number, (unsigned short) word_value );
#else
	(void) outpw( (unsigned short) port_number,
					(unsigned short) word_value );
#endif
}

MX_EXPORT mx_status_type
mxi_dos_portio_request_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_dos_portio_release_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length )
{
	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_LINUX */
