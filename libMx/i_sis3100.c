/*
 * Name:    i_sis3100.c
 *
 * Purpose: MX driver for the SIS1100/3100 PCI-to-VME bus interface.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2002-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_SIS3100_DEBUG	TRUE

#define MX_SIS3100_DEBUG_TIMING	FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_SIS3100

#if !defined( OS_LINUX )
#error This driver is currently only supported on Linux platforms.
#endif

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_vme.h"
#include "i_sis3100.h"

#include "sis3100_vme_calls.h"	/* Header file provided by Struck. */

#if MX_SIS3100_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_sis3100_record_function_list = {
	NULL,
	mxi_sis3100_create_record_structures,
	mxi_sis3100_finish_record_initialization,
	NULL,
	mxi_sis3100_print_structure,
	NULL,
	NULL,
	mxi_sis3100_open,
	mxi_sis3100_close,
	NULL,
	mxi_sis3100_resynchronize
};

MX_VME_FUNCTION_LIST mxi_sis3100_vme_function_list = {
	mxi_sis3100_input,
	mxi_sis3100_output,
	mxi_sis3100_multi_input,
	mxi_sis3100_multi_output,
	mxi_sis3100_get_parameter,
	mxi_sis3100_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxi_sis3100_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VME_STANDARD_FIELDS,
	MXI_SIS3100_STANDARD_FIELDS
};

