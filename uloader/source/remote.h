#ifndef _REMOTE_H_
#define _REMOTE_H_

#include <gccore.h>
#include "wbfs.h"
#include "usbstorage2.h"
#include "wdvd.h"
#include "disc.h"

#define REMOTE_BUSY 0xcacafea5

int remote_init(void);

void remote_end();

int remote_ret();

int remote_USBStorage2_Init(void);

int remote_call(void *func);

void remote_call_abort();





#endif
