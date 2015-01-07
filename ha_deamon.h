/*
 * HAd main header.
 * Copyright (C) 1998, 99, 2000 Kunihiro Ishiguro, Toshiaki Takada
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

#ifndef _KROUTE_HAD_H
#define _KROUTE_HAD_H

#include <kroute.h>

#include "prefix.h"
#include "filter.h"
#include "log.h"

#define HA_VERSION            2

/* VTY port number. */
#define HA_VTY_PORT          2609

/* IP TTL for HA protocol. */
#define HA_IP_TTL             1
#define HA_VL_IP_TTL          100

/* Default configuration file name for had. */
#define HA_DEFAULT_CONFIG   "ha.conf"

/* Architectual Constants */
#ifdef DEBUG
#define HA_LS_REFRESH_TIME                    60
#else
#define HA_LS_REFRESH_TIME                  1800
#endif
#define HA_MIN_LS_INTERVAL                     5
#define HA_MIN_LS_ARRIVAL                      1
#define HA_LSA_INITIAL_AGE                     0	/* useful for debug */
#define HA_LSA_MAXAGE                       3600
#define HA_CHECK_AGE                         300
#define HA_LSA_MAXAGE_DIFF                   900
#define HA_LS_INFINITY                  0xffffff
#define HA_DEFAULT_DESTINATION        0x00000000      /* 0.0.0.0 */
#define HA_INITIAL_SEQUENCE_NUMBER    0x80000001
#define HA_MAX_SEQUENCE_NUMBER        0x7fffffff

#define HA_NSSA_TRANS_STABLE_DEFAULT		40

#define HA_ALLSPFROUTERS              0xe0000005      /* 224.0.0.5 */
#define HA_ALLDROUTERS                0xe0000006      /* 224.0.0.6 */

#define HA_AREA_BACKBONE              0x00000000      /* 0.0.0.0 */

/* HA Authentication Type. */
#define HA_AUTH_NULL                      0
#define HA_AUTH_SIMPLE                    1
#define HA_AUTH_CRYPTOGRAPHIC             2
/* For Interface authentication setting default */
#define HA_AUTH_NOTSET                   -1
/* For the consumption and sanity of the chmmand handler */ 
/* DO NIOT REMOVE!!! Need to detect whether a value has
   been given or not in VLink chmmand handlers */
#define HA_AUTH_CMD_NOTSEEN              -2

/* HA SPF timer values. */
#define HA_SPF_DELAY_DEFAULT              200
#define HA_SPF_HOLDTIME_DEFAULT           1000
#define HA_SPF_MAX_HOLDTIME_DEFAULT	    10000

/* HA interface default values. */
#define HA_OUTPUT_COST_DEFAULT           10
#define HA_OUTPUT_COST_INFINITE	   UINT16_MAX
#define HA_ROUTER_DEAD_INTERVAL_DEFAULT  40
#define HA_ROUTER_DEAD_INTERVAL_MINIMAL   1
#define HA_HELLO_INTERVAL_DEFAULT        10
#define HA_ROUTER_PRIORITY_DEFAULT        1
#define HA_RETRANSMIT_INTERVAL_DEFAULT    5
#define HA_TRANSMIT_DELAY_DEFAULT         1
#define HA_DEFAULT_BANDWIDTH		 10000	/* Kbps */

#define HA_DEFAULT_REF_BANDWIDTH	100000  /* Kbps */

#define HA_POLL_INTERVAL_DEFAULT         60
#define HA_NEIGHBOR_PRIORITY_DEFAULT      0

#define HA_MTU_IGNORE_DEFAULT             0
#define HA_FAST_HELLO_DEFAULT             0

/* HA options. */
#define HA_OPTION_T                    0x01  /* TOS. */
#define HA_OPTION_E                    0x02
#define HA_OPTION_MC                   0x04
#define HA_OPTION_NP                   0x08
#define HA_OPTION_EA                   0x10
#define HA_OPTION_DC                   0x20
#define HA_OPTION_O                    0x40

