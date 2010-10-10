#ifndef _SDIO_H_
#define _SDIO_H_

#include "disc_io.h"
#include "types.h"

/* Prototypes */
bool sdio_Startup(void);
bool sdio_Shutdown(void);
bool sdio_ReadSectors(sec_t sector, sec_t numSectors, void *buffer);
bool sdio_WriteSectors(sec_t sector, sec_t numSectors, const void *buffer);
bool sdio_IsInserted(void);

/* Externs */
extern const DISC_INTERFACE __io_wiisd;

#endif
