#ifndef _WBFS_H_
#define _WBFS_H_

/* Prototypes */
s32 WBFS_Init(void);
s32 WBFS_Open(void);
s32 WBFS_Open2(int index); // index 0-3
s32 WBFS_Close();
s32 WBFS_Format(u32, u32);
s32 WBFS_GetCount(u32 *);
s32 WBFS_GetHeaders(void *, u32, u32);
s32 WBFS_CheckGame(u8 *);
s32 WBFS_AddGame(int mode);
s32 WBFS_RemoveGame(u8 *);
s32 WBFS_GameSize(u8 *, f32 *);
s32 WBFS_DiskSpace(f32 *, f32 *);

s32 WBFS_GetProfileDatas(u8 *discid, u8 *buff);
s32 WBFS_SetProfileDatas(u8 *discid, u8 *buff);
s32 WBFS_LoadCfg(void *data, s32 size);
s32 WBFS_SaveCfg(void *data, s32 size);

#endif
