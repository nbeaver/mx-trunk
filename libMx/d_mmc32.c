/*
 * Name:    d_mmc32.c 
 *
 * Purpose: MX motor driver to control NSLS-built MMC32 motor controllers.
 *
 * Warning: This driver is untested.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2004, 2006, 2010, 2013, 2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MMC32_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_gpib.h"
#include "d_mmc32.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mmc32_record_function_list = {
	NULL,
	mxd_mmc32_create_record_structures,
	mxd_mmc32_finish_record_initialization,
	NULL,
	mxd_mmc32_print_motor_structure,
	mxd_mmc32_open,
	mxd_mmc32_close
};

MX_MOTOR_FUNCTION_LIST mxd_mmc32_motor_function_list = {
	NULL,
	mxd_mmc32_move_absolute,
	mxd_mmc32_get_position,
	mxd_mmc32_set_position,
	mxd_mmc32_soft_abort,
	mxd_mmc32_immediate_abort,
	NULL,
	NULL,
	mxd_mmc32_raw_home_command,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_mmc32_get_status
};

/* MMC32 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mmc32_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_MMC32_STANDARD_FIELDS
};

long mxd_mmc32_num_record_fields
		= sizeof( mxd_mmc32_record_field_defaults )
			/ sizeof( mxd_mmc32_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mmc32_rfield_def_ptr
			= &mxd_mmc32_record_field_defaults[0];

/* ========== Private functions for the use of the driver =========== */

