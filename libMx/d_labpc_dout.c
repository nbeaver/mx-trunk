/*
 * Name:    d_labpc_dout.c
 *
 * Purpose: MX driver to control the digital output ports on a
 *          National Instruments Lab-PC+ data acquisition card under Linux.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#ifdef OS_LINUX		/* This driver is only useable under Linux. */

#include "mxconfig.h"

#if HAVE_LABPC		/* Is the LabPC+ driver software installed? */

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_driver.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>

#include "labpc.h"
#include "d_labpc.h"

/* Initialize the Lab-PC+ digital output driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_labpc_dout_record_function_list = {
	mxd_labpc_dout_initialize_type,
	mxd_labpc_dout_create_record_structures,
	mxd_labpc_dout_finish_record_initialization,
	mxd_labpc_dout_delete_record,
	mxd_labpc_dout_print_structure,
	mxd_labpc_dout_read_parms_from_hardware,
	mxd_labpc_dout_write_parms_to_hardware,
	mxd_labpc_dout_open,
	mxd_labpc_dout_close
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_labpc_dout_digital_output_function_list = {
	mxd_labpc_dout_read,
	mxd_labpc_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_labpc_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_LABPC_DOUT_STANDARD_FIELDS
};

mx_length_type mxd_labpc_dout_num_record_fields
		= sizeof( mxd_labpc_dout_record_field_defaults )
			/ sizeof( mxd_labpc_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_labpc_dout_rfield_def_ptr
			= &mxd_labpc_dout_record_field_defaults[0];

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_labpc_dout_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dout_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_labpc_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_LABPC_DOUT *labpc_dout;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
				malloc( sizeof(MX_DIGITAL_OUTPUT) );

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        labpc_dout = (MX_LABPC_DOUT *) malloc( sizeof(MX_LABPC_DOUT) );

        if ( labpc_dout == (MX_LABPC_DOUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_LABPC_DOUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = labpc_dout;
        record->class_specific_function_list
                                = &mxd_labpc_dout_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dout_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dout_delete_record( MX_RECORD *record )
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
mxd_labpc_dout_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_dout_print_structure()";

	MX_DIGITAL_OUTPUT *dout;
	MX_LABPC_DOUT *labpc_dout;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	dout = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( dout == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	labpc_dout = record->record_type_struct;

	if ( labpc_dout == (MX_LABPC_DOUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_LABPC_DOUT pointer for record '%s' is NULL.", record->name);
	}

	status = mxd_labpc_dout_read( dout );

	if ( status.code != MXE_SUCCESS ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to read current value for record '%s' failed.",
			record->name );
	}

	fprintf(file, "DIGITAL_OUTPUT parameters for digital output '%s':\n",
			record->name);
	fprintf(file, "  Digital output type = LABPC_DOUT.\n\n");

	fprintf(file, "  name            = %s\n", record->name);
	fprintf(file, "  value           = %ld (0x%02lx)\n",
						dout->value, dout->value);
	fprintf(file, "  port            = %s\n", labpc_dout->filename );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dout_read_parms_from_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_dout_read_parms_from_hardware()";

	MX_DIGITAL_OUTPUT *dout;
	mx_status_type status;

	MX_DEBUG(2, ("mxd_labpc_dout_read_parms_from_hardware()."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	dout = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( dout == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	status = mxd_labpc_dout_read( dout );

	return status;
}

MX_EXPORT mx_status_type
mxd_labpc_dout_write_parms_to_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_dout_write_parms_to_hardware()";

	MX_DIGITAL_OUTPUT *dout;
	mx_status_type status;

	MX_DEBUG(2, ("mxd_labpc_dout_write_parms_to_hardware()."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	dout = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( dout == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	/* Write the digital output value. */

	status = mxd_labpc_dout_write( dout );

	return status;
}