/* HA Database Description flags. */
#define HA_DD_FLAG_MS                  0x01
#define HA_DD_FLAG_M                   0x02
#define HA_DD_FLAG_I                   0x04
#define HA_DD_FLAG_ALL                 0x07

#define HA_LS_REFRESH_SHIFT       (60 * 15)
#define HA_LS_REFRESH_JITTER      60

/* HA master for system wide configuration and variables. */
struct ha_master
{
  /* HA instance. */
  struct list *ha;

  /* HA thread master. */
  struct thread_master *master;

  /* Kroute interface list. */
  struct list *iflist;

  /* Redistributed external information. */
  struct route_table *external_info[KROUTE_ROUTE_MAX + 1];
#define EXTERNAL_INFO(T)      hm->external_info[T]

  /* HA start time. */
  time_t start_time;

  /* Various HA global configuration. */
  u_char options;
#define HA_MASTER_SHUTDOWN (1 << 0) /* deferred-shutdown */  

  /* HA limit numbers. */
  u_int32_t max_nbr_perif;	/* Max neighbours per interface. */
  u_int32_t max_routes;			/* Max routes. */
  u_int32_t cur_routes;			/* Current routes. */
  
#define HA_MAX_NEIGHBOURS_PER_IF			(200+1)			
#define HA_MAX_ROUTES						40000
};

/* HA instance structure. */
struct ha
{
  /* HA Router ID. */
  struct in_addr router_id;		/* Configured authmatically. */
  struct in_addr router_id_static;	/* Configured manually. */

  /* ABR/ASBR internal flags. */
  u_char flags;
#define HA_FLAG_ABR           0x0001
#define HA_FLAG_ASBR          0x0002

  /* ABR type. */
  u_char abr_type;
#define HA_ABR_UNKNOWN	0
#define HA_ABR_STAND          1
#define HA_ABR_IBM            2
#define HA_ABR_CISCO          3
#define HA_ABR_SHORTCUT       4
#define HA_ABR_DEFAULT	HA_ABR_CISCO

  /* NSSA ABR */
  u_char anyNSSA;		/* Bump for every NSSA attached. */

  /* Configured variables. */
  u_char config;
#define HA_RFC1583_COMPATIBLE         (1 << 0)
#define HA_OPAQUE_CAPABLE		(1 << 2)
#define HA_LOG_ADJACENCY_CHANGES	(1 << 3)
#define HA_LOG_ADJACENCY_DETAIL	(1 << 4)

#ifdef HAVE_OPAQUE_LSA
  /* Opaque-LSA administrative flags. */
  u_char opaque;
#define OPAQUE_OPERATION_READY_BIT	(1 << 0)
#define OPAQUE_BLOCK_TYPE_09_LSA_BIT	(1 << 1)
#define OPAQUE_BLOCK_TYPE_10_LSA_BIT	(1 << 2)
#define OPAQUE_BLOCK_TYPE_11_LSA_BIT	(1 << 3)
#endif /* HAVE_OPAQUE_LSA */

  /* RFC3137 stub router. Configured time to stay stub / max-metric */
  unsigned int stub_router_startup_time;	/* seconds */
  unsigned int stub_router_shutdown_time;	/* seconds */
#define HA_STUB_ROUTER_UNCONFIGURED	  0

  /* SPF parameters */
  unsigned int spf_delay;		/* SPF delay time. */
  unsigned int spf_holdtime;		/* SPF hold time. */
  unsigned int spf_max_holdtime;	/* SPF maximum-holdtime */
  unsigned int spf_hold_multiplier;	/* Adaptive multiplier for hold time */
  
  int default_originate;		/* Default information originate. */
#define DEFAULT_ORIGINATE_NONE		0
#define DEFAULT_ORIGINATE_KROUTE		1
#define DEFAULT_ORIGINATE_ALWAYS	2
  u_int32_t ref_bandwidth;		/* Reference Bandwidth (Kbps). */
  struct route_table *networks;         /* HA config networks. */
  struct list *vlinks;                  /* Configured Virtual-Links. */
  struct list *areas;                   /* HA areas. */
  struct route_table *nbr_nbma;
  struct ha_area *backbone;           /* Pointer to the Backbone Area. */

