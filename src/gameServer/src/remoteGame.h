#ifndef __REMOTEGAME_H__
#define __REMOTEGAME_H__

#include <string>

class RemoteGame {
	public:
		RemoteGame (const std::string &filename);
		virtual ~RemoteGame();

		const std::string &gameId() const;
		const std::string &gameTitle() const;

		bool initialized(void) const { return _initialized; }

		//int ReadDiskId(void *);
		int UnencryptedRead(void *buf, int len, long long offset);

 	private:
		int _fd;
		bool _initialized;

		std::string _gameId;
		std::string _gameTitle;
};

#endif