MX_EXPORT mx_status_type
mxd_labpc_dout_open( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_dout_open()";

	MX_DIGITAL_OUTPUT *dout;
	MX_LABPC_DOUT *labpc_dout;
	int saved_errno;

	MX_DEBUG(2, ("mxd_labpc_dout_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	dout = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( dout == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	labpc_dout = (MX_LABPC_DOUT *) (record->record_type_struct);

	if ( labpc_dout == (MX_LABPC_DOUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DOUT pointer is NULL.");
	}

	if ( labpc_dout->filename == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"LabPC+ digital output filename is NULL.");
	}

	labpc_dout->file_handle = open( labpc_dout->filename, O_RDWR );

	if ( labpc_dout->file_handle == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to open '%s' failed.  Errno = %d, error text = '%s'",
			labpc_dout->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dout_close( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_dout_open()";

	MX_DIGITAL_OUTPUT *dout;
	MX_LABPC_DOUT *labpc_dout;
	int result, saved_errno;

	MX_DEBUG(2, ("mxd_labpc_dout_close() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	dout = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( dout == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	labpc_dout = (MX_LABPC_DOUT *) (record->record_type_struct);

	if ( labpc_dout == (MX_LABPC_DOUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DOUT pointer is NULL.");
	}

	if ( labpc_dout->filename == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"LabPC+ digital output filename is NULL.");
	}

	if ( labpc_dout->file_handle < 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"LabPC+ digital output '%s' was not open.", labpc_dout->filename );
	}

	result = close( labpc_dout->file_handle );

	if ( result == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to close '%s' failed.  Errno = %d, error text = '%s'",
			labpc_dout->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dout_read( MX_DIGITAL_OUTPUT *dout )
{
	/* Note:  The most recent release of the LabPC+ driver (version 0.5)
	 * determines whether or not a given port is a digital input port
	 * or a digital output port by whether or not you are attempting
	 * to read from it or write to it.  Thus, it is not safe to attempt
	 * to read from a digital output port since that will automatically
	 * reprogram it to be a digital input port.  The safest way to deal
	 * with this is to not attempt a read here and leave dout->value
	 * alone.  The higher level code will then simply return to the
	 * caller the value of dout->value that was saved during the last
	 * call to mx_digital_output_write().  That is the reason the 
	 * following code is currently commented out.
	 */
#if 0
	const char fname[] = "mxd_labpc_dout_read()";

	MX_LABPC_DOUT *labpc_dout;
	int status, saved_errno;
	unsigned long value;
	uint8_t data;

	MX_DEBUG(2, ("%s invoked.", fname));

	labpc_dout = (MX_LABPC_DOUT *) (dout->record->record_type_struct);

	if ( labpc_dout == (MX_LABPC_DOUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DOUT pointer is NULL.");
	}

	status = read( labpc_dout->file_handle, &data, sizeof(uint8_t) );

	if ( status == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Attempt to read from '%s' failed.  Errno = %d, error text = '%s'",
			labpc_dout->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	value = (unsigned long) data;

	value &= 0xff;		/* Mask off any high order bits. */

	dout->value = value;
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dout_write( MX_DIGITAL_OUTPUT *dout )
{
	const char fname[] = "mxd_labpc_dout_write()";

	MX_LABPC_DOUT *labpc_dout;
	int status, saved_errno;
	int8_t data;

	MX_DEBUG(2, ("%s invoked.", fname));

	labpc_dout = (MX_LABPC_DOUT *) (dout->record->record_type_struct);

	if ( labpc_dout == (MX_LABPC_DOUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DOUT pointer is NULL.");
	}

	data = (int8_t) dout->value;

	data &= 0xff;		/* Mask off any high order bits. */

	status = write( labpc_dout->file_handle, &data, sizeof(int8_t));

	if ( status == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Attempt to write to '%s' failed.  Errno = %d, error text = '%s'",
			labpc_dout->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_LABPC */

#endif /* OS_LINUX */

