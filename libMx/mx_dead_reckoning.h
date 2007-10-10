/*
 * Name:    mx_dead_reckoning.h
 *
 * Purpose: Header file for dead reckoning calculations of motor positions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_DEAD_RECKONING_H__
#define __MX_DEAD_RECKONING_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_CLOCK_TICK start_of_move_tick;
	MX_CLOCK_TICK end_of_acceleration_tick;
	MX_CLOCK_TICK start_of_deceleration_tick;
	MX_CLOCK_TICK end_of_move_tick;

	double start_position;
	double end_of_acceleration_position;
	double start_of_deceleration_position;
	double end_position;

	MX_MOTOR *motor;

} MX_DEAD_RECKONING;

MX_API mx_status_type mx_dead_reckoning_initialize(
					MX_DEAD_RECKONING *dead_reckoning,
					MX_MOTOR *motor );

MX_API mx_status_type mx_dead_reckoning_start_motion(
					MX_DEAD_RECKONING *dead_reckoning,
					double start_position,
					double end_position,
					MX_CLOCK_TICK *starting_clock_tick );

MX_API mx_status_type mx_dead_reckoning_predict_motion(
					MX_DEAD_RECKONING *dead_reckoning,
					double *present_position,
					double *present_velocity );

MX_API mx_status_type mx_dead_reckoning_abort_motion(
					MX_DEAD_RECKONING *dead_reckoning );

#ifdef __cplusplus
}
#endif

#endif /* __MX_DEAD_RECKONING_H__ */

