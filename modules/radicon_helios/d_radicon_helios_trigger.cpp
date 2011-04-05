/*
 * Name:    d_radicon_helios_trigger.c
 *
 * Purpose: MX pulse generator driver to generate a trigger pulse for the
 *          Radicon Helios detectors using the Pleora iPORT PLC.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_RADICON_HELIOS_TRIGGER_DEBUG		TRUE

#define MXD_RADICON_HELIOS_TRIGGER_DEBUG_BUSY		TRUE

#define MXD_RADICON_HELIOS_TRIGGER_DEBUG_SET_RCBIT	TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_area_detector.h"
#include "mx_pulse_generator.h"
#include "i_pleora_iport.h"
#include "d_pleora_iport_vinput.h"
#include "d_radicon_helios.h"
#include "d_radicon_helios_trigger.h"

/* Initialize the pulse generator driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_rh_trigger_record_function_list = {
	NULL,
	mxd_rh_trigger_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_rh_trigger_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST mxd_rh_trigger_pulser_function_list = {
	mxd_rh_trigger_is_busy,
	mxd_rh_trigger_start,
	mxd_rh_trigger_stop,
	mxd_rh_trigger_get_parameter,
	mxd_rh_trigger_set_parameter
};

/* MX MBC trigger timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_rh_trigger_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_RADICON_HELIOS_TRIGGER_STANDARD_FIELDS
};

long mxd_rh_trigger_num_record_fields
		= sizeof( mxd_rh_trigger_record_field_defaults )
		  / sizeof( mxd_rh_trigger_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_rh_trigger_rfield_def_ptr
			= &mxd_rh_trigger_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_rh_trigger_get_pointers( MX_PULSE_GENERATOR *pulser,
			MX_RADICON_HELIOS_TRIGGER **rh_trigger,
			MX_RADICON_HELIOS         **radicon_helios,
			MX_PLEORA_IPORT_VINPUT    **pleora_iport_vinput,
			MX_PLEORA_IPORT           **pleora_iport,
			const char *calling_fname )
{
	static const char fname[] = "mxd_rh_trigger_get_pointers()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger_ptr;
	MX_RECORD *radicon_helios_record;
	MX_RADICON_HELIOS *radicon_helios_ptr;
	MX_RECORD *video_input_record;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput_ptr;
	MX_RECORD *pleora_iport_record;

	if ( pulser == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PULSE_GENERATOR pointer passed by '%s' was NULL",
			calling_fname );
	}

	rh_trigger_ptr = (MX_RADICON_HELIOS_TRIGGER *)
					pulser->record->record_type_struct;

	if ( rh_trigger_ptr == (MX_RADICON_HELIOS_TRIGGER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RADICON_HELIOS_TRIGGER pointer for pulse generator "
		"record '%s' passed by '%s' is NULL",
				pulser->record->name, calling_fname );
	}

	if ( rh_trigger != (MX_RADICON_HELIOS_TRIGGER **) NULL ) {
		*rh_trigger = rh_trigger_ptr;
	}

	radicon_helios_record = rh_trigger_ptr->radicon_helios_record;

	if ( radicon_helios_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The radicon_helios_record pointer for pulse generator '%s' "
		"is NULL.", pulser->record->name );
	}

	radicon_helios_ptr = (MX_RADICON_HELIOS *)
				radicon_helios_record->record_type_struct;

	if ( radicon_helios_ptr == (MX_RADICON_HELIOS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RADICON_HELIOS pointer for record '%s' "
		"used by '%s' is NULL.",
			radicon_helios_record->name,
			pulser->record->name );
	}

	if ( radicon_helios != (MX_RADICON_HELIOS **) NULL ) {
		*radicon_helios = radicon_helios_ptr;
	}

	video_input_record = radicon_helios_ptr->video_input_record;

	if ( video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The video_input_record pointer for area detector '%s' "
		"used by '%s' is NULL.",
			radicon_helios_record->name,
			pulser->record->name );
	}

	pleora_iport_vinput_ptr = (MX_PLEORA_IPORT_VINPUT *)
					video_input_record->record_type_struct;

	if ( pleora_iport_vinput_ptr == (MX_PLEORA_IPORT_VINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PLEORA_IPORT_VINPUT pointer for video input '%s' "
		"used by area detector '%s' and pulse generator '%s' is NULL.",
			video_input_record->name,
			radicon_helios_record->name,
			pulser->record->name );
	}

	if ( pleora_iport_vinput != (MX_PLEORA_IPORT_VINPUT **) NULL ) {
		*pleora_iport_vinput = pleora_iport_vinput_ptr;
	}

	if ( pleora_iport != (MX_PLEORA_IPORT **) NULL ) {
		pleora_iport_record =
			pleora_iport_vinput_ptr->pleora_iport_record;

		if ( pleora_iport_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The pleora_iport_record pointer for video input '%s' "
			"used by area detector '%s' and pulse generator '%s' "
			"is NULL.",
				video_input_record->name,
				radicon_helios_record->name,
				pulser->record->name );
		}

		*pleora_iport = (MX_PLEORA_IPORT *)
				pleora_iport_record->record_type_struct;

		if ( (*pleora_iport) == (MX_PLEORA_IPORT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PLEORA_IPORT pointer for Pleora iPORT '%s' "
			"used by video input '%s', area detector '%s', and "
			"pulse generator '%s' is NULL.",
				pleora_iport_record->name,
				video_input_record->name,
				radicon_helios_record->name,
				pulser->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----*/

