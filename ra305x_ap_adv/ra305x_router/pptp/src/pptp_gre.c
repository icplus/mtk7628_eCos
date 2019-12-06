/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    pptp_gre.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <pptp.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socketvar.h>


extern struct pptp_buf pptp_tx_queue[];
extern int pptp_txq_head;
extern int pptp_txq_tail;
extern int pptp_tx_process_stop;
extern cyg_flag_t pptp_event_flags;


struct pptp_gre_hdr
{
	u_int8_t flags;
	u_int8_t version;
	u_int16_t protocol;
	u_int16_t payload_len;
	u_int16_t call_id;
	u_int32_t seq_num;
	u_int32_t ack_num;
};

#define PPTP_GRE_FLAGS_C		0x80	/* Checksum Present */
#define PPTP_GRE_FLAGS_R		0x40	/* Routing Present */
#define PPTP_GRE_FLAGS_K		0x20	/* Key Present */
#define PPTP_GRE_FLAGS_S		0x10	/* Sequence Number Present */
#define PPTP_GRE_VER_ACK		0x80	/* Acknowledge Number Present */

#define PPTP_GRE_MAX_PKTSIZE	1600//(8196/4)
#define PPTP_GRE_PROTO			0x880B
#define PPTP_GRE_VER			1

#ifdef PPTP_BOOST_VERSION //pptp boost version: pptp throughput 91Mbps @5350


int pptp_write(int s, struct mbuf *m)
{
	cyg_file *fp;
	int error;
	fp = cyg_fp_get( s );

	if( fp == NULL )
	{
		diag_printf("fp == NULL in my_write\n");
		//cyg_fp_free( fp );
		return 0;
	}
	if( m == NULL )
	{
		diag_printf("m == NULL in my_write\n");
		return 0;
	}
	
	cyg_file_lock( fp, fp->f_syncmode );
	m->m_pkthdr.rcvif = (struct ifnet *)0;
	error = (sosend((struct socket *)fp->f_data, (struct mbuf *)0,
		NULL, m, (struct mbuf *)0, 0, NULL));

	if (error)
	{	diag_printf("pptp write sosend error=[%d]\n",error);
		if (m)
		m_freem(m);
	}
	cyg_file_unlock( fp, fp->f_syncmode );
	cyg_fp_free( fp );					

	return error;
}

int pptp_read(int s, struct mbuf **m, int len)
{
	cyg_file *fp;
	cyg_uio uio;
	int cnt,result;
	
	memset(&uio,0,sizeof(cyg_uio));
	uio.uio_resid = len;
	cnt = len;
	
    fp = cyg_fp_get( s );

    if( fp == NULL )
    {
    	panic("fp == NULL in my_read\n");
        //cyg_fp_free( fp );
		return 0;
	}
	cyg_file_lock( fp, fp->f_syncmode );
    result = (soreceive((struct socket *)fp->f_data, (struct mbuf **)0,
                      &uio, (struct mbuf **)m, (struct mbuf **)0, (int *)0));
	cyg_file_unlock( fp, fp->f_syncmode );
	cnt -= uio.uio_resid; 
	cyg_fp_free( fp );  
	if(result)diag_printf("soreceive return error in pptp read\n");
	return cnt; 

}

