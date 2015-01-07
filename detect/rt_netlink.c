/* Kernel routing table updates using netlink over GNU/Linux system.
 * Copyright (C) 1997, 98, 99 Kunihiro Ishiguro
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

#include <kroute.h>

/* Hack for GNU libc version 2. */
#ifndef MSG_TRUNC
#define MSG_TRUNC      0x20
#endif /* MSG_TRUNC */

#include "linklist.h"
#include "if.h"
#include "log.h"
#include "prefix.h"
#include "connected.h"
#include "table.h"
#include "memory.h"
#include "thread.h"
#include "privs.h"

#include "ha_debug.h"
#include "ha_deamon.h"

#define NL_PKT_BUF_SIZE 4096

/* Socket interface to kernel */
struct nlsock
{
  int sock;
  int seq;
  struct sockaddr_nl snl;
  const char *name;
} netlink      = { -1, 0, {0}, "netlink-listen"},     /* kernel messages */
  netlink_cmd  = { -1, 0, {0}, "netlink-cmd"};        /* command channel */

static const struct message nlmsg_str[] = {
  {RTM_NEWROUTE, "RTM_NEWROUTE"},
  {RTM_DELROUTE, "RTM_DELROUTE"},
  {RTM_GETROUTE, "RTM_GETROUTE"},
  {RTM_NEWLINK,  "RTM_NEWLINK"},
  {RTM_DELLINK,  "RTM_DELLINK"},
  {RTM_GETLINK,  "RTM_GETLINK"},
  {RTM_NEWADDR,  "RTM_NEWADDR"},
  {RTM_DELADDR,  "RTM_DELADDR"},
  {RTM_GETADDR,  "RTM_GETADDR"},
  {0, NULL}
};

static const char *nexthop_types_desc[] =
{
  "none",
  "Directly connected",
  "Interface route",
  "IPv4 nexthop",
  "IPv4 nexthop with ifindex",
  "IPv4 nexthop with ifname",
  "IPv6 nexthop",
  "IPv6 nexthop with ifindex",
  "IPv6 nexthop with ifname",
  "Null0 nexthop",
};

extern u_int32_t nl_rcvbufsize;

/* Note: on netlink systems, there should be a 1-to-1 mapping between interface
   names and ifindex values. */
static void
set_ifindex(struct interface *ifp, unsigned int ifi_index)
{
  struct interface *oifp;

  if (((oifp = if_lookup_by_index(ifi_index)) != NULL) && (oifp != ifp))
    {
      if (ifi_index == IFINDEX_INTERNAL)
        zlog_err("Netlink is setting interface %s ifindex to reserved "
		 "internal value %u", ifp->name, ifi_index);
      else
        {
	  if (IS_DEBUG_HA(kroute, KROUTE))
	    zlog_debug("interface index %d was renamed from %s to %s",
	    	       ifi_index, oifp->name, ifp->name);
	  if (if_is_up(oifp))
	    zlog_err("interface rename detected on up interface: index %d "
		     "was renamed from %s to %s, results are uncertain!", 
	    	     ifi_index, oifp->name, ifp->name);
	  if_delete_update(oifp);
        }
    }
  ifp->ifindex = ifi_index;
}

static int
netlink_recvbuf (struct nlsock *nl, uint32_t newsize)
{
  u_int32_t oldsize;
  socklen_t newlen = sizeof(newsize);
  socklen_t oldlen = sizeof(oldsize);
  int ret;

  ret = getsockopt(nl->sock, SOL_SOCKET, SO_RCVBUF, &oldsize, &oldlen);
  if (ret < 0)
    {
      zlog (NULL, LOG_ERR, "Can't get %s receive buffer size: %s", nl->name,
	    safe_strerror (errno));
      return -1;
    }

  ret = setsockopt(nl->sock, SOL_SOCKET, SO_RCVBUF, &nl_rcvbufsize,
		   sizeof(nl_rcvbufsize));
  if (ret < 0)
    {
      zlog (NULL, LOG_ERR, "Can't set %s receive buffer size: %s", nl->name,
	    safe_strerror (errno));
      return -1;
    }

  ret = getsockopt(nl->sock, SOL_SOCKET, SO_RCVBUF, &newsize, &newlen);
  if (ret < 0)
    {
      zlog (NULL, LOG_ERR, "Can't get %s receive buffer size: %s", nl->name,
	    safe_strerror (errno));
      return -1;
    }

  zlog (NULL, LOG_INFO,
	"Setting netlink socket receive buffer size: %u -> %u",
	oldsize, newsize);
  return 0;
}