static void
mxd_rh_trigger_set_rcbit( CyGrabber *grabber, int bit_number, int bit_value )
{
	__int64 old_value;
	__int64 set_value, clear_value;
	__int64 bit_mask;

	CyDevice &device = grabber->GetDevice();

	CyDeviceExtension *extension =
			&device.GetExtension( CY_DEVICE_EXT_GPIO_CONTROL_BITS );

	extension->LoadFromDevice();

	extension->GetParameter( CY_GPIO_CONTROL_BITS_CURRENT_VALUE,
							old_value );

	bit_mask = ( 1 << bit_number );

	if ( bit_value == 0 ) {
		set_value = 0;

		clear_value = ( (~old_value) & 0xf ) | bit_mask;
	} else {
		set_value = old_value | bit_mask;

		clear_value = 0;
	}

#if MXD_RADICON_HELIOS_TRIGGER_DEBUG_SET_RCBIT
	static const char fname[] = "mxd_rh_trigger_set_rcbit()";

	MX_DEBUG(-2,("%s: bit_number = %d, bit_value = %d",
			fname, bit_number, bit_value));
#endif

#if 0
	MX_DEBUG(-2,("%s: old_value = %#x", fname, (unsigned long) old_value));
	MX_DEBUG(-2,("%s: bit_mask = %#x", fname, (unsigned long) bit_mask));
	MX_DEBUG(-2,("%s: set_value = %#x, clear_value = %#x",
			fname, (unsigned long) set_value,
			(unsigned long) clear_value));
#endif

	extension->SetParameter( CY_GPIO_CONTROL_BITS_SET, set_value );

	extension->SetParameter( CY_GPIO_CONTROL_BITS_CLEAR, clear_value );

	extension->SaveToDevice();

}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_rh_trigger_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_rh_trigger_create_record_structures()";

	MX_PULSE_GENERATOR *pulser;
	MX_RADICON_HELIOS_TRIGGER *rh_trigger;

	/* Allocate memory for the necessary structures. */

	pulser = (MX_PULSE_GENERATOR *) malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_PULSE_GENERATOR structure." );
	}

	rh_trigger = (MX_RADICON_HELIOS_TRIGGER *)
				malloc( sizeof(MX_RADICON_HELIOS_TRIGGER) );

	if ( rh_trigger == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_RADICON_HELIOS_TRIGGER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = pulser;
	record->record_type_struct = rh_trigger;
	record->class_specific_function_list
			= &mxd_rh_trigger_pulser_function_list;

	pulser->record = record;
	rh_trigger->record = record;

	rh_trigger->preset_value = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_rh_trigger_open()";

	MX_PULSE_GENERATOR *pulser;
	MX_RADICON_HELIOS_TRIGGER *rh_trigger;
	MX_RADICON_HELIOS *radicon_helios;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput;
	MX_PLEORA_IPORT *pleora_iport;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_rh_trigger_get_pointers( pulser, &rh_trigger,
					&radicon_helios, &pleora_iport_vinput,
					&pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	CyGrabber *grabber = pleora_iport_vinput->grabber;

	if ( grabber == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No grabber has been connected for record '%s'.",
			pleora_iport_vinput->record->name );
	}

	CyDevice &device = grabber->GetDevice();

	/* The PLC is to be configured as follows:
	 *
	 * PC remote control bit 0 (I5) is connected to pulse generator 1's
	 * trigger input (Q9) which is configured to run when the input 
	 * is high.  The output of pulse generator 1 (I6) is connected to
	 * the count up input (Q?) of the general purpose counter.  The
	 * general purpose counter is configured to generate the gate pulse
	 * (I3) for Counter 0 greater than zero.  The gate pulse is sent
	 * to Q1.  PC remote control bit 1 (I4) is used to clear the counter
	 * via Q11.
	 */

	/*-------------------------------------------------------------------*/

	/* Initialize the values of the remote control bits to all clear. */

	CyDeviceExtension *rcbits_extension = &device.GetExtension(
					CY_DEVICE_EXT_GPIO_CONTROL_BITS );

	rcbits_extension->SetParameter( CY_GPIO_CONTROL_BITS_CLEAR, 0xf );

	rcbits_extension->SetParameter( CY_GPIO_CONTROL_BITS_SET, 0 );

	rcbits_extension->SaveToDevice();

	/*-------------------------------------------------------------------*/

	/* Configure pulse generator 1 to generate a 1 kHz pulse train
	 * that is triggered when its trigger input is high.
	 */

	CyDeviceExtension *pg_extension = &device.GetExtension(
					CY_DEVICE_EXT_PULSE_GENERATOR + 1 );

	pg_extension->SetParameter( CY_PULSE_GEN_PARAM_GRANULARITY, 1666 );

	pg_extension->SetParameter( CY_PULSE_GEN_PARAM_WIDTH, 10 );

	pg_extension->SetParameter( CY_PULSE_GEN_PARAM_DELAY, 9 );

	pg_extension->SetParameter( CY_PULSE_GEN_PARAM_PERIODIC, false );

	pg_extension->SetParameter( CY_PULSE_GEN_PARAM_TRIGGER_MODE, 1 );

	pg_extension->SaveToDevice();

	/*-------------------------------------------------------------------*/

	/* Configure the counter.  The increment, decrement, and clear inputs
	 * are triggered on the rising edge of the input.  The clear input
	 * is configured for Q11.  The counter compare value is initialized
	 * to 0.
	 */

	CyDeviceExtension *ctr_extension = &device.GetExtension(
						CY_DEVICE_EXT_COUNTER );

	ctr_extension->SetParameter( CY_COUNTER_PARAM_UP_EVENT, 1 );

	ctr_extension->SetParameter( CY_COUNTER_PARAM_DOWN_EVENT, 1 );

	ctr_extension->SetParameter( CY_COUNTER_PARAM_CLEAR_EVENT, 1 );

	ctr_extension->SetParameter( CY_COUNTER_PARAM_CLEAR_INPUT, 5 );

	ctr_extension->SetParameter( CY_COUNTER_PARAM_COMPARE_VALUE, 0 );

	ctr_extension->SaveToDevice();

	/*-------------------------------------------------------------------*/

	/* Configure the Signal Routing Block and the PLC's Lookup Table
	 * for the pulse generator.  Note that I0, I2, and I7 are already
	 * in use by the 'radicon_helios' driver.
	 */

	CyDeviceExtension *lut_extension =
				&device.GetExtension( CY_DEVICE_EXT_GPIO_LUT );

	/* NOTE: The source of the values of the second parameter to the
	 * SetParameter() calls below is described in a comment block 
	 * before the start of the mxd_radicon_helios_open() function in
	 * d_radicon_helios.cpp.
	 */

	/* Connect "Counter 0 Greater" to I3. */

	lut_extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIG3, 15 );

	/* Connect "PLC control bit 1" to I4. (counter clear) */

	lut_extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIG4, 0 );

	/* Connect "PLC control bit 0" to I5. (pulse generator trigger) */

	lut_extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIG5, 0 );

