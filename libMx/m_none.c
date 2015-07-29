/*
 * Name:    m_none.c
 *
 * Purpose: Implements dummy measurements for MX scan records.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2012, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_scan.h"

#include "mx_measurement.h"
#include "m_none.h"

MX_MEASUREMENT_FUNCTION_LIST mxm_none_function_list = {
			mxm_none_configure,
			mxm_none_deconfigure,
			NULL,
			NULL,
			NULL,
			NULL,
			mxm_none_acquire_data,
};

MX_EXPORT mx_status_type
mxm_none_configure( MX_MEASUREMENT *measurement )
{
	measurement->measurement_type_struct = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_none_deconfigure( MX_MEASUREMENT *measurement )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_none_acquire_data( MX_MEASUREMENT *measurement )
{
	return MX_SUCCESSFUL_RESULT;
}

