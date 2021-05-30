/*
 * Name:    mx_net_interface.c
 *
 * Purpose: Functions for interacting with network interfaces such as Ethernet.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2016-2018, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_GET_INTERFACE_DEBUG		FALSE

#if ( defined( OS_WIN32 ) && defined( _MSC_VER ) && ( _MSC_VER >= 1300 ) )
#  include <winsock2.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_dynamic_library.h"
#include "mx_socket.h"
#include "mx_net_interface.h"

/*---*/

#if defined( OS_WIN32 )

#  if !defined( _MSC_VER ) || ( _MSC_VER < 1300 )

   /* Visual C++ 6 and before do not support this. */

MX_EXPORT mx_status_type
mx_network_get_interface_from_host_address( MX_NETWORK_INTERFACE **ni,
					struct sockaddr *host_address_struct )
{
	static const char fname[] =
		"mx_network_get_interface_from_host_address()";

	return mx_error( MXE_UNSUPPORTED, fname,
  "Not available for non-Visual C++ compilers or for Visual C++ 6 or before." );
}

#  else  /* Visual Studio 2002 or later. */

#include <iphlpapi.h>

/*======================= Windows XP or before =======================*/

/*---- Use GetIpAddrTable() which is IPV4 only ----*/

static mx_status_type
mx_network_giat_get_interface_from_host_address( MX_NETWORK_INTERFACE **ni,
				struct sockaddr *ip_address_struct )
{
	static const char fname[] =
		"mx_network_giat_get_interface_from_host_address()";

	MX_NETWORK_INTERFACE *ni_ptr;
	uint32_t ipv4_address, mib_ipv4_address, mib_ipv4_subnet_mask;
	MIB_IPADDRTABLE *addresses = NULL;
	MIB_IPADDRROW *address_entry_table = NULL;
	MIB_IPADDRROW *address_entry = NULL;
	long n, num_addresses;
	ULONG address_table_size;
	DWORD os_status;
	int i, max_attempts;
	mx_bool_type address_found;
	mx_status_type mx_status;

	typedef ULONG (*GetIpAddrTable_type)(MIB_IPADDRTABLE *, ULONG *, BOOL);

	static GetIpAddrTable_type
		ptr_GetIpAddrTable = NULL;

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

	/*----*/

	ipv4_address =
		((struct sockaddr_in *) ip_address_struct)->sin_addr.s_addr;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
	MX_DEBUG(-2,("%s: ipv4_address = %#lx", fname, ipv4_address));
#endif

	/* Try to load 'iphlpapi.dll' and get a pointer to the
	 * GetIpAddrTable() function.
	 */

