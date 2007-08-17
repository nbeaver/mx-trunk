/*
 * Name:    mx_driver_class.c
 *
 * Purpose: Describes the list of driver classes and superclasses supported
 *          by this version of the program.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_osdef.h"
#include "mx_stdint.h"

#include "mx_driver.h"

/* -- Define lists that relate types to classes and to function lists. -- */

  /****************** Record Superclasses ********************/

MX_DRIVER mx_superclass_list[] = {
{"list_head_sclass", 0, 0, MXR_LIST_HEAD,     NULL, NULL, NULL, NULL, NULL},
{"interface",        0, 0, MXR_INTERFACE,     NULL, NULL, NULL, NULL, NULL},
{"device",           0, 0, MXR_DEVICE,        NULL, NULL, NULL, NULL, NULL},
{"scan",             0, 0, MXR_SCAN,          NULL, NULL, NULL, NULL, NULL},
{"variable",         0, 0, MXR_VARIABLE,      NULL, NULL, NULL, NULL, NULL},
{"server",           0, 0, MXR_SERVER,        NULL, NULL, NULL, NULL, NULL},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

  /********************* Record Classes **********************/

MX_DRIVER mx_class_list[] = {

{"list_head_class", 0, MXL_LIST_HEAD,      MXR_LIST_HEAD,
				NULL, NULL, NULL, NULL, NULL},

  /* ================== Interface classes ================== */

{"rs232",          0, MXI_RS232,          MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"gpib",           0, MXI_GPIB,           MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"camac",          0, MXI_CAMAC,          MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"generic",        0, MXI_GENERIC,        MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"portio",         0, MXI_PORTIO,         MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"vme",            0, MXI_VME,            MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"mmio",           0, MXI_MMIO,           MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"modbus",         0, MXI_MODBUS,         MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"usb",            0, MXI_USB,            MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"camera_link",    0, MXI_CAMERA_LINK,    MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},

  /* =================== Device classes =================== */

{"analog_input",   0, MXC_ANALOG_INPUT,   MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"analog_output",  0, MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"digital_input",  0, MXC_DIGITAL_INPUT,  MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"digital_output", 0, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"motor",          0, MXC_MOTOR,          MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"encoder",        0, MXC_ENCODER,        MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"scaler",         0, MXC_SCALER,         MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"timer",          0, MXC_TIMER,          MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"amplifier",      0, MXC_AMPLIFIER,      MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"relay",          0, MXC_RELAY,          MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mca",            0, MXC_MULTICHANNEL_ANALYZER, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mce",            0, MXC_MULTICHANNEL_ENCODER, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mcs",            0, MXC_MULTICHANNEL_SCALER, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"table",          0, MXC_TABLE,          MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"autoscale",      0, MXC_AUTOSCALE,      MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"pulse_generator",0, MXC_PULSE_GENERATOR, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"sca",            0, MXC_SINGLE_CHANNEL_ANALYZER, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"ccd",            0, MXC_CCD,            MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"sample_changer", 0, MXC_SAMPLE_CHANGER, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mcai",           0, MXC_MULTICHANNEL_ANALOG_INPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mcao",           0, MXC_MULTICHANNEL_ANALOG_OUTPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mcdi",           0, MXC_MULTICHANNEL_DIGITAL_INPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mcdo",           0, MXC_MULTICHANNEL_DIGITAL_OUTPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"ptz",            0, MXC_PAN_TILT_ZOOM,  MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"area_detector",  0, MXC_AREA_DETECTOR, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"video_input",    0, MXC_VIDEO_INPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"video_output",   0, MXC_VIDEO_OUTPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},

  /* ==================== Scan classes ==================== */

{"linear_scan",    0, MXS_LINEAR_SCAN,    MXR_SCAN,
				NULL, NULL, NULL, NULL, NULL},
{"list_scan",      0, MXS_LIST_SCAN,      MXR_SCAN,
				NULL, NULL, NULL, NULL, NULL},
{"xafs_scan_class",0, MXS_XAFS_SCAN,      MXR_SCAN,
				NULL, NULL, NULL, NULL, NULL},
{"quick_scan",     0, MXS_QUICK_SCAN,     MXR_SCAN,
				NULL, NULL, NULL, NULL, NULL},

  /* ================== Variable classes ================== */

{"inline",         0, MXV_INLINE,         MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"net_variable",   0, MXV_NETWORK,        MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"epics_variable", 0, MXV_EPICS,          MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"calc",           0, MXV_CALC,           MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"pmac",           0, MXV_PMAC,           MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"spec_property",  0, MXV_SPEC,           MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"bluice_variable",0, MXV_BLUICE,         MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"file",           0, MXV_FILE,           MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},

  /* ================== Server classes ================== */

{"network",        0, MXN_NETWORK_SERVER, MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},
{"spec",           0, MXN_SPEC,           MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},
{"bluice",         0, MXN_BLUICE,         MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};


/* -- mx_type_list is now defined in another file. -- */

extern MX_DRIVER mx_type_list[];

/* -- Define the list of record types. -- */

MX_DRIVER *mx_list_of_types[] = {
	mx_superclass_list,
	mx_class_list,
	mx_type_list,
	NULL
};

MX_EXPORT MX_DRIVER **
mx_get_driver_lists( void )
{
	MX_DRIVER **ptr;

	ptr = mx_list_of_types;

	return ptr;
}

