#include "landvd.h"
#include "debug.h"

#include "net.h"
#include <stdio.h>
#include <string.h>

#include "disc.h"

static s32 lan_socket = 0;

#define WII_MAGIC	   0x5D1C9EA3
#define READINFO_SIZE_DATA 32


static u8  *diskid = (u8  *)0x80000000;
static u32 *buffer = (u32 *)0x93000000;

#define SERVER_IP "192.168.0.2"
#define SERVER_PORT 5000

#define CMD_BUFFER_SIZE 256

static char cmdBuffer[CMD_BUFFER_SIZE];

s32 LAN_Init()
{
	network_init();

	if (lan_socket)
		return lan_socket;

	lan_socket = tcp_connect (SERVER_IP, SERVER_PORT);
	if (lan_socket < 0) {
		lan_socket = 0;
	}
	return lan_socket;
}

s32 LAN_UnencryptedRead(void *buf, u32 len, u64 offset)
{
	sprintf(log_buffer, "LAN_UnencryptedRead called len=%d offset=%lld\n", len, offset);
	debug_log(log_buffer);

	u8 *buffer = (u8 *) buf;
	s32 ret;

	if (!lan_socket)
		LAN_Init();

	if (!lan_socket)
		return -1;

	snprintf(cmdBuffer, CMD_BUFFER_SIZE, "READ%d,%lld\n", len, offset);
	if (tcp_write(lan_socket, cmdBuffer, strlen(cmdBuffer))) {
		ret = tcp_read(lan_socket, &buffer, len)?0:-1;
	} else
		ret = -1;

	sprintf(log_buffer, "LAN_UnencryptedRead exited with status %d\n", ret);
	debug_log(log_buffer);
	return ret;
}

s32 LAN_Open(void)
{
	if (!lan_socket)
		LAN_Init();

	if (!lan_socket)
		return -1;

	/* Read disc ID */
	return LAN_ReadDiskId(diskid);
}

s32 LAN_ReadDiskId(void *id)
{
	return LAN_UnencryptedRead(id,READINFO_SIZE_DATA,0);
}

s32 LAN_ReadHeader(void *outbuf)
{
	/* Read disc header */
	return LAN_UnencryptedRead(outbuf, sizeof(struct discHdr), 0);
}

s32 LAN_IsWii(void)
{
	struct discHdr *header = (struct discHdr *)buffer;

	s32 ret;

	/* Read disc header */
	ret = LAN_ReadHeader(header);
	if (ret < 0)
		return ret;

	/* Check magic word */
	if (header->magic != WII_MAGIC)
		return -1;

	return 0;
}

s32 LAN_Wait(void)
{
	if (!lan_socket)
		LAN_Init();

	return 0;
}

