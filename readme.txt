uLoader v3.5 - by Hermes (www.elotrolado.net)
---------------------------------------------------

WARNING: run at first cios_installer.dol (3.0 required). Now you can select the IOS used to install

uLoader is a work based on Kwiirk's YAL and Waninkoko's usbloader that tries to give support to both custom IOS (cIOS222 and cIOS249) 
to launch backups from USB mass storage devices with a graphical user interface similar to Wii channels.

************************************************************************************************************************************************************

Features:
---------


- Support for DVD USB Devices: It can run only DVD backups from .iso (original don't work
  because DVD drivers don´t support the Wii format). Remember you must insert a DVD
  to work at start the program. See "Mode DVD USB" below in this readme for more details

- Added support to save CFG, alternative dol, parental control and game configuration for DVD mode.

- Support for BCA datas. You can add it from .ISO offset 0x100 (64 bytes). If this area is filled with zeroes it use one BCA by default (NSMB compatible).
  New ioctl 0xda function supported in dip_plugin and new option added for DVD mode to read the BCA Datas from the original DVD.
  Support added to save BCA data code in sd:/bca_database.txt file

- Support to load games from DVD with alternative DOL (press '2' without USB device or press the DVD icon from the upper-right corner 
  in the selection game screen)

- New dev/mload and EHCI module based in interruptions. New cIOS based in IOS 38 base (I have done test reading 21000+ seconds without errors).
  See operational mode below.

- Now you can add your own .mod music from sd:/apps/uloader/music.mod .

- Test Mode support: Press RESET button when you can see the start screen to enter in Test Mode. Press RESET or HOME to exit and to save the internal 
  log file in the SD (sd:/log_ehc.txt)

- Support for SD and USB FAT/FAT32: Now you can use cheats codes and load alternative .dol from the USB 2.0 device
  (FAT partition is required)

- Parental control added: by default the password is 00000 (the last 0 is the 'ENTER', so you can program a new password as XXXX0 ). 
 You can change it from special menu pressing HOME. You can exits from the  password box pressing B. Parental control list the last 8 games launched
 with date/time, enables the password box (automatic when you enter in the menu) and fix a new password.
 (from 1.8 00000 disbles Parental control)

- 15 games per screen, showing an icon that can be inserted in the device, using the attached wbfs_win.exe revision (see below "Notes about wbfs_win").

- It has a Favorites Screen (up to 16 games) (save __CFG_ in HDD)

- Allows to select the cIOS to use between cIOS 222, cIOS 223 and CIOS 249.

- Screen text in Spanish and English (auto-detect)

- Support for video mode forced, language selection and Ocarina (note: use "Auto" to change PAL to NTSC or NTSC to PAL games or use "Force PAL 60"
   to change  NTSC to PAL60 games)

- Support for multiples WBFS partitions (max 4).

- Possibility to use the alternative cIOS 223 (only to launch games)

- Option to add or delete PNG icons/covers from the SD to the game directly (PNG is added in the HDD)
  Now you can download Covers from Internet or to adquire covers automatically from the current folder in the SD

- Now you can load games as Red Steel and othrers using one alternative .dol (see ALTERNATIVE DOL NOTES in this document)

- Added direct access for Dol Alternative selection

- Now you can load ehcmodule.elf externally (put it in sd:/apps/uloader/ehcmodule.elf) to use old versions
  of the module (see "src/uloader/resources")

- Support rename games added

- Support added to record the selected cheats index when you use cheats from txt files


************************************************************************************************************************************************************

USB 2.0 INIT
------------

if you obtain some error wait some seconds and try unplug/plug method

In case of error you can exits from the loop pressing RESET or the HOME BUTTON in the wiimote

************************************************************************************************************************************************************

CONTROLS
---------

- It can be used with Wiimote pointer or with GH3 guitar stick

- A or Green Button: Select

- B or Red Button: Exit/Discard

- +/- Button: Next page/Previous page

- 1 or Yellow button: Go to favorites page/back to last page


- HOME: Special Menu (Add games, format, parental control, exit..)

- HOME (pointing one game): Complete Special Menu (options for PNG, Rename, alternative dol, delete...)

- LEFT/RIGHT: Volume control for .mod music (from game screen selection)

************************************************************************************************************************************************************

CONFIGURE BUTTON
-------------------

You can change from here the video, language and cIOS for the game

************************************************************************************************************************************************************

ICONS/COVERS
------------

If the game doesn't have an icon, a default one is added. See below "Notes about wbfs_win" to know how to add and icon.

Covers must be <200KB and 160x224 pixels

************************************************************************************************************************************************************

Favorites
---------

If you don't add one game at least, you won't see the screen at booting.

Configuration requires a custom file in the HDD 

To do that, press A on game icon's and you'll access a screen that will give you the options to add/erase the game
from favorites screen (erase action only appears when you access from favorites)

After adding you'll reach favorites screen and you'll see the icon shaded, attached to the cursor: you only have to drop it 
in any of the channels to REPLACE IT with the new.

