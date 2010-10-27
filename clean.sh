#!/bin/sh

BUILDTOOLS="filetochar  spf2o  stripios"
TOOLS="isotociso  wbfs  wiiscrubber_wbfs  wiiware_inst"
MODULES="ehcmodule  fatffs  mload  odip_plugin"
LIBS="libfat pngu screenlib tremor"

echo "Cleaning buildtools..."
cd buildtools
for tool in $BUILDTOOLS; do
	cd $tool
	make clean
	cd ..
done
cd ..

echo "Cleaning modules..."
cd modules
for module in $MODULES; do
	cd $module
	make clean
	cd ..
done
cd ..

echo "Cleaning tools..."
cd tools
for tool in $TOOLS; do
	cd $tool
	make clean
	cd ..
done
cd ..


echo "Cleaning libs..."
cd libs
for lib in $LIBS; do
	cd $lib
	make clean
	cd ..
done
rm -rf include/* lib/*
cd ..


echo "Cleaning cios_installer ..."
cd cios_installer
make clean
cd ..

echo "Cleaning uloader ..."
cd uloader
make clean
cd ..


