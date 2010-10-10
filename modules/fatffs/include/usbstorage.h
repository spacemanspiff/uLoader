#ifndef _USBSTORAGE_H_
#define _USBSTORAGE_H_

#include "disc_io.h"
#include "types.h"

/* Prototypes */
bool usbstorage_Init(void);
bool usbstorage_Shutdown(void);
bool usbstorage_IsInserted(void);
bool usbstorage_ReadSectors(u32 sector, u32 numSectors, void *buffer);
bool usbstorage_WriteSectors(u32 sector, u32 numSectors, void *buffer);
bool usbstorage_ClearStatus(void);

/* Externs */
extern const DISC_INTERFACE __io_usbstorage;

#endif
