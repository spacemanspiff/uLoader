
uloader: filetochar spf2o
	cd uloader && make

buildtools: filetochar spf2o stripios

filetochar:
		@echo Building filetochar
		@cd buildtools/filetochar && make
		@mkdir -p buildtools/bin && cp buildtools/filetochar/filetochar buildtools/bin

spf2o:
		@echo Building spf2o
		@cd buildtools/spf2o && make
		@mkdir -p buildtools/bin && cp buildtools/spf2o/spf2o buildtools/bin

stripios:
		@echo Building stripios
		@cd buildtools/stripios && make
		@mkdir -p buildtools/bin && cp buildtools/stripios/stripios.exe buildtools/bin
	
clean_libfat:
		@echo Cleaning libfat
		@cd libs/libfat && make clean

clean_pngu:
		@echo Cleaning pngu
		@cd libs/pngu && make clean

clean_screenlib:
		@echo Cleaning screenlib
		@cd libs/screenlib && make clean

clean_tremor:
		@echo Cleaning tremor
		@cd libs/tremor && make clean

clean_libs:	clean_libfat clean_pngu clean_screenlib clean_tremor
		@echo Cleaning installed libs
		@cd libs && rm -rf include/* libs/*

clean_filetochar:
		@echo Cleaning buildtool filetochar
		@cd buildtools/filetochar && make clean

clean_spf2o:
		@echo Cleaning buildtool spf2o 
		@cd buildtools/spf2o && make clean

clean_stripios:
		@echo Cleaning buildtool stripios 
		@cd buildtools/stripios && make clean

clean_buildtools: clean_filetochar clean_spf2o clean_stripios

clean_ehcmodule:
		@echo "Cleaning module EHCI (USB 2.0)" 
		@cd modules/ehcmodule && make clean

clean_fatffs:
		@echo Cleaning module FATFFS
		@cd modules/fatffs && make clean

clean_mload:
		@echo Cleaning module mload
		@cd modules/mload && make clean

clean_odip_plugin:
		@echo Cleaning ODIP Plugin
		@cd modules/odip_plugin && make clean

clean_modules: clean_ehcmodule clean_fatffs clean_mload clean_odip_plugin

clean_isotociso:
		@echo Cleaning isotociso tool
		@cd tools/isotociso && make clean
clean_wbfs:
		@echo Cleaning wbfs tool
		@cd tools/wbfs && make clean
clean_wiiscrubber_wbfs:
		@echo Cleaning wiiscrubber_wbfs tool
		@cd tools/wiiscrubber_wbfs && make clean

clean_wiiware_inst:
		@echo Cleaning wiiware_inst tool
		@cd tools/wiiware_inst && make clean

clean_ciosinstaller:
		@echo Cleaning cios_installer
		@cd cios_installer && make clean

clean_uloader:
		@echo Cleaning uloader
		@cd uloader && make clean

clean_tools: clean_isotociso  clean_wbfs  clean_wiiscrubber_wbfs  clean_wiiware_inst

clean: clean_libs clean_buildtools clean_modules clean_tools clean_ciosinstaller clean_uloader
