/*
 * Name:    mx_driver_linux_power_pmac.c
 *
 * Purpose: PowerPMAC systems typically are memory constrained, so this
 *          file only includes a small subset of the available MX drivers
 *          to reduce memory consumption by driver code.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_stdint.h"

#include "mx_driver.h"
#include "mx_interval_timer.h"

/* -- Include header files that define MX_XXX_FUNCTION_LIST structures. -- */

#include "mx_rs232.h"
#include "mx_vme.h"

#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_motor.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_relay.h"
#include "mx_pulse_generator.h"

#include "mx_variable.h"
#include "mx_vinline.h"

#include "mx_bluice.h"

#include "mx_list_head.h"

#include "mx_dead_reckoning.h"

#include "mx_hrt_debug.h"

/* Include the header files for all of the interfaces and devices. */

#include "i_tty.h"

  /********************** Record Types **********************/

MX_DRIVER mx_type_list[] = {

{"list_head",      MXT_LIST_HEAD,    MXL_LIST_HEAD,      MXR_LIST_HEAD,
				&mxr_list_head_record_function_list,
				NULL,
				NULL,
				&mxr_list_head_num_record_fields,
				&mxr_list_head_rfield_def_ptr},

  /* =================== Interface types =================== */

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_RTEMS)
{"tty",            MXI_232_TTY,        MXI_RS232,          MXR_INTERFACE,
				&mxi_tty_record_function_list,
				NULL,
				&mxi_tty_rs232_function_list,
				&mxi_tty_num_record_fields,
				&mxi_tty_rfield_def_ptr},
#endif /* OS_UNIX */

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