int pptp_gre_encap(PPTP_INFO *pInfo, struct mbuf *m)
{
	struct pptp_gre_hdr header = {0};
	char *p;
	struct mbuf *m1;
	int len, header_len;
	
	m1 = m;
	len = 0;
	while (m1)
	{
		len += m1->m_len;
		m1 = m1->m_next;
	}
	
	header.flags = PPTP_GRE_FLAGS_K;
	header.version = PPTP_GRE_VER;
	header.protocol = htons(PPTP_GRE_PROTO);
	header.payload_len = htons(len);
	header.call_id = htons(pInfo->call_id);
	
	if (m == NULL)
	{
		/* sequence number not present */
		if (pInfo->ack_sent != pInfo->seq_recv)
		{
			header.version |= PPTP_GRE_VER_ACK;
			header.seq_num = pInfo->seq_recv;
			pInfo->ack_sent = pInfo->seq_recv;
			return write(pInfo->gre_sock, &header, sizeof(header)-sizeof(header.seq_num));
		}
		return 0;
	}
	
	header.flags |= PPTP_GRE_FLAGS_S;
	header.seq_num = htonl(pInfo->seq_sent);
	if (pInfo->ack_sent != pInfo->seq_recv)
	{
		header.version |= PPTP_GRE_VER_ACK;
		header.ack_num = htonl(pInfo->seq_recv);
		pInfo->ack_sent = pInfo->seq_recv;
		header_len = sizeof(header);
	}
	else
	{
		header_len = sizeof(header) - sizeof(header.ack_num);
	}
	
	if (header_len + len >= (PPTP_GRE_MAX_PKTSIZE + sizeof(struct pptp_gre_hdr)))
	{
		/* packet too large */
		m_freem(m);
		return 0;
	}

	M_PREPEND(m, header_len, M_DONTWAIT);
	if (m == NULL)
	{	
		diag_printf("[PPTP]:M_PREPEND fail in pptp_gre_encap!\n");
		errno = ENOBUFS;
		return ENOBUFS;
	}
	memcpy(m->m_data, &header, header_len);
	
	pInfo->seq_sent++;
	
	pptp_write(pInfo->gre_sock, m);

	
	//return write(pInfo->gre_sock, buf, header_len+len);
}

int pptp_gre_input(PPTP_INFO *pInfo)
{
	struct pptp_gre_hdr *header;
	unsigned char *buf = NULL;
	int iphdr_len;
	unsigned int header_len, payload_len;
	u_int32_t ack_num;
	u_int32_t seq_num;
	int nbytes;
	struct mbuf *m;
	struct pptp_gre_hdr headerbuf={0};
	if ((nbytes = pptp_read(pInfo->gre_sock,&m,(PPTP_GRE_MAX_PKTSIZE + 64))) < 0)
	{
		m_freem(m);
		if (errno == ECONNREFUSED)
			return 0;
		else
		{
			diag_printf("pptp_gre_input: read error (%s)\n", strerror(errno));
			return nbytes;
		}
	}

	buf = m->m_data;
	
	/* strip off IP header if present */
	iphdr_len = 0;
	if ((buf[0] & 0xf0) == 0x40)
		iphdr_len = (buf[0] & 0x0f) << 2;
	
	/* validation */
	memcpy(&headerbuf,buf + iphdr_len,sizeof(struct pptp_gre_hdr));
	header=&headerbuf;
	//header = (struct pptp_gre_hdr *)(buf + iphdr_len);
	if (((header->version & 0x7f) != PPTP_GRE_VER) ||
		(ntohs(header->protocol) != PPTP_GRE_PROTO) ||
		(header->flags & PPTP_GRE_FLAGS_C) ||
		(header->flags & PPTP_GRE_FLAGS_R) ||
		(header->flags & PPTP_GRE_FLAGS_C) ||
		(!(header->flags & PPTP_GRE_FLAGS_K)) ||
		(header->flags & 0x0f))
	{
		diag_printf("pptp_gre_input: invalid header flags\n");
		m_freem(m);
		return 0;
	}
	
	if (header->version & PPTP_GRE_VER_ACK)
	{
		if (header->flags & PPTP_GRE_FLAGS_S)
			ack_num = header->ack_num;
		else
			ack_num = header->seq_num;
		
		if ((ack_num > pInfo->ack_recv) ||
			(((ack_num >> 31) == 0) && ((pInfo->ack_recv >> 31) == 1)))
		{
			pInfo->ack_recv = ack_num;
		}
	}
	
	if (header->flags & PPTP_GRE_FLAGS_S)
	{
		header_len = sizeof(*header);
		payload_len = ntohs(header->payload_len);
		seq_num = ntohl(header->seq_num);
		
		if (!(header->version & PPTP_GRE_VER_ACK))
			header_len -= sizeof(header->ack_num);
		
		if (nbytes - header_len < payload_len)
		{
			diag_printf("pptp_gre_input: packet too short\n");
			m_freem(m);
			return 0;
		}
		
		if ((seq_num > pInfo->seq_recv) ||
			(seq_num == 0 && pInfo->seq_recv == 0) ||
			(((seq_num >> 31) == 0) && ((pInfo->seq_recv >> 31) == 1)))
		{
			pInfo->seq_recv = seq_num;

			m->m_data += (iphdr_len + header_len);
			m->m_len = payload_len;
			m->m_pkthdr.len = payload_len;
			
			// indicate to ppp module
			ppppktin(&pInfo->if_ppp, m);
			
			return payload_len;
		}
		else
		{
			diag_printf("pptp_gre_input: out-of-order packet\n");
			m_freem(m);
			return 0;
		}
	}
	m_freem(m);
	return 0;
}