	mx_status = mx_dynamic_library_get_library_and_symbol_address(
				"iphlpapi.dll", "GetIpAddrTable", NULL,
				(void **) &ptr_GetIpAddrTable, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the MIB_IPADDRTABLE structure for this computer. */

	address_table_size = 15000;

	max_attempts = 3;

	for ( i = 0; i < max_attempts; i++ ) {
		addresses = (MIB_IPADDRTABLE *)
				malloc( address_table_size );

		if ( addresses == (MIB_IPADDRTABLE *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %lu byte "
			"MIB_IPADDRTABLE structure.",
				address_table_size );
		}

		os_status = (*ptr_GetIpAddrTable)( addresses,
						&address_table_size,
						FALSE );

		if ( os_status == ERROR_INSUFFICIENT_BUFFER ) {
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

	num_addresses = addresses->dwNumEntries;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
	MX_DEBUG(-2,("%s: num_addresses = %ld", fname, num_addresses ));
#endif

	if ( num_addresses <= 0 ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
	    "No network adapters are available.  This should never happen.");
	}

	address_entry_table = addresses->table;

	address_found = FALSE;

	for ( n = 0; n < num_addresses; n++ ) {
		address_entry = &address_entry_table[n];

		mib_ipv4_address = (uint32_t) address_entry->dwAddr;
		mib_ipv4_subnet_mask = (uint32_t) address_entry->dwMask;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
		MX_DEBUG(-2,
		("%s: mib_ipv4_address = %#lx, mib_ipv4_subnet_mask = %#lx",
			fname, mib_ipv4_address, mib_ipv4_subnet_mask ));
#endif

		if ( (ipv4_address & mib_ipv4_subnet_mask)
		  == (mib_ipv4_address & mib_ipv4_subnet_mask) )
		{
			address_found = TRUE;
			break;
		}
	}

	if ( address_found == FALSE ) {
		*ni = NULL;

		return mx_error( (MXE_NOT_FOUND | MXE_QUIET), fname,
		"No network interface was found for IP %#lx.",
			(unsigned long) ipv4_address );
	}

#if MXD_NETWORK_GET_INTERFACE_DEBUG
	MX_DEBUG(-2,("%s: address_found = %d", fname, address_found));
#endif

	ni_ptr = (MX_NETWORK_INTERFACE *)
		    malloc( sizeof(MX_NETWORK_INTERFACE) );

	if ( ni_ptr == (MX_NETWORK_INTERFACE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"MX_NETWORK_INTERFACE structure." );
	}

	strlcpy( ni_ptr->name, "", sizeof(ni_ptr->name) );
	strlcpy( ni_ptr->raw_name, "", sizeof(ni_ptr->name) );

	ni_ptr->ipv4_address = mib_ipv4_address;
	ni_ptr->ipv4_subnet_mask = mib_ipv4_subnet_mask;
	ni_ptr->mtu = 0;
	ni_ptr->net_private = NULL;

	*ni = ni_ptr;

	/* FIXME: For some reason, we crash with a NULL pointer exception
	 * here unless we put _two_ return statements here.  No idea why.
	 */

	return MX_SUCCESSFUL_RESULT;
	return MX_SUCCESSFUL_RESULT;
}

#if ( MX_WINVER >= 0x0600 )

/*======================= Windows Vista or later =======================*/

/*---- Use GetAdaptersAddresses() ----*/

static uint32_t
mxp_network_get_ipv4_subnet_mask_from_unicast_address(
				IP_ADAPTER_UNICAST_ADDRESS *unicast_address )
{
	static const char fname[] =
		"mxp_network_get_ipv4_subnet_mask_from_unicast_address()";

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

static mx_status_type
mx_network_gaa_get_interface_from_host_address( MX_NETWORK_INTERFACE **ni,
				struct sockaddr *ip_address_struct )
{
	static const char fname[] =
		"mx_network_gaa_get_interface_from_host_address()";

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

	/*----*/

	ipv4_address =
		((struct sockaddr_in *) ip_address_struct)->sin_addr.s_addr;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
	MX_DEBUG(-2,("%s: ipv4_address = %#lx", fname, ipv4_address));
#endif

	/* Try to load 'iphlpapi.dll' and get a pointer to the
	 * GetAdapterAddresses() function.
	 */

	mx_status = mx_dynamic_library_get_library_and_symbol_address(
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
	    /* If this adapter is not up, then skip over it. */

	    if ( current_address->OperStatus != IfOperStatusUp ) {
		current_address = current_address->Next;
		continue;
	    }

	    /* Otherwise, continue looking at this adapter. */

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

		return mx_error( (MXE_NOT_FOUND | MXE_QUIET), fname,
		"No network interface was found for IP %#lx.",
			ipv4_address );
	}
}

#endif /* ( MX_WINVER >= 0x0600 ) */

/*--------*/

/* We have to do different things depending on which version of Windows
 * that we are running on.
 */

MX_EXPORT mx_status_type
mx_network_get_interface_from_host_address( MX_NETWORK_INTERFACE **ni,
				struct sockaddr *ip_address_struct )
{
	int os_major, os_minor, os_update;
	mx_status_type mx_status;

	mx_status = mx_get_os_version( &os_major, &os_minor, &os_update );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if ( MX_WINVER >= 0x0600 )
	if ( os_major >= 6 ) {

		/* Windows Vista or later uses GetAdaptersAddresses(). */

		mx_status = mx_network_gaa_get_interface_from_host_address(
					ni, ip_address_struct );
	} else {
		/* Older versions use GetIpAddrTable(). */

		mx_status = mx_network_giat_get_interface_from_host_address(
					ni, ip_address_struct );
	}
#else
	mx_status = mx_network_giat_get_interface_from_host_address(
					ni, ip_address_struct );
#endif

	return mx_status;
}

#  endif  /* Visual Studio 2002 or later. */

/*-------------------------------------------------------------------------*/

#elif ( defined( OS_LINUX ) && ( MX_GLIBC_VERSION >= 2003000L ) ) \
	|| ( defined( OS_LINUX ) && defined( MX_MUSL_VERSION ) ) \
	|| ( defined( OS_SOLARIS ) && ( MX_SOLARIS_VERSION >= 5011000L ) ) \
	|| ( defined( OS_CYGWIN ) && (CYGWIN_VERSION_DLL_COMBINED >= 2000000) )\
	|| defined( OS_BSD ) || defined( OS_RTEMS ) || defined( OS_QNX ) \
	|| defined( OS_HURD )

/*---- Use getifaddrs() ----*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <net/if.h>
#include <ifaddrs.h>

#include "mx_unistd.h"

#if defined(OS_SOLARIS)
#  include <sys/sockio.h>
#endif

/* FIXME: This function currently supports only IPV4 addresses. */

MX_EXPORT mx_status_type
mx_network_get_interface_from_host_address( MX_NETWORK_INTERFACE **ni,
					struct sockaddr *host_address_struct )
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
	struct sockaddr_in host_address_struct_in;
	struct sockaddr_in address_struct;
	struct sockaddr_in netmask_struct;
	struct ifreq ifreq_struct;
	int test_socket;
	mx_bool_type address_found;

	if ( ni == (MX_NETWORK_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_INTERFACE pointer passed was NULL." );
	}
	if ( host_address_struct == (struct sockaddr *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The host_address_struct pointer passed was NULL." );
	}

	if ( host_address_struct->sa_family != AF_INET ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The host_address_struct argument passed was not for IPV4." );
	}

	memcpy( &host_address_struct_in,
		host_address_struct,
		sizeof(host_address_struct_in) );

	ipv4_address = host_address_struct_in.sin_addr.s_addr;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
	MX_DEBUG(-2,("%s: ipv4_address = %#lx", fname, ipv4_address));
#endif
	/* Open a socket that can be used with the SIOCGIFMTU ioctl().
	 * The socket just needs to exist.  It does not actually have
	 * to be connected to anything.
	 */

	test_socket = socket( PF_INET, SOCK_DGRAM, 0 );

	if ( test_socket < 0 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The attempt to create a socket for SIOCGIFMTU failed." );
	}

	/* Get a list of the network interfaces on this computer. */

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

#if MXD_NETWORK_GET_INTERFACE_DEBUG
		MX_DEBUG(-2,("%s: interface name = '%s'",
			fname, if_current->ifa_name));
#endif

		sockaddr_struct = if_current->ifa_addr;

		if ( sockaddr_struct->sa_family == AF_INET ) {

			memcpy( &address_struct,
				sockaddr_struct,
				sizeof( address_struct ) );

			host_ip_address = address_struct.sin_addr.s_addr;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
			MX_DEBUG(-2,("%s: host_ip_address = %#lx",
				fname, host_ip_address));
#endif

			memcpy( &netmask_struct,
				if_current->ifa_netmask,
				sizeof( netmask_struct ) );

			ipv4_subnet_mask = netmask_struct.sin_addr.s_addr;

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

#if MXD_NETWORK_GET_INTERFACE_DEBUG
				MX_DEBUG(-2,
				("%s: #1 if_current->ifa_name = '%s'",
				    fname, if_current->ifa_name));
				MX_DEBUG(-2,
				("%s: #1 ifreq_struct.ifr_name = '%s'",
				    fname, ifreq_struct.ifr_name));
#endif

				/* Get the MTU using the test socket. */

				os_status = ioctl(test_socket,
						SIOCGIFMTU, &ifreq_struct);

#if MXD_NETWORK_GET_INTERFACE_DEBUG
				MX_DEBUG(-2,
				("%s: #2 if_current->ifa_name = '%s'",
				    fname, if_current->ifa_name));
				MX_DEBUG(-2,
				("%s: #2 ifreq_struct.ifr_name = '%s'",
				    fname, ifreq_struct.ifr_name));
				MX_DEBUG(-2,
				("%s: ifreq_struct.ifr_mtu = %d",
				    fname, ifreq_struct.ifr_mtu));
#endif

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

	if ( address_found ) {
		*ni = ni_ptr;

		return MX_SUCCESSFUL_RESULT;
	} else {
		*ni = NULL;

		return mx_error( (MXE_NOT_FOUND | MXE_QUIET), fname,
		"No network interface was found for IP %#lx.",
			ipv4_address );
	}
}

#elif defined( OS_MACOSX ) || defined( OS_LINUX ) || defined( OS_ANDROID ) \
	|| defined( OS_SOLARIS ) || defined( OS_VXWORKS ) || defined( OS_VMS ) \
	|| defined( OS_DJGPP ) || defined( OS_UNIXWARE )

/*---- Use SIOCGIFCONF and SIOCGIFMTU ----*/

/* Note: This is a fallback for Linux if using Glibc versions before 2.3
 *       or Solaris before Solaris 11.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <net/if.h>

#if defined(OS_SOLARIS) || defined(OS_UNIXWARE)
#include <unistd.h>
#include <sys/sockio.h>
#endif

/* FIXME: This function currently supports only IPV4 addresses. */

MX_EXPORT mx_status_type
mx_network_get_interface_from_host_address( MX_NETWORK_INTERFACE **ni,
					struct sockaddr *host_address_struct )
{
	static const char fname[] =
		"mx_network_get_interface_from_host_address()";

	MX_NETWORK_INTERFACE *ni_ptr = NULL;
	struct ifconf if_conf;
	unsigned long ipv4_address, ipv4_subnet_mask;
	unsigned long host_ip_address;
	int os_status, saved_errno;
	struct sockaddr *sockaddr_struct = NULL;
	struct sockaddr_in address_struct;
	struct sockaddr_in host_ipv4_address_struct;
	struct ifreq *if_req_data = NULL;
	struct ifreq *if_req_item = NULL;
	int test_socket;
	int i, num_interfaces;
	mx_bool_type address_found;

	if ( ni == (MX_NETWORK_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_INTERFACE pointer passed was NULL." );
	}
	if ( host_address_struct == (struct sockaddr *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The host_address_struct pointer passed was NULL." );
	}

	if ( host_address_struct->sa_family != AF_INET ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The host_address_struct argument passed was not for IPV4." );
	}

	memcpy( &host_ipv4_address_struct,
		host_address_struct,
		sizeof(struct sockaddr_in) );

	ipv4_address = host_ipv4_address_struct.sin_addr.s_addr;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
	MX_DEBUG(-2,("%s: ipv4_address = %#lx", fname, ipv4_address));
#endif
	/* Open a socket that can be used with the SIOCGIFCONF ioctl().
	 * The socket just needs to exist.  It does not actually have
	 * to be connected to anything.
	 */

	test_socket = socket( PF_INET, SOCK_DGRAM, 0 );

	if ( test_socket < 0 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The attempt to create a socket for SIOCGIFMTU failed." );
	}

	/* We want to get a list of interfaces using SIOCGIFCONF.
	 * However, we first need to find out how much memory to
	 * allocate for that list.  SIOCGIFCONF can be used to 
	 * find that out too.
	 */

	memset( &if_conf, 0, sizeof(if_conf) );

	if_conf.ifc_ifcu.ifcu_req = NULL;
	if_conf.ifc_len = 0;

#if defined(OS_VXWORKS)
	os_status = ioctl( test_socket, SIOCGIFCONF, (int) &if_conf );
#else
	os_status = ioctl( test_socket, SIOCGIFCONF, &if_conf );
#endif

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"An attempt to find out how many network interfaces there are "
		"for this computer using ioctl( ..., SIOCGIFCONF, ... ) "
		"failed with errno = %d.  error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

	/* Allocate enough memory for the list of interfaces. */

	if_req_data = malloc( if_conf.ifc_len );

	if ( if_req_data == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d byte "
		"array of 'struct ifreq' data.", if_conf.ifc_len );
	}

	if_conf.ifc_ifcu.ifcu_req = if_req_data;

	/* Now we are ready to get the list using SIOCGIFCONF. */

#if defined(OS_VXWORKS)
	os_status = ioctl( test_socket, SIOCGIFCONF, (int) &if_conf );
#else
	os_status = ioctl( test_socket, SIOCGIFCONF, &if_conf );
#endif

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"An attempt to get a list of network interfaces "
		"for this computer using ioctl( ..., SIOCGIFCONF, ... ) "
		"failed with errno = %d.  error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

	num_interfaces = if_conf.ifc_len / sizeof(struct ifreq);

#if MXD_NETWORK_GET_INTERFACE_DEBUG
	MX_DEBUG(-2,("%s: This computer has %d network interfaces.",
		fname, num_interfaces));
#endif
	address_found = FALSE;

	for ( i = 0; i < num_interfaces; i++ ) {
		if_req_item = &if_req_data[i];

#if MXD_NETWORK_GET_INTERFACE_DEBUG
		MX_DEBUG(-2,("%s: interface '%s'",
			fname, if_req_item->ifr_ifrn.ifrn_name));
#endif

		sockaddr_struct = &(if_req_item->ifr_addr);

		if ( sockaddr_struct->sa_family == AF_INET ) {

			memcpy( &address_struct,
				sockaddr_struct,
				sizeof(struct sockaddr_in) );

			host_ip_address = address_struct.sin_addr.s_addr;

#if MXD_NETWORK_GET_INTERFACE_DEBUG
			MX_DEBUG(-2,("%s: host_ip_address = %#lx",
				fname, host_ip_address));
#endif
			/* FIXME: The following statement is bogus.
			 * It means a netmask of 255.255.255.0
			 */

			ipv4_subnet_mask = 0xffffff;

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

#if defined( OS_ANDROID )
				strlcpy( ni_ptr->name,
					if_req_item->ifr_ifrn.ifrn_name,
					sizeof( ni_ptr->name ) );
#else
				strlcpy( ni_ptr->name,
					if_req_item->ifr_name,
					sizeof( ni_ptr->name ) );
#endif

				ni_ptr->ipv4_address = host_ip_address;
				ni_ptr->ipv4_subnet_mask = ipv4_subnet_mask;
				ni_ptr->mtu = 0;
				ni_ptr->net_private = NULL;
			}
		}

		if ( address_found ) {
			break;
		}
	}

	if ( address_found ) {
		*ni = ni_ptr;

		return MX_SUCCESSFUL_RESULT;
	} else {
		*ni = NULL;

		return mx_error( (MXE_NOT_FOUND | MXE_QUIET), fname,
		"No network interface was found for IP %#lx.",
			ipv4_address );
	}
}

#elif defined( OS_MINIX ) || defined( OS_CYGWIN )

MX_EXPORT mx_status_type
mx_network_get_interface_from_host_address( MX_NETWORK_INTERFACE **ni,
					struct sockaddr *host_address_struct )
{
	static const char fname[] =
		"mx_network_get_interface_from_host_address()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Not yet implemented for this platform." );
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
	unsigned long mx_status_code;
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

	mx_status_code = mx_status.code & (~MXE_QUIET);

	switch( mx_status_code ) {
	case MXE_SUCCESS:
		return MX_SUCCESSFUL_RESULT;
		break;
	case MXE_NOT_FOUND:
		return mx_error( MXE_NOT_FOUND, fname,
		"No network interface was found for hostname '%s'.",
			hostname );
		break;
	default:
		return mx_status;
		break;
	}

	MXW_NOT_REACHED( return mx_status );
}

