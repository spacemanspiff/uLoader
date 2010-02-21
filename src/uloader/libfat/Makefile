ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif
 
export TOPDIR	:=	$(CURDIR)
 
export LIBFAT_MAJOR	:= 1
export LIBFAT_MINOR	:= 0
export LIBFAT_PATCH	:= 6

export VERSTRING	:=	$(LIBFAT_MAJOR).$(LIBFAT_MINOR).$(LIBFAT_PATCH)


default: release

all: release dist

release: include/libfatversion.h wii-release

wii-release:
	$(MAKE) -C libogc PLATFORM=wii BUILD=wii_release

clean: ogc-clean

ogc-clean:
	$(MAKE) -C libogc clean


ogc-dist-bin: ogc-release distribute/$(VERSTRING)
	$(MAKE) -C libogc dist-bin

dist-src: distribute/$(VERSTRING)
	@tar --exclude=.svn --exclude=*CVS* -cvjf distribute/$(VERSTRING)/libfat-src-$(VERSTRING).tar.bz2 \
	source include Makefile \
	nds/Makefile nds/include \
	gba/Makefile gba/include \
	libogc/Makefile libogc/include

dist: include/libfatversion.h dist-bin dist-src

distribute/$(VERSTRING):
	@[ -d $@ ] || mkdir -p $@

include/libfatversion.h : Makefile
	@echo "#ifndef __LIBFATVERSION_H__" > $@
	@echo "#define __LIBFATVERSION_H__" >> $@
	@echo >> $@
	@echo "#define _LIBFAT_MAJOR_	$(LIBFAT_MAJOR)" >> $@
	@echo "#define _LIBFAT_MINOR_	$(LIBFAT_MINOR)" >> $@
	@echo "#define _LIBFAT_PATCH_	$(LIBFAT_PATCH)" >> $@
	@echo >> $@
	@echo '#define _LIBFAT_STRING "libFAT Release '$(LIBFAT_MAJOR).$(LIBFAT_MINOR).$(LIBFAT_PATCH)'"' >> $@
	@echo >> $@
	@echo "#endif // __LIBFATVERSION_H__" >> $@

install:  ogc-install

ogc-install: wii-release
	$(MAKE) -C libogc install