#else	//no def PPTP_BOOST_VERSION, means original version.

unsigned char pptp_gre_rx_buf[PPTP_GRE_MAX_PKTSIZE + 64];
unsigned char pptp_gre_tx_buf[PPTP_GRE_MAX_PKTSIZE + sizeof(struct pptp_gre_hdr)];


int pptp_gre_encap(PPTP_INFO *pInfo, struct mbuf *m)
{
	struct pptp_gre_hdr header;
	unsigned char *buf = pptp_gre_tx_buf;
	char *p;
	struct mbuf *m1;
	int len, header_len;
	
	m1 = m;
	len = 0;
	memset(&header,0,sizeof(header));
	while (m1)
	{
		len += m1->m_len;
		m1 = m1->m_next;
	}
	
	header.flags = PPTP_GRE_FLAGS_K;
	header.version = PPTP_GRE_VER;
	header.protocol = htons(PPTP_GRE_PROTO);
	header.payload_len = htons(len);
	header.call_id = htons(pInfo->call_id);
	
	if (m == NULL)
	{
		/* sequence number not present */
		if (pInfo->ack_sent != pInfo->seq_recv)
		{
			header.version |= PPTP_GRE_VER_ACK;
			header.seq_num = pInfo->seq_recv;
			pInfo->ack_sent = pInfo->seq_recv;
			return write(pInfo->gre_sock, &header, sizeof(header)-sizeof(header.seq_num));
		}
		return 0;
	}
	
	header.flags |= PPTP_GRE_FLAGS_S;
	header.seq_num = htonl(pInfo->seq_sent);
	if (pInfo->ack_sent != pInfo->seq_recv)
	{
		header.version |= PPTP_GRE_VER_ACK;
		header.ack_num = htonl(pInfo->seq_recv);
		pInfo->ack_sent = pInfo->seq_recv;
		header_len = sizeof(header);
	}
	else
	{
		header_len = sizeof(header) - sizeof(header.ack_num);
	}
	
	if (header_len + len >= sizeof(pptp_gre_tx_buf))
	{
		/* packet too large */
		m_freem(m);
		return 0;
	}
	
	memcpy(buf, &header, header_len);
	p = buf + header_len;
	m1 = m;
	while (m1)
	{
		if (m1->m_len)
		{
			memcpy(p, mtod(m1, char *), m1->m_len);
			p += m1->m_len;
			m1 = m1->m_next;
		}
		else
			break;
	}
	m_freem(m);
	
	pInfo->seq_sent++;
	
	return write(pInfo->gre_sock, buf, header_len+len);
}

