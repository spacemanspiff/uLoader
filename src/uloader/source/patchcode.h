#ifndef __PATCHCODE_H__
#define __PATCHCODE_H__

// Globals
u32 hooktype;
int patched;
u8 configbytes[2];
u32 regionfree;

//---------------------------------------------------------------------------------
extern const u32 viwiihooks[4];
extern const u32 kpadhooks[4];
extern const u32 joypadhooks[4];
extern const u32 gxdrawhooks[4];
extern const u32 gxflushhooks[4];
extern const u32 ossleepthreadhooks[4];
extern const u32 axnextframehooks[4];
extern const u32 wpadbuttonsdownhooks[4];
extern const u32 wpadbuttonsdown2hooks[4];
//---------------------------------------------------------------------------------
bool dogamehooks(void *addr, u32 len);
void patchmenu(void *addr, u32 len, int patchnum);
void langpatcher(void *addr, u32 len);
void vidolpatcher(void *addr, u32 len);
void dochannelhooks(void *addr, u32 len);
void patchdebug(void *addr, u32 len);
void determine_libogc_hook(void *addr, u32 len);
//---------------------------------------------------------------------------------

#endif
