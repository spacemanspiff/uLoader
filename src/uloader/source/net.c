/*  http -- http convenience functions

    Copyright (C) 2008 bushing

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <malloc.h>
#include <string.h>
#include <network.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/errno.h>

#include "net.h"

#define DEFAULT_WINDOW_SIZE 65536

static int netInit = 0;

char net_error[256];

extern void net_deinit(void);

void network_deinit(void)
{
	net_deinit();
	netInit=0;
}

int network_init()
{
	int retval;
	if (!netInit)
		{

		while (1) 
			{
			retval = net_init ();
			if (retval < 0) 
				{
				if (retval != -EAGAIN) 
					{
					sprintf (net_error, "net_init failed: %d", retval);
					return -1;
					}
				}
			if (!retval) break;
			usleep(100000);
			}
		sleep(1);
		netInit = 1;
		}
	return netInit;
}

s32 tcp_connect (char *host, const u16 port) {
	struct hostent *hp;
	struct sockaddr_in sa;
	struct in_addr srvaddr;
	s32 s, res;
	s64 t;

	hp = net_gethostbyname (host);
	if (!hp || !(hp->h_addrtype == AF_INET)) {
		//sprintf (net_error, "net_gethostbyname failed: %d\n", errno);
		//return errno;
	}
	

	s = tcp_socket ();
	if (s < 0)
		return s;

	if (inet_aton(host, &srvaddr))	{
		// Host is numeric IP
		memset (&sa, 0, sizeof (struct sockaddr_in));
		sa.sin_family= PF_INET;
		sa.sin_len = sizeof (struct sockaddr_in);
		sa.sin_port= htons (port);
		memcpy ((char *) &sa.sin_addr, &srvaddr.s_addr, sizeof(struct in_addr));
	} else {
		memset (&sa, 0, sizeof (struct sockaddr_in));
		sa.sin_family= AF_INET;
		sa.sin_len = sizeof (struct sockaddr_in);
		sa.sin_port= htons (port);
		memcpy ((char *) &sa.sin_addr, hp->h_addr_list[0], hp->h_length);
	}

	t = gettime ();
	while (true) {
		if (ticks_to_millisecs (diff_ticks (t, gettime ())) >
				TCP_CONNECT_TIMEOUT) {
			sprintf (net_error, "tcp_connect timeout\n");
			net_close (s);

			return -ETIMEDOUT;
		}

		res = net_connect (s, (struct sockaddr *) &sa,
							sizeof (struct sockaddr_in));

		if (res < 0) {
			if (res == -EISCONN)
				break;

			if (res == -EINPROGRESS || res == -EALREADY) {
				usleep (20 * 1000);

				continue;
			}

			sprintf (net_error, "net_connect failed: %d\n", res);
			net_close (s);

			return res;
		}

		break;
	}

	return s;
}

bool tcp_write (const s32 s, const char *buffer, const u32 length) {
	const u8 *p;
	u32 step, left, block, sent;
	s64 t;
	s32 res;

	step = 0;
	p = (u8 *) buffer;
	left = length;
	sent = 0;

	t = gettime ();
	while (left) {
		if (ticks_to_millisecs (diff_ticks (t, gettime ())) >
				TCP_BLOCK_SEND_TIMEOUT) {

			sprintf (net_error, "tcp_write timeout\n");
			break;
		}

		block = left;
		if (block > 2048)
			block = 2048;

		res = net_write (s, p, block);

		if ((res == 0) || (res == -56)) {
			usleep (20 * 1000);
			continue;
		}

		if (res < 0) {
			sprintf (net_error, "net_write failed: %d\n", res);
			break;
		}

		sent += res;
		left -= res;
		p += res;

		if ((sent / TCP_BLOCK_SIZE) > step) {
			t = gettime ();
			step++;
		}
	}

	return left == 0;
}

s32 tcp_socket (void) {
	s32 s, res;

	s = net_socket (PF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		sprintf (net_error, "net_socket failed: %d\n", s);
		return s;
	}

	res = net_fcntl (s, F_GETFL, 0);
	if (res < 0) {
		sprintf (net_error, "F_GETFL failed: %d\n", res);
		net_close (s);
		return res;
	}

	res = net_fcntl (s, F_SETFL, res | 4);
	if (res < 0) {
		sprintf (net_error, "F_SETFL failed: %d\n", res);
		net_close (s);
		return res;
	}

	int window_size = DEFAULT_WINDOW_SIZE;
	net_setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &window_size, sizeof(window_size));
	net_setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *) &window_size, sizeof(window_size));
	return s;
}

char * tcp_readln (const s32 s, const u16 max_length, const u64 start_time, const u32 timeout)
{
	char *buf;
	u16 c;
	s32 res;
	char *ret;

	buf = (char *) malloc(max_length);

	c = 0;
	ret = NULL;
	while (true)
	{
		//if (ticks_to_millisecs(diff_ticks(start_time, gettime())) > timeout)
		//	break;

		res = net_read(s, &buf[c], 1);

		if ((res == 0) || (res == -EAGAIN))
		{
			usleep(20 * 1000);

			continue;
		}

		if (res < 0)
		{

			break;
		}

		if ((c > 0) && (buf[c - 1] == '\r') && (buf[c] == '\n'))
		{
			if (c == 1)
			{
				ret = strdup("");

				break;
			}

			ret = strndup(buf, c - 1);

			break;
		}

		c++;

		if (c == max_length)
			break;
	}

	free(buf);
	return ret;
}

bool tcp_read (const s32 s, u8 **buffer, const u32 length)
{
	u8 *p;
	u32 step, left, block, received;
	s64 t;
	s32 res;

	step = 0;
	p = *buffer;
	left = length;
	received = 0;

	t = gettime();
	while (left)
	{
//		if (ticks_to_millisecs(diff_ticks(t, gettime()))
//				> TCP_BLOCK_RECV_TIMEOUT)
//		{
//			break;
//		}

		block = left;
		if (block > 2048)
			block = 2048;

		res = net_read(s, p, block);

		if ((res == 0) || (res == -EAGAIN))
		{
			usleep(20 * 1000);

			continue;
		}

		if (res < 0)
		{
			break;
		}

		received += res;
		left -= res;
		p += res;

//		if ((received / TCP_BLOCK_SIZE) > step)
//		{
//			t = gettime();
//			step++;
//		}
	}

	return left == 0;
}
