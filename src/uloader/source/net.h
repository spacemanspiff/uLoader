#ifndef __NET_H__
#define __NET_H__

#include <gctypes.h>

#define TCP_CONNECT_TIMEOUT 5000
#define TCP_BLOCK_SIZE (16 * 1024)
#define TCP_BLOCK_RECV_TIMEOUT 4000
#define TCP_BLOCK_SEND_TIMEOUT 4000

#define HTTP_TIMEOUT 300000

extern char net_error[256];

void network_deinit(void);
int network_init(void);

s32 tcp_connect (char *host, const u16 port);
char * tcp_readln (const s32 s, const u16 max_length, const u64 start_time, const u32 timeout);
bool tcp_read (const s32 s, u8 **buffer, const u32 length);
bool tcp_write (const s32 s, const char *buffer, const u32 length);
s32 tcp_socket (void);
#endif