#if 1
	/* Connect "Pulse generator 1 output" to I6. (counter up) */

	lut_extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIG6, 0 );
#else
	/* Connect "PLC control bit 2" to I6. (counter up) */

	lut_extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIG6, 10 );
#endif

	lut_extension->SaveToDevice();

	/*-------------------------------------------------------------------*/

	/* Set the trigger input low and the trigger output low. */

	mxd_rh_trigger_set_rcbit( grabber, 0, 0 );

	char lut_program_low[] = 
			"Q1=0\r\n"
			"Q8=I5\r\n";

	mxd_pleora_iport_vinput_send_lookup_table_program( pleora_iport_vinput,
							lut_program_low );

	/* Connect pulse generator 1 output to the counter "up" input
	 * and connect PLC control input 1 to the counter's clear input.
	 */

	char lut_program_clear[] = 
			"Q11=I4\r\n"
			"Q17=I6\r\n";

	mxd_pleora_iport_vinput_send_lookup_table_program( pleora_iport_vinput,
							lut_program_clear );

	/* Clear the counter. */

	mxd_rh_trigger_set_rcbit( grabber, 1, 1 );

	mx_msleep(1000);

	mxd_rh_trigger_set_rcbit( grabber, 1, 0 );

	/* Enable the trigger output by connecting it to the output of
	 * the counter.
	 *
	 * The counter generates a 'greater than' signal, but what we
	 * actually need is a 'less than' signal.  So what we do is to
	 * invert the output of the counter plus do a logical 'and' with
	 * the pulse generator trigger.
	 */

	char lut_program_out[] = "Q1=I5 & !I3\r\n";

	mxd_pleora_iport_vinput_send_lookup_table_program( pleora_iport_vinput,
							lut_program_out );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_is_busy( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_rh_trigger_is_busy()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger;
	MX_RADICON_HELIOS *radicon_helios;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput;
	MX_PLEORA_IPORT *pleora_iport;
	__int64 current_value;
	mx_status_type mx_status;

	mx_status = mxd_rh_trigger_get_pointers( pulser, &rh_trigger,
					&radicon_helios, &pleora_iport_vinput,
					&pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	CyGrabber *grabber = pleora_iport_vinput->grabber;

	if ( grabber == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No grabber has been connected for record '%s'.",
			pleora_iport_vinput->record->name );
	}

	CyDevice &device = grabber->GetDevice();

	/* If the counter has not reached the specified counter value,
	 * then it is still busy.
	 */ 

	CyDeviceExtension *ctr_extension = &device.GetExtension(
						CY_DEVICE_EXT_COUNTER );

	ctr_extension->LoadFromDevice();

	ctr_extension->GetParameter( CY_COUNTER_PARAM_CURRENT_VALUE,
							current_value );

	if ( current_value <= rh_trigger->preset_value ) {
		pulser->busy = TRUE;
	} else {
		pulser->busy = FALSE;

		/* Deassert the input trigger for the pulse generator. */

		mxd_rh_trigger_set_rcbit( grabber, 0, 0 );
	}

#if MXD_RADICON_HELIOS_TRIGGER_DEBUG_BUSY
	MX_DEBUG(-2,("%s: busy = %d, current_value = %lu, preset_value = %lu",
		fname, pulser->busy,
		(unsigned long) current_value,
		rh_trigger->preset_value));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_start( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_rh_trigger_start()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger;
	MX_RADICON_HELIOS *radicon_helios;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput;
	MX_PLEORA_IPORT *pleora_iport;
	double gate_time_in_seconds;
	unsigned long gate_time_in_milliseconds;
	mx_status_type mx_status;

	mx_status = mxd_rh_trigger_get_pointers( pulser, &rh_trigger,
					&radicon_helios, &pleora_iport_vinput,
					&pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	gate_time_in_seconds = pulser->pulse_width;

	gate_time_in_milliseconds = mx_round( 1000.0 * gate_time_in_seconds );

	rh_trigger->preset_value = gate_time_in_milliseconds;

	/* Clear the counter. */

	CyGrabber *grabber = pleora_iport_vinput->grabber;

	if ( grabber == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No grabber has been connected for record '%s'.",
			pleora_iport_vinput->record->name );
	}

	mxd_rh_trigger_set_rcbit( grabber, 1, 1 );

	mxd_rh_trigger_set_rcbit( grabber, 1, 0 );

	/* Update the counter compare value. */

	CyDevice &device = grabber->GetDevice();

	CyDeviceExtension *ctr_extension = &device.GetExtension(
						CY_DEVICE_EXT_COUNTER );

	ctr_extension->SetParameter( CY_COUNTER_PARAM_COMPARE_VALUE,
					gate_time_in_milliseconds );

	ctr_extension->SaveToDevice();

	/* Assert the input trigger for the pulse generator. */

	mxd_rh_trigger_set_rcbit( grabber, 0, 1 );

#if MXD_RADICON_HELIOS_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: '%s' started with preset %lu",
		fname, pulser->record->name, rh_trigger->preset_value));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_stop( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_rh_trigger_stop()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger;
	MX_RADICON_HELIOS *radicon_helios;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput;
	MX_PLEORA_IPORT *pleora_iport;
	mx_status_type mx_status;

	mx_status = mxd_rh_trigger_get_pointers( pulser, &rh_trigger,
					&radicon_helios, &pleora_iport_vinput,
					&pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Stopping pulse generator '%s'.",
		fname, pulser->record->name ));
