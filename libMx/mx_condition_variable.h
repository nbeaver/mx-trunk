/*
 * Name:    mx_condition_variable.h
 *
 * Purpose: Header file for MX condition variable functions.
 *
 *          These functions are modeled on Posix condition variables.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_CONDITION_VARIABLE_H__
#define __MX_CONDITION_VARIABLE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_mutex.h"

typedef struct {
	void *cv_private;
} MX_CONDITION_VARIABLE;

MX_API mx_status_type mx_condition_variable_create(
						MX_CONDITION_VARIABLE **cv );

MX_API mx_status_type mx_condition_variable_destroy(
						MX_CONDITION_VARIABLE *cv );

MX_API mx_status_type mx_condition_variable_wait( MX_CONDITION_VARIABLE *cv,
							MX_MUTEX *mutex );

MX_API mx_status_type mx_condition_variable_timed_wait(
						MX_CONDITION_VARIABLE *cv,
						MX_MUTEX *mutex,
						const struct timespec *ts );

MX_API mx_status_type mx_condition_variable_signal( MX_CONDITION_VARIABLE *cv );

MX_API mx_status_type mx_condition_variable_broadcast(
						MX_CONDITION_VARIABLE *cv );

#ifdef __cplusplus
}
#endif

#endif /* __MX_CONDITION_VARIABLE_H__ */
