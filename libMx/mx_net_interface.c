/*
 * Name:    mx_net_interface.c
 *
 * Purpose: Functions for interacting with network interfaces such as Ethernet.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_GET_INTERFACE_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_dynamic_library.h"
#include "mx_net_interface.h"

#if !defined( OS_WIN32 )
#include "mx_socket.h"
#endif

/*---*/

#if defined( OS_WIN32 )

#include <winsock2.h>
#include <iphlpapi.h>

/* FIXME: winsock2.h must be included before mx_socket.h.  Figure out why. */

#include "mx_socket.h"

/*----*/

/* FIXME: The web page
 *   http://stackoverflow.com/questions/24702408/get-subnet-mask-from-getadapteraddresses
 *   appears to have useful information for what to do prior to Vista.
 */

static uint32_t
mxp_network_get_ipv4_subnet_mask_from_unicast_address(
				IP_ADAPTER_UNICAST_ADDRESS *unicast_address )
{
#if 0
	static const char fname[] =
		"mxp_network_get_ipv4_subnet_mask_from_unicast_address()";
#endif

	int i, subnet_mask_length;
	uint32_t ipv4_subnet_mask;

	/* FIXME: OnLinkPrefixLength only exists for Vista and later. */

	subnet_mask_length = unicast_address->OnLinkPrefixLength;

#if 0
	MX_DEBUG(-2,("%s: subnet_mask_length = %d", fname, subnet_mask_length));
#endif

	ipv4_subnet_mask = 0;

	for ( i = 0; i < subnet_mask_length; i++ ) {

		ipv4_subnet_mask <<= 1;

		ipv4_subnet_mask |= 1;
	}

	ipv4_subnet_mask <<= ( 32 - subnet_mask_length );

	/* Convert the subnet mask to network byte order. */

	ipv4_subnet_mask = htonl( ipv4_subnet_mask );

#if 0
	MX_DEBUG(-2,("%s: ipv4_subnet_mask = %#lx",
		fname, (unsigned long) ipv4_subnet_mask));
#endif

	return ipv4_subnet_mask;
}

/*----*/

/* FIXME: This function currently supports only IPV4 addresses. */

MX_EXPORT mx_status_type
mx_network_get_interface_from_host_address( MX_NETWORK_INTERFACE **ni,
				struct sockaddr *ip_address_struct )
{
	static const char fname[] =
		"mx_network_get_interface_from_host_address()";

	ULONG output_buffer_length = 0;
	DWORD os_status;
	MX_NETWORK_INTERFACE *ni_ptr;
	uint32_t ipv4_address, ipv4_subnet_mask;
	int i, max_attempts;
	mx_bool_type address_found;
	mx_status_type mx_status;

	typedef ULONG (*GetAdaptersAddresses_type)( ULONG, ULONG, VOID *,
					IP_ADAPTER_ADDRESSES *, ULONG * );

	static GetAdaptersAddresses_type
		ptr_GetAdaptersAddresses = NULL;

	IP_ADAPTER_ADDRESSES *addresses = NULL;
	IP_ADAPTER_ADDRESSES *current_address = NULL;
	IP_ADAPTER_UNICAST_ADDRESS *unicast_address = NULL;
	SOCKET_ADDRESS *socket_address = NULL;
	struct sockaddr *sockaddr = NULL;
	struct sockaddr_in *sockaddr_in = NULL;
	unsigned long local_address = 0;

