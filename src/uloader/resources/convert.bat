filetochar button1.png button1 -h
filetochar button2.png button2 -h
filetochar button3.png button3 -h

filetochar defpng.png defpng -h
filetochar defpng2.png defpng2 -h
filetochar lotus3_2.mod lotus3_2 -h

filetochar sound.raw sound -h

copy *.c ..\source\resources\*.c
copy *.h ..\source\resources\*.h

del *.c
del *.h

pause