#ifndef _KROUTE_HA_PACKET_H
#define _KROUTE_HA_PACKET_H

#define HA_MAX_PACKET_SIZE  65535U   /* includes IP Header size. */

/* Default protocol, port number. */
#ifndef IPPROTO_HA
#define IPPROTO_HA            0
#endif /* IPPROTO_HAIGP */

/* IP precedence. */
#ifndef IPTOS_PREC_INTERNETCONTROL
#define IPTOS_PREC_INTERNETCONTROL	0xC0
#endif /* IPTOS_PREC_INTERNETCONTROL */

extern int ha_read (struct thread *);
extern int ha_sock_init (void);

#endif /* _KROUTE_HA_PACKET_H */
