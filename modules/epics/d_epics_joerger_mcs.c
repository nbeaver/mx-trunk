/*
 * Name:    d_epics_joerger_mcs.c 
 *
 * Purpose: MX multichannel scaler driver for Joerger VSC8/16 counter/timers
 *          controlled using the EPICS Scaler record.
 *
 *          Please note that the EPICS Scaler record numbers scalers starting
 *          at 1, but the mcs->data_array numbers scalers starting at 0.
 *
 * Author:  William Lavender
 *
 * Warning: The EPICS Scaler record was not written to be used in this way,
 *          so do not be surprised if odd things happen.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_epics.h"
#include "mx_mcs.h"
#include "d_epics_joerger_mcs.h"

