/*
 * Name:    d_icplus_aio.c
 *
 * Purpose: MX analog input and output drivers to control the Oxford Danfysik
 *          IC PLUS high voltage and read the input current.
 *
 *          The analog output driver 'icplus_voltage' controls the high
 *          voltage setting, while the analog input driver 'icplus_current'
 *          returns the ion chamber current.
 *
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ICPLUS_AIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_amplifier.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "d_icplus.h"
#include "d_icplus_aio.h"

/* Initialize the ICPLUS analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_icplus_ain_record_function_list = {
	NULL,
	mxd_icplus_ain_create_record_structures,
	mx_analog_input_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_icplus_ain_open
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_icplus_ain_analog_input_function_list = {
	mxd_icplus_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_icplus_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_INT32_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_ICPLUS_AINPUT_STANDARD_FIELDS
};

mx_length_type mxd_icplus_ain_num_record_fields
		= sizeof( mxd_icplus_ain_record_field_defaults )
			/ sizeof( mxd_icplus_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_icplus_ain_rfield_def_ptr
			= &mxd_icplus_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_icplus_aout_record_function_list = {
	NULL,
	mxd_icplus_aout_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_icplus_aout_open
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_icplus_aout_analog_output_function_list = {
	mxd_icplus_aout_read,
	mxd_icplus_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_icplus_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_INT32_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_ICPLUS_AOUTPUT_STANDARD_FIELDS
};

mx_length_type mxd_icplus_aout_num_record_fields
		= sizeof( mxd_icplus_aout_record_field_defaults )
			/ sizeof( mxd_icplus_aout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_icplus_aout_rfield_def_ptr
			= &mxd_icplus_aout_record_field_defaults[0];

static mx_status_type
mxd_icplus_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_ICPLUS_AINPUT **icplus_ainput,
			MX_ICPLUS **icplus,
			const char *calling_fname )
{
	static const char fname[] = "mxd_icplus_ain_get_pointers()";

	MX_RECORD *icplus_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( icplus_ainput == (MX_ICPLUS_AINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ICPLUS_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( icplus == (MX_ICPLUS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ICPLUS pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*icplus_ainput = (MX_ICPLUS_AINPUT *)
				ainput->record->record_type_struct;

	if ( *icplus_ainput == (MX_ICPLUS_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ICPLUS_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	icplus_record = (*icplus_ainput)->icplus_record;

	if ( icplus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ICPLUS pointer for ICPLUS analog input "
		"record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( ( icplus_record->mx_type != MXT_AMP_ICPLUS )
	  && ( icplus_record->mx_type != MXT_AMP_QBPM ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"icplus_record '%s' for analog input '%s' is not an IC PLUS "
		"or QBPM record.  Instead, it is a '%s' record.",
			icplus_record->name, ainput->record->name,
			mx_get_driver_name( icplus_record ) );
	}

	*icplus = (MX_ICPLUS *) icplus_record->record_type_struct;

	if ( *icplus == (MX_ICPLUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ICPLUS pointer for IC PLUS record '%s' used by "
	"IC PLUS analog input record '%s' and passed by '%s' is NULL.",
			icplus_record->name,
			ainput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_icplus_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_ICPLUS_AOUTPUT **icplus_aoutput,
			MX_ICPLUS **icplus,
			const char *calling_fname )
{
	static const char fname[] = "mxd_icplus_aout_get_pointers()";

	MX_RECORD *icplus_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( icplus_aoutput == (MX_ICPLUS_AOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ICPLUS_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( icplus == (MX_ICPLUS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ICPLUS pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*icplus_aoutput = (MX_ICPLUS_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *icplus_aoutput == (MX_ICPLUS_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ICPLUS_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	icplus_record = (*icplus_aoutput)->icplus_record;

	if ( icplus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ICPLUS pointer for ICPLUS analog input "
		"record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( ( icplus_record->mx_type != MXT_AMP_ICPLUS )
	  && ( icplus_record->mx_type != MXT_AMP_QBPM ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"icplus_record '%s' for analog output '%s' is not an IC PLUS "
		"or QBPM record.  Instead, it is a '%s' record.",
			icplus_record->name, aoutput->record->name,
			mx_get_driver_name( icplus_record ) );
	}

	*icplus = (MX_ICPLUS *) icplus_record->record_type_struct;

	if ( *icplus == (MX_ICPLUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ICPLUS pointer for IC PLUS record '%s' used by "
	"IC PLUS analog input record '%s' and passed by '%s' is NULL.",
			icplus_record->name,
			aoutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_icplus_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_icplus_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_ICPLUS_AINPUT *icplus_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        icplus_ainput = (MX_ICPLUS_AINPUT *)
				malloc( sizeof(MX_ICPLUS_AINPUT) );

        if ( icplus_ainput == (MX_ICPLUS_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ICPLUS_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = icplus_ainput;
        record->class_specific_function_list
			= &mxd_icplus_ain_analog_input_function_list;

        analog_input->record = record;
	icplus_ainput->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_ain_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_icplus_ain_open()";

	MX_ANALOG_INPUT *ainput;
	MX_ICPLUS_AINPUT *icplus_ainput;
	MX_ICPLUS *icplus;
	char *value_name;
	int i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_icplus_ain_get_pointers( ainput,
					&icplus_ainput, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value_name = icplus_ainput->value_name;

	for ( i = 0; i < strlen( value_name ); i++ ) {
		if ( islower( (int)(value_name[i]) ) ) {
			value_name[i] = toupper( (int)(value_name[i]) );
		}
	}

	/* The ICPLUS monitor only supports reading the current. */

	if ( ainput->record->mx_type == MXT_AIN_ICPLUS ) {
		sprintf( icplus_ainput->command, ":READ%d:CURR?",
						icplus->address );
		return MX_SUCCESSFUL_RESULT;
	}

	/* The QPBM monitor supports many things. */

	if ( strcmp( value_name, "CURRALL" ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The 'CURRALL' value is not supported by the '%s' "
			"driver that record '%s' is currently configured "
			"for.  Use the 'qbpm_currall' driver instead.",
				mx_get_driver_name( record ), record->name );
	} else
	if ( ( strcmp( value_name, "CURR1" ) == 0 )
	  || ( strcmp( value_name, "CURR2" ) == 0 )
	  || ( strcmp( value_name, "CURR3" ) == 0 )
	  || ( strcmp( value_name, "CURR4" ) == 0 )
	  || ( strcmp( value_name, "POSX" ) == 0 )
	  || ( strcmp( value_name, "POSY" ) == 0 ) )
	{
		sprintf( icplus_ainput->command, ":READ%d:%s?",
					icplus->address, value_name );
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Requested value '%s' is not supported by the '%s' driver "
		"for record '%s'.", value_name, mx_get_driver_name( record ),
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_icplus_ain_read()";

	MX_ICPLUS_AINPUT *icplus_ainput;
	MX_ICPLUS *icplus;
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_icplus_ain_get_pointers( ainput,
					&icplus_ainput, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_icplus_command( icplus, icplus_ainput->command,
				response, sizeof( response ),
				MXD_ICPLUS_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%lg", &(ainput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Could not find a numerical value in the response to an '%s' "
		"command to IC PLUS controller '%s' for "
		"analog input record '%s'.  Response = '%s'",
			icplus_ainput->command,
			icplus->record->name,
			ainput->record->name,
			response );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_icplus_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_icplus_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_ICPLUS_AOUTPUT *icplus_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *)
					malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        icplus_aoutput = (MX_ICPLUS_AOUTPUT *)
				malloc( sizeof(MX_ICPLUS_AOUTPUT) );

        if ( icplus_aoutput == (MX_ICPLUS_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ICPLUS_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = icplus_aoutput;
        record->class_specific_function_list
			= &mxd_icplus_aout_analog_output_function_list;

        analog_output->record = record;
	icplus_aoutput->record = record;

	/* Raw analog output values are stored as double. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_aout_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_icplus_aout_open()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_ICPLUS_AOUTPUT *icplus_aoutput;
	MX_ICPLUS *icplus;
	char *value_name;
	int i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	aoutput = (MX_ANALOG_OUTPUT *) record->record_class_struct;

	mx_status = mxd_icplus_aout_get_pointers( aoutput,
					&icplus_aoutput, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_NONE;

	value_name = icplus_aoutput->value_name;

	for ( i = 0; i < strlen( value_name ); i++ ) {
		if ( islower( (int)(value_name[i]) ) ) {
			value_name[i] = toupper( (int)(value_name[i]) );
		}
	}

	/* The ICPLUS monitor only supports setting the voltage. */

	if ( aoutput->record->mx_type == MXT_AOU_ICPLUS ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_INTEGER;

		sprintf( icplus_aoutput->input_command, ":CONF%d:VOLT?",
						icplus->address );

		sprintf( icplus_aoutput->output_command, ":CONF%d:VOLT?",
						icplus->address );

		return MX_SUCCESSFUL_RESULT;
	}

	/* The QPBM monitor supports many things. */

	if ( strcmp( value_name, "AVERAGE" ) == 0 ) {

		/* 'AVERAGE' is a special case.  If AVERAGE is set to a
		 * positive value, the AVGCURR command is used.  If AVERAGE
		 * is set to a negative value, the absolute value is sent
		 * to the WDWCURR command.  If AVERAGE is 0, then SINGLE
		 * is used.
		 */

		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_AVERAGE;

		strcpy( icplus_aoutput->input_command, "" );
		strcpy( icplus_aoutput->output_command, "" );

		/* Initialize the variable value to match the value set by
		 * the primary QBPM record.
		 */

		aoutput->raw_value.double_value = icplus->default_averaging;

		aoutput->value = aoutput->offset + aoutput->scale
					* aoutput->raw_value.double_value;
	} else
	if ( strcmp( value_name, "AVGCURR" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_INTEGER;

		strcpy( icplus_aoutput->input_command, "" );

		sprintf( icplus_aoutput->output_command, ":READ%d:AVGCURR",
						icplus->address );
	} else
	if ( strcmp( value_name, "WDWCURR" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_INTEGER;

		strcpy( icplus_aoutput->input_command, "" );

		sprintf( icplus_aoutput->output_command, ":READ%d:WDWCURR",
						icplus->address );
	} else
	if ( strcmp( value_name, "SINGLE" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_NONE;

		strcpy( icplus_aoutput->input_command, "" );

		sprintf( icplus_aoutput->output_command, ":READ%d:SINGLE",
						icplus->address );
	} else
	if ( strcmp( value_name, "GX" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_DOUBLE;

		sprintf( icplus_aoutput->input_command, ":CONF%d:GX?",
						icplus->address );

		sprintf( icplus_aoutput->output_command, ":CONF%d:GX",
						icplus->address );
	} else
	if ( strcmp( value_name, "GY" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_DOUBLE;

		sprintf( icplus_aoutput->input_command, ":CONF%d:GY?",
						icplus->address );

		sprintf( icplus_aoutput->output_command, ":CONF%d:GY",
						icplus->address );
	} else
	if ( strcmp( value_name, "A1" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_DOUBLE;

		sprintf( icplus_aoutput->input_command, ":CONF%d:A1?",
						icplus->address );

		sprintf( icplus_aoutput->output_command, ":CONF%d:A1",
						icplus->address );
	} else
	if ( strcmp( value_name, "A2" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_DOUBLE;

		sprintf( icplus_aoutput->input_command, ":CONF%d:A2?",
						icplus->address );

		sprintf( icplus_aoutput->output_command, ":CONF%d:A2",
						icplus->address );
	} else
	if ( strcmp( value_name, "B1" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_DOUBLE;

		sprintf( icplus_aoutput->input_command, ":CONF%d:B1?",
						icplus->address );

		sprintf( icplus_aoutput->output_command, ":CONF%d:B1",
						icplus->address );
	} else
	if ( strcmp( value_name, "B2" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_DOUBLE;

		sprintf( icplus_aoutput->input_command, ":CONF%d:B2?",
						icplus->address );

		sprintf( icplus_aoutput->output_command, ":CONF%d:B2",
						icplus->address );
	} else
	if ( strcmp( value_name, "C1" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_DOUBLE;

		sprintf( icplus_aoutput->input_command, ":CONF%d:C1?",
						icplus->address );

		sprintf( icplus_aoutput->output_command, ":CONF%d:C1",
						icplus->address );
	} else
	if ( strcmp( value_name, "C2" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_DOUBLE;

		sprintf( icplus_aoutput->input_command, ":CONF%d:C2?",
						icplus->address );

		sprintf( icplus_aoutput->output_command, ":CONF%d:C2",
						icplus->address );
	} else
	if ( strcmp( value_name, "D1" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_DOUBLE;

		sprintf( icplus_aoutput->input_command, ":CONF%d:D1?",
						icplus->address );

		sprintf( icplus_aoutput->output_command, ":CONF%d:D1",
						icplus->address );
	} else
	if ( strcmp( value_name, "D2" ) == 0 ) {
		icplus_aoutput->argument_type = MXAT_ICPLUS_AOUTPUT_DOUBLE;

		sprintf( icplus_aoutput->input_command, ":CONF%d:D2?",
						icplus->address );

		sprintf( icplus_aoutput->output_command, ":CONF%d:D2",
						icplus->address );
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Requested value '%s' is not supported by the '%s' driver "
		"for record '%s'.", value_name, mx_get_driver_name( record ),
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_icplus_aout_read()";

	MX_ICPLUS_AOUTPUT *icplus_aoutput;
	MX_ICPLUS *icplus;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_icplus_aout_get_pointers( aoutput,
					&icplus_aoutput, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the length of the input command is 0, then just return the
	 * last value written.
	 */

	if ( strlen( icplus_aoutput->input_command ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mxd_icplus_command( icplus, icplus_aoutput->input_command,
				response, sizeof( response ),
				MXD_ICPLUS_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%lg", &(aoutput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Could not find a numerical value in the response to an '%s' "
		"command to IC PLUS controller '%s' for "
		"analog output record '%s'.  Response = '%s'",
			command,
			icplus->record->name,
			aoutput->record->name,
			response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_icplus_aout_write()";

	MX_ICPLUS_AOUTPUT *icplus_aoutput;
	MX_ICPLUS *icplus;
	char command[80];
	long long_value;
	mx_status_type mx_status;

	mx_status = mxd_icplus_aout_get_pointers( aoutput,
					&icplus_aoutput, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( icplus_aoutput->argument_type ) {
	case MXAT_ICPLUS_AOUTPUT_NONE:
		strcpy( command, icplus_aoutput->output_command );
		break;

	case MXAT_ICPLUS_AOUTPUT_INTEGER:
		sprintf( command, "%s %ld", icplus_aoutput->output_command,
				mx_round(aoutput->raw_value.double_value) );
		break;

	case MXAT_ICPLUS_AOUTPUT_DOUBLE:
		sprintf( command, "%s %g", icplus_aoutput->output_command,
				aoutput->raw_value.double_value );
		break;

	case MXAT_ICPLUS_AOUTPUT_AVERAGE:

		/* AVERAGE is a special case.  If AVERAGE is set to a
		 * positive value, the AVGCURR command is used.  If AVERAGE
		 * is set to a negative value, the absolute value is sent
		 * to the WDWCURR command.  If AVERAGE is 0, then SINGLE
		 * is used.
		 */

		/* Round the value to the nearest integer. */

		long_value = mx_round( aoutput->raw_value.double_value );

		aoutput->raw_value.double_value = (double) long_value;

		/* Construct the command. */

		if ( long_value > 100 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested averaging size of %ld for record '%s' is "
		"outside the allowed range of 1 to 100.",
				long_value, icplus->record->name );
		} else
		if ( long_value >= 1 ) {
			sprintf( command, ":READ%d:AVGCURR %ld",
					icplus->address, long_value );
		} else
		if ( long_value > -1 ) {
			sprintf( command, ":READ%d:SINGLE", icplus->address );
		} else
		if ( long_value >= -100 ) {
			sprintf( command, ":READ%d:WDWCURR %ld",
					icplus->address, -long_value );
		} else {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested moving average size of %ld for record '%s' is "
		"outside the allowed range of -1 to -100.",
				long_value, icplus->record->name );
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal argument type %d for record '%s'.",
			icplus_aoutput->argument_type, aoutput->record->name );
	}

	mx_status = mxd_icplus_command( icplus, command,
					NULL, 0, MXD_ICPLUS_AIO_DEBUG );

	return mx_status;
}