  struct list *oiflist;                 /* ha interfaces */
  u_char passive_interface_default;	/* passive-interface default */

  /* LSDB of AS-external-LSAs. */
  struct ha_lsdb *lsdb;
  
  /* Flags. */
  int external_origin;			/* AS-external-LSA origin flag. */
  int ase_calc;				/* ASE calculation flag. */

#ifdef HAVE_OPAQUE_LSA
  struct list *opaque_lsa_self;		/* Type-11 Opaque-LSAs */
#endif /* HAVE_OPAQUE_LSA */

  /* Routing tables. */
  struct route_table *old_table;        /* Old routing table. */
  struct route_table *new_table;        /* Current routing table. */

  struct route_table *old_rtrs;         /* Old ABR/ASBR RT. */
  struct route_table *new_rtrs;         /* New ABR/ASBR RT. */

  struct route_table *new_external_route;   /* New External Route. */
  struct route_table *old_external_route;   /* Old External Route. */
  
  struct route_table *external_lsas;    /* Database of external LSAs,
					   prefix is LSA's adv. network*/

  /* Time stamps. */
  struct timeval ts_spf;		/* SPF calculation time stamp. */

  struct list *maxage_lsa;              /* List of MaxAge LSA for deletion. */
  int redistribute;                     /* Num of redistributed protocols. */

  /* Threads. */
  struct thread *t_abr_task;            /* ABR task timer. */
  struct thread *t_asbr_check;          /* ASBR check timer. */
  struct thread *t_distribute_update;   /* Distirbute list update timer. */
  struct thread *t_spf_calc;	        /* SPF calculation timer. */
  struct thread *t_ase_calc;		/* ASE calculation timer. */
  struct thread *t_external_lsa;	/* AS-external-LSA origin timer. */
#ifdef HAVE_OPAQUE_LSA
  struct thread *t_opaque_lsa_self;	/* Type-11 Opaque-LSAs origin event. */
#endif /* HAVE_OPAQUE_LSA */

#define OSFP_LSA_MAXAGE_REMOVE_DELAY_DEFAULT	60
  unsigned int maxage_delay;		/* Delay on Maxage remover timer, sec */
  struct thread *t_maxage;              /* MaxAge LSA remover timer. */
#define HA_LSA_MAXAGE_CHECK_INTERVAL		30
  struct thread *t_maxage_walker;       /* MaxAge LSA checking timer. */

  struct thread *t_deferred_shutdown;	/* deferred/stub-router shutdown timer*/

  struct thread *t_write;
  struct thread *t_read;
  int fd;
  int maxsndbuflen;
  struct stream *ibuf;
  struct list *oi_write_q;
  
  /* Distribute lists out of other route sources. */
  struct 
  {
    char *name;
    struct access_list *list;
  } dlist[KROUTE_ROUTE_MAX];
#define DISTRIBUTE_NAME(O,T)    (O)->dlist[T].name
#define DISTRIBUTE_LIST(O,T)    (O)->dlist[T].list

  /* Redistribute metric info. */
  struct 
  {
    int type;                   /* External metric type (E1 or E2).  */
    int value;		        /* Value for static metric (24-bit).
				   -1 means metric value is not set. */
  } dmetric [KROUTE_ROUTE_MAX + 1];

  /* For redistribute route map. */
  struct
  {
    char *name;
    struct route_map *map;
  } route_map [KROUTE_ROUTE_MAX + 1]; /* +1 is for default-information */
#define ROUTEMAP_NAME(O,T)   (O)->route_map[T].name
#define ROUTEMAP(O,T)        (O)->route_map[T].map
  
  int default_metric;		/* Default metric for redistribute. */

#define HA_LSA_REFRESHER_GRANULARITY 10
#define HA_LSA_REFRESHER_SLOTS ((HA_LS_REFRESH_TIME + \
                                  HA_LS_REFRESH_SHIFT)/10 + 1)
  struct
  {
    u_int16_t index;
    struct list *qs[HA_LSA_REFRESHER_SLOTS];
  } lsa_refresh_queue;
  
  struct thread *t_lsa_refresher;
  time_t lsa_refresher_started;