static mx_status_type
mxd_mmc32_get_pointers( MX_MOTOR *motor,
			MX_MMC32 **mmc32,
			MX_RECORD **gpib_record,
			long *address,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mmc32_get_pointers()";

	MX_MMC32 *internal_mmc32;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	internal_mmc32 = (MX_MMC32 *) (motor->record->record_type_struct);

	if ( internal_mmc32 == (MX_MMC32 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MMC32 pointer for motor '%s' passed by '%s' is NULL.",
			motor->record->name, calling_fname );
	}
	if ( mmc32 != (MX_MMC32 **) NULL ) {
		*mmc32 = internal_mmc32;
	}
	if ( gpib_record != (MX_RECORD **) NULL ) {
		*gpib_record = internal_mmc32->gpib_interface.record;

		if ( *gpib_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The GPIB interface MX_RECORD pointer for motor '%s' passed by '%s' is NULL.",
				motor->record->name, calling_fname );
		}
	}
	if ( address != NULL ) {
		*address = (int) internal_mmc32->gpib_interface.address;

		if ( *address < 0 || *address > 31 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"GPIB address %ld for motor '%s' passed by '%s' "
			"is not in allowed range (0-31).",
				*address, motor->record->name, calling_fname );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_mmc32_command( MX_RECORD *gpib_record,
			long address, 
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxd_mmc32_command()";

	mx_status_type mx_status;

	if ( gpib_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"gpib_record pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"command buffer pointer passed was NULL.  No command sent.");
	}

	/* Send the command string. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: Sending command '%s'", fname, command));
	}

	mx_status = mx_gpib_putline( gpib_record, address, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response != NULL ) {
		mx_status = mx_gpib_getline( gpib_record, address,
			response, response_buffer_length, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,(
			"%s: Received response '%s'", fname, response));
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_mmc32_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_mmc32_create_record_structures()";

        MX_MOTOR *motor;
        MX_MMC32 *mmc32;

        /* Allocate memory for the necessary structures. */

        motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

        if ( motor == (MX_MOTOR *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MOTOR structure." );
        }

        mmc32 = (MX_MMC32 *) malloc( sizeof(MX_MOTOR) );

        if ( mmc32 == (MX_MMC32 *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MMC32 structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = motor;
        record->record_type_struct = mmc32;
        record->class_specific_function_list
                                = &mxd_mmc32_motor_function_list;

        motor->record = record;

        /* An NSLS MMC32 motor is treated as an stepper motor. */

        motor->subclass = MXC_MTR_STEPPER;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mmc32_finish_record_initialization( MX_RECORD *record )
{
	return mx_motor_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_mmc32_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_mmc32_print_motor_structure()";

	MX_MOTOR *motor;
	MX_MMC32 *mmc32;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	mmc32 = (MX_MMC32 *) (record->record_type_struct);

	if ( mmc32 == (MX_MMC32 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MMC32 pointer for record '%s' is NULL.", record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name );

	fprintf(file, "  Motor type          = MMC32.\n\n");
	fprintf(file, "  name                = %s\n", record->name);
	fprintf(file, "  gpib interface      = %s:%s\n",
		mmc32->gpib_interface.record->name,
		mmc32->gpib_interface.address_name );
	fprintf(file, "  motor number        = %ld\n", mmc32->motor_number );

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position            = %g %s  (%ld steps)\n",
		motor->position, motor->units,
		motor->raw_position.stepper );
	fprintf(file, "  scale               = %g %s per step.\n",
		motor->scale, motor->units );
	fprintf(file, "  offset              = %g %s.\n",
		motor->offset, motor->units );

	fprintf(file, "  backlash            = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper);

	fprintf(file, "  negative limit      = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper);

	fprintf(file, "  positive limit      = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper);

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband       = %g %s  (%ld steps)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mmc32_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_mmc32_open()";

	MX_MOTOR *motor;
	MX_MMC32 *mmc32;
	MX_RECORD *gpib_record;
	long address;
	char command[40];
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_mmc32_get_pointers(motor,
			&mmc32, &gpib_record, &address, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_gpib_open_device( gpib_record, address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*
	 * Program the flag byte:
	 *
	 * 1) P4 input mode = open.
	 * 2) SRQ off.
	 * 3) P3 output mode = step and direction.
	 */

	snprintf( command, sizeof(command),"N%ld F4", mmc32->motor_number );

	mx_status = mxd_mmc32_command( gpib_record, address,
					command, NULL, 0, MMC32_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the velocity multiplication factor. */

	snprintf( command, sizeof(command),
			"N%ld Z%f", mmc32->motor_number,
			mmc32->multiplication_factor );

	mx_status = mxd_mmc32_command( gpib_record, address,
					command, NULL, 0, MMC32_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the start velocity */

	snprintf( command, sizeof(command), "U%ld", mmc32->start_velocity );

	mx_status = mxd_mmc32_command( gpib_record, address,
					command, NULL, 0, MMC32_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the peak velocity */

	snprintf( command, sizeof(command), "V%ld", mmc32->peak_velocity );

	mx_status = mxd_mmc32_command( gpib_record, address,
					command, NULL, 0, MMC32_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the acceleration steps */

	snprintf( command, sizeof(command), "A%ld", mmc32->acceleration_steps );

	mx_status = mxd_mmc32_command( gpib_record, address,
					command, NULL, 0, MMC32_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mmc32_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_mmc32_close()";

	MX_MOTOR *motor;
	MX_MMC32 *mmc32;
	MX_RECORD *gpib_record;
	int num_items;
	long address, value;
	char command[40], response[40];
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_mmc32_get_pointers(motor,
			&mmc32, &gpib_record, &address, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Cannot ask for the multiplication factor, so we leave it alone. */

	/* Get the start velocity. */

	snprintf( command, sizeof(command), "N%ld IU", mmc32->motor_number );

	mx_status = mxd_mmc32_command( gpib_record, address,
			command, response, sizeof response, MMC32_DEBUG );

	num_items = sscanf( response, "%ld", &value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Can't get start velocity for motor '%s'",
			record->name );
	}

	mmc32->start_velocity = value;

	/* Get the peak velocity. */

	mx_status = mxd_mmc32_command( gpib_record, address,
			"IV", response, sizeof response, MMC32_DEBUG );

	num_items = sscanf( response, "%ld", &value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Can't get peak velocity for motor '%s'",
			record->name );
	}

	mmc32->peak_velocity = value;

	/* Get the acceleration steps. */

	mx_status = mxd_mmc32_command( gpib_record, address,
			"IA", response, sizeof response, MMC32_DEBUG );

	num_items = sscanf( response, "%ld", &value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Can't get acceleration steps for motor '%s'",
			record->name );
	}

	mmc32->acceleration_steps = value;

	/* Shutdown the connection to the device. */

	mx_status = mx_gpib_close_device( gpib_record, address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mmc32_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mmc32_move_absolute()";

	MX_MMC32 *mmc32;
	MX_RECORD *gpib_record;
	long address;
	char command[40];
	long motor_steps;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mxd_mmc32_get_pointers(motor,
			&mmc32, &gpib_record, &address, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_destination.stepper;

	snprintf( command, sizeof(command),
			"N%ld M%ld", mmc32->motor_number, motor_steps );

	mx_status = mxd_mmc32_command( gpib_record, address,
				command, NULL, 0, MMC32_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mmc32_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mmc32_get_position()";

	MX_MMC32 *mmc32;
	MX_RECORD *gpib_record;
	int num_items;
	long address;
	char command[40], response[40];
	long motor_steps;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mxd_mmc32_get_pointers(motor,
			&mmc32, &gpib_record, &address, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "N%ld IP", mmc32->motor_number );

	mx_status = mxd_mmc32_command( gpib_record, address,
			command, response, sizeof(response), MMC32_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &motor_steps );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Motor step position not found in MMC32 response string '%s'",
			response );
	}

	motor->raw_position.stepper = motor_steps;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mmc32_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mmc32_set_position()";

	MX_MMC32 *mmc32;
	MX_RECORD *gpib_record;
	long address;
	char command[40];
	long motor_steps;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mxd_mmc32_get_pointers(motor,
			&mmc32, &gpib_record, &address, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_set_position.stepper;

	snprintf( command, sizeof(command),
			"N%ld P%ld", mmc32->motor_number, motor_steps );

	mx_status = mxd_mmc32_command( gpib_record, address,
				command, NULL, 0, MMC32_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mmc32_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mmc32_soft_abort()";

	MX_MMC32 *mmc32;
	MX_RECORD *gpib_record;
	long address;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mxd_mmc32_get_pointers(motor,
			&mmc32, &gpib_record, &address, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_mmc32_command( gpib_record, address,
				"K", NULL, 0, MMC32_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mmc32_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mmc32_immediate_abort()";

	MX_MMC32 *mmc32;
	MX_RECORD *gpib_record;
	long address;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mxd_mmc32_get_pointers(motor,
			&mmc32, &gpib_record, &address, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_mmc32_command( gpib_record, address,
				"Q", NULL, 0, MMC32_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mmc32_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mmc32_raw_home_command()";

	MX_MMC32 *mmc32;
	MX_RECORD *gpib_record;
	long address;
	char command[40];
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mxd_mmc32_get_pointers(motor,
			&mmc32, &gpib_record, &address, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "N%ld H", mmc32->motor_number );

	mx_status = mxd_mmc32_command( gpib_record, address,
					command, NULL, 0, MMC32_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mmc32_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mmc32_get_status()";

	MX_MMC32 *mmc32;
	MX_RECORD *gpib_record;
	int num_items;
	long address, output_status;
	char command[40];
	char response[40];
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mxd_mmc32_get_pointers(motor,
			&mmc32, &gpib_record, &address, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "N%ld IO", mmc32->motor_number );

	mx_status = mxd_mmc32_command( gpib_record, address, command,
				response, sizeof(response), MMC32_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &output_status );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to read MMC32 output status byte failed.  "
		"(interface = '%s', address = %ld, motor_number = %ld)",
			gpib_record->name, address, mmc32->motor_number );
	}

	motor->status = 0;

	if ( output_status & 0x1 ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}
	if ( output_status & 0x2 ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}
	if ( output_status & 0x4 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	return MX_SUCCESSFUL_RESULT;
}

