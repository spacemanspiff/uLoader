#ifndef __TEST_H__
#define __TEST_H__

#define MEM_PRINT

#include <gccore.h>
u32 gettick(void); // From timesupp.h in libogc, don't know why it is not exported

#define ticks_to_msecs(ticks) ((u32)((ticks) / (TB_TIMER_CLOCK)))

void usb_test();
#ifdef MEM_PRINT
void save_log();
#endif

#endif