long mxi_sis3100_num_record_fields
		= sizeof( mxi_sis3100_record_field_defaults )
			/ sizeof( mxi_sis3100_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxi_sis3100_rfield_def_ptr
		= &mxi_sis3100_record_field_defaults[0];

/*--*/

/* The SIS1100/3100 Linux driver manual defines the following error codes.
 * However, the Struck provided software does not currently contain a
 * C header file that defines these values, so we must do this ourself.
 */

#ifndef RE_NRDY

#  define RE_NRDY	0x202
#  define RE_PROT	0x206
#  define RE_TO		0x207
#  define RE_BERR	0x211
#  define RE_RETRY	0x212
#  define RE_ARB	0x214

#endif

static const char *
mxi_sis3100_strerror( int vme_status )
{
	static struct {
		int code;
		const char message[60];
	} vme_status_table[] = {
	
		{ RE_NRDY,  "SIS3100 remote station not ready" },
		{ RE_PROT,  "SIS3100 protocol error" },
		{ RE_TO,    "SIS3100 protocol timeout" },
		{ RE_BERR,  "VME bus error" },
		{ RE_RETRY, "VME retry" },
		{ RE_ARB,
		  "Arbitration timeout: SIS3100 could not get bus mastership" },
	};

	static const char not_found_message[]
		= "Unrecognized SIS3100 error code";

	int num_entries = sizeof( vme_status_table )
				/ sizeof( vme_status_table[0] );

	int i;

	for ( i = 0; i < num_entries; i++ ) {
		if ( vme_status == vme_status_table[i].code ) {
			return vme_status_table[i].message;
		}
	}

	return not_found_message;
}

/*--*/

#if MX_SIS3100_DEBUG

/* MX_SIS3100_DEBUG_WARNING_SUPPRESS is used to suppress uninitialized
 * variable warnings from GCC.
 */

#  define MX_SIS3100_DEBUG_PRINT \
	do { \
		unsigned long value = 0; \
                                     \
		switch( vme->data_size ) { \
		case MXF_VME_D8:  \
		    value = (unsigned long) *(u_int8_t *) vme->data_pointer; \
		    break; \
		case MXF_VME_D16: \
		    value = (unsigned long) *(u_int16_t *) vme->data_pointer; \
		    break; \
		case MXF_VME_D32: \
		    value = (unsigned long) *(u_int32_t *) vme->data_pointer; \
		    break; \
		} \
		MX_DEBUG(-2,("%s: A%lu, D%lu, addr = %#lx, value = %#lx", \
			fname, \
			(unsigned long) vme->address_mode, \
			(unsigned long) vme->data_size, \
			(unsigned long) vme->address, \
			(unsigned long) value )); \
	} while(0)

#  define MX_SIS3100_MULTI_DEBUG_PRINT \
	do { \
		unsigned long value = 0; \
                                     \
		switch( vme->data_size ) { \
		case MXF_VME_D8:  \
		    value = (unsigned long) *(u_int8_t *) vme->data_pointer; \
		    break; \
		case MXF_VME_D16: \
		    value = (unsigned long) *(u_int16_t *) vme->data_pointer; \
		    break; \
		case MXF_VME_D32: \
		    value = (unsigned long) *(u_int32_t *) vme->data_pointer; \
		    break; \
		} \
	MX_DEBUG(-2,("%s: A%lu, D%lu, addr = %#lx, value = %#lx (%lu values)", \
			fname, \
			(unsigned long) vme->address_mode, \
			(unsigned long) vme->data_size, \
			(unsigned long) vme->address, \
			(unsigned long) value, \
			vme->num_values )); \
	} while(0)
#else

#  define MX_SIS3100_DEBUG_PRINT

#  define MX_SIS3100_MULTI_DEBUG_PRINT

#endif

/*==========================*/

static mx_status_type
mxi_sis3100_get_pointers( MX_VME *vme,
			MX_SIS3100 **sis3100,
			const char calling_fname[] )
{
	static const char fname[] = "mxi_sis3100_get_pointers()";

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( sis3100 == (MX_SIS3100 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SIS3100 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( vme->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*sis3100 = (MX_SIS3100 *) vme->record->record_type_struct;

	if ( *sis3100 == (MX_SIS3100 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SIS3100 pointer for record '%s' is NULL.",
				vme->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_sis3100_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_sis3100_create_record_structures()";

	MX_VME *vme;
	MX_SIS3100 *sis3100;

	/* Allocate memory for the necessary structures. */

	vme = (MX_VME *) malloc( sizeof(MX_VME) );

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VME structure." );
	}

	sis3100 = (MX_SIS3100 *) malloc( sizeof(MX_SIS3100) );

	if ( sis3100 == (MX_SIS3100 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SIS3100 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = vme;
	record->record_type_struct = sis3100;
	record->class_specific_function_list
			= &mxi_sis3100_vme_function_list;

	vme->record = record;

	sis3100->file_descriptor = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sis3100_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_vme_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_sis3100_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxi_sis3100_print_structure()";

	MX_VME *vme;
	MX_SIS3100 *sis3100;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_sis3100_get_pointers( vme, &sis3100, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf( file, "VME parameters for interface '%s'\n\n", record->name );

	fprintf( file, "  filename    = '%s'\n\n", sis3100->filename );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_sis3100_get_driver_major_number( MX_RECORD *record,
					int *driver_major_number )
{
	static const char fname[] = "mxi_sis3100_get_driver_major_number()";

	static const char proc_devices_name[] = "/proc/devices";
	FILE *devices_file;
	int num_items, saved_errno, length, major_number;
	char buffer[80], driver_name[80];

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}
	if ( driver_major_number == (int *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The driver_major_number pointer passed was NULL." );
	}

	devices_file = fopen( proc_devices_name, "r" );

	if ( devices_file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot open '%s'.  This should never happen on a Linux system "
		"and may indicate that something is wrong with your copy "
		"of Linux.  Error number = %d, error message = '%s'",
		    proc_devices_name, saved_errno, strerror( saved_errno ));
	}

	*driver_major_number = -1;

	while (1) {
		fgets( buffer, sizeof(buffer), devices_file );

		if ( feof(devices_file) ) {
			/* We have reached the end of /proc/device,
			 * so break out of the while() loop.
			 */

			break;
		}
		if ( ferror(devices_file) ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while reading from '%s'.  "
			"Error number = %d, error message = '%s'",
				proc_devices_name, saved_errno,
				strerror( saved_errno ) );
		}

		/* Delete any trailing newline. */

		length = strlen(buffer);

		if ( buffer[length-1] == '\n' ) {
			buffer[length-1] = '\0';
		}

		/* Is this a header line? */

		if ( strncmp( buffer, "Character", 9 ) == 0 ) {
			continue;
		} else
		if ( strncmp( buffer, "Block", 5 ) == 0 ) {
			/* If we get here, the sis1100 driver is not loaded. */

			break;
		}

		/* If not a header line, then try to parse it. */

		num_items = sscanf( buffer, "%d %s",
				&major_number, driver_name );

		if ( num_items != 2 ) {
			/* This line does not have the type of formatting
			 * that we are looking for, so skip it.
			 */

			continue;
		}

		/* Is this the driver we are looking for? */

		if ( strcmp( driver_name, "sis1100" ) == 0 ) {
			*driver_major_number = major_number;
			break;
		}
	}

	if ( *driver_major_number < 0 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The 'sis1100' kernel module needed by the MX driver "
		"for record '%s' is not currently loaded.  "
		"Please check the output from the Linux 'dmesg' and 'lsmod' "
		"commands to help determine why this is.",
			record->name );
	}

	MX_DEBUG( 2,("%s: *driver_major_number = %d",
		fname, *driver_major_number));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sis3100_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_sis3100_open()";

	MX_VME *vme;
	MX_SIS3100 *sis3100;
	u_int32_t version_register, control_register;
	int os_status, vme_status, saved_errno;
	int device_node_major_number, device_node_minor_number;
	int driver_major_number;
	struct stat stat_struct;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_sis3100_get_pointers( vme, &sis3100, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is the requested filename a character device? */

	os_status = stat( sis3100->filename, &stat_struct );

	if ( os_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot get file status for SIS1100 device file '%s'.  "
		"Error code = %d, error message = '%s'",
			sis3100->filename, saved_errno,
			strerror( saved_errno ) );
	}

	if ( S_ISCHR( stat_struct.st_mode ) == 0 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"SIS1100 device file '%s' is not a character device node.",
			sis3100->filename );
	}

	/* Does this device node have the same major number as the
	 * Linux device node?
	 */

	device_node_major_number = major( stat_struct.st_rdev );
	device_node_minor_number = minor( stat_struct.st_rdev );

	mx_status = mxi_sis3100_get_driver_major_number( record,
							&driver_major_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
		("%s: device_node_major_number = %d, driver_major_number = %d",
			fname, device_node_major_number, driver_major_number));

	if ( device_node_major_number != driver_major_number ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The device node major and minor numbers (%d,%d) for SIS3100 "
		"device node '%s' do not match the driver major number %d "
		"reported by /proc/devices.  Are you sure you are using "
		"the correct device filename?",
			device_node_major_number,
			device_node_minor_number,
			sis3100->filename,
			driver_major_number );
	}

	/* Open the SIS1100 device. */

	sis3100->file_descriptor = open( sis3100->filename, O_RDWR, 
						sis3100->crate_number );

	if ( sis3100->file_descriptor < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot open SIS1100 device file '%s'.  "
		"Error code = %d, error message = '%s'",
			sis3100->filename, saved_errno,
			strerror( saved_errno ) );
	}

	/* Read the SIS3100 version information. */

	vme_status = s3100_control_read( sis3100->file_descriptor, 0x0,
							&version_register );

	if ( vme_status != 0 ) { return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Error reading the version register for VME controller '%s'.  VME status = %d",
			record->name, vme_status );
	}

#if MX_SIS3100_DEBUG
	MX_DEBUG(-2,("%s: version_register = %#lx",
				fname, (unsigned long) version_register));
#endif

	sis3100->version_register = (unsigned long) version_register;

	/* Turn on the VME controller's user LED to indicate that the
	 * driver is ready to be used.
	 */

#if MX_SIS3100_DEBUG
	MX_DEBUG(-2,("%s: Turning on user LED.", fname));
#endif

	control_register = 0x80;

	vme_status = s3100_control_write( sis3100->file_descriptor, 0x100,
							control_register );

	if ( vme_status != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Error turning on the user LED for VME controller '%s'.  VME status = %d",
			record->name, vme_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sis3100_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_sis3100_close()";

	MX_VME *vme;
	MX_SIS3100 *sis3100;
	u_int32_t control_register;
	int close_status, vme_status, saved_errno;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_sis3100_get_pointers( vme, &sis3100, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off the VME controller's user LED to indicate that the
	 * driver is no longer running.
	 */

#if MX_SIS3100_DEBUG
	MX_DEBUG(-2,("%s: Turning off user LED.", fname));
#endif

	control_register = 0x800000;

	vme_status = s3100_control_write( sis3100->file_descriptor, 0x100,
							control_register );

	if ( vme_status != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Error turning off the user LED for VME controller '%s'.  VME status = %d",
			record->name, vme_status );
	}

	if ( sis3100->file_descriptor >= 0 ) {
		close_status = close( sis3100->file_descriptor );

		if ( close_status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while closing SIS3100 device file '%s'."
		"for VME interface '%s'.  "
		"Error code = %d, error message = '%s'",
				sis3100->filename, vme->record->name,
				saved_errno, strerror( saved_errno ) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sis3100_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_sis3100_resynchronize()";

	MX_VME *vme;
	MX_SIS3100 *sis3100;
	int vme_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_sis3100_get_pointers( vme, &sis3100, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vme_status = vmesysreset( sis3100->file_descriptor );

	if ( vme_status != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Error resetting the VME bus for VME controller '%s'.  VME status = %d",
			record->name, vme_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*===============*/

MX_EXPORT mx_status_type
mxi_sis3100_input( MX_VME *vme )
{
	static const char fname[] = "mxi_sis3100_input()";

	MX_SIS3100 *sis3100;
	int vme_status;
	mx_status_type mx_status;

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_sis3100_get_pointers( vme, &sis3100, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_status = vme_A16D8_read( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					(u_int8_t *) vme->data_pointer );
			break;
		case MXF_VME_D16:
			vme_status = vme_A16D16_read( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					(u_int16_t *) vme->data_pointer );
			break;
		case MXF_VME_D32:
			vme_status = vme_A16D32_read( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					(u_int32_t *) vme->data_pointer );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
		break;
	case MXF_VME_A24:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_status = vme_A24D8_read( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					(u_int8_t *) vme->data_pointer );
			break;
		case MXF_VME_D16:
			vme_status = vme_A24D16_read( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					(u_int16_t *) vme->data_pointer );
			break;
		case MXF_VME_D32:
			vme_status = vme_A24D32_read( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					(u_int32_t *) vme->data_pointer );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
		break;
	case MXF_VME_A32:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_status = vme_A32D8_read( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					(u_int8_t *) vme->data_pointer );
			break;
		case MXF_VME_D16:
			vme_status = vme_A32D16_read( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					(u_int16_t *) vme->data_pointer );
			break;
		case MXF_VME_D32:
			vme_status = vme_A32D32_read( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					(u_int32_t *) vme->data_pointer );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"A%lu D%lu address = %p", vme->address_mode, vme->data_size,
		vme->data_pointer );
#endif

	MX_SIS3100_DEBUG_PRINT;

	if ( vme_status == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error reading from VME interface '%s', crate %lu, "
			"A%lu, D%lu, address %#lx, "
			"VME status = %#x, error message = '%s'",
				vme->record->name, vme->crate,
				vme->address_mode, vme->data_size,
				vme->address, vme_status,
				mxi_sis3100_strerror( vme_status ) );

		if ( vme->vme_flags & MXF_VME_IGNORE_BUS_ERRORS ) {
			if ( vme_status == RE_BERR ) {
				return MX_SUCCESSFUL_RESULT;
			}
		}
		return mx_status;
	}
}

MX_EXPORT mx_status_type
mxi_sis3100_output( MX_VME *vme )
{
	static const char fname[] = "mxi_sis3100_output()";

	MX_SIS3100 *sis3100;
	int vme_status;
	mx_status_type mx_status;

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_sis3100_get_pointers( vme, &sis3100, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_status = vme_A16D8_write( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					*((u_int8_t *) vme->data_pointer) );
			break;
		case MXF_VME_D16:
			vme_status = vme_A16D16_write( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					*((u_int16_t *) vme->data_pointer) );
			break;
		case MXF_VME_D32:
			vme_status = vme_A16D32_write( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					*((u_int32_t *) vme->data_pointer) );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
		break;
	case MXF_VME_A24:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_status = vme_A24D8_write( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					*((u_int8_t *) vme->data_pointer) );
			break;
		case MXF_VME_D16:
			vme_status = vme_A24D16_write( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					*((u_int16_t *) vme->data_pointer) );
			break;
		case MXF_VME_D32:
			vme_status = vme_A24D32_write( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					*((u_int32_t *) vme->data_pointer) );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
		break;
	case MXF_VME_A32:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_status = vme_A32D8_write( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					*((u_int8_t *) vme->data_pointer) );
			break;
		case MXF_VME_D16:
			vme_status = vme_A32D16_write( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					*((u_int16_t *) vme->data_pointer) );
			break;
		case MXF_VME_D32:
			vme_status = vme_A32D32_write( sis3100->file_descriptor,
					(u_int32_t) vme->address,
					*((u_int32_t *) vme->data_pointer) );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"A%lu D%lu address = %p", vme->address_mode, vme->data_size,
		vme->data_pointer );
#endif

	MX_SIS3100_DEBUG_PRINT;

	if ( vme_status == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error writing to VME interface '%s', crate %lu, "
			"A%lu, D%lu, address %#lx, "
			"VME status = %#x, error message = '%s'",
				vme->record->name, vme->crate,
				vme->address_mode, vme->data_size,
				vme->address, vme_status,
				mxi_sis3100_strerror( vme_status ) );

		if ( vme->vme_flags & MXF_VME_IGNORE_BUS_ERRORS ) {
			if ( vme_status == RE_BERR ) {
				return MX_SUCCESSFUL_RESULT;
			}
		}
		return mx_status;
	}
}

MX_EXPORT mx_status_type
mxi_sis3100_multi_input( MX_VME *vme )
{
	static const char fname[] = "mxi_sis3100_multi_input()";

	MX_SIS3100 *sis3100;
	u_int32_t vme_address, read_address_increment;
	u_int8_t *vme_8bit_data_pointer;
	u_int16_t *vme_16bit_data_pointer;
	u_int32_t *vme_32bit_data_pointer;
	u_int32_t num_values_transferred;
	unsigned long i;
	int vme_status;
	mx_status_type mx_status;

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif
	vme_status = 0;

	mx_status = mxi_sis3100_get_pointers( vme, &sis3100, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	vme_address = (u_int32_t) vme->address;
	read_address_increment = vme->read_address_increment;
	num_values_transferred = 0;

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_8bit_data_pointer = (u_int8_t *) vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A16D8_read(
						sis3100->file_descriptor,
						vme_address,
						vme_8bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += read_address_increment;
				vme_8bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D16:
			vme_16bit_data_pointer = (u_int16_t *)vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A16D16_read(
						sis3100->file_descriptor,
						vme_address,
						vme_16bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += read_address_increment;
				vme_16bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D32:
			vme_32bit_data_pointer = (u_int32_t *)vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A16D32_read(
						sis3100->file_descriptor,
						vme_address,
						vme_32bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += read_address_increment;
				vme_32bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
	case MXF_VME_A24:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_8bit_data_pointer = (u_int8_t *) vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A24D8_read(
						sis3100->file_descriptor,
						vme_address,
						vme_8bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += read_address_increment;
				vme_8bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D16:
			vme_16bit_data_pointer = (u_int16_t *)vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A24D16_read(
						sis3100->file_descriptor,
						vme_address,
						vme_16bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += read_address_increment;
				vme_16bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D32:
			/* A24, D32 has optimized routines. */

			if ( read_address_increment == 0 ) {
				vme_status = vme_A24BLT32FIFO_read(
					sis3100->file_descriptor,
					vme_address,
					(u_int32_t *) vme->data_pointer,
					(u_int32_t) vme->num_values,
					&num_values_transferred );

			} else if ( read_address_increment == 4 ) {

				vme_status = vme_A24BLT32_read(
					sis3100->file_descriptor,
					vme_address,
					(u_int32_t *) vme->data_pointer,
					(u_int32_t) vme->num_values,
					&num_values_transferred );
			} else {
				vme_32bit_data_pointer = (u_int32_t *)
							vme->data_pointer;

				for ( i = 0; i < vme->num_values; i++ ) {
					vme_status = vme_A24D32_read(
						sis3100->file_descriptor,
						vme_address,
						vme_32bit_data_pointer );

					if ( vme_status != 0 )
						break;	/*Exit the for() loop.*/

					vme_address += read_address_increment;
					vme_32bit_data_pointer++;
					num_values_transferred++;
				}
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
		break;
	case MXF_VME_A32:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_8bit_data_pointer = (u_int8_t *) vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A32D8_read(
						sis3100->file_descriptor,
						vme_address,
						vme_8bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += read_address_increment;
				vme_8bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D16:
			vme_16bit_data_pointer = (u_int16_t *)vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A32D16_read(
						sis3100->file_descriptor,
						vme_address,
						vme_16bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += read_address_increment;
				vme_16bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D32:
			/* A32, D32 has optimized routines. */

			if ( read_address_increment == 0 ) {
				vme_status = vme_A32BLT32FIFO_read(
					sis3100->file_descriptor,
					vme_address,
					(u_int32_t *) vme->data_pointer,
					(u_int32_t) vme->num_values,
					&num_values_transferred );

			} else if ( read_address_increment == 4 ) {

				vme_status = vme_A32BLT32_read(
					sis3100->file_descriptor,
					vme_address,
					(u_int32_t *) vme->data_pointer,
					(u_int32_t) vme->num_values,
					&num_values_transferred );
			} else {
				vme_32bit_data_pointer = (u_int32_t *)
							vme->data_pointer;

				for ( i = 0; i < vme->num_values; i++ ) {
					vme_status = vme_A32D32_read(
						sis3100->file_descriptor,
						vme_address,
						vme_32bit_data_pointer );

					if ( vme_status != 0 )
						break;	/*Exit the for() loop.*/

					vme_address += read_address_increment;
					vme_32bit_data_pointer++;
					num_values_transferred++;
				}
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"A%lu D%lu address = %p", vme->address_mode, vme->data_size,
		vme->data_pointer );
#endif

	MX_SIS3100_MULTI_DEBUG_PRINT;

	if ( vme_status == 0 ) {
		if ( num_values_transferred != (u_int32_t) vme->num_values ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Only %lu values were transferred out of %lu values requested "
		"for VME interface '%s'.",
				(unsigned long) num_values_transferred,
				vme->num_values,
				vme->record->name );
		} else {
			return MX_SUCCESSFUL_RESULT;
		}
	} else {
		mx_status = mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error reading from VME interface '%s', crate %lu, "
			"A%lu, D%lu, address %#lx, "
			"VME status = %#x, error message = '%s'",
				vme->record->name, vme->crate,
				vme->address_mode, vme->data_size,
				vme->address, vme_status,
				mxi_sis3100_strerror( vme_status ) );

		if ( vme->vme_flags & MXF_VME_IGNORE_BUS_ERRORS ) {
			if ( vme_status == RE_BERR ) {
				return MX_SUCCESSFUL_RESULT;
			}
		}
		return mx_status;
	}
}

MX_EXPORT mx_status_type
mxi_sis3100_multi_output( MX_VME *vme )
{
	static const char fname[] = "mxi_sis3100_multi_output()";

	MX_SIS3100 *sis3100;
	u_int32_t vme_address, write_address_increment;
	u_int8_t *vme_8bit_data_pointer;
	u_int16_t *vme_16bit_data_pointer;
	u_int32_t *vme_32bit_data_pointer;
	u_int32_t num_values_transferred;
	unsigned long i;
	int vme_status;
	mx_status_type mx_status;

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif
	vme_status = 0;

	mx_status = mxi_sis3100_get_pointers( vme, &sis3100, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif
	vme_address = (u_int32_t) vme->address;
	write_address_increment = vme->write_address_increment;
	num_values_transferred = 0;

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_8bit_data_pointer = (u_int8_t *) vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A16D8_write(
						sis3100->file_descriptor,
						vme_address,
						*vme_8bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += write_address_increment;
				vme_8bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D16:
			vme_16bit_data_pointer = (u_int16_t *)vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A16D16_write(
						sis3100->file_descriptor,
						vme_address,
						*vme_16bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += write_address_increment;
				vme_16bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D32:
			vme_32bit_data_pointer = (u_int32_t *)vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A16D32_write(
						sis3100->file_descriptor,
						vme_address,
						*vme_32bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += write_address_increment;
				vme_32bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
		break;
	case MXF_VME_A24:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_8bit_data_pointer = (u_int8_t *) vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A24D8_write(
						sis3100->file_descriptor,
						vme_address,
						*vme_8bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += write_address_increment;
				vme_8bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D16:
			vme_16bit_data_pointer = (u_int16_t *)vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A24D16_write(
						sis3100->file_descriptor,
						vme_address,
						*vme_16bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += write_address_increment;
				vme_16bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D32:
			if ( write_address_increment == 4 ) {

				/* A24, D32 has an optimized routine. */

				vme_status = vme_A24BLT32_write(
					sis3100->file_descriptor,
					vme_address,
					(u_int32_t *) vme->data_pointer,
					(u_int32_t) vme->num_values,
					&num_values_transferred );
			} else {
				vme_32bit_data_pointer = (u_int32_t *)
							    vme->data_pointer;

				for ( i = 0; i < vme->num_values; i++ ) {
					vme_status = vme_A24D32_write(
						sis3100->file_descriptor,
						vme_address,
						*vme_32bit_data_pointer );

					if ( vme_status != 0 )
						break; /* Exit the for() loop.*/

					vme_address += write_address_increment;
					vme_32bit_data_pointer++;
					num_values_transferred++;
				}
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
		break;
	case MXF_VME_A32:
		switch( vme->data_size ) {
		case MXF_VME_D8:
			vme_8bit_data_pointer = (u_int8_t *) vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A32D8_write(
						sis3100->file_descriptor,
						vme_address,
						*vme_8bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += write_address_increment;
				vme_8bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D16:
			vme_16bit_data_pointer = (u_int16_t *)vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				vme_status = vme_A32D16_write(
						sis3100->file_descriptor,
						vme_address,
						*vme_16bit_data_pointer );

				if ( vme_status != 0 )
					break;	/* Exit the for() loop. */

				vme_address += write_address_increment;
				vme_16bit_data_pointer++;
				num_values_transferred++;
			}
			break;
		case MXF_VME_D32:
			if ( vme->write_address_increment == 4 ) {

				/* A32, D32 has an optimized routine. */

				vme_status = vme_A32BLT32_write(
					sis3100->file_descriptor,
					vme_address,
					(u_int32_t *) vme->data_pointer,
					(u_int32_t) vme->num_values,
					&num_values_transferred );
			} else {
				vme_32bit_data_pointer = (u_int32_t *)
							    vme->data_pointer;

				for ( i = 0; i < vme->num_values; i++ ) {
					vme_status = vme_A32D32_write(
						sis3100->file_descriptor,
						vme_address,
						*vme_32bit_data_pointer );

					if ( vme_status != 0 )
						break; /* Exit the for() loop.*/

					vme_address += write_address_increment;
					vme_32bit_data_pointer++;
					num_values_transferred++;
				}
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

#if MX_SIS3100_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"A%lu D%lu address = %p", vme->address_mode, vme->data_size,
		vme->data_pointer );
#endif

	MX_SIS3100_MULTI_DEBUG_PRINT;

	if ( vme_status == 0 ) {
		if ( num_values_transferred != (u_int32_t) vme->num_values ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Only %lu values were transferred out of %lu values requested "
		"for VME interface '%s'.",
				(unsigned long) num_values_transferred,
				vme->num_values,
				vme->record->name );
		} else {
			return MX_SUCCESSFUL_RESULT;
		}
	} else {
		mx_status = mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error writing to VME interface '%s', crate %lu, "
			"A%lu, D%lu, address %#lx, "
			"VME status = %#x, error message = '%s'",
				vme->record->name, vme->crate,
				vme->address_mode, vme->data_size,
				vme->address, vme_status,
				mxi_sis3100_strerror( vme_status ) );

		if ( vme->vme_flags & MXF_VME_IGNORE_BUS_ERRORS ) {
			if ( vme_status == RE_BERR ) {
				return MX_SUCCESSFUL_RESULT;
			}
		}
		return mx_status;
	}
}

MX_EXPORT mx_status_type
mxi_sis3100_get_parameter( MX_VME *vme )
{
	static const char fname[] = "mxi_sis3100_get_parameter()";

	MX_SIS3100 *sis3100;
	mx_status_type mx_status;

	mx_status = mxi_sis3100_get_pointers( vme, &sis3100, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for VME interface '%s' for parameter type '%s' (%d).",
		fname, vme->record->name,
		mx_get_field_label_string( vme->record, vme->parameter_type ),
		vme->parameter_type));

	switch( vme->parameter_type ) {
	case MXLV_VME_READ_ADDRESS_INCREMENT:
	case MXLV_VME_WRITE_ADDRESS_INCREMENT:
		/* The controller itself does not have a place to store
		 * this information, so we just return the value we have
		 * stored locally.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%d) is not supported by the MX driver "
		"for VME interface '%s'.",
			mx_get_field_label_string(
				vme->record, vme->parameter_type ),
			vme->parameter_type, vme->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sis3100_set_parameter( MX_VME *vme )
{
	static const char fname[] = "mxi_sis3100_set_parameter()";

	MX_SIS3100 *sis3100;
	mx_status_type mx_status;

	mx_status = mxi_sis3100_get_pointers( vme, &sis3100, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for VME interface '%s' for parameter type '%s' (%d).",
		fname, vme->record->name,
		mx_get_field_label_string( vme->record, vme->parameter_type ),
		vme->parameter_type));

	switch( vme->parameter_type ) {
	case MXLV_VME_READ_ADDRESS_INCREMENT:
	case MXLV_VME_WRITE_ADDRESS_INCREMENT:
		/* The controller itself does not have a place to store
		 * this information, so we just store the information
		 * locally.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%d) is not supported by the MX driver "
		"for VME interface '%s'.",
			mx_get_field_label_string(
				vme->record, vme->parameter_type ),
			vme->parameter_type, vme->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_SIS3100 */
