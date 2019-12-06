#define the ecos packages
ECOS_REPOSITORY := $(shell pwd)/packages
#define the ecos tool path for ecosconfig
ECOS_TOOL_PATH := $(shell pwd)/tools/bin:$(PATH)
#define cross compiler path
ECOS_MIPSTOOL_PATH := $(shell pwd)/tools/mipsisa32-elf/bin
BRANCH = ADV
#Configuration file selection
PRJ_NAME = ap_router
#CHIPSET - 5350, 7620, mt7628
CHIPSET = mt7628
WIFI_MODE = AP

# -----------------------------
# Second wifi device
# -----------------------------
#iNIC WIFI CHIPSET - NONE, 7610E, 7603E
INIC_WIFI = NONE
#iNIC Flash Type - flash, efuse
INIC_FLASH = flash

TARGET = ECOS
#Web Language - English or TChinese
WEB_LANG = English
TFTP_DIR = $(shell pwd)
#$(warning  ===================>  $(TFTP_DIR))
#Flash Layout - SMALL_UBOOT_PARTITION, NORMAL_UBOOT_PARTITION, see document in detail
FLASH_LAYOUT = NORMAL_UBOOT_PARTITION
#Output image file: <IMAGE_NAME>.img
IMAGE_NAME = eCos_$(shell whoami)


PATH :=$(ECOS_TOOL_PATH):$(ECOS_MIPSTOOL_PATH):$(PATH)
#export PATH ECOS_REPOSITORY BRANCH BOOT_CODE PRJ_NAME CHIPSET WIFI_MODE TARGET TFTP_DIR IMAGE_NAME CONFIG_CROSS_COMPILER_PATH WEB_LANG FLASH_LAYOUT
export PATH ECOS_REPOSITORY BRANCH BOOT_CODE PRJ_NAME CHIPSET WIFI_MODE TARGET TFTP_DIR IMAGE_NAME CONFIG_CROSS_COMPILER_PATH WEB_LANG FLASH_LAYOUT INIC_WIFI INIC_FLASH

all: kernel module
	cp -fr eCos_cwp.img /home/share/eCos

kernel: compiler
	chmod -Rf 777 tools/bin/*
	make -C ra305x_ap_adv kernel

module: compiler
	chmod -Rf 777 tools/bin/*
	make -C ra305x_ap_adv module

compiler:
	@if [ ! -d $(ECOS_MIPSTOOL_PATH) ]; \
	then tar zxvf tools/mipsisa32-elf.tgz -C tools; \
	fi

clean: module_clean kernel_clean

kernel_clean:
	make -C ra305x_ap_adv kernel_clean

module_clean:
	make -C ra305x_ap_adv module_clean

release:
	make -C ra305x_ap_adv/ra305x_router release

menuconfig:
	make -C ra305x_ap_adv menuconfig
