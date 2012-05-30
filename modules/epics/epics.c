/*
 * Name:    epics.c
 *
 * Purpose: Module wrapper for MX drivers that communicate with the 
 *          EPICS control system from http://www.aps.anl.gov/epics/.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_EPICS_INIT	FALSE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_module.h"
#include "mx_image.h"

#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_area_detector.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_mca.h"
#include "mx_mcs.h"
#include "mx_motor.h"
#include "mx_scaler.h"
#include "mx_timer.h"

#include "mx_gpib.h"
#include "mx_rs232.h"
#include "mx_vme.h"

#include "mx_variable.h"

#include "mx_epics.h"
#include "mx_vepics.h"

#include "d_epics_aio.h"
#include "d_epics_area_detector.h"
#include "d_epics_ccd.h"
#include "d_epics_dio.h"
#include "d_epics_mca.h"
#include "d_epics_mcs.h"
#include "d_epics_motor.h"
#include "d_epics_scaler.h"
#include "d_epics_timer.h"
#include "i_epics_gpib.h"
#include "i_epics_rs232.h"
#include "i_epics_vme.h"
#include "v_epics_timeout.h"

MX_DRIVER epics_driver_table[] = {

{"epics_ainput", -1,	MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_epics_ain_record_function_list,
				NULL,
				&mxd_epics_ain_analog_input_function_list,
				&mxd_epics_ain_num_record_fields,
				&mxd_epics_ain_rfield_def_ptr},

{"epics_aoutput", -1,	MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_epics_aout_record_function_list,
				NULL,
				&mxd_epics_aout_analog_output_function_list,
				&mxd_epics_aout_num_record_fields,
				&mxd_epics_aout_rfield_def_ptr},

{"epics_area_detector", -1, MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_epics_ad_record_function_list,
				NULL,
				&mxd_epics_ad_ad_function_list,
				&mxd_epics_ad_num_record_fields,
				&mxd_epics_ad_rfield_def_ptr},

{"epics_ccd", -1,	MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_epics_ccd_record_function_list,
				NULL,
				&mxd_epics_ccd_ad_function_list,
				&mxd_epics_ccd_num_record_fields,
				&mxd_epics_ccd_rfield_def_ptr},

{"epics_dinput", -1,	MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_epics_din_record_function_list,
				NULL,
				&mxd_epics_din_digital_input_function_list,
				&mxd_epics_din_num_record_fields,
				&mxd_epics_din_rfield_def_ptr},

{"epics_doutput", -1,	MXC_DIGITAL_OUTPUT,  MXR_DEVICE,
				&mxd_epics_dout_record_function_list,
				NULL,
				&mxd_epics_dout_digital_output_function_list,
				&mxd_epics_dout_num_record_fields,
				&mxd_epics_dout_rfield_def_ptr},

{"epics_mca", -1,	MXC_MULTICHANNEL_ANALYZER,  MXR_DEVICE,
				&mxd_epics_mca_record_function_list,
				NULL,
				&mxd_epics_mca_mca_function_list,
				&mxd_epics_mca_num_record_fields,
				&mxd_epics_mca_rfield_def_ptr},

{"epics_mcs", -1,	MXC_MULTICHANNEL_SCALER,  MXR_DEVICE,
				&mxd_epics_mcs_record_function_list,
				NULL,
				&mxd_epics_mcs_mcs_function_list,
				&mxd_epics_mcs_num_record_fields,
				&mxd_epics_mcs_rfield_def_ptr},

{"epics_motor", -1,	MXC_MOTOR,  MXR_DEVICE,
				&mxd_epics_motor_record_function_list,
				NULL,
				&mxd_epics_motor_motor_function_list,
				&mxd_epics_motor_num_record_fields,
				&mxd_epics_motor_rfield_def_ptr},

{"epics_scaler", -1,	MXC_SCALER,  MXR_DEVICE,
				&mxd_epics_scaler_record_function_list,
				NULL,
				&mxd_epics_scaler_scaler_function_list,
				&mxd_epics_scaler_num_record_fields,
				&mxd_epics_scaler_rfield_def_ptr},

{"epics_timer", -1,	MXC_TIMER,  MXR_DEVICE,
				&mxd_epics_timer_record_function_list,
				NULL,
				&mxd_epics_timer_timer_function_list,
				&mxd_epics_timer_num_record_fields,
				&mxd_epics_timer_rfield_def_ptr},

{"epics_gpib", -1,	MXI_GPIB,  MXR_INTERFACE,
				&mxi_epics_gpib_record_function_list,
				NULL,
				&mxi_epics_gpib_gpib_function_list,
				&mxi_epics_gpib_num_record_fields,
				&mxi_epics_gpib_rfield_def_ptr},

{"epics_rs232", -1,	MXI_RS232,  MXR_INTERFACE,
				&mxi_epics_rs232_record_function_list,
				NULL,
				&mxi_epics_rs232_rs232_function_list,
				&mxi_epics_rs232_num_record_fields,
				&mxi_epics_rs232_rfield_def_ptr},

{"epics_vme", -1,	MXI_VME,  MXR_INTERFACE,
				&mxi_epics_vme_record_function_list,
				NULL,
				&mxi_epics_vme_vme_function_list,
				&mxi_epics_vme_num_record_fields,
				&mxi_epics_vme_rfield_def_ptr},

{"epics_string", -1,	MXV_EPICS,  MXR_VARIABLE,
				&mxv_epics_variable_record_function_list,
				&mxv_epics_variable_variable_function_list,
				NULL,
				&mxv_epics_string_variable_num_record_fields,
				&mxv_epics_string_variable_def_ptr},
{"epics_char", -1,	MXV_EPICS,  MXR_VARIABLE,
				&mxv_epics_variable_record_function_list,
				&mxv_epics_variable_variable_function_list,
				NULL,
				&mxv_epics_char_variable_num_record_fields,
				&mxv_epics_char_variable_def_ptr},
{"epics_short", -1,	MXV_EPICS,  MXR_VARIABLE,
				&mxv_epics_variable_record_function_list,
				&mxv_epics_variable_variable_function_list,
				NULL,
				&mxv_epics_short_variable_num_record_fields,
				&mxv_epics_short_variable_def_ptr},
{"epics_long", -1,	MXV_EPICS,  MXR_VARIABLE,
				&mxv_epics_variable_record_function_list,
				&mxv_epics_variable_variable_function_list,
				NULL,
				&mxv_epics_long_variable_num_record_fields,
				&mxv_epics_long_variable_def_ptr},
{"epics_float", -1,	MXV_EPICS,  MXR_VARIABLE,
				&mxv_epics_variable_record_function_list,
				&mxv_epics_variable_variable_function_list,
				NULL,
				&mxv_epics_float_variable_num_record_fields,
				&mxv_epics_float_variable_def_ptr},
{"epics_double", -1,	MXV_EPICS,  MXR_VARIABLE,
				&mxv_epics_variable_record_function_list,
				&mxv_epics_variable_variable_function_list,
				NULL,
				&mxv_epics_double_variable_num_record_fields,
				&mxv_epics_double_variable_def_ptr},

{"epics_timeout", -1,	MXV_EPICS,  MXR_VARIABLE,
				&mxv_epics_timeout_record_function_list,
				&mxv_epics_timeout_variable_function_list,
				NULL,
				&mxv_epics_timeout_num_record_fields,
				&mxv_epics_timeout_field_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

/*-----------------------------------------------------------------------*/

