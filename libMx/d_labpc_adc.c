/*
 * Name:    d_labpc_adc.c
 *
 * Purpose: MX driver to control the ADC ports on a National Instruments
 *          Lab-PC+ data acquisition card under Linux.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004 Illinois Institute of Technology
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

/* Initialize the Lab-PC+ ADC driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_labpc_adc_record_function_list = {
	NULL,
	mxd_labpc_adc_create_record_structures,
	mx_analog_input_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_labpc_adc_open,
	mxd_labpc_adc_close
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_labpc_adc_analog_input_function_list = {
	mxd_labpc_adc_read
};

MX_RECORD_FIELD_DEFAULTS mxd_labpc_adc_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_LABPC_ADC_STANDARD_FIELDS
};

long mxd_labpc_adc_num_record_fields = 
		sizeof( mxd_labpc_adc_record_field_defaults )
		/ sizeof( mxd_labpc_adc_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_labpc_adc_rfield_def_ptr
			= &mxd_labpc_adc_record_field_defaults[0];

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_labpc_adc_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_labpc_adc_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_LABPC_ADC *labpc_adc;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        labpc_adc = (MX_LABPC_ADC *) malloc( sizeof(MX_LABPC_ADC) );

        if ( labpc_adc == (MX_LABPC_ADC *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_LABPC_ADC structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = labpc_adc;
        record->class_specific_function_list
                                = &mxd_labpc_adc_analog_input_function_list;

        analog_input->record = record;

	/* Raw analog input values are stored as longs. */

	analog_input->subclass = MXT_AIN_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_adc_open( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_adc_open()";

	MX_ANALOG_INPUT *adc;
	MX_LABPC_ADC *labpc_adc;
	int fd, result, saved_errno;


	MX_DEBUG(2, ("mxd_labpc_adc_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	adc = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( adc == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ANALOG_INPUT pointer for record '%s' is NULL.", record->name);
	}

	labpc_adc = (MX_LABPC_ADC *) (record->record_type_struct);

	if ( labpc_adc == (MX_LABPC_ADC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_ADC pointer is NULL.");
	}

	if ( labpc_adc->filename == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"LabPC+ ADC filename is NULL.");
	}

	labpc_adc->file_handle = open( labpc_adc->filename, O_RDWR );

	if ( labpc_adc->file_handle == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to open '%s' failed.  Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	fd = labpc_adc->file_handle;

	/* Select the channel before doing anything else. */

	result = ioctl( fd, ADC_SET_CHANNEL, (long) (labpc_adc->channel) );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to select ADC channel for '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	/* Set the scan method to "off". */

	result = ioctl( fd, ADC_SET_SCAN, 0 );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to do ioctl(,ADC_SET_SCAN,) on '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	/* Setup DMA. */

	result = ioctl( fd, ADC_SET_DMA, labpc_adc->use_dma );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to do ioctl(,ADC_SET_DMA,) on '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	/* Gain */

	result = ioctl( fd, ADC_SET_GAIN, (long) (labpc_adc->gain) );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to do ioctl(,ADC_SET_GAIN,) on '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	/* Input polarity. */

	result = ioctl( fd, ADC_SET_POLARITY, labpc_adc->use_unipolar_input );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to do ioctl(,ADC_SET_POLARITY,) on '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	/* Use a fixed timebase. */

	result = ioctl( fd, ADC_SET_TIMEBASE, 2 );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to do ioctl(,ADC_SET_TIMEBASE,) on '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	/* Trigger mode. */

	result = ioctl( fd, ADC_SET_TRIGGER_MODE,
				(long) (labpc_adc->trigger_type) );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to do ioctl(,ADC_SET_TRIGGER_MODE,) on '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	/* Clock */

	result = ioctl( fd, ADC_SET_CLOCK, 0 );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to do ioctl(,ADC_SET_CLOCK,) on '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	/* Single-ended or differential */

	result = ioctl( fd, ADC_SET_CONNECTION,
			(long) (labpc_adc->use_differential_input)  );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to do ioctl(,ADC_SET_CONNECTION,) on '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	/* Fixed buffer size. */

	result = ioctl( fd, ADC_SET_BUFSIZE, 1 );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to do ioctl(,ADC_SET_BUFSIZE,) on '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_adc_close( MX_RECORD *record )
{
	const char fname[] = "mxd_labpc_adc_open()";

	MX_ANALOG_INPUT *adc;
	MX_LABPC_ADC *labpc_adc;
	int result, saved_errno;

	MX_DEBUG(2, ("mxd_labpc_adc_close() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	adc = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( adc == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ANALOG_INPUT pointer for record '%s' is NULL.", record->name);
	}

	labpc_adc = (MX_LABPC_ADC *) (record->record_type_struct);

	if ( labpc_adc == (MX_LABPC_ADC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_ADC pointer is NULL.");
	}

	if ( labpc_adc->filename == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"LabPC+ ADC filename is NULL.");
	}

	if ( labpc_adc->file_handle < 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"LabPC+ ADC '%s' was not open.", labpc_adc->filename );
	}

	result = close( labpc_adc->file_handle );

	if ( result == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to close '%s' failed.  Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labpc_adc_read( MX_ANALOG_INPUT *adc )
{
	const char fname[] = "mxd_labpc_adc_read()";

	MX_LABPC_ADC *labpc_adc;
	int status, saved_errno;
	mx_sint16_type data;

	MX_DEBUG(2, ("%s invoked.", fname));

	labpc_adc = (MX_LABPC_ADC *) (adc->record->record_type_struct);

	if ( labpc_adc == (MX_LABPC_ADC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LABPC_ADC pointer is NULL.");
	}

	/* First select the channel. */

	status = ioctl( labpc_adc->file_handle, ADC_SET_CHANNEL,
					(long) (labpc_adc->channel) );

	if ( status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to select ADC channel for '%s' failed.\n"
		"Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	/* Now read out the voltage. */

	status = read( labpc_adc->file_handle, &data, sizeof(mx_sint16_type) );

	if ( status == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Attempt to read from '%s' failed.  Errno = %d, error text = '%s'",
			labpc_adc->filename, saved_errno, 
			strerror( saved_errno ) );
	}

	adc->raw_value.long_value = (long) data;

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_LABPC */

#endif /* OS_LINUX */

