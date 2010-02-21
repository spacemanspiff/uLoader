filetochar button1.png button1 -h
filetochar button2.png button2 -h
filetochar button3.png button3 -h

filetochar defpng.png defpng -h
filetochar defpng2.png defpng2 -h
filetochar bg_music.ogg bg_music -h

filetochar sound.raw sound -h

copy *.c ..\source\resources_alt\*.c
copy *.h ..\source\resources_alt\*.h

del *.c
del *.h

pause