#endif

	CyGrabber *grabber = pleora_iport_vinput->grabber;

	if ( grabber == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No grabber has been connected for record '%s'.",
			pleora_iport_vinput->record->name );
	}

	/* Stop counting. */

	mxd_rh_trigger_set_rcbit( grabber, 0, 0 );

	mx_msleep(500);

	/* Clear the counter. */

	mxd_rh_trigger_set_rcbit( grabber, 1, 1 );

	mx_msleep(500);

	mxd_rh_trigger_set_rcbit( grabber, 1, 0 );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_get_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_rh_trigger_get_parameter()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger = NULL;
	MX_RADICON_HELIOS *radicon_helios;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput;
	MX_PLEORA_IPORT *pleora_iport;
	mx_status_type mx_status;

	mx_status = mxd_rh_trigger_get_pointers( pulser, &rh_trigger,
					&radicon_helios, &pleora_iport_vinput,
					&pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_TRIGGER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		break;
	case MXLV_PGN_PULSE_WIDTH:
		break;
	case MXLV_PGN_PULSE_DELAY:
		break;
	case MXLV_PGN_MODE:
		break;
	case MXLV_PGN_PULSE_PERIOD:
		break;
	default:
		return
		    mx_pulse_generator_default_get_parameter_handler( pulser );
	}

	MX_DEBUG(-2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_set_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_rh_trigger_set_parameter()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger;
	MX_RADICON_HELIOS *radicon_helios;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput;
	MX_PLEORA_IPORT *pleora_iport;
	mx_status_type mx_status;

	mx_status = mxd_rh_trigger_get_pointers( pulser, &rh_trigger,
					&radicon_helios, &pleora_iport_vinput,
					&pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_TRIGGER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		pulser->num_pulses = 1;
		break;

	case MXLV_PGN_PULSE_WIDTH:
		/* This value is used by the start() routine above. */
		break;

	case MXLV_PGN_PULSE_DELAY:
		pulser->pulse_delay = 0;
		break;

	case MXLV_PGN_MODE:
		pulser->mode = MXF_PGN_SQUARE_WAVE;
		break;

	case MXLV_PGN_PULSE_PERIOD:
		pulser->pulse_period = pulser->pulse_width;
		break;

	default:
		return
		    mx_pulse_generator_default_set_parameter_handler( pulser );
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

