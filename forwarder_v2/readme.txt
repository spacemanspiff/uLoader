uLoader forwarder
-----------------

- Use it to create a channel to launch uLoader

- Copy loadstructor or Yaulct content folders to create customs channels for games using
this applications

Loadstructor:

http://clusterrr.com/Loadstructor.html

Yaulct:

http://gbatemp.net/index.php?showtopic=151410&st=0



You need the uLoader application from apps/uloader/boot.dol in the SD

Channels with Loadstructor/Yaulct and uLoader from WBFS
-------------------------------------------------------

uLoader saves the game configuration and the icon (.png) in the WBFS partition using a space filled
with zeroes corrupting opening.bnr with this action. opening.bnr is used from loadstructor to create
the WAD.

To solve it, You need to use the new wiiscrubber_wbfs.exe included.

Also you can solve it using the new wbfs_win.exe (directly without params) 
with the option 6 to remove CFG. This delete CFG and PNG Icon filling this area with zeroes restoring the opening.bnr.

Now you can use loadstructor selecting "uLoader forwarder" as loader to create the WAD
