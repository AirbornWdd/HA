/*
 * HAd main routine.
 *   Copyright (C) 1998, 99 Kunihiro Ishiguro, Toshiaki Takada
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

#include <lib/version.h>
#include "getopt.h"
#include "thread.h"
#include "prefix.h"
#include "linklist.h"
#include "if.h"
#include "vector.h"
#include "vty.h"
#include "command.h"
#include "filter.h"
#include "plist.h"
#include "stream.h"
#include "log.h"
#include "memory.h"
#include "privs.h"
#include "sigevent.h"
#include "zclient.h"

#include "ha_deamon.h"
#include "ha_debug.h"
#include "interface.h"
#include "ha_vty.h"
#include "ha_kroute.h"

/* Configuration filename and directory. */
char config_default[] = SYSCONFDIR HA_DEFAULT_CONFIG;

#ifdef HAVE_NETLINK
/* Receive buffer size for netlink socket */
u_int32_t nl_rcvbufsize = 0;
#endif /* HAVE_NETLINK */

/* HAd options. */
struct option longopts[] = 
{
  { "daemon",      no_argument,       NULL, 'd'},
  { "config_file", required_argument, NULL, 'f'},
  { "pid_file",    required_argument, NULL, 'i'},
  { "socket",      required_argument, NULL, 'z'},
  { "help",        no_argument,       NULL, 'h'},
  { "vty_addr",    required_argument, NULL, 'A'},
  { "vty_port",    required_argument, NULL, 'P'},
  { "user",        required_argument, NULL, 'u'},
  { "group",       required_argument, NULL, 'g'},
#ifdef HAVE_NETLINK
  { "nl-bufsize",  required_argument, NULL, 's'},
#endif /* HAVE_NETLINK */
  { "version",     no_argument,       NULL, 'v'},
  { 0 }
};

/* Master of threads. */
struct thread_master *master;

/* Process ID saved for use by init system */
const char *pid_file = PATH_HA_PID;

/* Help information display. */
static void __attribute__ ((noreturn))
usage (char *progname, int status)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    {    
      printf ("Usage : %s [OPTION...]\n\
            Daemon which manages HA.\n\n\
            -d, --daemon       Runs in daemon mode\n\
            -f, --config_file  Set configuration file name\n\
            -i, --pid_file     Set process identifier file name\n\
            -z, --socket       Set path of kroute socket\n\
            -A, --vty_addr     Set vty's bind address\n\
            -P, --vty_port     Set vty's port number\n\
            -u, --user         User to run as\n\
            -g, --group        Group to run as\n", progname);
#ifdef HAVE_NETLINK
      printf ("-s, --nl-bufsize   Set netlink receive buffer size\n");
#endif /* HAVE_NETLINK */
      printf ("-v, --version      Print program version\n"\
                    "-h, --help         Display this help and exit\n"\
                    "\n"\
                    "Report bugs to %s\n", KROUTE_BUG_ADDRESS);

    }
  exit (status);
}

/* SIGHUP handler. */
static void 
sighup (void)
{
  zlog (NULL, LOG_INFO, "SIGHUP received");
}

/* SIGINT / SIGTERM handler. */
static void
sigint (void)
{
  zlog_notice ("Terminating on signal");
  ha_terminate ();
}

/* SIGUSR1 handler. */
static void
sigusr1 (void)
{
  zlog_rotate (NULL);
}

struct bane_signal_t ha_signals[] =
{
  {
    .signal = SIGHUP,
    .handler = &sighup,
  },
  {
    .signal = SIGUSR1,
    .handler = &sigusr1,
  },  
  {
    .signal = SIGINT,
    .handler = &sigint,
  },
  {
    .signal = SIGTERM,
    .handler = &sigint,
  },
};

/* HAd main routine. */
int
main (int argc, char **argv)
{
  char *p;
  char *vty_addr = NULL;
  int vty_port = HA_VTY_PORT;
  int daemon_mode = 0;
  char *config_file = NULL;
  char *progname;
  struct thread thread;

  /* Set umask before anything for security */
  umask (0027);

  /* get program name */
  progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

  while (1) 
    {
      int opt;

#ifdef HAVE_NETLINK  
      opt = getopt_long (argc, argv, "df:i:z:hA:P:u:g:s:v", longopts, 0);
#else
      opt = getopt_long (argc, argv, "df:i:z:hA:P:u:g:v", longopts, 0);
#endif /* HAVE_NETLINK */
    
      if (opt == EOF)
	break;

      switch (opt) 
	{
	case 0:
	  break;
	case 'd':
	  daemon_mode = 1;
	  break;
	case 'f':
	  config_file = optarg;
	  break;
	case 'A':
	  vty_addr = optarg;
	  break;
    case 'i':
      pid_file = optarg;
      break;
	case 'z':
	  zclient_serv_path_set (optarg);
	  break;
	case 'P':
          /* Deal with atoi() returning 0 on failure, and had not
             listening on had port... */
          if (strcmp(optarg, "0") == 0) 
            {
              vty_port = 0;
              break;
            } 
          vty_port = atoi (optarg);
          if (vty_port <= 0 || vty_port > 0xffff)
            vty_port = HA_VTY_PORT;
  	  break;
#ifdef HAVE_NETLINK
    case 's':
      nl_rcvbufsize = atoi (optarg);
      break;
#endif /* HAVE_NETLINK */
	case 'v':
	  print_version (progname);
	  exit (0);
	  break;
	case 'h':
	  usage (progname, 0);
	  break;
	default:
	  usage (progname, 1);
	  break;
	}
    }

  /* Invoked by a priviledged user? -- endo. */
  if (geteuid () != 0)
    {
      errno = EPERM;
      perror (progname);
      exit (1);
    }

  zlog_default = openzlog (progname, ZLOG_HA,
			   LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);

  /* HA master init. */
  ha_master_init ();

  /* Initializations. */
  master = hm->master;

  /* Library inits. */
  signal_init (master, Q_SIGC(ha_signals), ha_signals);
  cmd_init (1);
  debug_init ();
  vty_init (master);
  memory_init ();

  /* HAd inits. */
  ha_if_init ();
  ha_kroute_init ();

  /* HA vty inits. */
  ha_vty_init ();
  ha_vty_show_init ();

  sort_node ();

  /* Get configuration file. */
  vty_read_config (config_file, config_default);

  /* Change to the daemon program. */
  if (daemon_mode && daemon (0, 0) < 0)
    {
      zlog_err("HAd daemon failed: %s", strerror(errno));
      exit (1);
    }

  /* Process id file create. */
  pid_output (pid_file);

  /* Create VTY socket */
  vty_serv_sock (vty_addr, vty_port, HA_VTYSH_PATH);

  /* Print banner. */
  zlog_notice ("HAd %s starting: vty@%d", BANE_VERSION, vty_port);

  /* Fetch next active thread. */
  while (thread_fetch (master, &thread))
    thread_call (&thread);

  /* Not reached. */
  return (0);
}

