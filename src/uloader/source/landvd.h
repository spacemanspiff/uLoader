#ifndef __LANDVD_H__
#define __LANDVD_H__

#include <gctypes.h>

s32 LAN_UnencryptedRead(void *buf, u32 len, u64 offset);
s32 LAN_Wait(void);
s32 LAN_IsWii(void);
s32 LAN_ReadHeader(void *outbuf);
s32 LAN_ReadDiskId(void *id);
s32 LAN_Open(void);

#endif
