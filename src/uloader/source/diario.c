/*
	DIARIO.C
	This code allows to modify play_rec.dat in order to store the
	game time in Wii's log correctly.

	by Marc
	Thanks to tueidj for giving me some hints on how to do it :)
	Most of the code was taken from here:
	http://forum.wiibrew.org/read.php?27,22130
*/

#include <stdio.h>
#include <ogcsys.h>

#define SECONDS_TO_2000 946684800LL
#define TICKS_PER_SECOND 60750000LL

// Thanks to Dr. Clipper
u64 getWiiTime(void) {
time_t uTime;
uTime = time(NULL);
return TICKS_PER_SECOND * (uTime - SECONDS_TO_2000);
}

#define PLAYRECPATH "/title/00000001/00000002/data/play_rec.dat"

typedef struct{
	u32 checksum;
	union{
		u32 data[0x1f];
		struct{
			char name[84];	//u16 name[42];
			u64 ticks_boot;
			u64 ticks_last;
			char title_id[4];
			char title_gid[2];
			//u8 unknown[18];
		} ATTRIBUTE_PACKED;
	};
} playrec_struct;

playrec_struct playrec_buf;


int Diario_ActualizarDiario(char* bannerTitle,char* gameId){
	s32 ret,playrec_fd;
	u32 sum = 0;
	u8 i;

    ISFS_Deinitialize();
	ISFS_Initialize();

	
	//Open play_rec.dat
	playrec_fd = IOS_Open(PLAYRECPATH, IPC_OPEN_RW);
	if(playrec_fd < 0) {
		//printf("* ERROR: abriendo play_rec.dat (%d)\n",playrec_fd);
		goto error_1;
		
	}

	//Read play_rec.dat
	ret = IOS_Read(playrec_fd, &playrec_buf, sizeof(playrec_buf));
	if(ret != sizeof(playrec_buf)){
		//printf("* ERROR: leyendo play_rec.dat (%d)\n",ret);
		goto error_2;
	}

	if(IOS_Seek(playrec_fd, 0, 0)<0) goto error_2;
    
	playrec_buf.ticks_boot=getWiiTime();
	playrec_buf.ticks_last=getWiiTime();

	//Update channel name and ID
	for(i=0;i<84;i++)
		playrec_buf.name[i]=bannerTitle[i];

	playrec_buf.title_id[0]=gameId[0];
	playrec_buf.title_id[1]=gameId[1];
	playrec_buf.title_id[2]=gameId[2];
	playrec_buf.title_id[3]=gameId[3];
	playrec_buf.title_gid[0]=gameId[4];
	playrec_buf.title_gid[1]=gameId[5];

	//Calculate and update checksum
	for(i=0; i<0x1f; i++)
		sum += playrec_buf.data[i];
	playrec_buf.checksum=sum;


	ret = IOS_Write(playrec_fd, &playrec_buf, sizeof(playrec_buf));
	if(ret!=sizeof(playrec_buf)){
		//printf("* ERROR: guardando play_rec.dat (%d)\n",ret);
		goto error_2;
	}


	IOS_Close(playrec_fd);

	ISFS_Deinitialize();

	return 0;

error_1:

	IOS_Close(playrec_fd);

error_2:

	ISFS_Deinitialize();
	return -1;
}

int Diario_InvalidarDiario(void){
	s32 ret,playrec_fd;

    ISFS_Deinitialize();
	ISFS_Initialize();

	
	//Open play_rec.dat
	playrec_fd = IOS_Open(PLAYRECPATH, IPC_OPEN_RW);
	if(playrec_fd < 0) {
		//printf("* ERROR: abriendo play_rec.dat (%d)\n",playrec_fd);
		goto error_1;
		
	}

	//Read play_rec.dat
	ret = IOS_Read(playrec_fd, &playrec_buf, sizeof(playrec_buf));
	if(ret != sizeof(playrec_buf)){
		//printf("* ERROR: leyendo play_rec.dat (%d)\n",ret);
		goto error_2;
	}

	if(IOS_Seek(playrec_fd, 0, 0)<0) goto error_2;
    
	// invalidate checksum

	playrec_buf.checksum=0;

	ret = IOS_Write(playrec_fd, &playrec_buf, sizeof(playrec_buf));
	if(ret!=sizeof(playrec_buf)){
		//printf("* ERROR: guardando play_rec.dat (%d)\n",ret);
		goto error_2;
	}


	IOS_Close(playrec_fd);

	ISFS_Deinitialize();

	return 0;

error_1:

	IOS_Close(playrec_fd);

error_2:

	ISFS_Deinitialize();
	return -1;
}
