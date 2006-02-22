/*
 * Name:    d_labpc_din.c
 *
 * Purpose: MX driver to control the digital input ports on a
 *          National Instruments Lab-PC+ data acquisition card under Linux.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
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

/* Initialize the Lab-PC+ digital input driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_labpc_din_record_function_list = {
	mxd_labpc_din_initialize_type,
	mxd_labpc_din_create_record_structures,
	mxd_labpc_din_finish_record_initialization,
	mxd_labpc_din_delete_record,
	mxd_labpc_din_print_structure,
	mxd_labpc_din_read_parms_from_hardware,
	mxd_labpc_din_write_parms_to_hardware,
	mxd_labpc_din_open,
	mxd_labpc_din_close
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_labpc_din_digital_input_function_list = {
	mxd_labpc_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_labpc_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_LABPC_DIN_STANDARD_FIELDS
};

long mxd_labpc_din_num_record_fields
		= sizeof( mxd_labpc_din_record_field_defaults )
			/ sizeof( mxd_labpc_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_labpc_din_rfield_def_ptr
			= &mxd_labpc_din_record_field_defaults[0];

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_labpc_din_initialize_type( long type )
{
	/* Nothing needed here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_din_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_labpc_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_LABPC_DIN *labpc_din;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        labpc_din = (MX_LABPC_DIN *) malloc( sizeof(MX_LABPC_DIN) );

        if ( labpc_din == (MX_LABPC_DIN *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_LABPC_DIN structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = labpc_din;
        record->class_specific_function_list
                                = &mxd_labpc_din_digital_input_function_list;

        digital_input->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_din_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_din_delete_record( MX_RECORD *record )
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
mxd_labpc_din_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_din_print_structure()";

	MX_DIGITAL_INPUT *din;
	MX_LABPC_DIN *labpc_din;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	din = (MX_DIGITAL_INPUT *) (record->record_class_struct);

	if ( din == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DIGITAL_INPUT pointer for record '%s' is NULL.", record->name);
	}

	labpc_din = record->record_type_struct;

	if ( labpc_din == (MX_LABPC_DIN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_LABPC_DIN pointer for record '%s' is NULL.", record->name);
	}

	status = mxd_labpc_din_read( din );

	if ( status.code != MXE_SUCCESS ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to read current value for record '%s' failed.",
			record->name );
	}

	fprintf(file, "DIGITAL_INPUT parameters for digital input '%s':\n",
			record->name);
	fprintf(file, "  Digital input type = LABPC_DIN.\n\n");

	fprintf(file, "  name            = %s\n", record->name);
	fprintf(file, "  value           = %ld (0x%02lx)\n",
						din->value, din->value);
	fprintf(file, "  port            = %s\n", labpc_din->filename );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_din_read_parms_from_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_din_read_parms_from_hardware()";

	MX_DIGITAL_INPUT *din;
	MX_LABPC_DIN *labpc_din;
	mx_status_type status;

	MX_DEBUG(2, ("mxd_labpc_din_read_parms_from_hardware()."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	din = (MX_DIGITAL_INPUT *) (record->record_class_struct);

	if ( din == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DIGITAL_INPUT pointer for record '%s' is NULL.", record->name);
	}

	labpc_din = (MX_LABPC_DIN *) (record->record_type_struct);

	if ( labpc_din == (MX_LABPC_DIN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DIN pointer is NULL.");
	}

	status = mxd_labpc_din_read( din );

	return status;
}

MX_EXPORT mx_status_type
mxd_labpc_din_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_din_open( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_din_open()";

	MX_LABPC_DIN *labpc_din;
	int saved_errno;

	MX_DEBUG(2, ("mxd_labpc_din_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	labpc_din = (MX_LABPC_DIN *) (record->record_type_struct);

	if ( labpc_din == (MX_LABPC_DIN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DIN pointer is NULL.");
	}

	if ( labpc_din->filename == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"LabPC+ digital input filename is NULL.");
	}

	labpc_din->file_handle = open( labpc_din->filename, O_RDWR );

	if ( labpc_din->file_handle == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to open '%s' failed.  Errno = %d, error text = '%s'",
			labpc_din->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_din_close( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_din_open()";

	MX_LABPC_DIN *labpc_din;
	int result, saved_errno;

	MX_DEBUG(2, ("mxd_labpc_din_close() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	labpc_din = (MX_LABPC_DIN *) (record->record_type_struct);

	if ( labpc_din == (MX_LABPC_DIN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DIN pointer is NULL.");
	}

	if ( labpc_din->filename == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"LabPC+ digital input filename is NULL.");
	}

	if ( labpc_din->file_handle < 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"LabPC+ digital input '%s' was not open.", labpc_din->filename );
	}

	result = close( labpc_din->file_handle );

	if ( result == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to close '%s' failed.  Errno = %d, error text = '%s'",
			labpc_din->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_din_read( MX_DIGITAL_INPUT *din )
{
	const char fname[] = "mxd_labpc_din_read()";

	MX_LABPC_DIN *labpc_din;
	int status, saved_errno;
	long value;
	uint8_t data;

	MX_DEBUG(2, ("%s invoked.", fname));

	labpc_din = (MX_LABPC_DIN *) (din->record->record_type_struct);

	if ( labpc_din == (MX_LABPC_DIN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DIN pointer is NULL.");
	}

	status = read( labpc_din->file_handle, &data, sizeof(uint8_t) );

	if ( status == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Attempt to read from '%s' failed.  Errno = %d, error text = '%s'",
			labpc_din->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	value = (long) data;

	value &= 0xff;		/* Mask off any high order bits. */

	/* Save this value in the record structure. */

	din->value = value;

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_LABPC */

#endif /* OS_LINUX */

