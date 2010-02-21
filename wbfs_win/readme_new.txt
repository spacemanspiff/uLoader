Modified by Hermes

NEW: 

-run without params to use the GUI

- Integrity Check added (for files bad deleted in olders versions)

The new support add/extract in .ciso format (Compact ISO) and one new command to add a .png file (<200KB) to use on the uLoader application.

Added one option to delete CONF and PNG of uLoader (use this before to create WAD from Loadstructor)



=== Usage ===
Before using wbfs tool, you must initialize the drive. Preferably pre-format your USB drive to FAT32.

'X' in the following examples represents your drive letter, for example, G.

*init the partition:
 wbfs_win.exe X init

*estimate the size of the iso on the USB drive
 wbfs_win.exe X estimate <your_wiidisc.iso>

*add an iso to your partition
 wbfs_win.exe X add <your_wiidisc.iso>

*list the wiidisc that are on the wbfs, you will get the DISCID, game name, number of wide sectors used, and number of GB used.
 wbfs_win.exe X list

*count the number of wide sectors / GB available on your partition
 wbfs_win.exe X info

*build Homebrew Channel directories for all the games in your partition
This will actually make a directory for each game with the DISCID of the game, 
copy the icon.png and boot.dol of the current directory, and make a meta.xml with the name of the game
 wbfs_win.exe X makehbc
Then copy all the directories in the apps directory of your sdcard.

*remove a disc from wbfs
 wbfs_win.exe X remove DISCID

*extract an ciso from wbfs
 wbfs_win.exe X extract DISCID

*extract an iso from wbfs
 wbfs_win.exe X isoextract DISCID

*add a .PNG file to the wbfs disc
 wbfs_win.exe X png DISCID <your_file.png>

The .png is added from offset 1024 in the disc (must be <200KB and i supose it is a filled with zero area)