/* Make socket for Linux netlink interface. */
static int
netlink_socket (struct nlsock *nl, unsigned long groups)
{
  int ret;
  struct sockaddr_nl snl;
  int sock;
  int namelen;
  int save_errno;

  sock = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0)
    {
      zlog (NULL, LOG_ERR, "Can't open %s socket: %s", nl->name,
            safe_strerror (errno));
      return -1;
    }

  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;
  snl.nl_groups = groups;

  ret = bind (sock, (struct sockaddr *) &snl, sizeof snl);
  save_errno = errno;

  if (ret < 0)
    {
      zlog (NULL, LOG_ERR, "Can't bind %s socket to group 0x%x: %s",
            nl->name, snl.nl_groups, safe_strerror (save_errno));
      close (sock);
      return -1;
    }

  /* multiple netlink sockets will have different nl_pid */
  namelen = sizeof snl;
  ret = getsockname (sock, (struct sockaddr *) &snl, (socklen_t *) &namelen);
  if (ret < 0 || namelen != sizeof snl)
    {
      zlog (NULL, LOG_ERR, "Can't get %s socket name: %s", nl->name,
            safe_strerror (errno));
      close (sock);
      return -1;
    }

  nl->snl = snl;
  nl->sock = sock;
  return ret;
}

/* Get type specified information from netlink. */
static int
netlink_request (int family, int type, struct nlsock *nl)
{
  int ret;
  struct sockaddr_nl snl;
  int save_errno;

  struct
  {
    struct nlmsghdr nlh;
    struct rtgenmsg g;
  } req;


  /* Check netlink socket. */
  if (nl->sock < 0)
    {
      zlog (NULL, LOG_ERR, "%s socket isn't active.", nl->name);
      return -1;
    }

  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;

  memset (&req, 0, sizeof req);
  req.nlh.nlmsg_len = sizeof req;
  req.nlh.nlmsg_type = type;
  req.nlh.nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
  req.nlh.nlmsg_pid = nl->snl.nl_pid;
  req.nlh.nlmsg_seq = ++nl->seq;
  req.g.rtgen_family = family;

  /* linux appears to check capabilities on every message 
   * have to raise caps for every message sent
   */

  ret = sendto (nl->sock, (void *) &req, sizeof req, 0,
                (struct sockaddr *) &snl, sizeof snl);
  save_errno = errno;

  if (ret < 0)
    {
      zlog (NULL, LOG_ERR, "%s sendto failed: %s", nl->name,
            safe_strerror (save_errno));
      return -1;
    }

  return 0;
}

/* Receive message from netlink interface and pass those information
   to the given function. */
static int
netlink_parse_info (int (*filter) (struct sockaddr_nl *, struct nlmsghdr *),
                    struct nlsock *nl)
{
  int status;
  int ret = 0;
  int error;

