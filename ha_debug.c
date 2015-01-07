#include <kroute.h>

#include "linklist.h"
#include "thread.h"
#include "prefix.h"
#include "command.h"
#include "stream.h"
#include "log.h"
#include "sockopt.h"

#include "ha_debug.h"

/* Configuration debug option variables. */
unsigned long conf_debug_ha_packet[5] = {0, 0, 0, 0, 0};
unsigned long conf_debug_ha_event = 0;
unsigned long conf_debug_ha_ism = 0;
unsigned long conf_debug_ha_nsm = 0;
unsigned long conf_debug_ha_lsa = 0;
unsigned long conf_debug_ha_kroute = 0;
unsigned long conf_debug_ha_nssa = 0;

/* Enable debug option variables -- valid only session. */
unsigned long term_debug_ha_packet[5] = {0, 0, 0, 0, 0};
unsigned long term_debug_ha_event = 0;
unsigned long term_debug_ha_ism = 0;
unsigned long term_debug_ha_nsm = 0;
unsigned long term_debug_ha_lsa = 0;
unsigned long term_debug_ha_kroute = 0;
unsigned long term_debug_ha_nssa = 0;

/* Debug node. */
static struct cmd_node debug_node =
{
  DEBUG_NODE,
  "",
  1 /* VTYSH */
};

struct timeval
tv_adjust (struct timeval a)
{
  while (a.tv_usec >= 1000000)
    {
      a.tv_usec -= 1000000;
      a.tv_sec++;
    }

  while (a.tv_usec < 0)
    {
      a.tv_usec += 1000000;
      a.tv_sec--;
    }

  return a;
}

int
tv_ceil (struct timeval a)
{
  a = tv_adjust (a);

  return (a.tv_usec ? a.tv_sec + 1 : a.tv_sec);
}

int
tv_floor (struct timeval a)
{
  a = tv_adjust (a);

  return a.tv_sec;
}

struct timeval
int2tv (int a)
{
  struct timeval ret;

  ret.tv_sec = a;
  ret.tv_usec = 0;

  return ret;
}

struct timeval
tv_add (struct timeval a, struct timeval b)
{
  struct timeval ret;

  ret.tv_sec = a.tv_sec + b.tv_sec;
  ret.tv_usec = a.tv_usec + b.tv_usec;

  return tv_adjust (ret);
}

struct timeval
tv_sub (struct timeval a, struct timeval b)
{
  struct timeval ret;

  ret.tv_sec = a.tv_sec - b.tv_sec;
  ret.tv_usec = a.tv_usec - b.tv_usec;

  return tv_adjust (ret);
}

int
tv_cmp (struct timeval a, struct timeval b)
{
  return (a.tv_sec == b.tv_sec ?
	  a.tv_usec - b.tv_usec : a.tv_sec - b.tv_sec);
}

const char *
ha_timeval_dump (struct timeval *t, char *buf, size_t size)
{
  /* Making formatted timer strings. */
#define MINUTE_IN_SECONDS	60
#define HOUR_IN_SECONDS		(60*MINUTE_IN_SECONDS)
#define DAY_IN_SECONDS		(24*HOUR_IN_SECONDS)
#define WEEK_IN_SECONDS		(7*DAY_IN_SECONDS)
  unsigned long w, d, h, m, s, ms;
  
  if (!t)
    return "inactive";
  
  w = d = h = m = s = ms = 0;
  memset (buf, 0, size);
  
  ms = t->tv_usec / 1000;
  
  if (ms >= 1000)
    {
      t->tv_sec += ms / 1000;
      ms %= 1000;
    }
  
  if (t->tv_sec > WEEK_IN_SECONDS)
    {
      w = t->tv_sec / WEEK_IN_SECONDS;
      t->tv_sec -= w * WEEK_IN_SECONDS;
    }
  
  if (t->tv_sec > DAY_IN_SECONDS)
    {
      d = t->tv_sec / DAY_IN_SECONDS;
      t->tv_sec -= d * DAY_IN_SECONDS;
    }
  
  if (t->tv_sec >= HOUR_IN_SECONDS)
    {
      h = t->tv_sec / HOUR_IN_SECONDS;
      t->tv_sec -= h * HOUR_IN_SECONDS;
    }
  
  if (t->tv_sec >= MINUTE_IN_SECONDS)
    {
      m = t->tv_sec / MINUTE_IN_SECONDS;
      t->tv_sec -= m * MINUTE_IN_SECONDS;
    }
  
  if (w > 99)
    snprintf (buf, size, "%ldw%1ldd", w, d);
  else if (w)
    snprintf (buf, size, "%ldw%1ldd%02ldh", w, d, h);
  else if (d)
    snprintf (buf, size, "%1ldd%02ldh%02ldm", d, h, m);
  else if (h)
    snprintf (buf, size, "%ldh%02ldm%02lds", h, m, t->tv_sec);
  else if (m)
    snprintf (buf, size, "%ldm%02lds", m, t->tv_sec);
  else
    snprintf (buf, size, "%ld.%03lds", t->tv_sec, ms);
  
  return buf;
}

const char *
ha_timer_dump (struct thread *t, char *buf, size_t size)
{
  struct timeval result;
  if (!t)
    return "inactive";
  
  result = tv_sub (t->u.sands, recent_relative_time());
  return ha_timeval_dump (&result, buf, size);
}

static int
config_write_debug (struct vty *vty)
{
    int write = 0;
    
    return write;
}

DEFUN (show_debugging_ha,
       show_debugging_ha_cmd,
       "show debugging ha",
       SHOW_STR
       DEBUG_STR
       HA_STR)
{
  vty_out (vty, "HA debugging status:%s", VTY_NEWLINE);

  /* Show debug status for events. */
  if (IS_DEBUG_HA_EVENT)
    vty_out (vty, "  HA event debugging is on%s", VTY_NEWLINE);
  
  return CMD_SUCCESS;
}

/* Initialize debug commands. */
void
debug_init ()
{
  install_node (&debug_node, config_write_debug);

  install_element (ENABLE_NODE, &show_debugging_ha_cmd);
}
