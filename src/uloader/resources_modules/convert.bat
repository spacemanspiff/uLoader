
filetochar dip_plugin.bin dip_plugin -h -align 32

copy *.c ..\source\*.c
copy *.h ..\source\*.h

filetochar ehcmodule.elf ehcmodule -h -align 32

copy *.c ..\source\*.c
copy *.h ..\source\*.h

filetochar fat-module.elf fat_module -h -align 32

copy *.c ..\source\*.c
copy *.h ..\source\*.h


del *.c
del *.h