  while (1)
    {
      char buf[NL_PKT_BUF_SIZE];
      struct iovec iov = { buf, sizeof buf };
      struct sockaddr_nl snl;
      struct msghdr msg = { (void *) &snl, sizeof snl, &iov, 1, NULL, 0, 0 };
      struct nlmsghdr *h;

      status = recvmsg (nl->sock, &msg, 0);
      if (status < 0)
        {
          if (errno == EINTR)
            continue;
          if (errno == EWOULDBLOCK || errno == EAGAIN)
            break;
          zlog (NULL, LOG_ERR, "%s recvmsg overrun: %s",
	  	nl->name, safe_strerror(errno));
          continue;
        }

      if (status == 0)
        {
          zlog (NULL, LOG_ERR, "%s EOF", nl->name);
          return -1;
        }

      if (msg.msg_namelen != sizeof snl)
        {
          zlog (NULL, LOG_ERR, "%s sender address length error: length %d",
                nl->name, msg.msg_namelen);
          return -1;
        }
      
      for (h = (struct nlmsghdr *) buf; NLMSG_OK (h, (unsigned int) status);
           h = NLMSG_NEXT (h, status))
        {
          /* Finish of reading. */
          if (h->nlmsg_type == NLMSG_DONE)
            return ret;

          /* Error handling. */
          if (h->nlmsg_type == NLMSG_ERROR)
            {
              struct nlmsgerr *err = (struct nlmsgerr *) NLMSG_DATA (h);
	      int errnum = err->error;
	      int msg_type = err->msg.nlmsg_type;

              /* If the error field is zero, then this is an ACK */
              if (err->error == 0)
                {
                  if (IS_DEBUG_HA(kroute, KROUTE))
                    {
                      zlog_debug ("%s: %s ACK: type=%s(%u), seq=%u, pid=%u",
                                 __FUNCTION__, nl->name,
                                 lookup (nlmsg_str, err->msg.nlmsg_type),
                                 err->msg.nlmsg_type, err->msg.nlmsg_seq,
                                 err->msg.nlmsg_pid);
                    }

                  /* return if not a multipart message, otherwise continue */
                  if (!(h->nlmsg_flags & NLM_F_MULTI))
                    {
                      return 0;
                    }
                  continue;
                }

              if (h->nlmsg_len < NLMSG_LENGTH (sizeof (struct nlmsgerr)))
                {
                  zlog (NULL, LOG_ERR, "%s error: message truncated",
                        nl->name);
                  return -1;
                }

              /* Deal with errors that occur because of races in link handling */
	      if (nl == &netlink_cmd
		  && ((msg_type == RTM_DELROUTE &&
		       (-errnum == ENODEV || -errnum == ESRCH))
		      || (msg_type == RTM_NEWROUTE && -errnum == EEXIST)))
		{
		  if (IS_DEBUG_HA(kroute, KROUTE))
		    zlog_debug ("%s: error: %s type=%s(%u), seq=%u, pid=%u",
				nl->name, safe_strerror (-errnum),
				lookup (nlmsg_str, msg_type),
				msg_type, err->msg.nlmsg_seq, err->msg.nlmsg_pid);
		  return 0;
		}

	      zlog_err ("%s error: %s, type=%s(%u), seq=%u, pid=%u",
			nl->name, safe_strerror (-errnum),
			lookup (nlmsg_str, msg_type),
			msg_type, err->msg.nlmsg_seq, err->msg.nlmsg_pid);
              return -1;
            }

          /* OK we got netlink message. */
          if (IS_DEBUG_HA(kroute, KROUTE))
            zlog_debug ("netlink_parse_info: %s type %s(%u), seq=%u, pid=%u",
                       nl->name,
                       lookup (nlmsg_str, h->nlmsg_type), h->nlmsg_type,
                       h->nlmsg_seq, h->nlmsg_pid);

          /* skip unsolicited messages originating from command socket */
          if (nl != &netlink_cmd && h->nlmsg_pid == netlink_cmd.snl.nl_pid)
            {
              if (IS_DEBUG_HA(kroute, KROUTE))
                zlog_debug ("netlink_parse_info: %s packet comes from %s",
                            netlink_cmd.name, nl->name);
              continue;
            }

          error = (*filter) (&snl, h);
          if (error < 0)
            {
              zlog (NULL, LOG_ERR, "%s filter function error", nl->name);
              ret = error;
            }
        }

      /* After error care. */
      if (msg.msg_flags & MSG_TRUNC)
        {
          zlog (NULL, LOG_ERR, "%s error: message truncated", nl->name);
          continue;
        }
      if (status)
        {
          zlog (NULL, LOG_ERR, "%s error: data remnant size %d", nl->name,
                status);
          return -1;
        }
    }
  return ret;
}

/* Utility function for parse rtattr. */
static void
netlink_parse_rtattr (struct rtattr **tb, int max, struct rtattr *rta,
                      int len)
{
  while (RTA_OK (rta, len))
    {
      if (rta->rta_type <= max)
        tb[rta->rta_type] = rta;
      rta = RTA_NEXT (rta, len);
    }
}

/* Utility function to parse hardware link-layer address and update ifp */
static void
netlink_interface_update_hw_addr (struct rtattr **tb, struct interface *ifp)
{
  int i;

  if (tb[IFLA_ADDRESS])
    {
      int hw_addr_len;

      hw_addr_len = RTA_PAYLOAD (tb[IFLA_ADDRESS]);

      if (hw_addr_len > INTERFACE_HWADDR_MAX)
        zlog_warn ("Hardware address is too large: %d", hw_addr_len);
      else
        {
          ifp->hw_addr_len = hw_addr_len;
          memcpy (ifp->hw_addr, RTA_DATA (tb[IFLA_ADDRESS]), hw_addr_len);

          for (i = 0; i < hw_addr_len; i++)
            if (ifp->hw_addr[i] != 0)
              break;

          if (i == hw_addr_len)
            ifp->hw_addr_len = 0;
          else
            ifp->hw_addr_len = hw_addr_len;
        }
    }
}

