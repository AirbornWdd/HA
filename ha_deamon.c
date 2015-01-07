#include <kroute.h>

#include "thread.h"
#include "vty.h"
#include "command.h"
#include "linklist.h"
#include "prefix.h"
#include "table.h"
#include "if.h"
#include "memory.h"
#include "stream.h"
#include "log.h"
#include "sockunion.h"          /* for inet_aton () */
#include "zclient.h"
#include "plist.h"
#include "sockopt.h"

#include "ha_deamon.h"
#include "ha_packet.h"
#include "ha_debug.h"

/* HA process wide configuration. */
static struct ha_master ha_master;

/* HA process wide configuration pointer to export. */
struct ha_master *hm;

/* Allocate new ha structure. */
static struct ha *
ha_new (void)
{
  struct ha *new = XCALLOC (MTYPE_HA_TOP, sizeof (struct ha));

  if ((new->fd = ha_sock_init()) < 0)
    {
      zlog_err("ha_new: fatal error: ha_sock_init was unable to open "
	       "a socket");
      exit(1);
    }
  new->maxsndbuflen = getsockopt_so_sendbuf (new->fd);
  if (IS_DEBUG_HA (kroute, KROUTE_INTERFACE))
    zlog_debug ("%s: starting with HA send buffer size %d",
      __func__, new->maxsndbuflen);
  if ((new->ibuf = stream_new(HA_MAX_PACKET_SIZE+1)) == NULL)
    {
      zlog_err("ha_new: fatal error: stream_new(%u) failed allocating ibuf",
	       HA_MAX_PACKET_SIZE+1);
      exit(1);
    }
  new->t_read = thread_add_read (master, ha_read, new, new->fd);
  new->oi_write_q = list_new ();

  return new;
}

struct ha *
ha_lookup ()
{
  if (listcount (hm->ha) == 0)
    return NULL;

  return listgetdata (listhead (hm->ha));
}

static void
ha_add (struct ha *ha)
{
  listnode_add (hm->ha, ha);
}

static void
ha_delete (struct ha *ha)
{
  listnode_delete (hm->ha, ha);
}

struct ha *
ha_get ()
{
  struct ha *ha;

  ha = ha_lookup ();
  if (ha == NULL)
    {
      ha = ha_new ();
      ha_add (ha);
    }

  return ha;
}

/* Shut down the entire process */
void
ha_terminate (void)
{
  struct ha *ha;
  struct listnode *node, *nnode;
  
  /* shutdown already in progress */
  if (CHECK_FLAG (hm->options, HA_MASTER_SHUTDOWN))
    return;
  
  SET_FLAG (hm->options, HA_MASTER_SHUTDOWN);

  /* exit immediately if HA not actually running */
  if (listcount(hm->ha) == 0)
    exit(0);

  for (ALL_LIST_ELEMENTS (hm->ha, node, nnode, ha))
    ha_finish (ha);

  /* Deliberately go back up, hopefully to thread scheduler, as
   * One or more ha_finish()'s may have deferred shutdown to a timer
   * thread
   */
}

void
ha_finish (struct ha *ha)
{
      
  /* if ha_deferred_shutdown returns, then ha_finish_final is
   * deferred to expiry of G-S timer thread. Return back up, hopefully
   * to thread scheduler.
   */
  return;
}

void
ha_master_init ()
{
  memset (&ha_master, 0, sizeof (struct ha_master));

  hm = &ha_master;
  hm->ha = list_new ();
  hm->master = thread_master_create ();
  hm->start_time = bane_time (NULL);
}
