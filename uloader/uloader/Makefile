#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

define module2o
	bin2s -a 32 $< | $(AS) -o $(@)
	echo "extern u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(<F) | tr . _)`.h
	echo "extern u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(<F) | tr . _)`.h
	echo "extern u32" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(<F) | tr . _)`.h
endef

define spf2o
	../../../buildtools/spf2o/spf2o $< $(<F)
	$(CC) -c -o $(@) $(<F).c
endef
#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	../source ../source/libwbfs ../source/resources ../source/pintor
DATA		:=	../data ../resources_modules ../resources
INCLUDES	:=	

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

CFLAGS	= -g -Os -mrvl -Wall $(MACHDEP) $(INCLUDE) $(DEBUG)
CXXFLAGS	=	$(CFLAGS)

LDFLAGS	=	-g $(MACHDEP) -mrvl -Wl,-Map,$(notdir $@).map -T../../rvl.ld

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=	-lfat-hermes -lasnd -lmodplay -lscreen -lpngu -lpng -lwiiuse -lbte  -logc -lm -lz #../../tremor/libtremor.a


#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CURDIR)/../../libs $(DEVKITPPC)/lib $(CURDIR)/.. $(PORTLIBS) 

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES), -iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC)

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					-L$(LIBOGC_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ppc ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol

#---------------------------------------------------------------------------------
run:
	wiiload $(TARGET).dol


#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

#---------------------------------------------------------------------------------
# This rule links in binary data with the .jpg extension
#---------------------------------------------------------------------------------
%.jpg.o	:	%.jpg
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)
	
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(module2o)

%.elf.o : %.elf
	@echo $(notdir $<)
	$(module2o)

%.mod.o : %.mod
	@echo $(notdir $<)
	$(bin2o)

%.png.o : %.png
	@echo $(notdir $<)
	$(bin2o)

%.raw.o : %.raw
	@echo $(notdir $<)
	$(bin2o)

%.spf.o : %.spf
	@echo $(notdir $<)
	$(spf2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
