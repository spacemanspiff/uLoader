
filetochar odip_plugin.bin dip_plugin -h -align 32

copy *.c ..\source\*.c
copy *.h ..\source\*.h

filetochar ehcmodule.elf ehcmodule -h -align 32

copy *.c ..\source\*.c
copy *.h ..\source\*.h

filetochar fatffs-module.elf fatffs_module -h -align 32

copy *.c ..\source\*.c
copy *.h ..\source\*.h


del *.c
del *.h



