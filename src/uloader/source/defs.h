#ifndef DEFS_H
#define DEFS_H
enum
{
        PAL_Default	= 'P',
        PAL_Germany	= 'D',
        PAL_France	= 'F',
        Euro_X		= 'X',
        Euro_Y		= 'Y',
        NTSC_USA	= 'E',
        NTSC_Japan	= 'J'
};
struct DiscHeader
{
	u32	ID;
	u16	Publisher;
	u16	Version;
	u8	Audio_Stream;
	u8	Buffer_Size;
	u8	Unused[14];
	u32	Magic;					// 0x5d1c9ea3 or disc is invalid
	u8	Unused2[4];
	u8	Title[60];
	u8	Disable_Verify;			// Will fail if set
	u8	Disable_Encryption;		// Will fail if set
	u8	Padding[0x7a2];
} __attribute__((__packed__));

struct Partition_Descriptor
{
	u32	Primary_Count;
	u32	Primary_Offset;
	u32	Secondary_Count;
	u32	Secondary_Offset;
	u32	Tertiary_Count;
	u32	Tertiary_Offset;
	u32	Quaternary_Count;
	u32	Quaternary_Offset;
} __attribute__((__packed__));

struct Partition_Info
{
	u32	Offset;
	u32	Type;
} __attribute__((__packed__));
#define        Disc_ID			((u32*) 0x80000000)
#define        Disc_Region		((u32*) 0x80000003)
#define        Disc_Magic		((u32*) 0x80000018)
#define        Sys_Magic		((u32*) 0x80000020)
#define        Version			((u32*) 0x80000024)
#define        Mem_Size			((u32*) 0x80000028)
#define        Board_Model		((u32*) 0x8000002c)
#define        Arena_L			((u32*) 0x80000030)
#define        Arena_H			((u32*) 0x80000034)
#define        FST			((u32*) 0x80000038)
#define        Max_FST			((u32*) 0x8000003c)
#define        Video_Mode		((u32*) 0x800000cc)
#define        Simulated_Mem		((u32*) 0x800000f0)
#define        BI2			((u32*) 0x800000f4)
#define        Bus_Speed		((u32*) 0x800000f8)
#define        CPU_Speed		((u32*) 0x800000fc)
#define        Dol_Params		((u32*) 0x800030f0)
#define        Online_Check		((u32*) 0x80003180)
#define        Apploader		((u32*) 0x81200000)
#define        Exit_Stub		((u32*) 0x8000180)

enum
{
        NTSC		= 0x00,
        PAL		= 0x01,
        Debug		= 0x02,
        Debug_PAL	= 0x03,
        MPAL		= 0x04,
        PAL60		= 0x05
};

typedef void 	(*AppLoaderReport)	(const char*, ...);
typedef void 	(*AppLoaderEnter)	(AppLoaderReport);
typedef int 	(*AppLoaderLoad)		(void** Dest, int* Size, int* Offset);
typedef void* 	(*AppLoaderExit)		();
typedef void 	(*AppLoaderStart)	(AppLoaderEnter*,AppLoaderLoad*,AppLoaderExit*);

struct AppLoaderHeader
{
        char	Revision[16];		// Apploader compile date
        AppLoaderStart	Entry_Point;		// Pointer to entry point
        int		Size;				// Size of Apploader
        int		Trailer_Size;		// Size of trailer
        u8	Padding[4];			// Padded with zeroes
} __attribute__((packed));

#define ALIGNED(x) __attribute__((aligned(x)))
#endif
