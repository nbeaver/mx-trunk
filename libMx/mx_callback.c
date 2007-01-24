/*
 * Name:    mx_callback.c
 *
 * Purpose: MX callback handling functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_net.h"
#include "mx_callback.h"

MX_EXPORT mx_status_type
mx_network_add_callback( MX_NETWORK_FIELD *nf,
			double poll_interval,
			mx_status_type ( *callback_function )(
						MX_NETWORK_FIELD *, void * ),
			void *callback_argument )
{
	static const char fname[] = "mx_network_add_callback()";

	MX_DEBUG(-2,("%s invoked.", fname));
	MX_DEBUG(-2,("%s: server = '%s', nfname = '%s', handle = (%ld,%ld)",
		fname, nf->server_record->name, nf->nfname,
		nf->record_handle, nf->field_handle ));

	return mx_error( MXE_NOT_YET_IMPLEMENTED,
			fname, "Not yet implemented.");
}

