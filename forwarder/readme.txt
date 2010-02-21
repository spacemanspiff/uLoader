uLoader forwarder
-----------------

- Use it to create a channel to launch uLoader

- Copy forwarder.dol in loadstructor "loader" folder and replace loaders.ini by the 
included here to create customs channels for games

NOTE:

Loadstructor is original from here:

http://clusterrr.com/Loadstructor.html

You need the uLoader application from apps/uloader/boot.dol in the SD

Channels with Loadstructor and uLoader from WBFS
------------------------------------------------

uLoader saves the game configuration and the icon (.png) in the WBFS partition using a space filled
with zeroes corrupting opening.bnr with this action. opening.bnr is used from loadstructor to create
the WAD.

To solve it, use the new wbfs_win.exe (directly without params) and uses the option 6 to remove CFG.

This delete CFG and PNG Icon filling this area with zeroes restoring the opening.bnr.

Now you can use loadstructor selecting "uLoader forwarder" as loader to create the WAD
