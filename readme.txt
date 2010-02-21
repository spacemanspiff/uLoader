uLoader v2.6 BETA - by Hermes (www.elotrolado.net)
---------------------------------------------------

WARNING: run at first cios_installer.dol (2.6 required). Now you can select the IOS used to install

uLoader is a work based on Kwiirk's YAL and Waninkoko's usbloader that tries to give support to both custom IOS (cIOS222 and cIOS249) 
to launch backups from USB mass storage devices with a graphical user interface similar to Wii channels.

************************************************************************************************************************************************************

Features:
---------

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

- Now you can load games as Red Steel using one alternative .dol (see ALTERNATIVE DOL NOTES in this document)

- Now you can load ehcmodule.elf externally (put it in sd:/apps/uloader/ehcmodule.elf) to use old versions
  of the module (see "src/uloader/resources")

- Support rename games added

- Support added to record the selected cheats index when you use cheats from txt files

- New USB code and more

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


- HOME: Special Menu

- HOME (pointing one game): Complete Special Menu

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

You needs to create a "games" folder in the root of the SD or USB (FAT).

The dol file is loaded in automatic form using the game id as filename 

For example REDP41.dol for Red Steel: sd:/games/REDP41.dol

How to extract the dol file for Red Steel.

- Use wiiscrubber and open the red steel iso and select the partition 1

- Extract redsteel.dol and rename it as REDP41.dol

- Put the dol file in the "games" folder in the SD

- Of course: install red steel iso in the hdd.

NOTE: don´t remove or modify the original main.dol