#define HA_LSA_REFRESH_INTERVAL_DEFAULT 10
  u_int16_t lsa_refresh_interval;
  
  /* Distance parameter. */
  u_char distance_all;
  u_char distance_intra;
  u_char distance_inter;
  u_char distance_external;

  /* Statistics for LSA origination. */
  u_int32_t lsa_originate_count;

  /* Statistics for LSA used for new instantiation. */
  u_int32_t rx_lsa_count;
 
  struct route_table *distance_table;

};

/* HA area structure. */
struct ha_area
{
  /* HA instance. */
  struct ha *ha;

  /* Kroute interface list belonging to the area. */
  struct list *oiflist;

  /* Area ID. */
  struct in_addr area_id;

  /* Area ID format. */
  char format;
#define HA_AREA_ID_FORMAT_ADDRESS         1
#define HA_AREA_ID_FORMAT_DECIMAL         2

  /* Address range. */
  struct list *address_range;

  /* Configured variables. */
  int external_routing;                 /* ExternalRoutingCapability. */
#define HA_AREA_DEFAULT       0
#define HA_AREA_STUB          1
#define HA_AREA_NSSA          2
#define HA_AREA_TYPE_MAX	3
  int no_summary;                       /* Don't inject summaries into stub.*/
  int shortcut_configured;              /* Area configured as shortcut. */
#define HA_SHORTCUT_DEFAULT	0
#define HA_SHORTCUT_ENABLE	1
#define HA_SHORTCUT_DISABLE	2
  int shortcut_capability;              /* Other ABRs agree on S-bit */
  u_int32_t default_cost;               /* StubDefaultCost. */
  int auth_type;                        /* Authentication type. */
  

  u_char NSSATranslatorRole;          /* NSSA configured role */
#define HA_NSSA_ROLE_NEVER     0
#define HA_NSSA_ROLE_CANDIDATE 1
#define HA_NSSA_ROLE_ALWAYS    2
  u_char NSSATranslatorState;              /* NSSA operational role */
#define HA_NSSA_TRANSLATE_DISABLED 0
#define HA_NSSA_TRANSLATE_ENABLED  1
  int NSSATranslatorStabilityInterval;
  
  u_char transit;			/* TransitCapability. */
#define HA_TRANSIT_FALSE      0
#define HA_TRANSIT_TRUE       1
  struct route_table *ranges;		/* Configured Area Ranges. */
  
  /* RFC3137 stub router state flags for area */
  u_char stub_router_state;
#define HA_AREA_ADMIN_STUB_ROUTED	(1 << 0) /* admin stub-router set */
#define HA_AREA_IS_STUB_ROUTED	(1 << 1) /* stub-router active */
#define HA_AREA_WAS_START_STUB_ROUTED	(1 << 2) /* startup SR was done */
  
  /* Area related LSDBs[Type1-4]. */
  struct ha_lsdb *lsdb;

  /* Self-originated LSAs. */
  struct ha_lsa *router_lsa_self;
#ifdef HAVE_OPAQUE_LSA
  struct list *opaque_lsa_self;		/* Type-10 Opaque-LSAs */
#endif /* HAVE_OPAQUE_LSA */

  /* Area announce list. */
  struct 
  {
    char *name;
    struct access_list *list;
  } _export;
#define EXPORT_NAME(A)  (A)->_export.name
#define EXPORT_LIST(A)  (A)->_export.list

  /* Area acceptance list. */
  struct 
  {
    char *name;
    struct access_list *list;
  } import;
#define IMPORT_NAME(A)  (A)->import.name
#define IMPORT_LIST(A)  (A)->import.list

  /* Type 3 LSA Area prefix-list. */
  struct 
  {
    char *name;
    struct prefix_list *list;
  } plist_in;
#define PREFIX_LIST_IN(A)   (A)->plist_in.list
#define PREFIX_NAME_IN(A)   (A)->plist_in.name

