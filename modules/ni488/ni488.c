/*
 * Name:    ni488.c
 *
 * Purpose: Module wrapper for the National Instruments IEEE-488 library
 *          or for the open source Linux-GPIB library.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#if ( defined(HAVE_NI488) && defined(HAVE_LINUX_GPIB) )
#error You must not define both HAVE_NI488 and HAVE_LINUX_GPIB.  They are incompatible since the National Instruments library and the Linux-GPIB library both export functions and variables with the same names.
#endif

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_module.h"
#include "mx_gpib.h"
#include "i_ni488.h"

/*-------------------------------------------------------------------------*/

#if HAVE_NI488

MX_DRIVER ni488_driver_table[] = {

{"ni488", -1, MXI_GPIB, MXR_INTERFACE,
		&mxi_ni488_record_function_list,
		NULL,
		&mxi_ni488_gpib_function_list,
		&mxi_ni488_num_record_fields,
		&mxi_ni488_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"ni488",
	MX_VERSION,
	ni488_driver_table,
	NULL,
	NULL
};

#endif /* HAVE_NI488 */

/*-------------------------------------------------------------------------*/

#if HAVE_LINUX_GPIB

MX_DRIVER ni488_driver_table[] = {

{"linux_gpib",  -1,    MXI_GPIB,  MXR_INTERFACE,
		&mxi_ni488_record_function_list,
		NULL,
		&mxi_ni488_gpib_function_list,
		&mxi_ni488_num_record_fields,
		&mxi_ni488_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"linux_gpib",
	MX_VERSION,
	ni488_driver_table,
	NULL,
	NULL
};

#endif /* HAVE_LINUX_GPIB */

/*-------------------------------------------------------------------------*/

