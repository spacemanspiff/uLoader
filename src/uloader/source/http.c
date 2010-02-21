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
#include <stdio.h>
#include <unistd.h>

#include <ogcsys.h>
#include <network.h>
#include <ogc/lwp_watchdog.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>



#include "http.h"

char net_error[256];

char *http_host=NULL;
u16 http_port;
char *http_path=NULL;


http_res result;
u32 http_status;
u32 content_length;
u8 *http_data;

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

	return s;
}


s32 tcp_connect (char *host, const u16 port) {
	struct hostent *hp;
	struct sockaddr_in sa;
	s32 s, res;
	s64 t;

	hp = net_gethostbyname (host);
	if (!hp || !(hp->h_addrtype == AF_INET)) {
		sprintf (net_error, "net_gethostbyname failed: %d\n", errno);
		return errno;
	}
	

	s = tcp_socket ();
	if (s < 0)
		return s;

	memset (&sa, 0, sizeof (struct sockaddr_in));
	sa.sin_family= AF_INET;
	sa.sin_len = sizeof (struct sockaddr_in);
	sa.sin_port= htons (port);
	memcpy ((char *) &sa.sin_addr, hp->h_addr_list[0], hp->h_length);



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


bool tcp_write (const s32 s, const u8 *buffer, const u32 length) {
	const u8 *p;
	u32 step, left, block, sent;
	s64 t;
	s32 res;

	step = 0;
	p = buffer;
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

bool http_split_url (char **host, char **path, const char *url) {
	const char *p;
	char *c;

	if (strncasecmp (url, "http://", 7))
		return false;

	p = url + 7;
	c = strchr (p, '/');

	if (c[0] == 0)
		return false;

	*host = strndup (p, c - p);
	*path = strdup (c);

	return true;
}


struct header_block
{
	u32 size;
    u8 *data;
};

#define HTTP_BUFFER_SIZE 1024*5

#define HTTP_MAX_BUFFER 512*1024

struct header_block * read_block(s32 s)
{
      
        static struct header_block buffer;

		u32 offset = 0;
        buffer.data = malloc(HTTP_MAX_BUFFER+HTTP_BUFFER_SIZE);
        buffer.size = 0;

		int retry=10;

        if(buffer.data == NULL) {
                return NULL;
        }
	
        
		usleep(200*1000);
        while(1)
        {
		if(offset>=HTTP_MAX_BUFFER)
			{
			free(buffer.data);buffer.data=NULL;
                        
            return NULL;
			}
		retry--;
			
              
        s32 bytes_read = net_read(s, buffer.data + offset, HTTP_BUFFER_SIZE);
                
             
		if(bytes_read< 0)
			{
			if(retry>0 && bytes_read == -EAGAIN) {usleep(100*1000);continue;}
			free(buffer.data);buffer.data=NULL;	
			return NULL;
			
			}
		else retry=10;
		
	   
		if(bytes_read == 0)
			{
			  break;
			}
		
		offset += bytes_read;
                          
        }

        buffer.size = offset;
                
        return &buffer;
}

bool http_request (const char *url, const u32 max_size) {
	http_status = 404;
	content_length = 0;
	http_data = NULL;
	unsigned char *content_start = NULL;

	if (!http_split_url(&http_host, &http_path, url)) return false;

	http_port = 80;

	int s = tcp_connect (http_host, http_port);
//	debug_printf("tcp_connect(%s, %hu) = %d\n", http_host, http_port, s);
	if (s < 0) {
		result = HTTPR_ERR_CONNECT;
		goto error;
	}

	char *request = (char *) malloc (1024);
	sprintf (request, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: WiiEarthh 1.0\r\n\r\n", http_path, http_host);

	


//	debug_printf("request = %s\n", request);

	bool b = tcp_write (s, (u8 *) request, strlen (request));
//	debug_printf("tcp_write returned %d\n", b);
	free (request);

	if(!b) goto error;

    struct header_block *response = (void *) read_block(s);
	net_close (s);
	
		if (!response) {
			http_status = 404;
			result = HTTPR_ERR_REQUEST;
			goto error;
		}

		
	result = HTTPR_OK;

    content_length = 0;
        int i;
        for(i = 3; i < response->size; i++)
        {
        if(response->data[i] == '\n' && response->data[i-1] == '\r' &&
           response->data[i-2] == '\n' && response->data[i-3] == '\r')
                {
                content_start  = response->data + i + 1;
                content_length = response->size - i - 1;
                break;
                }
        }
         
		
		if(content_start == NULL) goto error;
		
        http_data = (u8 *) malloc (content_length);
		  
        if(http_data == NULL) goto error;

		memcpy(http_data, content_start, content_length);

		free (response->data); response->data=NULL;
		
    if(http_host) free (http_host); http_host=NULL;
	if(http_path) free (http_path); http_path=NULL;

	result = HTTPR_OK;

	return true;

error:

	if(http_host) free (http_host); http_host=NULL;
	if(http_path) free (http_path); http_path=NULL;

return false;
}

bool http_get_result (u32 *_http_status, u8 **content, u32 *length) {
	if (http_status) *_http_status = http_status;

	if (result == HTTPR_OK) {
		*content = http_data;
		*length = content_length;

	} else {
		*content = NULL;
		*length = 0;
	}

	

	return true;
}


static int netInit = 0;

extern void net_deinit(void);

void http_deinit(void)
{
	net_deinit();
	netInit=0;
}

int download_file(char *url, u8 **outbuf, u32 *outlen)
{

int retval;
u32 http_status;
int retry=2;


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
    while(1)
	{
	retry--;
	retval = http_request(url, (u32) (1 << 31));
	if (!retval) 
		{
		sprintf (net_error, "Error making http request");
		if(retry<0) return -2; else continue;
		}
	break;
	}
	
	retval = http_get_result(&http_status, outbuf, outlen);
	if (!retval) 
		{
		sprintf (net_error, "Error getting http file");
		return -3;
		}
return 0;
}
