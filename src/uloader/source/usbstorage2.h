#ifndef _USBSTORAGE2_H_
#define _USBSTORAGE2_H_ 

/* Prototypes */
s32  USBStorage2_GetCapacity(u32 *);
s32  USBStorage2_Init(void);
void USBStorage2_Deinit(void);
s32  USBStorage2_ReadSectors(u32, u32, void *);
s32  USBStorage2_WriteSectors(u32, u32, void *);

#endif
