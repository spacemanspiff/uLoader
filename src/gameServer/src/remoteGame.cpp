#include "remoteGame.h"

#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>

#include <iostream>

#include <string.h>

struct wiiDisc {
	char id[6];	                    // 00
	unsigned char diskNumber;           // 06
	unsigned char diskVersion;          // 07
	unsigned char audioStreaming;       // 08
	unsigned char streamingBufferSize;  // 09
	unsigned char unused[14];           // 0A
	unsigned char magic[4];             // 
	unsigned char paddingtitle[4];
	char gameTitle[64];
	unsigned char disableHash;
	unsigned char disableDiscEnc;
	unsigned char padding[380];
} __attribute__((packed));

RemoteGame::RemoteGame (const std::string &filename)
{
	_initialized = false;
	_fd = open(filename.c_str(), O_RDONLY | O_LARGEFILE);

	if (_fd < 0)
		return;
	struct wiiDisc gameInfo;

	if (UnencryptedRead(&gameInfo, sizeof(struct wiiDisc), 0) < 0) 
		return;

	if (gameInfo.magic[0] != 0x5d || gameInfo.magic[1] != 0x1C || 
		gameInfo.magic[2] != 0x9E || gameInfo.magic[3] != 0xA3)
	{
		return;	
	}
	_initialized = true;
	//int titleLen = 0;
	//while (gameInfo.gameTitle[titleLen])
	//	++titleLen;	
	_gameTitle =gameInfo.gameTitle;

	char tmp[7];
	strncpy(tmp, gameInfo.id, 6);
	tmp[6]= 0;
	_gameId = tmp;
}

RemoteGame::~RemoteGame()
{
	if (_initialized)
		close(_fd);
}

//int RemoteGame::ReadDiskId(void *) {}
//

int RemoteGame::UnencryptedRead(void *buf, int len, long long offset)
{
	lseek(_fd, offset, SEEK_SET);
	return read(_fd, buf, len);
}

const std::string &RemoteGame::gameId() const
{
	return _gameId;
}

const std::string &RemoteGame::gameTitle() const
{
	return _gameTitle;
}