	if ( ni == (MX_NETWORK_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_INTERFACE pointer passed was NULL." );
	}
	if ( ip_address_struct == (struct sockaddr *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The ip_address_struct pointer passed was NULL." );
	}
	if ( ip_address_struct->sa_family != AF_INET ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The ip_address_struct argument passed was not for IPV4." );
	}

	ipv4_address =
		((struct sockaddr_in *) ip_address_struct)->sin_addr.s_addr;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
	MX_DEBUG(-2,("%s: ipv4_address = %#lx", fname, ipv4_address));
#endif

	/* Try to load 'iphlpapi.dll' and get a pointer to the
	 * GetAdapterAddresses() function.
	 */

	mx_status = mx_dynamic_library_get_library_and_symbol(
				"iphlpapi.dll", "GetAdaptersAddresses", NULL,
				(void **) &ptr_GetAdaptersAddresses, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the list of IP_ADAPTER_ADDRESSES structures for this computer. */

	output_buffer_length = 15000;

	max_attempts = 3;

	for ( i = 0; i < max_attempts; i++ ) {
		addresses = (IP_ADAPTER_ADDRESSES *)
				malloc( output_buffer_length );

		if ( addresses == (IP_ADAPTER_ADDRESSES *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %lu byte "
			"buffer of IP_ADAPTER_ADDRESSES.",
				output_buffer_length );
		}

		os_status = (*ptr_GetAdaptersAddresses)( AF_INET, 0, NULL,
					addresses, &output_buffer_length );

		if ( os_status == ERROR_BUFFER_OVERFLOW ) {
			/* Throw away the buffer that we allocated above,
			 * since we have been told that the buffer needs
			 * to be longer.
			 */

			mx_free( addresses );
			addresses = NULL;
		} else {
			/* Exit the loop early. */

			break;
		}
	}

	if ( os_status != NO_ERROR ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to get a list of network adapters failed "
		"with error code = %d.", (int) os_status );
	}

	/* Walk through the list of addresses looking for the address
	 * that we want.
	 */

	address_found = FALSE;

	current_address = addresses;

	while ( current_address != NULL ) {

#if MXD_NETWORK_GET_INTERFACE_DEBUG
	    MX_DEBUG(-2,("*************************************************"));
	    MX_DEBUG(-2,
	    ("%s: adapter name = '%s', friendly name = '%wS', "
	    "description = '%wS'",
		fname, current_address->AdapterName,
		current_address->FriendlyName,
		current_address->Description));
#endif

	    unicast_address = current_address->FirstUnicastAddress;

	    while ( unicast_address != NULL ) {
		socket_address = &(unicast_address->Address);

		ipv4_subnet_mask =
			mxp_network_get_ipv4_subnet_mask_from_unicast_address(
				unicast_address );

		if ( socket_address != NULL ) {
		    sockaddr = socket_address->lpSockaddr;

		    if ( sockaddr != NULL ) {
			if ( sockaddr->sa_family == AF_INET ) {
			    sockaddr_in = (struct sockaddr_in *) sockaddr;

			    /* The socket address is in network byte order
			     * also known as bigendian, so we must reverse
			     * the endianness.
			     */

			    local_address = sockaddr_in->sin_addr.s_addr;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
			    MX_DEBUG(-2,
				("%s: local address = %lu (%#lx)", fname,
				local_address, local_address));
			    MX_DEBUG(-2,
				("%s: remote address = %lu (%#lx)", fname,
				ipv4_address, ipv4_address));
			    MX_DEBUG(-2,
				("%s: subnet mask = %lu (%#lx)", fname,
				ipv4_subnet_mask, ipv4_subnet_mask));
#endif

			    /* Check to see if this address is in the same
			     * subnet as the IPV4 address passed in one of
			     * the arguments to this function.
			     */

			    if ( ( local_address & ipv4_subnet_mask )
				== ( ipv4_address & ipv4_subnet_mask ) )
			    {
				mx_breakpoint();

				address_found = TRUE;
			    } else {
				address_found = FALSE;
			    }

			    if ( address_found ) {

#if MXD_NETWORK_GET_INTERFACE_DEBUG
				MX_DEBUG(-2,("%s: address_found = %d",
				    fname, address_found));
#endif

				ni_ptr = (MX_NETWORK_INTERFACE *)
				    malloc( sizeof(MX_NETWORK_INTERFACE) );

				if ( ni_ptr == (MX_NETWORK_INTERFACE *) NULL ) {
				    return mx_error( MXE_OUT_OF_MEMORY, fname,
				    "Ran out of memory trying to allocate an "
				    "MX_NETWORK_INTERFACE structure." );
				}

				WideCharToMultiByte( CP_ACP, 0,
					current_address->FriendlyName, -1,
					ni_ptr->name,
					sizeof(ni_ptr->name),
					NULL, NULL );

#if 1
				strlcpy( ni_ptr->raw_name,
					current_address->AdapterName,
					sizeof(ni_ptr->raw_name) );
#endif

				ni_ptr->ipv4_address = local_address;
				ni_ptr->ipv4_subnet_mask = ipv4_subnet_mask;
				ni_ptr->mtu = current_address->Mtu;
				ni_ptr->net_private = NULL;
			    }
			}
		    }
		}

		if ( address_found ) {
		    break;
		}
		unicast_address = unicast_address->Next;
	    }

	    if ( address_found ) {
		break;
	    }
	    current_address = current_address->Next;
	}

	if ( address_found ) {
		*ni = ni_ptr;

		return MX_SUCCESSFUL_RESULT;
	} else {
		*ni = NULL;

		return mx_error( MXE_NOT_FOUND, fname,
		"No network interface was found for IP %#lx.",
			ipv4_address );
	}
}

#elif defined( OS_LINUX ) || defined( OS_CYGWIN )

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <net/if.h>
#include <ifaddrs.h>

/* FIXME: This function currently supports only IPV4 addresses. */

MX_EXPORT mx_status_type
mx_network_get_interface_from_host_address( MX_NETWORK_INTERFACE **ni,
			struct sockaddr *ip_address_struct,
			struct sockaddr *subnet_mask_struct )
{
	static const char fname[] =
		"mx_network_get_interface_from_host_address()";

	MX_NETWORK_INTERFACE *ni_ptr = NULL;
	struct ifaddrs *if_list = NULL;
	struct ifaddrs *if_current = NULL;
	unsigned long ipv4_address, ipv4_subnet_mask;
	unsigned long host_ip_address;
	int os_status, saved_errno;
	struct sockaddr *sockaddr_struct = NULL;
	struct sockaddr_in *sockaddr_in_struct = NULL;
	struct ifreq ifreq_struct;
	mx_bool_type address_found;

	if ( ni == (MX_NETWORK_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_INTERFACE pointer passed was NULL." );
	}
	if ( ip_address_struct == (struct sockaddr *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The ip_address_struct pointer passed was NULL." );
	}
	if ( subnet_mask_struct == (struct sockaddr *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The subnet_mask_struct pointer passed was NULL." );
	}

	if ( ip_address_struct->sa_family != AF_INET ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The ip_address_struct argument passed was not for IPV4." );
	}
	if ( subnet_mask_struct->sa_family != AF_INET ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The subnet_mask_struct argument passed was not for IPV4." );
	}

	ipv4_address =
		((struct sockaddr_in *) ip_address_struct)->sin_addr.s_addr;

	ipv4_subnet_mask =
		((struct sockaddr_in *) subnet_mask_struct)->sin_addr.s_addr;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
	MX_DEBUG(-2,("%s: ipv4_address = %#lx", fname, ipv4_address));
	MX_DEBUG(-2,("%s: ipv4_subnet_mask = %#lx", fname, ipv4_subnet_mask));
#endif

	os_status = getifaddrs( &if_list );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"A call to getifaddrs() failed with errno = %d.  "
		"error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

	address_found = FALSE;

	if_current = if_list;

	while ( if_current != NULL ) {
		MX_DEBUG(-2,("%s: interface name = '%s'",
			fname, if_current->ifa_name));

		sockaddr_struct = if_current->ifa_addr;

		if ( sockaddr_struct->sa_family == AF_INET ) {
			sockaddr_in_struct = ( struct sockaddr_in *)
						sockaddr_struct;

			host_ip_address =
				sockaddr_in_struct->sin_addr.s_addr;

			MX_DEBUG(-2,("%s: host_ip_address = %#lx",
				fname, host_ip_address));

			if ( ( host_ip_address & ipv4_subnet_mask )
			  == ( ipv4_address & ipv4_subnet_mask ) )
			{
				address_found = TRUE;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
				MX_DEBUG(-2,("%s: address_found = %d",
				    fname, address_found));
#endif

				ni_ptr = (MX_NETWORK_INTERFACE *)
				    malloc( sizeof(MX_NETWORK_INTERFACE) );

				if ( ni_ptr == (MX_NETWORK_INTERFACE *) NULL ) {
				    return mx_error( MXE_OUT_OF_MEMORY, fname,
				    "Ran out of memory trying to allocate an "
				    "MX_NETWORK_INTERFACE structure." );
				}

				memset(&ifreq_struct, 0, sizeof(ifreq_struct));

				strlcpy( ifreq_struct.ifr_name,
					if_current->ifa_name,
					sizeof( ifreq_struct.ifr_name ) );

				/* FIXME: need a file descriptor for
				 * the ioctl() call.
				 */

				os_status = ioctl(0, SIOCGIFMTU, &ifreq_struct);

				strlcpy( ni_ptr->name,
					if_current->ifa_name,
					sizeof( ni_ptr->name ) );

				ni_ptr->ipv4_address = host_ip_address;
				ni_ptr->ipv4_subnet_mask = ipv4_subnet_mask;
				ni_ptr->mtu = ifreq_struct.ifr_mtu;
				ni_ptr->net_private = NULL;
			}
		}

		if ( address_found ) {
			break;
		}

		if_current = if_current->ifa_next;
	}

	return MX_SUCCESSFUL_RESULT;
}

#else
#error MX network interface functions have not yet been defined for this target.
#endif

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_network_get_interface_from_hostname( MX_NETWORK_INTERFACE **ni,
						char *hostname )
{
	static const char fname[] = "mx_network_get_interface_from_hostname()";

	unsigned long inet_address;
	struct sockaddr_in sockaddr_in;
	mx_status_type mx_status;

	if ( ni == (MX_NETWORK_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_INTERFACE pointer passed was NULL." );
	}
	if ( hostname == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The hostname pointer passed was NULL." );
	}

	mx_status = mx_socket_get_inet_address( hostname, &inet_address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	memset( &sockaddr_in, 0, sizeof(sockaddr_in) );

	/* FIXME: The following is IPV4 specific. */

	sockaddr_in.sin_family = AF_INET;
	sockaddr_in.sin_port = 0;
	sockaddr_in.sin_addr.s_addr = inet_address;

	mx_status = mx_network_get_interface_from_host_address( ni,
					(struct sockaddr *) &sockaddr_in );

	return mx_status;
}