If you want to swap channels, keep pressing B and then press A, and the shaded icon will appear. Drop it where you want to SWAP IT.

You can only move the icons in favorites screen.

Remember that you can go back from favorites to last page pressing 1.


RUNNING GAMES
-------------

Running games screen will allow you to select custom IOS (if cIOS 222 is not available, cIOS 249 will be used), add to favorites and
launch the game immediately.

************************************************************************************************************************************************************

OCARINA
-------

Cheats codes must be in the "codes" folder of the SD or USB (FAT) with .gct as extension and the name equal to the disc ID.

Example:

:/codes/RB4P08.gct -> Cheats codes for Resident Evil 4 Wii


Now you can personalize the cheats including it as .txt file:


Example:

:/codes/RB4P08.txt -> trucos de Resident Evil 4 Wii


Within of the .txt must be:

RB4P08 /-> ID of the game (always. Same name of the file)
Resident Evil 4 Wii edition /-> Nombre del juego (optional, immediate line of the ID, one line of text)

In Game /-> Group (optional. Separated with one line)

Unlimited Ammo for all guns [Dr.Pepper] /-> Cheat name (always, one line)
48000000 8031F44C /-> Cheat codes
DE000000 80008180
12000008 000000FF
E0000000 80008000
Ammo infinite for you /-> commentaries (optional, one line)

Unlimited Pitas/Money [Dr.Pepper] /-> Second Cheat name to display (always, one line)
14337D58 000FFFFF /-> Cheat code

Note: The group name and the Cheat name cannot have more of 63 characters (the cheat name have max. of 39 characters)

************************************************************************************************************************************************************

Notes about wbfs_win
--------------------

wbfs_win is a command line utility that can be used both from GUI and WBFS Gui:

http://www.elotrolado.net/hilo_yal-juegos-sin-usbloader-desde-homebrew-channel_1213714

The utility have some changes that allows to extract a .ciso by default (if you rename it to .iso it will also work on another GUIs that
 force file type), so we can save some GB in the disc; upload both .iso and .ciso, and the possibility to add an icon to the uploaded iso.

It also supports iso extraction using "isoextract" command (see readme_new.txt)


To add an icon:

wbfs_win.exe Z png RSPP01 HBC_icon_WiiSports.png

Where:

 -Z is the drive letter
 -png is the command
 -RSPP01 is wiisports id
 -HBC_icon_WiiSports.png png file

PNG file size must be less than 200KB, but it's much better if it's less than 100KB, because it will directly affect main screen's load times.

Both width and height are better if divisible by 16 to avoid problems with textures, and lot better if dimensions are at most 256x192. 
My recommendation is a size of 128x96 average.

If you have doubts, please ask in the forums.

Remember, you have now one option to upload icons from the SD on uLoader 
application

Some elotrolado.net users have uploaded a lot of icons from here:

http://www.elotrolado.net/wiki/Iconos_de_juegos_para_uLoader

(save the page content to get alls icons quickly)


************************************************************************************************************************************************************

ALTERNATIVE DOL NOTES
---------------------

NOTE: Old alternative dol method is unsupported now

- Press HOME (pointing one game)

- Press PLUS to change the page and enter in "altertanive.dol"

- if you can see it you can delete the alternative dol using "Delete Alternative .dol"

- Press "Search .dol" to find alternative dols

- Select one (if it find one, of course)

NOTE: it can list 5 dols only, but i think it is sufficient at the momment (normally it find one or none). When you select one, it is saved 
in the _CFG_ WBFS file created by uLoader. It support 1024 games

NOTE2: Of course, when you "delete alternative .dol" it delete the list entry in _CFG_ and not 
delete the .dol :P

NOTE3: You can delete the olds sd:/games or usb:/games directories

Operational Mode for EHCI Driver
---------------------------------

- Device Mount: frontal LED frontal on from a second and when it finish LED off

- Mounting device fail or non recoverable error: frontal LED blinking slowly

- Unit Mounted disconnected: frontal LED bliking 3 times and LED off in secuence

- Device mismatch (different device): fast frontal LED blinking

- Interrupt support to free devices as USB 1.1 for others ports (remember you port 0 is always for USB 2.0 operations)

Mode DVD USB
------------

At Start:

- Run uLoader with a DVD device connected in the USB port and put a disc.
  
  1- while the device is waiting to be mounted it display the next message "ERROR: Can't Mount Device". 
 
  2- if it use expensive time to get the DVD, maybe the DVD device can´t read the disc (unknown format or other problem). Try reconnect the DVD device
     or change the disc by other.

  3- if it display "ERROR: DVD Device Sector Size must be 2048 bytes" you are using a bad format disc (for example CD audio XD)


In the DVD screen loader:

- It work exactly as DVD Wii device: You can eject and change the disc as you want from the new device

NOTE: in this mode you can´t load disc from the DVD Wii device how it is obvious