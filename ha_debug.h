/*
 * HAd dump routine.
 * Copyright (C) 1999 Toshiaki Takada
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
 * along with GNU Kroute; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA. 
 */

#ifndef _KROUTE_HA_DUMP_H
#define _KROUTE_HA_DUMP_H

/* Debug Flags. */
#define HA_DEBUG_HELLO	0x01
#define HA_DEBUG_DB_DESC	0x02
#define HA_DEBUG_LS_REQ	0x04
#define HA_DEBUG_LS_UPD	0x08
#define HA_DEBUG_LS_ACK	0x10
#define HA_DEBUG_ALL		0x1f

#define HA_DEBUG_SEND		0x01
#define HA_DEBUG_RECV		0x02
#define HA_DEBUG_SEND_RECV    0x03
#define HA_DEBUG_DETAIL	0x04

#define HA_DEBUG_ISM_STATUS	0x01
#define HA_DEBUG_ISM_EVENTS	0x02
#define HA_DEBUG_ISM_TIMERS	0x04
#define HA_DEBUG_ISM		0x07
#define HA_DEBUG_NSM_STATUS	0x01
#define HA_DEBUG_NSM_EVENTS	0x02
#define HA_DEBUG_NSM_TIMERS   0x04
#define HA_DEBUG_NSM		0x07

#define HA_DEBUG_LSA_GENERATE 0x01
#define HA_DEBUG_LSA_FLOODING	0x02
#define HA_DEBUG_LSA_INSTALL  0x04
#define HA_DEBUG_LSA_REFRESH  0x08
#define HA_DEBUG_LSA		0x0F

#define HA_DEBUG_KROUTE_INTERFACE     0x01
#define HA_DEBUG_KROUTE_REDISTRIBUTE  0x02
#define HA_DEBUG_KROUTE	       0x03

#define HA_DEBUG_EVENT        0x01
#define HA_DEBUG_NSSA		0x02

/* Macro for setting debug option. */
#define CONF_DEBUG_PACKET_ON(a, b)	    conf_debug_ha_packet[a] |= (b)
#define CONF_DEBUG_PACKET_OFF(a, b)	    conf_debug_ha_packet[a] &= ~(b)
#define TERM_DEBUG_PACKET_ON(a, b)	    term_debug_ha_packet[a] |= (b)
#define TERM_DEBUG_PACKET_OFF(a, b)	    term_debug_ha_packet[a] &= ~(b)
#define DEBUG_PACKET_ON(a, b) \
    do { \
      CONF_DEBUG_PACKET_ON(a, b); \
      TERM_DEBUG_PACKET_ON(a, b); \
    } while (0)
#define DEBUG_PACKET_OFF(a, b) \
    do { \
      CONF_DEBUG_PACKET_OFF(a, b); \
      TERM_DEBUG_PACKET_OFF(a, b); \
    } while (0)

#define CONF_DEBUG_ON(a, b)	 conf_debug_ha_ ## a |= (HA_DEBUG_ ## b)
#define CONF_DEBUG_OFF(a, b)	 conf_debug_ha_ ## a &= ~(HA_DEBUG_ ## b)
#define TERM_DEBUG_ON(a, b)	 term_debug_ha_ ## a |= (HA_DEBUG_ ## b)
#define TERM_DEBUG_OFF(a, b)	 term_debug_ha_ ## a &= ~(HA_DEBUG_ ## b)
#define DEBUG_ON(a, b) \
     do { \
       CONF_DEBUG_ON(a, b); \
       TERM_DEBUG_ON(a, b); \
     } while (0)
#define DEBUG_OFF(a, b) \
     do { \
       CONF_DEBUG_OFF(a, b); \
       TERM_DEBUG_OFF(a, b); \
     } while (0)

/* Macro for checking debug option. */
#define IS_DEBUG_HA_PACKET(a, b) \
	(term_debug_ha_packet[a] & HA_DEBUG_ ## b)
#define IS_DEBUG_HA(a, b) \
	(term_debug_ha_ ## a & HA_DEBUG_ ## b)
#define IS_DEBUG_HA_EVENT IS_DEBUG_HA(event,EVENT)

#define IS_DEBUG_HA_NSSA  IS_DEBUG_HA(nssa,NSSA)

#define IS_CONF_DEBUG_HA_PACKET(a, b) \
	(conf_debug_ha_packet[a] & HA_DEBUG_ ## b)
#define IS_CONF_DEBUG_HA(a, b) \
	(conf_debug_ha_ ## a & HA_DEBUG_ ## b)

#ifdef ORIGINAL_CODING
#else /* ORIGINAL_CODING */
struct stream;
#endif /* ORIGINAL_CODING */

/* Extern debug flag. */
extern unsigned long term_debug_ha_packet[];
extern unsigned long term_debug_ha_event;
extern unsigned long term_debug_ha_ism;
extern unsigned long term_debug_ha_nsm;
extern unsigned long term_debug_ha_lsa;
extern unsigned long term_debug_ha_kroute;
extern unsigned long term_debug_ha_nssa;

/* Prototypes. */
extern const char *ha_timer_dump (struct thread *, char *, size_t);
extern const char *ha_timeval_dump (struct timeval *, char *, size_t);
extern void debug_init (void);

/* Appropriate buffer size to use with ha_timer_dump and ha_timeval_dump: */
#define HA_TIME_DUMP_SIZE	16

#endif /* _KROUTE_HA_DUMP_H */
