/* 
 * Name:    mx_service.h
 *
 * Purpose: MX header file for interacting with service control managers.
 *
 * Author:  William Lavender
 *
 * This set of functions is for interacting with platform-specific
 * service control managers.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_SERVICE_H__
#define __MX_SERVICE_H__

#include "mx_util.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXSCM_RUNNING	1
#define MXSCM_STOPPED	2

#define MXSCM_STARTING	3
#define MXSCM_STOPPING	4

#define MXSCM_ERROR	5

MX_API mx_status_type mx_scm_notify( int notification_type,
					const char *message );

#ifdef __cplusplus
}
#endif

#endif /* __MX_SERVICE_H__ */