/* Called from interface_lookup_netlink().  This function is only used
   during bootstrap. */
static int
netlink_interface (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  int len;
  struct ifinfomsg *ifi;
  struct rtattr *tb[IFLA_MAX + 1];
  struct interface *ifp;
  char *name;

  ifi = NLMSG_DATA (h);

  if (h->nlmsg_type != RTM_NEWLINK)
    return 0;

  len = h->nlmsg_len - NLMSG_LENGTH (sizeof (struct ifinfomsg));
  if (len < 0)
    return -1;

  /* Looking up interface name. */
  memset (tb, 0, sizeof tb);
  netlink_parse_rtattr (tb, IFLA_MAX, IFLA_RTA (ifi), len);
  
#ifdef IFLA_WIRELESS
  /* check for wireless messages to ignore */
  if ((tb[IFLA_WIRELESS] != NULL) && (ifi->ifi_change == 0))
    {
      if (IS_DEBUG_HA(kroute, KROUTE))
        zlog_debug ("%s: ignoring IFLA_WIRELESS message", __func__);
      return 0;
    }
#endif /* IFLA_WIRELESS */

  if (tb[IFLA_IFNAME] == NULL)
    return -1;
  name = (char *) RTA_DATA (tb[IFLA_IFNAME]);

  /* Add interface. */
  ifp = if_get_by_name (name);
  set_ifindex(ifp, ifi->ifi_index);
  ifp->flags = ifi->ifi_flags & 0x0000fffff;
  ifp->mtu6 = ifp->mtu = *(uint32_t *) RTA_DATA (tb[IFLA_MTU]);
  ifp->metric = 1;

  /* Hardware type and address. */
  ifp->hw_type = ifi->ifi_type;
  netlink_interface_update_hw_addr (tb, ifp);

  if_add_update (ifp);

  return 0;
}

