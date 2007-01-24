/*
 * Name:    mx_callback.h
 *
 * Purpose: Header for MX callback handling functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_CALLBACK_H__
#define __MX_CALLBACK_H__

/*--- Network client callbacks ---*/

MX_API mx_status_type mx_network_add_callback(
					MX_NETWORK_FIELD *nf,
					double poll_interval,
					mx_status_type ( *callback_function )
						( MX_NETWORK_FIELD *, void * ),
					void *callback_argument );
					

#endif /* __MX_CALLBACK_H__ */