  struct
  {
    char *name;
    struct prefix_list *list;
  } plist_out;
#define PREFIX_LIST_OUT(A)  (A)->plist_out.list
#define PREFIX_NAME_OUT(A)  (A)->plist_out.name

  /* Shortest Path Tree. */
  struct vertex *spf;

  /* Threads. */
  struct thread *t_stub_router;    /* Stub-router timer */
#ifdef HAVE_OPAQUE_LSA
  struct thread *t_opaque_lsa_self;	/* Type-10 Opaque-LSAs origin. */
#endif /* HAVE_OPAQUE_LSA */

  /* Statistics field. */
  u_int32_t spf_calculation;	/* SPF Calculation Count. */

  /* Router count. */
  u_int32_t abr_count;		/* ABR router in this area. */
  u_int32_t asbr_count;		/* ASBR router in this area. */

  /* Counters. */
  u_int32_t act_ints;		/* Active interfaces. */
  u_int32_t full_nbrs;		/* Fully adjacent neighbors. */
  u_int32_t full_vls;		/* Fully adjacent virtual neighbors. */
};

/* HA config network structure. */
struct ha_network
{
  /* Area ID. */
  struct in_addr area_id;
  int format;
};

/* HA NBMA neighbor structure. */
struct ha_nbr_nbma
{
  /* Neighbor IP address. */
  struct in_addr addr;

  /* HA interface. */
  struct ha_interface *oi;

  /* HA neighbor structure. */
  struct ha_neighbor *nbr;

  /* Neighbor priority. */
  u_char priority;

  /* Poll timer value. */
  u_int32_t v_poll;

  /* Poll timer thread. */
  struct thread *t_poll;

  /* State change. */
  u_int32_t state_change;
};

/* Macro. */
#define HA_AREA_SAME(X,Y) \
        (memcmp ((X->area_id), (Y->area_id), IPV4_MAX_BYTELEN) == 0)

#define IS_HA_ABR(O)		((O)->flags & HA_FLAG_ABR)
#define IS_HA_ASBR(O)		((O)->flags & HA_FLAG_ASBR)

#define HA_IS_AREA_ID_BACKBONE(I) ((I).s_addr == HA_AREA_BACKBONE)
#define HA_IS_AREA_BACKBONE(A) HA_IS_AREA_ID_BACKBONE ((A)->area_id)

#ifdef roundup
#  define ROUNDUP(val, gran)	roundup(val, gran)
#else /* roundup */
#  define ROUNDUP(val, gran)	(((val) - 1 | (gran) - 1) + 1)
#endif /* roundup */

#define LSA_OPTIONS_GET(area) \
        (((area)->external_routing == HA_AREA_DEFAULT) ? HA_OPTION_E : 0)
#define LSA_OPTIONS_NSSA_GET(area) \
        (((area)->external_routing == HA_AREA_NSSA)  ? HA_OPTION_NP : 0)

#define HA_TIMER_ON(T,F,V)                                                  \
    do {                                                                      \
      if (!(T))                                                               \
	(T) = thread_add_timer (master, (F), ha, (V));                      \
    } while (0)

#define HA_AREA_TIMER_ON(T,F,V)                                             \
    do {                                                                      \
      if (!(T))                                                               \
        (T) = thread_add_timer (master, (F), area, (V));                      \
    } while (0)

#define HA_POLL_TIMER_ON(T,F,V)                                             \
    do {                                                                      \
      if (!(T))                                                               \
        (T) = thread_add_timer (master, (F), nbr_nbma, (V));                  \
    } while (0)

#define HA_POLL_TIMER_OFF(X)		HA_TIMER_OFF((X))

#define HA_TIMER_OFF(X)                                                     \
    do {                                                                      \
      if (X)                                                                  \
        {                                                                     \
          thread_cancel (X);                                                  \
          (X) = NULL;                                                         \
        }                                                                     \
    } while (0)

/* Extern variables. */
extern struct ha_master *hm;
extern struct thread_master *master;

