/* Interface function header.
 * Copyright (C) 1999 Kunihiro Ishiguro
 *
 * This file is part of GNU Kroute.
 *
 * GNU Kroute is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Kroute is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Kroute; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#ifndef _KROUTE_INTERFACE_H
#define _KROUTE_INTERFACE_H

#include "table.h"

/* For interface multicast configuration. */
#define IF_KROUTE_MULTICAST_UNSPEC 0
#define IF_KROUTE_MULTICAST_ON     1
#define IF_KROUTE_MULTICAST_OFF    2

/* For interface shutdown configuration. */
#define IF_KROUTE_SHUTDOWN_UNSPEC 0
#define IF_KROUTE_SHUTDOWN_ON     1
#define IF_KROUTE_SHUTDOWN_OFF    2

struct ha_if_info
{
    /* Shutdown configuration. */
    u_char shutdown;

    /* Installed addresses chains tree. */
    struct route_table *ipv4_subnets;
};

extern void ha_if_init (void);
extern void if_delete_update (struct interface *ifp);
extern void if_add_update (struct interface *ifp);
extern void if_up (struct interface *);
extern void if_down (struct interface *);
extern void if_refresh (struct interface *);
extern void if_flags_update (struct interface *, uint64_t);
extern int if_subnet_add (struct interface *, struct connected *);
extern int if_subnet_delete (struct interface *, struct connected *);

#ifdef HAVE_PROC_NET_DEV
extern void ifstat_update_proc (void);
#endif /* HAVE_PROC_NET_DEV */
#ifdef HAVE_NET_RT_IFLIST
extern void ifstat_update_sysctl (void);

#endif /* HAVE_NET_RT_IFLIST */
#ifdef HAVE_PROC_NET_DEV
extern int interface_list_proc (void);
#endif /* HAVE_PROC_NET_DEV */
#ifdef HAVE_PROC_NET_IF_INET6
extern int ifaddr_proc_ipv6 (void);
#endif /* HAVE_PROC_NET_IF_INET6 */

#ifdef BSDI
extern int if_kvm_get_mtu (struct interface *);
#endif /* BSDI */

#endif /* _KROUTE_INTERFACE_H */