/* Lookup interface IPv4/IPv6 address. */
static int
netlink_interface_addr (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  int len;
  struct ifaddrmsg *ifa;
  struct rtattr *tb[IFA_MAX + 1];
  struct interface *ifp;
  void *addr;
  void *broad;
  u_char flags = 0;
  char *label = NULL;

  ifa = NLMSG_DATA (h);

  if (ifa->ifa_family != AF_INET
#ifdef HAVE_IPV6
      && ifa->ifa_family != AF_INET6
#endif /* HAVE_IPV6 */
    )
    return 0;

  if (h->nlmsg_type != RTM_NEWADDR && h->nlmsg_type != RTM_DELADDR)
    return 0;

  len = h->nlmsg_len - NLMSG_LENGTH (sizeof (struct ifaddrmsg));
  if (len < 0)
    return -1;

  memset (tb, 0, sizeof tb);
  netlink_parse_rtattr (tb, IFA_MAX, IFA_RTA (ifa), len);

  ifp = if_lookup_by_index (ifa->ifa_index);
  if (ifp == NULL)
    {
      zlog_err ("netlink_interface_addr can't find interface by index %d",
                ifa->ifa_index);
      return -1;
    }

  if (IS_DEBUG_HA(kroute, KROUTE))    /* remove this line to see initial ifcfg */
    {
      char buf[BUFSIZ];
      zlog_debug ("netlink_interface_addr %s %s:",
                 lookup (nlmsg_str, h->nlmsg_type), ifp->name);
      if (tb[IFA_LOCAL])
        zlog_debug ("  IFA_LOCAL     %s/%d",
		    inet_ntop (ifa->ifa_family, RTA_DATA (tb[IFA_LOCAL]),
			       buf, BUFSIZ), ifa->ifa_prefixlen);
      if (tb[IFA_ADDRESS])
        zlog_debug ("  IFA_ADDRESS   %s/%d",
		    inet_ntop (ifa->ifa_family, RTA_DATA (tb[IFA_ADDRESS]),
                               buf, BUFSIZ), ifa->ifa_prefixlen);
      if (tb[IFA_BROADCAST])
        zlog_debug ("  IFA_BROADCAST %s/%d",
		    inet_ntop (ifa->ifa_family, RTA_DATA (tb[IFA_BROADCAST]),
			       buf, BUFSIZ), ifa->ifa_prefixlen);
      if (tb[IFA_LABEL] && strcmp (ifp->name, RTA_DATA (tb[IFA_LABEL])))
        zlog_debug ("  IFA_LABEL     %s", (char *)RTA_DATA (tb[IFA_LABEL]));
      
      if (tb[IFA_CACHEINFO])
        {
          struct ifa_cacheinfo *ci = RTA_DATA (tb[IFA_CACHEINFO]);
          zlog_debug ("  IFA_CACHEINFO pref %d, valid %d",
                      ci->ifa_prefered, ci->ifa_valid);
        }
    }
  
  /* logic copied from iproute2/ip/ipaddress.c:print_addrinfo() */
  if (tb[IFA_LOCAL] == NULL)
    tb[IFA_LOCAL] = tb[IFA_ADDRESS];
  if (tb[IFA_ADDRESS] == NULL)
    tb[IFA_ADDRESS] = tb[IFA_LOCAL];
  
  /* local interface address */
  addr = (tb[IFA_LOCAL] ? RTA_DATA(tb[IFA_LOCAL]) : NULL);

  /* is there a peer address? */
  if (tb[IFA_ADDRESS] &&
      memcmp(RTA_DATA(tb[IFA_ADDRESS]), RTA_DATA(tb[IFA_LOCAL]), RTA_PAYLOAD(tb[IFA_ADDRESS])))
    {
      broad = RTA_DATA(tb[IFA_ADDRESS]);
      SET_FLAG (flags, KROUTE_IFA_PEER);
    }
  else
    /* seeking a broadcast address */
    broad = (tb[IFA_BROADCAST] ? RTA_DATA(tb[IFA_BROADCAST]) : NULL);

  /* addr is primary key, SOL if we don't have one */
  if (addr == NULL)
    {
      zlog_debug ("%s: NULL address", __func__);
      return -1;
    }

  /* Flags. */
  if (ifa->ifa_flags & IFA_F_SECONDARY)
    SET_FLAG (flags, KROUTE_IFA_SECONDARY);

  /* Label */
  if (tb[IFA_LABEL])
    label = (char *) RTA_DATA (tb[IFA_LABEL]);

  if (ifp && label && strcmp (ifp->name, label) == 0)
    label = NULL;

  /* Register interface address to the interface. */
  if (ifa->ifa_family == AF_INET)
    {
      if (h->nlmsg_type == RTM_NEWADDR)
        connected_add_ipv4 (ifp, flags,
                            (struct in_addr *) addr, ifa->ifa_prefixlen,
                            (struct in_addr *) broad, label);
      else
        connected_delete_ipv4 (ifp, flags,
                               (struct in_addr *) addr, ifa->ifa_prefixlen,
                               (struct in_addr *) broad);
    }
#ifdef HAVE_IPV6
  if (ifa->ifa_family == AF_INET6)
    {
      if (h->nlmsg_type == RTM_NEWADDR)
        connected_add_ipv6 (ifp, flags,
                            (struct in6_addr *) addr, ifa->ifa_prefixlen,
                            (struct in6_addr *) broad, label);
      else
        connected_delete_ipv6 (ifp,
                               (struct in6_addr *) addr, ifa->ifa_prefixlen,
                               (struct in6_addr *) broad);
    }
#endif /* HAVE_IPV6 */

  return 0;
}

static const struct message rtproto_str[] = {
  {RTPROT_REDIRECT, "redirect"},
  {RTPROT_KERNEL,   "kernel"},
  {RTPROT_BOOT,     "boot"},
  {RTPROT_STATIC,   "static"},
  {RTPROT_GATED,    "GateD"},
  {RTPROT_RA,       "router advertisement"},
  {RTPROT_MRT,      "MRT"},
  {RTPROT_ZEBRA,    "Kroute"},
#ifdef RTPROT_BIRD
  {RTPROT_BIRD,     "BIRD"},
#endif /* RTPROT_BIRD */
  {0,               NULL}
};

