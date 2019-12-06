#eCos kerenl configuration for chipset
ifeq ($(CHIPSET),3050)
ECOS_ECC := ra305x_bsp.ecc
endif

ifeq ($(CHIPSET),3052)
ECOS_ECC := ra3052_bsp.ecc
endif

ifeq ($(CHIPSET),3350)
ECOS_ECC := ra305x_bsp.ecc
endif

ifeq ($(CHIPSET),3352)
ECOS_ECC := ra3352_bsp.ecc
endif

ifeq ($(CHIPSET),5350)
ECOS_ECC := ra5350_bsp.ecc
endif

ifeq ($(CHIPSET),7620)
ECOS_ECC := ra7620_bsp.ecc
endif
#Remind:change to ra7628_bsp.ecc when porting MT7628KN
ifeq ($(CHIPSET),mt7628)
ECOS_ECC := ra7628_bsp.ecc
endif
# need to modify file after code tree build
# the modify file is isoinfra.h in install directory
# so we use sed to do the modification

all: kernel module

kernel:
	if [ ! -d ra305x_bsp/install ]; \
	then cd ra305x_bsp;ecosconfig --config=$(ECOS_ECC) tree; \
	make;\
	fi
	@echo "finish build bsp ..."

module:
	@if [ ! -d ra305x_router/config/$(PRJ_NAME) ]; \
	then echo "Cann't find the ra305x_router/config/$(PRJ_NAME) directory"; exit;\
	fi
	@if [ ! -d ra305x_bsp/install ]; \
	then cd ra305x_bsp;ecosconfig --config=$(ECOS_ECC) tree; \
	make;\
	fi
	@if [ ! -d ra305x_router/webpages/web_c ]; \
	then make -C ra305x_router/config/$(PRJ_NAME) ;\
	else make -C ra305x_router ;\
	fi

clean: module_clean kernel_clean

kernel_clean:
	rm -Rf ra305x_bsp/compat
	rm -Rf ra305x_bsp/devs
	rm -Rf ra305x_bsp/error
	rm -Rf ra305x_bsp/hal
	rm -Rf ra305x_bsp/infra
	rm -Rf ra305x_bsp/install
	rm -Rf ra305x_bsp/io
	rm -Rf ra305x_bsp/isoinfra
	rm -Rf ra305x_bsp/kernel
	rm -Rf ra305x_bsp/language
	rm -Rf ra305x_bsp/net
	rm -Rf ra305x_bsp/services
	rm -f ra305x_bsp/makefile
	@echo "finish clean bsp ..."

module_clean:
	make -C ra305x_router clean
	@echo "finish clean module ..."

menuconfig:
	make -C ra305x_router menuconfig


