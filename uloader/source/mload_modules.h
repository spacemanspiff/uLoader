#ifndef _MLOAD_MODULES_H_
#define _MLOAD_MODULES_H_

#include "global.h"

#include "fatffs_module_elf.h"
#include "odip_plugin_bin.h"
#include "ehcmodule_elf.h"
#include "mload.h"

extern void *external_ehcmodule;
extern int size_external_ehcmodule;
extern int use_port1;

int load_ehc_module();
int load_fatffs_module(u8 *discid);

void enable_ES_ioctlv_vector(void);
void Set_DIP_BCA_Datas(u8 *bca_data);

void test_and_patch_for_port1();

int enable_ffs(int mode);
void disable_ffs_patch(void);

void free_usb_ports();

#endif


