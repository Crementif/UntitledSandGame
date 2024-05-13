#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)

#-------------------------------------------------------------------------------
# APP_NAME sets the long name of the application
# APP_SHORTNAME sets the short name of the application
# APP_AUTHOR sets the author of the application
#-------------------------------------------------------------------------------
APP_NAME		:=	Tank Trap
APP_SHORTNAME	:=	Tank Trap
APP_AUTHOR		:=	I made this :)

include $(DEVKITPRO)/wut/share/wut_rules

#-------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# CONTENT is the path to the bundled folder that will be mounted as /vol/content/
# ICON is the game icon, leave blank to use default rule
# TV_SPLASH is the image displayed during bootup on the TV, leave blank to use default rule
# DRC_SPLASH is the image displayed during bootup on the DRC, leave blank to use default rule
#-------------------------------------------------------------------------------
ifneq ($(BUILD_DEBUG),1)
TARGET		:=	sand
BUILD		:=	build_release
else
TARGET		:=	sand_dbg
BUILD		:=	build_debug
endif

SOURCES		:=	source/common source/framework source/framework/physics source/framework/noise source/framework/fileformat source/framework/multiplayer source/framework/audio source/game
DATA		:=	data
INCLUDES	:=	include
CONTENT		:=	source/assets
ICON		:=	dist/icon.png
TV_SPLASH	:=	dist/tv-splash.png
DRC_SPLASH	:=	dist/drc-splash.png

#-------------------------------------------------------------------------------
# options for code generation
#-------------------------------------------------------------------------------
ifneq ($(BUILD_DEBUG),1)
CFLAGS		:=	-g -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -Wno-strict-aliasing -O3 -fno-math-errno -ffast-math -funsafe-math-optimizations -ftree-vectorize $(MACHDEP)
CFLAGS		+=	$(INCLUDE) -DNDEBUG -D__WIIU__ -D__WUT__
else
CFLAGS		:=	-g -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -Wno-strict-aliasing -O0 -ffunction-sections -fdata-sections $(MACHDEP)
CFLAGS		+=	$(INCLUDE) -DDEBUG -D__WIIU__ -D__WUT__
endif

CXXFLAGS	:=	$(CFLAGS) -std=gnu++20

ASFLAGS		:=	-g $(ARCH)
LDFLAGS		=	-g $(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map)

LIBS		:=	-lwut

#-------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level
# containing include and lib
#-------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(WUT_ROOT)

#-------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#-------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#-------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES		:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#-------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#-------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#-------------------------------------------------------------------------------
	export LD	:=	$(CC)
#-------------------------------------------------------------------------------
else
#-------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 		:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifneq (,$(strip $(CONTENT)))
	export APP_CONTENT := $(TOPDIR)/$(CONTENT)
endif

ifneq (,$(strip $(ICON)))
	export APP_ICON := $(TOPDIR)/$(ICON)
else ifneq (,$(wildcard $(TOPDIR)/$(TARGET).png))
	export APP_ICON := $(TOPDIR)/$(TARGET).png
else ifneq (,$(wildcard $(TOPDIR)/icon.png))
	export APP_ICON := $(TOPDIR)/icon.png
endif

ifneq (,$(strip $(TV_SPLASH)))
	export APP_TV_SPLASH := $(TOPDIR)/$(TV_SPLASH)
else ifneq (,$(wildcard $(TOPDIR)/tv-splash.png))
	export APP_TV_SPLASH := $(TOPDIR)/tv-splash.png
else ifneq (,$(wildcard $(TOPDIR)/splash.png))
	export APP_TV_SPLASH := $(TOPDIR)/splash.png
endif

ifneq (,$(strip $(DRC_SPLASH)))
	export APP_DRC_SPLASH := $(TOPDIR)/$(DRC_SPLASH)
else ifneq (,$(wildcard $(TOPDIR)/drc-splash.png))
	export APP_DRC_SPLASH := $(TOPDIR)/drc-splash.png
else ifneq (,$(wildcard $(TOPDIR)/splash.png))
	export APP_DRC_SPLASH := $(TOPDIR)/splash.png
endif

.PHONY: $(BUILD) both release debug clean all

#-------------------------------------------------------------------------------
all: both

$(BUILD):
	@$(shell [ ! -d $@ ] && mkdir -p $@)
	$(MAKE) CC=$(DEVKITPPC)/bin/powerpc-eabi-gcc CXX=$(DEVKITPPC)/bin/powerpc-eabi-g++ -C $(BUILD) -f $(CURDIR)/Makefile

release:
	@$(shell [ ! -d build_release ] && mkdir -p build_release)
	$(MAKE) build_release BUILD_DEBUG=0 -f $(CURDIR)/Makefile

debug:
	@$(shell [ ! -d build_debug ] && mkdir -p build_debug)
	$(MAKE) build_debug BUILD_DEBUG=1 -f $(CURDIR)/Makefile

both:
	# run release and debug builds sequentially
	$(MAKE) build_release BUILD_DEBUG=0 -f $(CURDIR)/Makefile
	$(MAKE) build_debug BUILD_DEBUG=1 -f $(CURDIR)/Makefile

#-------------------------------------------------------------------------------
clean:
	@echo clean...
	@rm -fr build_debug build_release $(TARGET).wua $(TARGET).wuhb $(TARGET).rpx $(TARGET).elf $(TARGET)_dbg.wua $(TARGET)_dbg.wuhb $(TARGET)_dbg.rpx $(TARGET)_dbg.elf

#-------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS			:=	$(OFILES:.o=.d)
CONTENT_DEPENDS	:=	$(shell find $(APP_CONTENT) -type f) $(APP_ICON) $(APP_TV_SPLASH) $(APP_DRC_SPLASH)

#-------------------------------------------------------------------------------
# main targets
#-------------------------------------------------------------------------------
all: $(OUTPUT).wuhb $(OUTPUT).wua

$(OUTPUT).wuhb:	$(OUTPUT).rpx
$(OUTPUT).wua: $(OUTPUT).rpx
	@echo Creating wua...
	@cp $(OUTPUT).rpx $(TOPDIR)/dist/wua/00050000102b2b2b_v0/code/sand.rpx
	@rm -rf $(TOPDIR)/dist/wua/00050000102b2b2b_v0/content/
	@mkdir $(TOPDIR)/dist/wua/00050000102b2b2b_v0/content/
	@cp -r $(TOPDIR)/$(CONTENT)/. $(TOPDIR)/dist/wua/00050000102b2b2b_v0/content/
	@rm -f $(OUTPUT).wua
	@$(TOPDIR)/dist/zarchive.exe $(shell wslpath -m $(TOPDIR)/dist/wua || echo $(TOPDIR)/dist/wua) $(shell wslpath -m $(OUTPUT).wua || echo $(OUTPUT).wua) || echo "zarchive.exe couldn't be executed, skipping wua creation..."
	@echo built ... sand.wua
$(OUTPUT).rpx: $(OUTPUT).elf $(CONTENT_DEPENDS)
$(OUTPUT).elf: $(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#-------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#-------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#-------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------