static int
netlink_link_change (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  int len;
  struct ifinfomsg *ifi;
  struct rtattr *tb[IFLA_MAX + 1];
  struct interface *ifp;
  char *name;

  ifi = NLMSG_DATA (h);

  if (!(h->nlmsg_type == RTM_NEWLINK || h->nlmsg_type == RTM_DELLINK))
    {
      /* If this is not link add/delete message so print warning. */
      zlog_warn ("netlink_link_change: wrong kernel message %d\n",
                 h->nlmsg_type);
      return 0;
    }

  len = h->nlmsg_len - NLMSG_LENGTH (sizeof (struct ifinfomsg));
  if (len < 0)
    return -1;

  /* Looking up interface name. */
  memset (tb, 0, sizeof tb);
  netlink_parse_rtattr (tb, IFLA_MAX, IFLA_RTA (ifi), len);

#ifdef IFLA_WIRELESS
  /* check for wireless messages to ignore */
  if ((tb[IFLA_WIRELESS] != NULL) && (ifi->ifi_change == 0))
    {
      if (IS_DEBUG_HA(kroute, KROUTE))
        zlog_debug ("%s: ignoring IFLA_WIRELESS message", __func__);
      return 0;
    }
#endif /* IFLA_WIRELESS */
  
  if (tb[IFLA_IFNAME] == NULL)
    return -1;
  name = (char *) RTA_DATA (tb[IFLA_IFNAME]);

  /* Add interface. */
  if (h->nlmsg_type == RTM_NEWLINK)
    {
      ifp = if_lookup_by_name (name);

      if (ifp == NULL || !CHECK_FLAG (ifp->status, KROUTE_INTERFACE_ACTIVE))
        {
          if (ifp == NULL)
            ifp = if_get_by_name (name);

          set_ifindex(ifp, ifi->ifi_index);
          ifp->flags = ifi->ifi_flags & 0x0000fffff;
          ifp->mtu6 = ifp->mtu = *(int *) RTA_DATA (tb[IFLA_MTU]);
          ifp->metric = 1;

          netlink_interface_update_hw_addr (tb, ifp);

          /* If new link is added. */
          if_add_update (ifp);
        }
      else
        {
          /* Interface status change. */
          set_ifindex(ifp, ifi->ifi_index);
          ifp->mtu6 = ifp->mtu = *(int *) RTA_DATA (tb[IFLA_MTU]);
          ifp->metric = 1;

          netlink_interface_update_hw_addr (tb, ifp);

          if (if_is_operative (ifp))
            {
              ifp->flags = ifi->ifi_flags & 0x0000fffff;
              if (!if_is_operative (ifp))
                if_down (ifp);
	      else
		/* Must notify client daemons of new interface status. */
	        kroute_interface_up_update (ifp);
            }
          else
            {
              ifp->flags = ifi->ifi_flags & 0x0000fffff;
              if (if_is_operative (ifp))
                if_up (ifp);
            }
        }
    }
  else
    {
      /* RTM_DELLINK. */
      ifp = if_lookup_by_name (name);

      if (ifp == NULL)
        {
          zlog (NULL, LOG_WARNING, "interface %s is deleted but can't find",
                name);
          return 0;
        }

      if_delete_update (ifp);
    }

  return 0;
}

static int
netlink_information_fetch (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  /* JF: Ignore messages that aren't from the kernel */
  if ( snl->nl_pid != 0 )
    {
      zlog ( NULL, LOG_ERR, "Ignoring message from pid %u", snl->nl_pid );
      return 0;
    }

  switch (h->nlmsg_type)
    {
    case RTM_NEWLINK:
      return netlink_link_change (snl, h);
      break;
    case RTM_DELLINK:
      return netlink_link_change (snl, h);
      break;
    case RTM_NEWADDR:
      return netlink_interface_addr (snl, h);
      break;
    case RTM_DELADDR:
      return netlink_interface_addr (snl, h);
      break;
    case RTM_NEWROUTE:
    case RTM_DELROUTE:
    default:
      zlog_warn ("Unknown netlink nlmsg_type %d\n", h->nlmsg_type);
      break;
    }
  return 0;
}

/* Interface lookup by netlink socket. */
int
interface_lookup_netlink (void)
{
  int ret;

  /* Get interface information. */
  ret = netlink_request (AF_PACKET, RTM_GETLINK, &netlink_cmd);
  if (ret < 0)
    return ret;
  ret = netlink_parse_info (netlink_interface, &netlink_cmd);
  if (ret < 0)
    return ret;

  /* Get IPv4 address of the interfaces. */
  ret = netlink_request (AF_INET, RTM_GETADDR, &netlink_cmd);
  if (ret < 0)
    return ret;
  ret = netlink_parse_info (netlink_interface_addr, &netlink_cmd);
  if (ret < 0)
    return ret;

#ifdef HAVE_IPV6
  /* Get IPv6 address of the interfaces. */
  ret = netlink_request (AF_INET6, RTM_GETADDR, &netlink_cmd);
  if (ret < 0)
    return ret;
  ret = netlink_parse_info (netlink_interface_addr, &netlink_cmd);
  if (ret < 0)
    return ret;
#endif /* HAVE_IPV6 */

  return 0;
}

