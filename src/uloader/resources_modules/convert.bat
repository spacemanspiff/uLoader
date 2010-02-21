
filetochar dip_plugin.bin dip_plugin -h -align 32

copy *.c ..\source\*.c
copy *.h ..\source\*.h

del *.c
del *.h

filetochar ehcmodule.elf ehcmodule -h -align 32

copy *.c ..\source\port0\*.c
copy *.h ..\source\port0\*.h

del *.c
del *.h

filetochar ehcmodule_port1.elf ehcmodule -h -align 32

copy *.c ..\source\port1\*.c
copy *.h ..\source\port1\*.h

del *.c
del *.h


