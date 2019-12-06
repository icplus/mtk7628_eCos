#undef _KERNEL

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>


#define NTP_DEFAULT_PORT	123
#define NTP_DIFF_1970_1990	0x83aa7e80

/* multiply x by 4294.967296 */
#define NTP_FRAC(x) ( 4294*(x) + ( (1981*(x))>>11 ) )
/* divide x by 4294.967296 */
#define NTP_USEC(x) ( ( (x) >> 12 ) - 759 * ( ( ( (x) >> 10 ) + 32768 ) >> 16 ) )

extern int ntp_zonetime_offset;
extern int ntp_GMTtime_offset;
extern int gettimeofday(struct timeval *tv,  struct  timezone *tz);

extern int Check_IP(char* str);

struct ntp_timestamp
{
	unsigned int sec;
	unsigned int frac;
};

static int ntpc_do_connect(int sock, char *server)
{
	static int fail_count = 0;
	struct sockaddr_in dest;
	struct hostent *host;
	
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	if (Check_IP(server) < 0)
	{
		if ((host = gethostbyname(server)) == NULL)
		{
			if (fail_count++ % 20 == 0)
				diag_printf("ntpc look up %s failed\n", server);
			goto err_out;
		}
		
		fail_count = 0;
		
		if (host->h_length != 4)
			goto err_out;
		memcpy(&dest.sin_addr.s_addr, host->h_addr_list[0], 4);
	}
	else
		inet_aton(server, &dest.sin_addr);
	dest.sin_port = htons(NTP_DEFAULT_PORT);
	
	if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) != 0)
	{
		diag_printf("ntpc connect failed: %s\n", strerror(errno));
		goto err_out;
	}
	
	return 0;
	
err_out:
	return -1;
}


#define NTP_PROBE_LI		0		/* leap indicator: no warning */
#define NTP_PROBE_VN		3		/* version number: 3 */
#define NTP_PROBE_MODE		3		/* mode: client */
#define NTP_PROBE_STRATUM	0		/* stratum level: unspecified */
#define NTP_PROBE_POLL		4		/* poll interval */
#define NTP_PROBE_PRECISION	-6		/* precision */

static int ntpc_sendprobe(int sock)
{
	uint32_t msg[12];	/* without authentication */
	struct timeval timestamp;
	
	/* fill NTP message header */
	memset(msg, 0, sizeof(msg));
	msg[0] = htonl(
		(NTP_PROBE_LI << 30) |
		(NTP_PROBE_VN << 27) |
		(NTP_PROBE_MODE << 24) |
		(NTP_PROBE_STRATUM << 16) |
		(NTP_PROBE_POLL << 8) |
		(NTP_PROBE_PRECISION & 0xff));
	/* root delay */
	msg[1] = htonl(1 << 16);
	/* root dispersion */
	msg[2] = htonl(1 << 16);
	gettimeofday(&timestamp, NULL);
	timestamp.tv_sec += ntp_zonetime_offset;
	/* transmit timestamp */
	msg[10] = htonl(timestamp.tv_sec + NTP_DIFF_1970_1990);
	msg[11] = htonl(NTP_FRAC(timestamp.tv_usec));
	
	send(sock, msg, sizeof(msg), 0);
	
	return 0;
}


static int ntpc_parse(uint32_t *msg, struct ntp_timestamp *ntp_ts)
{
	struct ntp_timestamp tx_ts;
	struct timeval tv_set;
	struct timeval tv_get;
	
	tx_ts.sec	= ntohl(msg[10]);
	tx_ts.frac	= ntohl(msg[11]);
	
	tv_set.tv_sec  = tx_ts.sec - NTP_DIFF_1970_1990;
	tv_set.tv_usec = NTP_USEC(tx_ts.frac);
	gettimeofday(&tv_get, NULL);
	tv_get.tv_sec += ntp_zonetime_offset;
	/* commit */
	ntp_GMTtime_offset = tv_set.tv_sec - time(0);

	return 0;
}


int ntpc_synchonize(char *server)
{
	int ntpc_sock = -1;
	fd_set fdset;
	struct timeval tv = { 0, 0 };
	int probe_sent = 0;
	int n, rlen;
	char buf[1500] = {0};
	struct sockaddr from;
	int from_len;
	struct timeval timestamp;
	struct ntp_timestamp ntp_ts;
	
	ntpc_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (ntpc_sock < 0) {
		diag_printf("ntpc socket failed: %s\n", strerror(errno));
		goto err_out;
	}
	
	/* resolve and connect to server */
	if (ntpc_do_connect(ntpc_sock, server) != 0)
		goto err_out;
	
	while (1)
	{
		FD_ZERO(&fdset);
		FD_SET(ntpc_sock, &fdset);
		n = select(ntpc_sock+1, &fdset, NULL, NULL, &tv);
		if (n > 0 && FD_ISSET(ntpc_sock, &fdset))
		{
			from_len = sizeof(from);
			rlen = recvfrom(ntpc_sock, buf, sizeof(buf), 0, &from, (socklen_t *)&from_len);
			if (rlen < 0)
			{
				diag_printf("ntpc recvfrom error: %s\n", strerror(errno));
				goto err_out;
			}
			
			gettimeofday(&timestamp, NULL);
			timestamp.tv_sec += ntp_zonetime_offset;
			ntp_ts.sec = timestamp.tv_sec + NTP_DIFF_1970_1990;
			ntp_ts.frac = NTP_FRAC(timestamp.tv_usec);
			ntpc_parse((uint32_t *)buf, &ntp_ts);
			break;
		}
		else if (probe_sent)
		{
			if (n < 0 && errno != EINTR)
			{
				diag_printf("ntpc select failed: %s\n", strerror(errno));
				goto err_out;
			}
			continue;
		}
		
		ntpc_sendprobe(ntpc_sock);
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		probe_sent++;
	}
	
	close(ntpc_sock);
	return 0;
	
err_out:
	if (ntpc_sock >= 0)
		close(ntpc_sock);
	return -1;
}