/* Utility function  comes from iproute2. 
   Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru> */
static int
addattr_l (struct nlmsghdr *n, int maxlen, int type, void *data, int alen)
{
  int len;
  struct rtattr *rta;

  len = RTA_LENGTH (alen);

  if (NLMSG_ALIGN (n->nlmsg_len) + len > maxlen)
    return -1;

  rta = (struct rtattr *) (((char *) n) + NLMSG_ALIGN (n->nlmsg_len));
  rta->rta_type = type;
  rta->rta_len = len;
  memcpy (RTA_DATA (rta), data, alen);
  n->nlmsg_len = NLMSG_ALIGN (n->nlmsg_len) + len;

  return 0;
}

static int
rta_addattr_l (struct rtattr *rta, int maxlen, int type, void *data, int alen)
{
  int len;
  struct rtattr *subrta;

  len = RTA_LENGTH (alen);

  if (RTA_ALIGN (rta->rta_len) + len > maxlen)
    return -1;

  subrta = (struct rtattr *) (((char *) rta) + RTA_ALIGN (rta->rta_len));
  subrta->rta_type = type;
  subrta->rta_len = len;
  memcpy (RTA_DATA (subrta), data, alen);
  rta->rta_len = NLMSG_ALIGN (rta->rta_len) + len;

  return 0;
}

/* Utility function comes from iproute2. 
   Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru> */
static int
addattr32 (struct nlmsghdr *n, int maxlen, int type, int data)
{
  int len;
  struct rtattr *rta;

  len = RTA_LENGTH (4);

  if (NLMSG_ALIGN (n->nlmsg_len) + len > maxlen)
    return -1;

  rta = (struct rtattr *) (((char *) n) + NLMSG_ALIGN (n->nlmsg_len));
  rta->rta_type = type;
  rta->rta_len = len;
  memcpy (RTA_DATA (rta), &data, 4);
  n->nlmsg_len = NLMSG_ALIGN (n->nlmsg_len) + len;

  return 0;
}

static int
netlink_talk_filter (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  zlog_warn ("netlink_talk: ignoring message type 0x%04x", h->nlmsg_type);
  return 0;
}

/* sendmsg() to netlink socket then recvmsg(). */
static int
netlink_talk (struct nlmsghdr *n, struct nlsock *nl)
{
  int status;
  struct sockaddr_nl snl;
  struct iovec iov = { (void *) n, n->nlmsg_len };
  struct msghdr msg = { (void *) &snl, sizeof snl, &iov, 1, NULL, 0, 0 };
  int save_errno;

  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;

  n->nlmsg_seq = ++nl->seq;

  /* Request an acknowledgement by setting NLM_F_ACK */
  n->nlmsg_flags |= NLM_F_ACK;

  if (IS_DEBUG_HA(kroute, KROUTE))
    zlog_debug ("netlink_talk: %s type %s(%u), seq=%u", nl->name,
               lookup (nlmsg_str, n->nlmsg_type), n->nlmsg_type,
               n->nlmsg_seq);

  /* Send message to netlink interface. */
  status = sendmsg (nl->sock, &msg, 0);
  save_errno = errno;

  if (status < 0)
    {
      zlog (NULL, LOG_ERR, "netlink_talk sendmsg() error: %s",
            safe_strerror (save_errno));
      return -1;
    }


  /* 
   * Get reply from netlink socket. 
   * The reply should either be an acknowlegement or an error.
   */
  return netlink_parse_info (netlink_talk_filter, nl);
}

/* Interface address modification. */
static int
netlink_address (int cmd, int family, struct interface *ifp,
                 struct connected *ifc)
{
  int bytelen;
  struct prefix *p;

  struct
  {
    struct nlmsghdr n;
    struct ifaddrmsg ifa;
    char buf[NL_PKT_BUF_SIZE];
  } req;

  p = ifc->address;
  memset (&req, 0, sizeof req);

  bytelen = (family == AF_INET ? 4 : 16);

  req.n.nlmsg_len = NLMSG_LENGTH (sizeof (struct ifaddrmsg));
  req.n.nlmsg_flags = NLM_F_REQUEST;
  req.n.nlmsg_type = cmd;
  req.ifa.ifa_family = family;

