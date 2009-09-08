/*
 * Name:    cmd.h
 *
 * Purpose: Command interpreter functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __COMMAND_H__
#define __COMMAND_H__

#define COMMAND_NAME_LENGTH	15

typedef struct {
	int (*function_ptr)( int argc, char *argv[] );
	int min_required_chars;
	char name[COMMAND_NAME_LENGTH + 1];
} COMMAND;

extern int cmd_execute_command_line( int num_commands,
				COMMAND *command_list, char *command_line );

extern COMMAND *cmd_get_command_from_list( int num_commands, 
				COMMAND *command_list, char *string );

extern char **cmd_parse_command_line( int *argc, char *command_line );

extern int cmd_split_command_line( char *command_line,
				int *cmd_argc, char ***cmd_argv,
				char **split_buffer );

#define cmd_free_command_line(av,sb) \
	do {			\
		mx_free( av );	\
		mx_free( sb );	\
	} while (0)

extern int cmd_run_command( char *command_line );

extern char *cmd_read_next_command_line( char *prompt );

extern char *cmd_program_name( void );

extern void cmd_set_program_name( char *name );

#endif /* __COMMAND_H__ */

