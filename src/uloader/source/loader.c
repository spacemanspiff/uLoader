// loader.c

#include "global.h"
#include "gfx.h"
#include "mload_modules.h"

#include <ogc/lwp_watchdog.h>
#include "patchcode.h"
#include "kenobiwii.h"

#include "diario.h"
#include "asndlib.h"

extern int cios;
extern u32 nand_mode;

extern u32 load_dol();

#define CERTS_SIZE	0xA00

static const char certs_fs[] ATTRIBUTE_ALIGN(32) = "/sys/cert.sys";

s32 GetCerts(signed_blob** Certs, u32* Length)
{
	static unsigned char		Cert[CERTS_SIZE] ATTRIBUTE_ALIGN(32);
	memset(Cert, 0, CERTS_SIZE);
	s32				fd, ret;

	fd = IOS_Open(certs_fs, ISFS_OPEN_READ);
	if (fd < 0) return fd;

	ret = IOS_Read(fd, Cert, CERTS_SIZE);
	if (ret < 0)
	{
		if (fd >0) IOS_Close(fd);
		return ret;
	}

	*Certs = (signed_blob*)(Cert);
	*Length = CERTS_SIZE;

	if (fd > 0) IOS_Close(fd);

	return 0;
}



//---------------------------------------------------------------------------------
/* from YAL loader */
//---------------------------------------------------------------------------------

GXRModeObj*		vmode;					// System Video Mode
unsigned int	_Video_Mode;				// System Video Mode (NTSC, PAL or MPAL)	
u32 *xfb; 

extern int forcevideo;