int pptp_gre_input(PPTP_INFO *pInfo)
{
	struct pptp_gre_hdr *header;
	unsigned char *buf = pptp_gre_rx_buf;
	int iphdr_len;
	unsigned int header_len, payload_len;
	u_int32_t ack_num;
	u_int32_t seq_num;
	int nbytes;
	struct mbuf *m;
	
	if ((nbytes = read(pInfo->gre_sock, buf, sizeof(pptp_gre_rx_buf))) < 0)
	{
		if (errno == ECONNREFUSED)
			return 0;
		else
		{
			diag_printf("pptp_gre_input: read error (%s)\n", strerror(errno));
			return nbytes;
		}
	}
	
	/* strip off IP header if present */
	iphdr_len = 0;
	if ((buf[0] & 0xf0) == 0x40)
		iphdr_len = (buf[0] & 0x0f) << 2;
	
	/* validation */
	header = (struct pptp_gre_hdr *)(buf + iphdr_len);
	if (((header->version & 0x7f) != PPTP_GRE_VER) ||
		(ntohs(header->protocol) != PPTP_GRE_PROTO) ||
		(header->flags & PPTP_GRE_FLAGS_C) ||
		(header->flags & PPTP_GRE_FLAGS_R) ||
		/*(header->flags & PPTP_GRE_FLAGS_C) ||*/
		(!(header->flags & PPTP_GRE_FLAGS_K)) ||
		(header->flags & 0x0f))
	{
		diag_printf("pptp_gre_input: invalid header flags\n");
		return 0;
	}
	
	if (header->version & PPTP_GRE_VER_ACK)
	{
		if (header->flags & PPTP_GRE_FLAGS_S)
			ack_num = header->ack_num;
		else
			ack_num = header->seq_num;
		
		if ((ack_num > pInfo->ack_recv) ||
			(((ack_num >> 31) == 0) && ((pInfo->ack_recv >> 31) == 1)))
		{
			pInfo->ack_recv = ack_num;
		}
	}
	
	if (header->flags & PPTP_GRE_FLAGS_S)
	{
		header_len = sizeof(*header);
		payload_len = ntohs(header->payload_len);
		seq_num = ntohl(header->seq_num);
		
		if (!(header->version & PPTP_GRE_VER_ACK))
			header_len -= sizeof(header->ack_num);
		
		if (nbytes - header_len < payload_len)
		{
			diag_printf("pptp_gre_input: packet too short\n");
			return 0;
		}
		
		if ((seq_num > pInfo->seq_recv) ||
			(seq_num == 0 && pInfo->seq_recv == 0) ||
			(((seq_num >> 31) == 0) && ((pInfo->seq_recv >> 31) == 1)))
		{
			pInfo->seq_recv = seq_num;
			
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL)
				return 0;
			
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0)
			{
				m_freem(m);
				return 0;
			}
			m->m_data += (16 + iphdr_len + header_len);
			m->m_len = payload_len;
			bcopy(buf+iphdr_len+header_len, m->m_data, payload_len);
			m->m_flags |= M_PKTHDR;
			m->m_pkthdr.len = payload_len;
			m->m_pkthdr.rcvif = &pInfo->if_ppp.ifnet;
			
			// indicate to ppp module
			ppppktin(&pInfo->if_ppp, m);
			
			return payload_len;
		}
		else
		{
			diag_printf("pptp_gre_input: out-of-order packet\n");
			return 0;
		}
	}
	
	return 0;
}

#endif //end PPTP_BOOST_VERSION

int pptp_gre_output(PPTP_INFO *pInfo, struct mbuf *m)
{
	if (pptp_tx_process_stop)
	{
		m_freem(m);
		return 0;
	}
	
	pptp_tx_queue[pptp_txq_tail].info = pInfo;
	pptp_tx_queue[pptp_txq_tail].m = m;
	if (++pptp_txq_tail >= PPTP_TXQ_SIZE)
		pptp_txq_tail = 0;
	cyg_flag_setbits(&pptp_event_flags, PPTP_EVENT_TX);
	return 0;
}

int pptp_gre_open(PPTP_INFO *pInfo)
{
	struct pptp_param *param = &pInfo->param;
	struct sockaddr_in addr;
	int buf_size;
	
	pInfo->gre_sock = socket(AF_INET, SOCK_RAW, PPTP_PROTO);
	if (pInfo->gre_sock < 0)
	{
		diag_printf("PPTP_GRE: socket failed (%s)\n", strerror(errno));
		return -1;
	}
	
	addr.sin_family = AF_INET;
//	addr.sin_addr.s_addr = htonl(param->server_ip);
	addr.sin_addr.s_addr = param->server_ip;

	addr.sin_port = 0;
	
	buf_size = 8192 * 15;
	setsockopt(pInfo->gre_sock, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
	
	if (connect(pInfo->gre_sock, (struct sockaddr *)&addr, sizeof(addr)) != 0)
	{
		diag_printf("PPTP_GRE: connect failed (%s)\n", strerror(errno));
		close(pInfo->gre_sock);
		pInfo->gre_sock = -1;
		return -1;
	}

	pInfo->ack_sent = 0;
	pInfo->ack_recv = 0;
	pInfo->seq_sent = 0;
	pInfo->seq_recv = 0;
	
	return 0;
}


