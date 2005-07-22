/*
 * Name:    d_labpc_dac.c
 *
 * Purpose: MX driver to control the DAC ports on a National Instruments
 *          Lab-PC+ data acquisition card under Linux.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
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
#include "mx_types.h"
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

/* Initialize the Lab-PC+ DAC driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_labpc_dac_record_function_list = {
	mxd_labpc_dac_initialize_type,
	mxd_labpc_dac_create_record_structures,
	mxd_labpc_dac_finish_record_initialization,
	mxd_labpc_dac_delete_record,
	mxd_labpc_dac_print_structure,
	mxd_labpc_dac_read_parms_from_hardware,
	mxd_labpc_dac_write_parms_to_hardware,
	mxd_labpc_dac_open,
	mxd_labpc_dac_close
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_labpc_dac_analog_output_function_list = {
	mxd_labpc_dac_read,
	mxd_labpc_dac_write
};

MX_RECORD_FIELD_DEFAULTS mxd_labpc_dac_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_LABPC_DAC_STANDARD_FIELDS
};

long mxd_labpc_dac_num_record_fields =
		sizeof( mxd_labpc_dac_record_field_defaults )
		/ sizeof( mxd_labpc_dac_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_labpc_dac_rfield_def_ptr
			= &mxd_labpc_dac_record_field_defaults[0];

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_labpc_dac_initialize_type( long type )
{
	/* Nothing needed here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dac_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_labpc_dac_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_LABPC_DAC *labpc_dac;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        labpc_dac = (MX_LABPC_DAC *) malloc( sizeof(MX_LABPC_DAC) );

        if ( labpc_dac == (MX_LABPC_DAC *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_LABPC_DAC structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = labpc_dac;
        record->class_specific_function_list
                                = &mxd_labpc_dac_analog_output_function_list;

        analog_output->record = record;

	/* Raw analog output values are stored as longs. */

	analog_output->subclass = MXT_AOU_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dac_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dac_delete_record( MX_RECORD *record )
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
mxd_labpc_dac_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_dac_print_structure()";

	MX_ANALOG_OUTPUT *dac;
	MX_LABPC_DAC *labpc_dac;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	dac = (MX_ANALOG_OUTPUT *) (record->record_class_struct);

	if ( dac == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ANALOG_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	labpc_dac = (MX_LABPC_DAC *) record->record_type_struct;

	if ( labpc_dac == (MX_LABPC_DAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_LABPC_DAC pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "ANALOG_OUTPUT parameters for analog output '%s':\n",
			record->name);
	fprintf(file, "  Analog output type = LABPC_DAC.\n\n");

	fprintf(file, "  name            = %s\n", record->name);
	fprintf(file, "  value           = %ld (%g %s)\n",
				dac->raw_value.long_value,
				dac->value, dac->units);
	fprintf(file, "  port            = %s\n", labpc_dac->filename );
	fprintf(file, "  scale           = %g\n", dac->scale );
	fprintf(file, "  offset          = %g\n", dac->offset );

	fprintf(file, "  unipolar output = %d\n",
				labpc_dac->use_unipolar_output);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dac_read_parms_from_hardware( MX_RECORD *record )
{
	/* The Lab-PC+ DAC driver cannot report back the current
	 * output setting, so there is nothing we can do here.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dac_write_parms_to_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_dac_write_parms_to_hardware()";

	MX_ANALOG_OUTPUT *dac;
	MX_LABPC_DAC *labpc_dac;
	int fd, result, saved_errno;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	dac = (MX_ANALOG_OUTPUT *) (record->record_class_struct);

	if ( dac == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ANALOG_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	labpc_dac = (MX_LABPC_DAC *) (record->record_type_struct);

	if ( labpc_dac == (MX_LABPC_DAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DAC pointer is NULL.");
	}

	fd = labpc_dac->file_handle;

	/* For now, we do not support the use of the waveform modes. */

	result = ioctl( fd, DAC_SET_MODE, DAC_WRITE_MODE );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to do ioctl(,DAC_SET_MODE,) on '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_dac->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	if ( labpc_dac->use_unipolar_output ) {
		result = ioctl( fd, DAC_SET_POLARITY, DAC_UNIPOLAR );
	} else {
		result = ioctl( fd, DAC_SET_POLARITY, DAC_BIPOLAR );
	}

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to do ioctl(,DAC_SET_POLARITY,) on '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_dac->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	dac->value = dac->offset + dac->scale * dac->raw_value.long_value;

	/* Write the DAC output value. */

	status = mxd_labpc_dac_write( dac );

	return status;
}

MX_EXPORT mx_status_type
mxd_labpc_dac_open( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_dac_open()";

	MX_ANALOG_OUTPUT *dac;
	MX_LABPC_DAC *labpc_dac;
	int saved_errno;

	MX_DEBUG(2, ("mxd_labpc_dac_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	dac = (MX_ANALOG_OUTPUT *) (record->record_class_struct);

	if ( dac == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ANALOG_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	labpc_dac = (MX_LABPC_DAC *) (record->record_type_struct);

	if ( labpc_dac == (MX_LABPC_DAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DAC pointer is NULL.");
	}

	if ( labpc_dac->filename == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"LabPC+ DAC filename is NULL.");
	}

	labpc_dac->file_handle = open( labpc_dac->filename, O_RDWR );

	if ( labpc_dac->file_handle == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to open '%s' failed.  Errno = %d, error text = '%s'",
			labpc_dac->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dac_close( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_dac_open()";

	MX_ANALOG_OUTPUT *dac;
	MX_LABPC_DAC *labpc_dac;
	int result, saved_errno;

	MX_DEBUG(2, ("mxd_labpc_dac_close() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	dac = (MX_ANALOG_OUTPUT *) (record->record_class_struct);

	if ( dac == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ANALOG_OUTPUT pointer for record '%s' is NULL.", record->name);
	}

	labpc_dac = (MX_LABPC_DAC *) (record->record_type_struct);

	if ( labpc_dac == (MX_LABPC_DAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DAC pointer is NULL.");
	}

	if ( labpc_dac->filename == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"LabPC+ DAC filename is NULL.");
	}

	if ( labpc_dac->file_handle < 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"LabPC+ DAC '%s' was not open.", labpc_dac->filename );
	}

	result = close( labpc_dac->file_handle );

	if ( result == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to close '%s' failed.  Errno = %d, error text = '%s'",
			labpc_dac->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dac_read( MX_ANALOG_OUTPUT *dac )
{
	/* The Lab-PC+ DAC driver cannot report back the current
	 * output setting, so we rely on the calling routine which is
	 * usually mx_analog_output_read() or mx_analog_output_read_raw()
	 * report back the most recently written value which is in
	 * the variable dac->raw_value.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_dac_write( MX_ANALOG_OUTPUT *dac )
{
	const char fname[] = "mxd_labpc_dac_write()";

	MX_LABPC_DAC *labpc_dac;
	int status, saved_errno;
	mx_sint16_type data;

	MX_DEBUG(2, ("%s invoked.", fname));

	labpc_dac = (MX_LABPC_DAC *) (dac->record->record_type_struct);

	if ( labpc_dac == (MX_LABPC_DAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_DAC pointer is NULL.");
	}

	data = (mx_sint16_type) dac->raw_value.long_value;

	if ( labpc_dac->use_unipolar_output ) {
		if ( data > 4095 )
			data = 4095;

		if ( data < 0 )
			data = 0;
	} else {
		if ( data > 2047 )
			data = 2047;

		if ( data < -2048 )
			data = -2048;
	}

	status = write( labpc_dac->file_handle, &data, sizeof(mx_sint16_type));

	if ( status == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Attempt to write to '%s' failed.  Errno = %d, error text = '%s'",
			labpc_dac->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_LABPC */

#endif /* OS_LINUX */