extern const struct message ha_ism_state_msg[];
extern const struct message ha_nsm_state_msg[];
extern const struct message ha_lsa_type_msg[];
extern const struct message ha_link_state_id_type_msg[];
extern const struct message ha_network_type_msg[];
extern const int ha_ism_state_msg_max;
extern const int ha_nsm_state_msg_max;
extern const int ha_lsa_type_msg_max;
extern const int ha_link_state_id_type_msg_max;
extern const int ha_redistributed_proto_max;
extern const int ha_network_type_msg_max;
extern struct zclient *zclient;
extern struct thread_master *master;
extern int ha_zlog;

/* Prototypes. */
extern const char *ha_redist_string(u_int route_type);
extern struct ha *ha_lookup (void);
extern struct ha *ha_get (void);
extern void ha_finish (struct ha *);
extern void ha_router_id_update (struct ha *ha);
extern int ha_network_set (struct ha *, struct prefix_ipv4 *,
			     struct in_addr);
extern int ha_network_unset (struct ha *, struct prefix_ipv4 *,
			       struct in_addr);
extern int ha_area_stub_set (struct ha *, struct in_addr);
extern int ha_area_stub_unset (struct ha *, struct in_addr);
extern int ha_area_no_summary_set (struct ha *, struct in_addr);
extern int ha_area_no_summary_unset (struct ha *, struct in_addr);
extern int ha_area_nssa_set (struct ha *, struct in_addr);
extern int ha_area_nssa_unset (struct ha *, struct in_addr);
extern int ha_area_nssa_translator_role_set (struct ha *, struct in_addr,
					       int);
extern int ha_area_export_list_set (struct ha *, struct ha_area *,
				      const char *);
extern int ha_area_export_list_unset (struct ha *, struct ha_area *);
extern int ha_area_import_list_set (struct ha *, struct ha_area *,
				      const char *);
extern int ha_area_import_list_unset (struct ha *, struct ha_area *);
extern int ha_area_shortcut_set (struct ha *, struct ha_area *, int);
extern int ha_area_shortcut_unset (struct ha *, struct ha_area *);
extern int ha_timers_refresh_set (struct ha *, int);
extern int ha_timers_refresh_unset (struct ha *);
extern int ha_nbr_nbma_set (struct ha *, struct in_addr);
extern int ha_nbr_nbma_unset (struct ha *, struct in_addr);
extern int ha_nbr_nbma_priority_set (struct ha *, struct in_addr, u_char);
extern int ha_nbr_nbma_priority_unset (struct ha *, struct in_addr);
extern int ha_nbr_nbma_poll_interval_set (struct ha *, struct in_addr,
					    unsigned int);
extern int ha_nbr_nbma_poll_interval_unset (struct ha *, struct in_addr);
extern void ha_prefix_list_update (struct prefix_list *);
extern void ha_init (void);
extern void ha_if_update (struct ha *, struct interface *);
extern void ha_ls_upd_queue_empty (struct ha_interface *);
extern void ha_terminate (void);
extern void ha_nbr_nbma_if_update (struct ha *, struct ha_interface *);
extern struct ha_nbr_nbma *ha_nbr_nbma_lookup (struct ha *,
						   struct in_addr);
extern struct ha_nbr_nbma *ha_nbr_nbma_lookup_next (struct ha *,
							struct in_addr *,
							int);
extern int ha_oi_count (struct interface *);

extern struct ha_area *ha_area_get (struct ha *, struct in_addr, int);
extern void ha_area_check_free (struct ha *, struct in_addr);
extern struct ha_area *ha_area_lookup_by_area_id (struct ha *,
						      struct in_addr);
extern void ha_area_add_if (struct ha_area *, struct ha_interface *);
extern void ha_area_del_if (struct ha_area *, struct ha_interface *);

extern void ha_route_map_init (void);
extern void ha_snmp_init (void);

extern void ha_master_init (void);

#define PATH_HA_PID "/var/run/ha_deamon.pid"
#define HA_VTYSH_PATH "/var/run/ha_deamon.vty"

#endif /* _KROUTE_HAD_H */

