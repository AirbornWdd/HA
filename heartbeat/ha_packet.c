#include <kroute.h>

#include "thread.h"
#include "memory.h"
#include "linklist.h"
#include "prefix.h"
#include "if.h"
#include "table.h"
#include "sockunion.h"
#include "stream.h"
#include "log.h"
#include "sockopt.h"
#include "checksum.h"
#include "md5.h"

#include "ha_packet.h"
#include "ha_deamon.h"

int
ha_sock_init (void)
{
  int ha_sock;
  int ret, hincl = 1;

  ha_sock = socket (AF_INET, SOCK_RAW, IPPROTO_HA);
  if (ha_sock < 0)
    {
      int save_errno = errno;
      zlog_err ("ha_read_sock_init: socket: %s", safe_strerror (save_errno));
      exit(1);
    }
    
#ifdef IP_HDRINCL
  /* we will include IP header with packet */
  ret = setsockopt (ha_sock, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof (hincl));
  if (ret < 0)
    {
      int save_errno = errno;
      zlog_warn ("Can't set IP_HDRINCL option for fd %d: %s",
             ha_sock, safe_strerror(save_errno));
    }
#elif defined (IPTOS_PREC_INTERNETCONTROL)
#warning "IP_HDRINCL not available on this system"
#warning "using IPTOS_PREC_INTERNETCONTROL"
  ret = setsockopt_ipv4_tos(ha_sock, IPTOS_PREC_INTERNETCONTROL);
  if (ret < 0)
    {
      int save_errno = errno;
      zlog_warn ("can't set sockopt IP_TOS %d to socket %d: %s",
             tos, ha_sock, safe_strerror(save_errno));
      close (ha_sock);    /* Prevent sd leak. */
      return ret;
    }
#else /* !IPTOS_PREC_INTERNETCONTROL */
#warning "IP_HDRINCL not available, nor is IPTOS_PREC_INTERNETCONTROL"
  zlog_warn ("IP_HDRINCL option not available");
#endif /* IP_HDRINCL */

  ret = setsockopt_ifindex (AF_INET, ha_sock, 1);

  if (ret < 0)
     zlog_warn ("Can't set pktinfo option for fd %d", ha_sock);

  return ha_sock;
}

static struct stream *
ha_recv_packet (int fd, struct interface **ifp, struct stream *ibuf)
{
  int ret;
  struct ip *iph;
  u_int16_t ip_len;
  unsigned int ifindex = 0;
  struct iovec iov;
  /* Header and data both require alignment. */
  char buff [CMSG_SPACE(SOPT_SIZE_CMSG_IFINDEX_IPV4())];
  struct msghdr msgh;

  memset (&msgh, 0, sizeof (struct msghdr));
  msgh.msg_iov = &iov;
  msgh.msg_iovlen = 1;
  msgh.msg_control = (caddr_t) buff;
  msgh.msg_controllen = sizeof (buff);
  
  ret = stream_recvmsg (ibuf, fd, &msgh, 0, HA_MAX_PACKET_SIZE+1);
  if (ret < 0)
    {
      zlog_warn("stream_recvmsg failed: %s", safe_strerror(errno));
      return NULL;
    }
  if ((unsigned int)ret < sizeof(iph)) /* ret must be > 0 now */
    {
      zlog_warn("ha_recv_packet: discarding runt packet of length %d "
		"(ip header size is %u)",
		ret, (u_int)sizeof(iph));
      return NULL;
    }
  
  /* Note that there should not be alignment problems with this assignment
     because this is at the beginning of the stream data buffer. */
  iph = (struct ip *) STREAM_DATA(ibuf);
  sockopt_iphdrincl_swab_systoh (iph);
  
  ip_len = iph->ip_len;
  
#if !defined(GNU_LINUX) && (OpenBSD < 200311) && (__FreeBSD_version < 1000000)
  /*
   * Kernel network code touches incoming IP header parameters,
   * before protocol specific processing.
   *
   *   1) Convert byteorder to host representation.
   *      --> ip_len, ip_id, ip_off
   *
   *   2) Adjust ip_len to strip IP header size!
   *      --> If user process receives entire IP packet via RAW
   *          socket, it must consider adding IP header size to
   *          the "ip_len" field of "ip" structure.
   *
   * For more details, see <netinet/ip_input.c>.
   */
  ip_len = ip_len + (iph->ip_hl << 2);
#endif
  
#if defined(__DragonFly__)
  /*
   * in DragonFly's raw socket, ip_len/ip_off are read 
   * in network byte order.
   * As OpenBSD < 200311 adjust ip_len to strip IP header size!
   */
  ip_len = ntohs(iph->ip_len) + (iph->ip_hl << 2);
#endif

  ifindex = getsockopt_ifindex (AF_INET, &msgh);
  
  *ifp = if_lookup_by_index (ifindex);

  if (ret != ip_len)
    {
      zlog_warn ("ha_recv_packet read length mismatch: ip_len is %d, "
       		 "but recvmsg returned %d", ip_len, ret);
      return NULL;
    }
  
  return ibuf;
}

/* Starting point of packet process function. */
int
ha_read (struct thread *thread)
{
  struct stream *ibuf;
  struct ha *ha;
  struct interface *ifp;

  /* first of all get interface pointer. */
  ha = THREAD_ARG (thread);

  /* prepare for next packet. */
  ha->t_read = thread_add_read (master, ha_read, ha, ha->fd);

  stream_reset(ha->ibuf);
  if (!(ibuf = ha_recv_packet (ha->fd, &ifp, ha->ibuf)))
    return -1;
  /* This raw packet is known to be at least as big as its IP header. */

  return 0;
}