  req.ifa.ifa_index = ifp->ifindex;
  req.ifa.ifa_prefixlen = p->prefixlen;

  addattr_l (&req.n, sizeof req, IFA_LOCAL, &p->u.prefix, bytelen);

  if (family == AF_INET && cmd == RTM_NEWADDR)
    {
      if (!CONNECTED_PEER(ifc) && ifc->destination)
        {
          p = ifc->destination;
          addattr_l (&req.n, sizeof req, IFA_BROADCAST, &p->u.prefix,
                     bytelen);
        }
    }

  if (CHECK_FLAG (ifc->flags, KROUTE_IFA_SECONDARY))
    SET_FLAG (req.ifa.ifa_flags, IFA_F_SECONDARY);

  if (ifc->label)
    addattr_l (&req.n, sizeof req, IFA_LABEL, ifc->label,
               strlen (ifc->label) + 1);

  return netlink_talk (&req.n, &netlink_cmd);
}

int
kernel_address_add_ipv4 (struct interface *ifp, struct connected *ifc)
{
  return netlink_address (RTM_NEWADDR, AF_INET, ifp, ifc);
}

int
kernel_address_delete_ipv4 (struct interface *ifp, struct connected *ifc)
{
  return netlink_address (RTM_DELADDR, AF_INET, ifp, ifc);
}


extern struct thread_master *master;

/* Kernel route reflection. */
static int
kernel_read (struct thread *thread)
{
  netlink_parse_info (netlink_information_fetch, &netlink);
  thread_add_read (hm->master, kernel_read, NULL, netlink.sock);

  return 0;
}

/* Filter out messages from self that occur on listener socket,
   caused by our actions on the command socket
 */
static void netlink_install_filter (int sock, __u32 pid)
{
  struct sock_filter filter[] = {
    /* 0: ldh [4]	          */
    BPF_STMT(BPF_LD|BPF_ABS|BPF_H, offsetof(struct nlmsghdr, nlmsg_type)),
    /* 1: jeq 0x18 jt 3 jf 6  */
    BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, htons(RTM_NEWROUTE), 1, 0),
    /* 2: jeq 0x19 jt 3 jf 6  */
    BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, htons(RTM_DELROUTE), 0, 3),
    /* 3: ldw [12]		  */
    BPF_STMT(BPF_LD|BPF_ABS|BPF_W, offsetof(struct nlmsghdr, nlmsg_pid)),
    /* 4: jeq XX  jt 5 jf 6   */
    BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, htonl(pid), 0, 1),
    /* 5: ret 0    (skip)     */
    BPF_STMT(BPF_RET|BPF_K, 0),
    /* 6: ret 0xffff (keep)   */
    BPF_STMT(BPF_RET|BPF_K, 0xffff),
  };

  struct sock_fprog prog = {
    .len = sizeof(filter) / sizeof(filter[0]),
    .filter = filter,
  };

  if (setsockopt(sock, SOL_SOCKET, SO_ATTACH_FILTER, &prog, sizeof(prog)) < 0)
    zlog_warn ("Can't install socket filter: %s\n", safe_strerror(errno));
}

/* Exported interface function.  This function simply calls
   netlink_socket (). */
void
kernel_init (void)
{
  unsigned long groups;

  groups = RTMGRP_LINK | RTMGRP_IPV4_ROUTE | RTMGRP_IPV4_IFADDR;
#ifdef HAVE_IPV6
  groups |= RTMGRP_IPV6_ROUTE | RTMGRP_IPV6_IFADDR;
#endif /* HAVE_IPV6 */
  netlink_socket (&netlink, groups);
  netlink_socket (&netlink_cmd, 0);

  /* Register kernel socket. */
  if (netlink.sock > 0)
    {
      /* Only want non-blocking on the netlink event socket */
      if (fcntl (netlink.sock, F_SETFL, O_NONBLOCK) < 0)
	zlog (NULL, LOG_ERR, "Can't set %s socket flags: %s", netlink.name,
		safe_strerror (errno));

      /* Set receive buffer size if it's set from command line */
      if (nl_rcvbufsize)
	netlink_recvbuf (&netlink, nl_rcvbufsize);

      netlink_install_filter (netlink.sock, netlink_cmd.snl.nl_pid);
      thread_add_read (hm->master, kernel_read, NULL, netlink.sock);
    }
}
