#include <kroute.h>

#include "vector.h"
#include "vty.h"
#include "command.h"
#include "prefix.h"
#include "table.h"
#include "stream.h"
#include "zclient.h"
#include "linklist.h"
#include "log.h"

#include "ha_debug.h"

/* Interface up information. */
void
kroute_interface_up_update (struct interface *ifp)
{
  if (IS_DEBUG_HA(kroute,KROUTE))
    zlog_debug ("MESSAGE: KROUTE_INTERFACE_UP %s", ifp->name);
}

/* Interface down information. */
void
kroute_interface_down_update (struct interface *ifp)
{
  if (IS_DEBUG_HA(kroute,KROUTE))
    zlog_debug ("MESSAGE: KROUTE_INTERFACE_DOWN %s", ifp->name);
}

/* Interface information update. */
void
kroute_interface_add_update (struct interface *ifp)
{
  if (IS_DEBUG_HA(kroute,KROUTE))
    zlog_debug ("MESSAGE: KROUTE_INTERFACE_ADD %s", ifp->name);
}

void
kroute_interface_delete_update (struct interface *ifp)
{
  if (IS_DEBUG_HA(kroute,KROUTE))
    zlog_debug ("MESSAGE: KROUTE_INTERFACE_DELETE %s", ifp->name);
}

/* Interface address addition. */
void
kroute_interface_address_add_update (struct interface *ifp,
				    struct connected *ifc)
{
  struct prefix *p;

  if (IS_DEBUG_HA(kroute,KROUTE))
    {
      char buf[INET6_ADDRSTRLEN];

      p = ifc->address;
      zlog_debug ("MESSAGE: KROUTE_INTERFACE_ADDRESS_ADD %s/%d on %s",
		  inet_ntop (p->family, &p->u.prefix, buf, INET6_ADDRSTRLEN),
		  p->prefixlen, ifc->ifp->name);
    }
}

/* Interface address deletion. */
void
kroute_interface_address_delete_update (struct interface *ifp,
				       struct connected *ifc)
{
  struct prefix *p;

  if (IS_DEBUG_HA(kroute,KROUTE))
    {
      char buf[INET6_ADDRSTRLEN];

      p = ifc->address;
      zlog_debug ("MESSAGE: KROUTE_INTERFACE_ADDRESS_DELETE %s/%d on %s",
		  inet_ntop (p->family, &p->u.prefix, buf, INET6_ADDRSTRLEN),
		 p->prefixlen, ifc->ifp->name);
    }
}

void
ha_kroute_init ()
{
    /* Make kernel routing socket. */
    kernel_init ();
    interface_lookup_netlink ();
}