/* The primary job of epics_driver_init() here is to see if network
 * debugging is turned on in the running MX database.  If it is,
 * then this function turns on EPICS network debugging.
 */

static mx_bool_type
epics_driver_init( MX_MODULE *module )
{
	static const char fname[] = "epics_driver_init()";

	MX_RECORD *record_list;
	MX_LIST_HEAD *list_head;
	int epics_debug_flag;

#if DEBUG_EPICS_INIT
	MX_DEBUG(-2,("%s invoked for '%s'", fname, module->name));
#endif

	record_list = module->record_list;

#if DEBUG_EPICS_INIT
	MX_DEBUG(-2,("%s: record_list = %p", fname, record_list));
#endif
	/* If the 'epics' module does not know about a record list,
	 * then there is nothing further for it to do.
	 */

	if ( record_list == (MX_RECORD *) NULL )
		return TRUE;

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the MX record list "
		"used by module '%s' is NULL.", module->name );

		return FALSE;
	}

	/* Use the network debug flags from the MX list head structure. */

	if ( list_head->network_debug_flags == 0 ) {
		epics_debug_flag = FALSE;
	} else {
		epics_debug_flag = TRUE;
	}

#if DEBUG_EPICS_INIT
	MX_DEBUG(-2,("%s: epics_debug_flag = %d", fname, epics_debug_flag));
#endif

	mx_epics_set_debug_flag( epics_debug_flag );

	return TRUE;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"epics",
	MX_VERSION,
	epics_driver_table,
	NULL,
	NULL
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT_epics__ = epics_driver_init;