void Determine_VideoMode(char Region)
{
	u32 progressive;
	// Get vmode and Video_Mode for system settings first
	u32 tvmode = CONF_GetVideo();
	// Attention: This returns &TVNtsc480Prog for all progressive video modes
        vmode = VIDEO_GetPreferredMode(0);

	switch(forcevideo)
	{
	case 0:
				switch (tvmode) 
				{
					case CONF_VIDEO_PAL:
							if (CONF_GetEuRGB60() > 0) 
									_Video_Mode = PAL60;
							else 
									_Video_Mode = PAL;
							break;
					case CONF_VIDEO_MPAL:
							_Video_Mode = MPAL;
							break;

					case CONF_VIDEO_NTSC:
					default:
							_Video_Mode = NTSC;
				}
				
			#if 0

			// Overwrite vmode and Video_Mode when disc region video mode is selected and Wii region doesn't match disc region
				switch (Region) 
				{
				case PAL_Default:
				case PAL_France:
				case PAL_Germany:
				case Euro_X:
				case Euro_Y:
						if (CONF_GetVideo() != CONF_VIDEO_PAL)
						{
								_Video_Mode = PAL60;

								if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
										vmode = &TVNtsc480Prog; // This seems to be correct!
								else
										vmode = &TVEurgb60Hz480IntDf;
						}
						break;

				case NTSC_USA:
				case NTSC_Japan:
						if (CONF_GetVideo() != CONF_VIDEO_NTSC)
						{
								_Video_Mode = NTSC;
								if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
										vmode = &TVNtsc480Prog;
								else
										vmode = &TVNtsc480IntDf;
						}
				default:
						break;
				}
				#endif
				break;

		 case 1:
				/* GAME LAUNCHED WITH 1 - FISHEARS*/
                _Video_Mode = PAL60;
                progressive = (CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable();
                vmode     = (progressive) ? &TVNtsc480Prog : &TVEurgb60Hz480IntDf;
                break;
         case 2:
                /* GAME LAUNCHED WITH 2 - FISHEARS*/
                _Video_Mode = NTSC;
				
                progressive = (CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable();
                vmode     = (progressive) ? &TVNtsc480Prog : &TVNtsc480IntDf;
                break;

		case 3:
				// PAL 50
				_Video_Mode = PAL;
				progressive = (CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable();
				vmode     = (progressive) ? &TVNtsc480Prog : &TVPal528IntDf;
				break;
		
		}		

/* Set video mode register */
	*Video_Mode = _Video_Mode;

	
}

void Set_VideoMode(void)
{
    // TODO: Some exception handling is needed here
 
    // The video mode (PAL/NTSC/MPAL) is determined by the value of 0x800000cc
    // The combination Video_Mode = NTSC and vmode = [PAL]576i, results in an error
    
    *Video_Mode = _Video_Mode;

    VIDEO_Configure(vmode);
	//VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(vmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
}


// from coverflow loader

bool compare_videomodes(GXRModeObj* mode1, GXRModeObj* mode2)
{
	
	if (mode1->viTVMode != mode2->viTVMode || mode1->fbWidth != mode2->fbWidth ||	mode1->efbHeight != mode2->efbHeight || mode1->xfbHeight != mode2->xfbHeight ||
	mode1->viXOrigin != mode2->viXOrigin || mode1->viYOrigin != mode2->viYOrigin || mode1->viWidth != mode2->viWidth || mode1->viHeight != mode2->viHeight ||
	mode1->xfbMode != mode2->xfbMode || mode1->field_rendering != mode2->field_rendering || mode1->aa != mode2->aa || mode1->sample_pattern[0][0] != mode2->sample_pattern[0][0] ||
	mode1->sample_pattern[1][0] != mode2->sample_pattern[1][0] ||	mode1->sample_pattern[2][0] != mode2->sample_pattern[2][0] ||
	mode1->sample_pattern[3][0] != mode2->sample_pattern[3][0] ||	mode1->sample_pattern[4][0] != mode2->sample_pattern[4][0] ||
	mode1->sample_pattern[5][0] != mode2->sample_pattern[5][0] ||	mode1->sample_pattern[6][0] != mode2->sample_pattern[6][0] ||
	mode1->sample_pattern[7][0] != mode2->sample_pattern[7][0] ||	mode1->sample_pattern[8][0] != mode2->sample_pattern[8][0] ||
	mode1->sample_pattern[9][0] != mode2->sample_pattern[9][0] ||	mode1->sample_pattern[10][0] != mode2->sample_pattern[10][0] ||
	mode1->sample_pattern[11][0] != mode2->sample_pattern[11][0] || mode1->sample_pattern[0][1] != mode2->sample_pattern[0][1] ||
	mode1->sample_pattern[1][1] != mode2->sample_pattern[1][1] ||	mode1->sample_pattern[2][1] != mode2->sample_pattern[2][1] ||
	mode1->sample_pattern[3][1] != mode2->sample_pattern[3][1] ||	mode1->sample_pattern[4][1] != mode2->sample_pattern[4][1] ||
	mode1->sample_pattern[5][1] != mode2->sample_pattern[5][1] ||	mode1->sample_pattern[6][1] != mode2->sample_pattern[6][1] ||
	mode1->sample_pattern[7][1] != mode2->sample_pattern[7][1] ||	mode1->sample_pattern[8][1] != mode2->sample_pattern[8][1] ||
	mode1->sample_pattern[9][1] != mode2->sample_pattern[9][1] ||	mode1->sample_pattern[10][1] != mode2->sample_pattern[10][1] ||
	mode1->sample_pattern[11][1] != mode2->sample_pattern[11][1] || mode1->vfilter[0] != mode2->vfilter[0] ||
	mode1->vfilter[1] != mode2->vfilter[1] ||	mode1->vfilter[2] != mode2->vfilter[2] || mode1->vfilter[3] != mode2->vfilter[3] ||	mode1->vfilter[4] != mode2->vfilter[4] ||
	mode1->vfilter[5] != mode2->vfilter[5] ||	mode1->vfilter[6] != mode2->vfilter[6] )
	{
		return false;
	} else
	{
		
		return true;
	}
	
}



void patch_videomode(GXRModeObj* mode1, GXRModeObj* mode2)
{
	mode1->viTVMode = mode2->viTVMode;
	mode1->fbWidth = mode2->fbWidth;
	mode1->efbHeight = mode2->efbHeight;
	mode1->xfbHeight = mode2->xfbHeight;
	mode1->viXOrigin = mode2->viXOrigin;
	mode1->viYOrigin = mode2->viYOrigin;
	mode1->viWidth = mode2->viWidth;
	mode1->viHeight = mode2->viHeight;
	mode1->xfbMode = mode2->xfbMode;
	mode1->field_rendering = mode2->field_rendering;
	mode1->aa = mode2->aa;
	mode1->sample_pattern[0][0] = mode2->sample_pattern[0][0];
	mode1->sample_pattern[1][0] = mode2->sample_pattern[1][0];
	mode1->sample_pattern[2][0] = mode2->sample_pattern[2][0];
	mode1->sample_pattern[3][0] = mode2->sample_pattern[3][0];
	mode1->sample_pattern[4][0] = mode2->sample_pattern[4][0];
	mode1->sample_pattern[5][0] = mode2->sample_pattern[5][0];
	mode1->sample_pattern[6][0] = mode2->sample_pattern[6][0];
	mode1->sample_pattern[7][0] = mode2->sample_pattern[7][0];
	mode1->sample_pattern[8][0] = mode2->sample_pattern[8][0];
	mode1->sample_pattern[9][0] = mode2->sample_pattern[9][0];
	mode1->sample_pattern[10][0] = mode2->sample_pattern[10][0];
	mode1->sample_pattern[11][0] = mode2->sample_pattern[11][0];
	mode1->sample_pattern[0][1] = mode2->sample_pattern[0][1];
	mode1->sample_pattern[1][1] = mode2->sample_pattern[1][1];
	mode1->sample_pattern[2][1] = mode2->sample_pattern[2][1];
	mode1->sample_pattern[3][1] = mode2->sample_pattern[3][1];
	mode1->sample_pattern[4][1] = mode2->sample_pattern[4][1];
	mode1->sample_pattern[5][1] = mode2->sample_pattern[5][1];
	mode1->sample_pattern[6][1] = mode2->sample_pattern[6][1];
	mode1->sample_pattern[7][1] = mode2->sample_pattern[7][1];
	mode1->sample_pattern[8][1] = mode2->sample_pattern[8][1];
	mode1->sample_pattern[9][1] = mode2->sample_pattern[9][1];
	mode1->sample_pattern[10][1] = mode2->sample_pattern[10][1];
	mode1->sample_pattern[11][1] = mode2->sample_pattern[11][1];
	mode1->vfilter[0] = mode2->vfilter[0];
	mode1->vfilter[1] = mode2->vfilter[1];
	mode1->vfilter[2] = mode2->vfilter[2];
	mode1->vfilter[3] = mode2->vfilter[3];
	mode1->vfilter[4] = mode2->vfilter[4];
	mode1->vfilter[5] = mode2->vfilter[5];
	mode1->vfilter[6] = mode2->vfilter[6];
}

GXRModeObj* vmodes[] = {
	&TVNtsc240Ds,
	&TVNtsc240DsAa,
	&TVNtsc240Int,
	&TVNtsc240IntAa,
	&TVNtsc480IntDf,
	&TVNtsc480IntAa,
	&TVNtsc480Prog,
	&TVMpal480IntDf,
	&TVPal264Ds,
	&TVPal264DsAa,
	&TVPal264Int,
	&TVPal264IntAa,
	&TVPal524IntAa,
	&TVPal528Int,
	&TVPal528IntDf,
	&TVPal574IntDfScale,
	&TVEurgb60Hz240Ds,
	&TVEurgb60Hz240DsAa,
	&TVEurgb60Hz240Int,
	&TVEurgb60Hz240IntAa,
	&TVEurgb60Hz480Int,
	&TVEurgb60Hz480IntDf,
	&TVEurgb60Hz480IntAa,
	&TVEurgb60Hz480Prog,
	&TVEurgb60Hz480ProgSoft,
	&TVEurgb60Hz480ProgAa
};

GXRModeObj* PAL2NTSC[]={
	&TVMpal480IntDf,		&TVNtsc480IntDf,
	&TVPal264Ds,			&TVNtsc240Ds,
	&TVPal264DsAa,			&TVNtsc240DsAa,
	&TVPal264Int,			&TVNtsc240Int,
	&TVPal264IntAa,			&TVNtsc240IntAa,
	&TVPal524IntAa,			&TVNtsc480IntAa,
	&TVPal528Int,			&TVNtsc480IntAa,
	&TVPal528IntDf,			&TVNtsc480IntDf,
	&TVPal574IntDfScale,	&TVNtsc480IntDf,
	&TVEurgb60Hz240Ds,		&TVNtsc240Ds,
	&TVEurgb60Hz240DsAa,	&TVNtsc240DsAa,
	&TVEurgb60Hz240Int,		&TVNtsc240Int,
	&TVEurgb60Hz240IntAa,	&TVNtsc240IntAa,
	&TVEurgb60Hz480Int,		&TVNtsc480IntAa,
	&TVEurgb60Hz480IntDf,	&TVNtsc480IntDf,
	&TVEurgb60Hz480IntAa,	&TVNtsc480IntAa,
	&TVEurgb60Hz480Prog,	&TVNtsc480Prog,
	&TVEurgb60Hz480ProgSoft,&TVNtsc480Prog,
	&TVEurgb60Hz480ProgAa,  &TVNtsc480Prog,
	0,0
};

GXRModeObj* NTSC2PAL[]={
	&TVNtsc240Ds,			&TVPal264Ds,
	&TVNtsc240DsAa,			&TVPal264DsAa,
	&TVNtsc240Int,			&TVPal264Int,
	&TVNtsc240IntAa,		&TVPal264IntAa,
	&TVNtsc480IntDf,		&TVPal528IntDf,
	&TVNtsc480IntAa,		&TVPal524IntAa,
	&TVNtsc480Prog,			&TVPal528IntDf,
	0,0
};

GXRModeObj* NTSC2PAL60[]={
	&TVNtsc240Ds,			&TVEurgb60Hz240Ds,
	&TVNtsc240DsAa,			&TVEurgb60Hz240DsAa,
	&TVNtsc240Int,			&TVEurgb60Hz240Int,
	&TVNtsc240IntAa,		&TVEurgb60Hz240IntAa,
	&TVNtsc480IntDf,		&TVEurgb60Hz480IntDf,
	&TVNtsc480IntAa,		&TVEurgb60Hz480IntAa,
	&TVNtsc480Prog,			&TVEurgb60Hz480Prog,
	0,0
};

bool Search_and_patch_Video_Modes(void *Address, u32 Size )
{
	u8 *Addr = (u8 *)Address;
	bool found = 0;
	u32 i;
	GXRModeObj** Table= NULL;

	switch(_Video_Mode)
			{
			case PAL:
			case MPAL:
				Table = NTSC2PAL;break;

			case PAL60:
				Table = NTSC2PAL60;break;

            default:
				Table = PAL2NTSC;
				break;
			}

	while(Size >= sizeof(GXRModeObj))
	{



		for(i = 0; Table[i]; i+=2)
		{


			if(compare_videomodes(Table[i], (GXRModeObj*)Addr))

			{
				found = 1;
				patch_videomode((GXRModeObj*)Addr, Table[i+1]);
				Addr += (sizeof(GXRModeObj)-4);
				Size -= (sizeof(GXRModeObj)-4);
				break;
			}
		}

		Addr += 4;
		Size -= 4;
	}


	return found;
}

#if 0
void mem_patch(void *buffer, u32 len, u8 *s, int s_size, int p_skip, u8 *p, int p_size)
{
int n;

   /* Patch data*/

for(n=0;n<(len-s_size);n+=4)
   {
   if(!memcmp(buffer+n, (void *) s, s_size))
      {
      memcpy(buffer+n+p_skip, (void *) p,  p_size);
      }
   }
}
#endif


int compare_hex_str(u8 *buff, const u8 *hex_str)
{
u8 dat=0;

while(*hex_str!=0)
	{
	
	//while(*hex_str<=' ') {hex_str++;if(hex_str==0) return 0;}

	if(*hex_str>='A' && *hex_str<='F') dat=(10+*hex_str-'A')<<4;
	else if(*hex_str>='a' && *hex_str<='f') dat=(10+*hex_str-'a')<<4;
	else if(*hex_str>='0' && *hex_str<='9') dat=(*hex_str-'0')<<4;
	else if(*hex_str=='x' || *hex_str<='X') dat= *buff & 0xf0;
	else {return 0;}

	hex_str++;
	if(hex_str==0) return 0;

	//while(*hex_str<=' ') {hex_str++;if(hex_str==0) return 0;}
	
	if(*hex_str>='A' && *hex_str<='F') dat|=(10+*hex_str-'A');
	else if(*hex_str>='a' && *hex_str<='f') dat|=(10+*hex_str-'a');
	else if(*hex_str>='0' && *hex_str<='9') dat|=(*hex_str-'0');
	else if(*hex_str=='x' || *hex_str<='X') dat|= *buff & 0xf;
	else {return 0;}	
	
	if(dat!=*buff) return 0;

	buff++; hex_str++;
	}

return 1;
}

int set_hex_str(u8 *buff, const u8 *hex_str)
{
u8 dat=0;

while(*hex_str!=0)
	{
	
	//while(*hex_str<=' ') {hex_str++;if(hex_str==0) return 0;}

	if(*hex_str>='A' && *hex_str<='F') dat=(10+*hex_str-'A')<<4;
	else if(*hex_str>='a' && *hex_str<='f') dat=(10+*hex_str-'a')<<4;
	else if(*hex_str>='0' && *hex_str<='9') dat=(*hex_str-'0')<<4;
	else if(*hex_str=='x' || *hex_str<='X') dat= *buff & 0xf0;
	else {return 0;}

	hex_str++;
	if(hex_str==0) return 0;

	//while(*hex_str<=' ') {hex_str++;if(hex_str==0) return 0;}
	
	if(*hex_str>='A' && *hex_str<='F') dat|=(10+*hex_str-'A');
	else if(*hex_str>='a' && *hex_str<='f') dat|=(10+*hex_str-'a');
	else if(*hex_str>='0' && *hex_str<='9') dat|=(*hex_str-'0');
	else if(*hex_str=='x' || *hex_str<='X') dat|= *buff & 0xf;
	else {return 0;}	
	
	*buff++=dat;
	hex_str++;
	}

return 1;
}


void patch_hex_str(u8 *buffer, u32 len, u32 step, const u8 *search, int skip_patch, const u8 *patch, int counter )
{
 u32 cnt;

for (cnt = 0; cnt < len; cnt+=step) 
	{
	u8 *ptr = buffer + cnt;

		/* Replace code if found */
		if (compare_hex_str(ptr, search))
			{
			set_hex_str(ptr+skip_patch, patch);
			cnt=(((cnt+(strlen((char *) search)>>1))/step)*step)-step;
			
			counter--;
			if(counter==0) return;
		
			}
			
	}
}


#if 0
void __Patch_DiscSeek(void *buffer, u32 len)
{
   const   u8 SearchPattern[] =  "38A000DA7CA401AE7C671B787FCAF3783929XXXX800DBXXC388000DA80ADB3XX"; 
   const   u8 PatchData[] =      "38A000717CA401AE7C671B787FCAF3783929XXXX800DB3XX3880007180ADB3XX";
   
   
   patch_hex_str((u8 *) buffer, (len - (strlen((char *) SearchPattern)>>1)),4 , SearchPattern, 0, PatchData, 0);

  
}
#endif

// Based in Waninkoko patch

void __Patch_CoverRegister(void *buffer, u32 len)
{
   const u8 oldcode[] = "5460F7FF4082000C546007FF4182000C";
   const u8 newcode[] = /*"5460F7FF4082000C546007FF"*/ "4800000C";

   patch_hex_str((u8 *) buffer, (len - (strlen((char *) oldcode)>>1)),4 , oldcode, 12, newcode, 0);

}

void __Patch_Error001(void *buffer, u32 len)
{
	const u8 oldcode[] = "4082000C386000014800024438610018";
	const u8 newcode[] = "40820004";

	patch_hex_str((u8 *) buffer, (len - (strlen((char *) oldcode)>>1)),4 , oldcode, 0, newcode, 0);
}


#if 0
void __Patch_NSMBW(void *buffer, u32 len)
{

const u8 oldcode[] = "9421FFD07C0802A690010034396100304812XXXX7C7B1B787C9C23787CBD2B78";
const u8 newcode[] = "4E800020";

	patch_hex_str((u8 *) buffer, (len - (strlen((char *) oldcode)>>1)),4 , oldcode, 0, newcode, 0);
}
#endif





void patch_dol(void *Address, int Section_Size, int mode)
{
	DCFlushRange(Address, Section_Size);
	//if(mode)
	
	__Patch_Error001((void *) Address, Section_Size);

	
	if(cios==249)
		__Patch_CoverRegister(Address, Section_Size);

	//__Patch_DiscSeek(Address, Section_Size);

	//__Patch_NSMBW((void *) Address, Section_Size);
	
	
	/*HOOKS STUFF - FISHEARS*/
	dogamehooks(Address, Section_Size);

	/*LANGUAGE PATCH - FISHEARS*/
	langpatcher(Address, Section_Size);

	if(!forcevideo || forcevideo==1)
		Search_and_patch_Video_Modes(Address, Section_Size);

	vidolpatcher(Address, Section_Size);


	/*HOOKS STUFF - FISHEARS*/

	DCFlushRange(Address, Section_Size);
}

void set_language_and_ocarina(void)
{
	switch(langsel)
		{
			case 0:
					configbytes[0] = 0xCD;
			break;

			case 1:
					configbytes[0] = 0x00;
			break;

			case 2:
					configbytes[0] = 0x01;
			break;

			case 3:
					configbytes[0] = 0x02;
			break;

			case 4:
					configbytes[0] = 0x03;
			break;

			case 5:
					configbytes[0] = 0x04;
			break;

			case 6:
					configbytes[0] = 0x05;
			break;

			case 7:
					configbytes[0] = 0x06;
			break;

			case 8:
					configbytes[0] = 0x07;
			break;

			case 9:
					configbytes[0] = 0x08;
			break;

			case 10:
					configbytes[0] = 0x09;
			break;
		}

	
		hooktype = 0;
		
		
		if((len_cheats && buff_cheats))
			{ 
			void *codelist=(void*)0x800022A8;

			// OLD METHOD
			if(hook_selected==8)
				{
			
				/*HOOKS STUFF - FISHEARS*/
				memset((void*)0x80001800,0,kenobiwii_size);
				memcpy((void*)0x80001800,kenobiwii,kenobiwii_size);
				memcpy((void*)0x80001800, (char*)0x80000000, 6);	// For WiiRD
				DCFlushRange((void*)0x80001800,kenobiwii_size);
				memcpy((void*)0x800027E8, buff_cheats, len_cheats);
				*(vu8*)0x80001807 = 0x01;
				hooktype =1;
				}
            else
				{
				// new from Neogamma
			
				#include"codehandleronly.h"

           
				memset((void*)0x80001800,0,codehandleronly_size);
				memcpy((void*)0x80001800,codehandleronly,codehandleronly_size);
				memcpy((void*)0x80001906, &codelist, 2);
				memcpy((void*)0x8000190A, ((u8*) &codelist) + 2, 2);
				memcpy((void*)0x80001800, (char*)0x80000000, 6);	// For WiiRD
				*(vu8*)0x80001807 = 0x01;
				DCFlushRange((void*)0x80001800,codehandleronly_size);
				hooktype = hook_selected;

				switch(hooktype)
				{
					case 0x01:
						memcpy((void*)0x8000119C,viwiihooks,12);
						memcpy((void*)0x80001198,viwiihooks+3,4);
						break;
					case 0x02:
						memcpy((void*)0x8000119C,kpadhooks,12);
						memcpy((void*)0x80001198,kpadhooks+3,4);
						break;
					case 0x03:
						memcpy((void*)0x8000119C,joypadhooks,12);
						memcpy((void*)0x80001198,joypadhooks+3,4);
						break;
					case 0x04:
						memcpy((void*)0x8000119C,gxdrawhooks,12);
						memcpy((void*)0x80001198,gxdrawhooks+3,4);
						break;
					case 0x05:
						memcpy((void*)0x8000119C,gxflushhooks,12);
						memcpy((void*)0x80001198,gxflushhooks+3,4);
						break;
					case 0x06:
						memcpy((void*)0x8000119C,ossleepthreadhooks,12);
						memcpy((void*)0x80001198,ossleepthreadhooks+3,4);
						break;
					case 0x07:
						memcpy((void*)0x8000119C,axnextframehooks,12);
						memcpy((void*)0x80001198,axnextframehooks+3,4);
						break;
					
				}
				DCFlushRange((void*)0x80001198,16);
			  
			
				memcpy((void*) codelist, buff_cheats, len_cheats);

				DCFlushRange(codelist, len_cheats);
				}
		
			}
		
}

static void __noprint(const char *fmt, ...)
{
}


void *title_dol= NULL;

typedef void (*entrypoint) (void);

u32 entryPoint;

#include "fatffs_util.h"

/*

bootTitle: It use some parts from "Triiforce" 

   Copyright (c) 2009 The Lemon Man
   Copyright (c) 2009 Nicksasa
   Copyright (c) 2009 WiiPower

*/

void _unstub_start();

int bootTitle(u64 titleid)
{
entrypoint appJump;
int ret;

	cabecera2( "Loading...");

    dol_data=title_dol;

	WDVD_Init();
    WDVD_SetUSBMode(NULL, 0);
	WDVD_Reset();
	WDVD_Close();

	//Determine_VideoMode(*Disc_Region);

	Determine_VideoMode((((u32) titleid) & 0xff));

	set_language_and_ocarina();
	

		DCFlushRange((void*)0x80000000, 0x3f00);

	if(title_dol) 
		{
		
		if(nand_mode & 3)
		global_mount|= (nand_mode & 3);
		else global_mount&=~3;
	

		if(global_mount & 3)
			{
			
			if(load_fatffs_module(NULL)<0)  {cabecera2( "Fail Loading FAT FFS Module!!!");sleep(4);return 17;}
			}
		
		 /* enable_ffs:
		  bit 0   -> 0 SD 1-> USB
		  bit 1-2 -> 0-> /nand, 1-> /nand2, 2-> /nand3, 3-> /nand4
		  bit 3   -> led on in save operations
		  bit 4-  -> verbose level: 0-> disabled, 1-> logs from FAT operations, 2 -> logs FFS operations
		  bit 7   -> FFS emulation enabled/disabled

		  bit 8-9 -> Emulation mode: 0->default 1-> DLC redirected to device:/nand, 2-> Full Mode  3-> Fullmode with DLC redirected to device:/nand
		  */

		if(nand_mode & 3) enable_ffs( ((nand_mode-1) & 1) | ((nand_mode>>1) & 6) | 8 | 128 );

		// mounting FFS system
		ISFS_Deinitialize();
		ISFS_Initialize();
		usleep(500*1024);
		ISFS_Deinitialize();
		
		}
	
	
	if(FAT_Identify()<0) {cabecera2( "ES_Identify error!!!");sleep(4);return 888;}

	ret = ES_SetUID(titleid);
	if (ret < 0)
	{
        cabecera2( "SetUID Error!!!");
		sleep(4);
		return 890;
	}	

	ASND_StopVoice(1);
	ASND_End();
	usleep(100*1000);

	
	remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
	remote_end();
	flag_snow=0;
	Screen_flip();
	Screen_flip();
	WPAD_Shutdown();

	// Cleanup loader information
    //   WDVD_Close();
	usleep(500*1000);
   

	entryPoint = load_dol();
	
	if(!entryPoint) return -999;



	if (vmode)
			Set_VideoMode();
	

	   VIDEO_SetBlack(TRUE);
	   VIDEO_Flush();
	   VIDEO_WaitVSync();
	   VIDEO_SetBlack(TRUE);
	   VIDEO_Flush();
	   VIDEO_WaitVSync();

	// Set the clock
	settime(secs_to_ticks(time(NULL) - 946684800));

	

	// Remove 002 error
	
	*(u32 *)0x80003140 = (title_ios<<16) | 0xffff;
	*(u32 *)0x80003188 = *(u32 *)0x80003140;
	DCFlushRange((void*)0x80003140, 4);
	DCFlushRange((void*)0x80003188, 4);

   


      if (entryPoint != 0x3400)
		{
	   // Patch in info missing from apploader reads
        *Sys_Magic	= 0x0d15ea5e;
        *Version	= 1;
        *Arena_L	= 0x00000000;
		*BI2		= 0x817E5480;
        *Bus_Speed	= 0x0E7BE2C0;
        *CPU_Speed	= 0x2B73A840;

		/* Setup low memory */
		*(vu32 *)0x80000060 = 0x38A00040;
		*(vu32 *)0x800000E4 = 0x80431A80;
		*(vu32 *)0x800000EC = 0x81800000;

		*(vu32 *)0x800000F0 = 0x01800000;       // Simulated Memory Size

		}	
	
	   // fix for PeppaPig
	   memcpy((char *) temp_data, (void*)0x800000F4,4);
	
       SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);

       // fix for PeppaPig
	   memcpy((void*)0x800000F4,(char *) temp_data, 4);

	   *Disc_ID=0x10001;

	   memcpy(Online_Check, Disc_ID, 4);

	   appJump = (entrypoint)entryPoint;

	   // Flush application memory range
       DCFlushRange((void*)0x80000000, 0x17fffff);	// TODO: Remove these hardcoded value
	

	if (entryPoint != 0x3400)
	{
	if (hooktype)
		{
			__asm__(
						"lis %r3, entryPoint@h\n"
						"ori %r3, %r3, entryPoint@l\n"
						"lwz %r3, 0(%r3)\n"
						"mtlr %r3\n"
						"lis %r3, 0x8000\n"
						"ori %r3, %r3, 0x18A8\n"
						"mtctr %r3\n"
						"bctr\n"
						);
						
		} else
		{
			appJump();	
		}
	} else
	{
		if (hooktype)
		{
			__asm__(
						"lis %r3, returnpoint@h\n"
						"ori %r3, %r3, returnpoint@l\n"
						"mtlr %r3\n"
						"lis %r3, 0x8000\n"
						"ori %r3, %r3, 0x18A8\n"
						"mtctr %r3\n"
						"bctr\n"
						"returnpoint:\n"
						"bl DCDisable\n"
						"bl ICDisable\n"
						"li %r3, 0\n"
						"mtsrr1 %r3\n"
						"lis %r4, entryPoint@h\n"
						"ori %r4,%r4,entryPoint@l\n"
						"lwz %r4, 0(%r4)\n"
						"mtsrr0 %r4\n"
						"rfi\n"
						);
		} else
		{
			_unstub_start();
		}
	}

return 0;

}

int load_disc(u8 *discid)
{
        static struct DiscHeader Header ATTRIBUTE_ALIGN(32);
        static struct Partition_Descriptor Descriptor ATTRIBUTE_ALIGN(32);
        static struct Partition_Info Partition_Info ATTRIBUTE_ALIGN(32);
		signed_blob* Certs		= 0;
        signed_blob* Ticket		= 0;
        signed_blob* Tmd		= 0;
        
        unsigned int C_Length	= 0;
        unsigned int T_Length	= 0;
        unsigned int MD_Length	= 0;
        
        static u8	Ticket_Buffer[0x800] ATTRIBUTE_ALIGN(32);
        static u8	Tmd_Buffer[0x49e4] ATTRIBUTE_ALIGN(32);

        int i;
		

        memset(&Header, 0, sizeof(Header));
        memset(&Descriptor, 0, sizeof(Descriptor));
        memset(&Partition_Info, 0, sizeof(Partition_Info));

				
		cabecera2( "Loading...");


		if(discid[6]!=0) is_fat=0;
        
		if(nand_mode & 3)
			global_mount|= (nand_mode & 3);
		else global_mount&=~3;
		

		if(is_fat)
			{
			
			if(load_fatffs_module(discid)<0) return 17;
			}
		else
			{
			if(global_mount & 3)
				{
				
				if(load_fatffs_module(NULL)<0) return 17;
				}
			}

		
		WDVD_Init();
       

		if(is_fat) // FAT device
			{
			//  Adjust dip_plugin to work from 'FAT' device (sd: or usb:) 
			// use "_DEV" to seek access using byte units (file_offset) or "_DEVW" for words units (file_offset/4)

			if(WDVD_SetUSBMode((u8 *) "_DEV", 0)) return 1;
			}
	    else
        if(discid[6]==2) // DVD USB
			{
			if(WDVD_SetUSBMode((u8 *) "_DVD", multi_ciso[ciso_sel].lba)!=0) return 1;
			}
		else
		if(discid[6]!=0)
			{
			if(WDVD_SetUSBMode(NULL, multi_ciso[ciso_sel].lba)!=0) return 1;
			}
		else
			{
			if(cios==249 && current_partition!=0) return 101;


			if(WDVD_SetUSBMode(discid, current_partition)!=0) return 1;
			
			}
		

        WDVD_Reset();

		
        memset(Disc_ID, 0, 0x20);
        WDVD_ReadDiskId(Disc_ID);
		

        if (*Disc_ID==0x10001 || *Disc_ID==0x10001)
                return 2;
		DCFlushRange((void*)0x80000000, 0x20); // very, very important: the Disc_ID is used for group attr in the saves
		 
		 /* enable_ffs:
		  bit 0   -> 0 SD 1-> USB
		  bit 1-2 -> 0-> /nand, 1-> /nand2, 2-> /nand3, 3-> /nand4
		  bit 3   -> led on in save operations
		  bit 4-  -> verbose level: 0-> disabled, 1-> logs from FAT operations, 2 -> logs FFS operations
		  bit 7   -> FFS emulation enabled/disabled

		  bit 8-9 -> Emulation mode: 0->default 1-> DLC redirected to device:/nand, 2-> Full Mode  3-> Fullmode with DLC redirected to device:/nand
		  */

        
		if(nand_mode & 3) enable_ffs( ((nand_mode-1) & 1) | ((nand_mode>>1) & 6) | 8 | 128 | (256* ((nand_mode & 16)!=0)));
		
		// mounting FFS system
		ISFS_Deinitialize();
		ISFS_Initialize();
		usleep(500*1024);
		ISFS_Deinitialize();
		 
        Determine_VideoMode(*Disc_Region);
	   
		WDVD_UnencryptedRead(&Header, sizeof(Header), 0);

	
		/* BCA Data can be present at 0x100 offset in the .ISO (normally is a padding area filled with zeroes, but uLoader use it for BCA datas) */
        
		if(bca_mode & 1)
			{
			Set_DIP_BCA_Datas(BCA_Data);
			}
		else
			{
			for(i=0xa2;i<0xe2;i++) if(Header.Padding[i]!=0) break; // test for bca data
			
			if(i==0xe2) // if filled with zeroes set for NSMB bca datas
				{
				memset(BCA_Data,0,64);
				BCA_Data[0x33]=1;
				
				Set_DIP_BCA_Datas(BCA_Data);
				}
			else
				{
				memcpy(BCA_Data, &Header.Padding[0xa2],64);

				Set_DIP_BCA_Datas(BCA_Data);
				}
			}

		if(discid[6]==0)
			{
			if(strncmp((char *) &Header, (char *) discid, 6)) 
				return 666; // if headerid != discid (on hdd) error
			}
		
		//else  memcpy((char *) discid, (char *) &Header, 6);

		cabecera2( "Loading...");

        u64 Offset = 0x00040000; // Offset into disc to partition descriptor
        WDVD_UnencryptedRead(&Descriptor, sizeof(Descriptor), Offset);

        Offset = ((u64) Descriptor.Primary_Offset) << 2;

		

        u32 PartSize = sizeof(Partition_Info);
        u32 BufferLen = Descriptor.Primary_Count * PartSize;
        
        // Length must be multiple of 0x20
        BufferLen += 0x20 - (BufferLen % 0x20);
        u8 *PartBuffer = (u8*)memalign(0x20, BufferLen);

        memset(PartBuffer, 0, BufferLen);
        WDVD_UnencryptedRead(PartBuffer, BufferLen, Offset);

		cabecera2("Loading...");

        struct Partition_Info *Partitions = (struct Partition_Info*)PartBuffer;
        for ( i = 0; i < Descriptor.Primary_Count; i++)
        {
                if (Partitions[i].Type == 0)
                {
                        memcpy(&Partition_Info, PartBuffer + (i * PartSize), PartSize);
                        break;
                }
        }
       
		Offset = ((u64) Partition_Info.Offset) << 2;

        free(PartBuffer);
        if (!Offset)
                return 3;

		

        WDVD_Seek(Offset);


        Offset = 0;
          
        
        GetCerts(&Certs, &C_Length);
        WDVD_UnencryptedRead(Ticket_Buffer, 0x800, ((u64) Partition_Info.Offset) << 2);
        Ticket		= (signed_blob*)(Ticket_Buffer);
        T_Length	= SIGNED_TIK_SIZE(Ticket);


		cabecera2( "Loading...");


		 // Patch in info missing from apploader reads
        *Sys_Magic	= 0x0d15ea5e;
        *Version	= 1;
        *Arena_L	= 0x00000000;
		*BI2		= 0x817E5480;
        *Bus_Speed	= 0x0E7BE2C0;
        *CPU_Speed	= 0x2B73A840;

		/* Setup low memory */
		*(vu32 *)0x80000060 = 0x38A00040;
		*(vu32 *)0x800000E4 = 0x80431A80;
		*(vu32 *)0x800000EC = 0x81800000;

		*(vu32 *)0x800000F0 = 0x01800000;       // Simulated Memory Size

		if ((strncmp((char *) discid, "R3XE6U", 6)==0) || (strncmp((char *) discid, "R3XP6V", 6)==0))
			{
				*((u32*) 0x80003184)	= 0x80000000;    // Game ID Address
			}

		DCFlushRange((void*)0x80000000, 0x3f00);


        // Open Partition and get the TMD buffer
       
		if (WDVD_OpenPartition((u64) Partition_Info.Offset, 0,0,0, Tmd_Buffer) != 0)
                return 4;
        Tmd = (signed_blob*)(Tmd_Buffer);
        MD_Length = SIGNED_TMD_SIZE(Tmd);
        static struct AppLoaderHeader Loader ATTRIBUTE_ALIGN(32);

        WDVD_Read(&Loader, sizeof(Loader), 0x00002440);// Offset into the partition to apploader header
        DCFlushRange((void*)(((u32)&Loader) + 0x20),Loader.Size + Loader.Trailer_Size);

		cabecera2( "Loading...");


        // Read apploader from 0x2460
        WDVD_Read(Apploader, Loader.Size + Loader.Trailer_Size, 0x00002440 + 0x20);
        DCFlushRange((void*)(((int)&Loader) + 0x20),Loader.Size + Loader.Trailer_Size);

		cabecera2( "Loading...");
	

        AppLoaderStart	Start	= Loader.Entry_Point;
        AppLoaderEnter	Enter	= 0;
        AppLoaderLoad		Load	= 0;
        AppLoaderExit		Exit	= 0;
        Start(&Enter, &Load, &Exit);

		Enter(__noprint);

        void*	Address = 0;
        int		Section_Size;
        int		Partition_Offset;

		set_language_and_ocarina();
		
		void* Entry;

		
		while (Load(&Address, &Section_Size, &Partition_Offset))
		{
	
				if (!Address) return 5;
				cabecera2("Loading...");
				WDVD_Read(Address, Section_Size, ((u64) Partition_Offset) << 2);

				patch_dol(Address, Section_Size,0);

		}
		

		if(!strncmp((void *) AlternativeDol_infodat.id, (void *) discid, 6))
			{
			
			cabecera2("Loading Alternative .dol");
			
		
			WDVD_Read(dol_data, dol_len, AlternativeDol_infodat.offset);
		

			}

		ASND_StopVoice(1);
	    ASND_End();
		usleep(100*1000);


		WPAD_Shutdown();

		// Cleanup loader information
        WDVD_Close();

		
		
		#if 1
        // Identify as the game
        if (IS_VALID_SIGNATURE(Certs) 	&& IS_VALID_SIGNATURE(Tmd) 	&& IS_VALID_SIGNATURE(Ticket) 
            &&  C_Length > 0 				&& MD_Length > 0 			&& T_Length > 0)
        {
                int ret = ES_Identify(Certs, C_Length, Tmd, MD_Length, Ticket, T_Length, NULL);
                if (ret < 0)
                        return ret;
        }

		

		#endif
  
   
		// Retrieve application entry point
		

			if(dol_data)
			{
			
			
				cabecera2( "Loading Alternative .dol");
						
				
				Entry=(void *) load_dol();
			}
		else
			Entry= Exit();


		if(!Entry) return -999;

		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		remote_end();
		flag_snow=0;
		Screen_flip();
		Screen_flip();

        // Enable online mode in games
        memcpy(Online_Check, Disc_ID, 4);
		
		title_ios =(u32)(Tmd_Buffer[0x18b]);
		*(u32 *)0x80003140 = (title_ios<<16) | 0xffff;
		//*(u32 *)0x80003140 = *(u32 *)0x80003188; // removes 002-Error (by WiiPower: http://gbatemp.net/index.php?showtopic=158885&hl=)
        *(u32 *)0x80003188 = *(u32 *)0x80003140;

		DCFlushRange((void*)0x80000000, 0x17fffff);
			
		
        // Set Video Mode based on Configuration
		if (vmode)
			Set_VideoMode();
		

 
	    usleep(100*1000);

       // debug_printf("start %p\n",Entry);
	   settime(secs_to_ticks(time(NULL) - 946684800));

     
	   VIDEO_SetBlack(TRUE);
	   VIDEO_Flush();
	   VIDEO_WaitVSync();

	
	   // fix for PeppaPig
	   memcpy((char *) temp_data, (void*)0x800000F4,4);
	
       SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);

       // fix for PeppaPig
	   memcpy((void*)0x800000F4,(char *) temp_data, 4);

	   // Flush application memory range
       DCFlushRange((void*)0x80000000, 0x17fffff);	// TODO: Remove these hardcoded value
	

        __asm__ __volatile__
                (
                        "mtlr %0;"			// Move the entry point into link register
                        "blr"				// Branch to address in link register
                        :					// No output registers
                        :	"r" (Entry)		// Input register
                                //:					// difference between C and cpp mode??
                        );
	return 0;
}
