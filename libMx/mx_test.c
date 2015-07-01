/*
 * Name:    mx_test.c
 *
 * Purpose: Some routines commonly used for testing MX.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 2002, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_hrt.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "mx_test.h"

#define MX_TEST_MAX_OUTPUTS	4

static MX_RECORD *mx_test_record[ MX_TEST_MAX_OUTPUTS ];

MX_EXPORT mx_status_type
mx_test_setup_outputs( MX_RECORD *record_list )
{
	static const char fname[] = "mx_test_setup_outputs()";

	char record_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	int i;

	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	for ( i = 0; i < MX_TEST_MAX_OUTPUTS; i++ ) {

		snprintf( record_name, sizeof(record_name), "mx_test%d", i );

		/* Find the specified record, or store a NULL to indicate
		 * that the record does not exist.
		 */

		mx_test_record[i] = mx_get_record( record_list, record_name );

		if ( mx_test_record[i] == (MX_RECORD *) NULL ) {
		    mx_info( "%s: mx_test_record[%d] = NULL", fname, i );
		} else {
		    if ( ( mx_test_record[i]->mx_superclass != MXR_DEVICE )
		      || ( mx_test_record[i]->mx_class != MXC_DIGITAL_OUTPUT ))
		    {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Test record '%s' is not a digital output record.",
				record_name );
		    } else {
			mx_info( "%s: mx_test_record[%d] = '%s'",
				fname, i, mx_test_record[i]->name );
		    }
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_test_output_on( int output_number )
{
	static const char fname[] = "mx_test_output_on()";

	MX_RECORD *digital_output_record;
	mx_status_type mx_status;

	digital_output_record = mx_test_record[ output_number ];

	if ( digital_output_record == (MX_RECORD *) NULL ) {
		mx_warning( "%s: Digital output %d is not yet configured.",
			fname, output_number );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_digital_output_write( digital_output_record, 1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_test_output_off( int output_number )
{
	static const char fname[] = "mx_test_output_off()";

	MX_RECORD *digital_output_record;
	mx_status_type mx_status;

	digital_output_record = mx_test_record[ output_number ];

	if ( digital_output_record == (MX_RECORD *) NULL ) {
		mx_warning( "%s: Digital output %d is not yet configured.",
			fname, output_number );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_digital_output_write( digital_output_record, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_test_pulse( int output_number, unsigned long pulse_length_in_microseconds )
{
	static const char fname[] = "mx_test_pulse()";

	MX_RECORD *digital_output_record;
	mx_status_type mx_status;

	digital_output_record = mx_test_record[ output_number ];

	if ( digital_output_record == (MX_RECORD *) NULL ) {
		mx_warning( "%s: Digital output %d is not yet configured.",
			fname, output_number );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_digital_output_write( digital_output_record, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pulse_length_in_microseconds > 0 ) {
		mx_udelay( pulse_length_in_microseconds );
	}

	mx_status = mx_digital_output_write( digital_output_record, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_test_dip( int output_number, unsigned long pulse_length_in_microseconds )
{
	static const char fname[] = "mx_test_dip()";

	MX_RECORD *digital_output_record;
	mx_status_type mx_status;

	digital_output_record = mx_test_record[ output_number ];

	if ( digital_output_record == (MX_RECORD *) NULL ) {
		mx_warning( "%s: Digital output %d is not yet configured.",
			fname, output_number );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_digital_output_write( digital_output_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pulse_length_in_microseconds > 0 ) {
		mx_udelay( pulse_length_in_microseconds );
	}

	mx_status = mx_digital_output_write( digital_output_record, 1 );

	return mx_status;
}

