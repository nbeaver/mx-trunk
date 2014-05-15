/*
 * Name:    Socket_MX.c
 *
 * Purpose: A replacement for the Socket.cpp file in the Newport XPS source
 *          code that implements socket communication using MX facilities.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "Socket.h"

#include "mx_util.h"
#include "mx_socket.h"

int
ConnectToServer( char *Ip_Address,
		int Ip_Port,
		double Timeout )
{
	return (-1);
}

void
SetTCPTimeout( int SocketID,
		double Timeout )
{
	return;
}

void
SendAndReceive( int SocketID,
		char sSendString[],
		char sReturnString[],
		int iReturnStringSize )
{
	return;
}

void
CloseSocket( int SocketID )
{
	return;
}

char *
GetError( int SocketID )
{
	static char f[] = "Placeholder";

	return f;
}

void
strncpyWithEOS( char *szStringOut,
		const char *szStringIn,
		int nNumberOfCharToCopy,
		int nStringOutSize )
{
	return;
}

