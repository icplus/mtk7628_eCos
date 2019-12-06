include $(TOPDIR)/.config
include $(TOPDIR)/rules.mak
include $(RT28xx_DIR)/os/ecos/config.mk

#ifdef CONFIG_AP_SUPPORT
ifeq ($(RT28xx_MODE),AP)
MOD_NAME = rt$(MODULE)ap
endif
#endif // CONFIG_AP_SUPPORT //

obj-m := $(MOD_NAME).o

#ifdef CONFIG_AP_SUPPORT
ifeq ($(RT28xx_MODE),AP)
$(MOD_NAME)-objs := \
	../../common/crypt_md5.o\
	../../common/crypt_sha2.o\
	../../common/crypt_hmac.o\
	../../common/crypt_aes.o\
	../../common/crypt_arc4.o\
	../../common/mlme.o\
	../../common/cmm_wep.o\
	../../common/action.o\
	../../common/cmm_data.o\
	../../common/rtmp_init.o\
	../../common/rtmp_init_inf.o\
	../../common/cmm_tkip.o\
	../../common/cmm_aes.o\
	../../common/cmm_sync.o\
	../../common/eeprom.o\
	../../common/cmm_sanity.o\
	../../common/cmm_info.o\
	../../common/cmm_cfg.o\
	../../common/cmm_wpa.o\
	../../common/cmm_radar.o\
	../../common/spectrum.o\
	../../common/rtmp_timer.o\
	../../common/rt_channel.o\
	../../common/cmm_profile.o\
	../../common/cmm_asic.o\
	../../common/cmm_cmd.o\
	../../common/ps.o\
	../../rate_ctrl/ra_ctrl.o\
	../../rate_ctrl/alg_legacy.o\
	../../rate_ctrl/alg_ags.o\
	../../os/ecos/rt_profile.o \
	../../ap/ap_mbss.o\
	../../chips/rtmp_chip.o\
	../../ap/ap.o\
	../../ap/ap_assoc.o\
	../../ap/ap_auth.o\
	../../ap/ap_connect.o\
	../../ap/ap_mlme.o\
	../../ap/ap_sanity.o\
	../../ap/ap_sync.o\
	../../ap/ap_wpa.o\
	../../ap/ap_data.o\
	../../common/uapsd.o\
	../../ap/ap_autoChSel.o\
	../../ap/ap_qload.o\
	../../ap/ap_cfg.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += ../../ate/common/rt_ate.o
endif

ifeq ($(HAS_QA_SUPPORT),y)
$(MOD_NAME)-objs += ../../ate/common/rt_qa.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	../../ap/ap_mbss_inf.o\
	../../common/rt_os_util.o\
	../../os/ecos/ap_ioctl.o\
	../../os/ecos/rt_ecos.o \
	../../os/ecos/rt_main_end.o
endif


ifeq ($(HAS_NEW_RATE_ADAPT_SUPPORT),y)
$(MOD_NAME)-objs += ../../rate_ctrl/alg_grp.o
endif

#ifdef DOT11_N_SUPPORT
ifeq ($(HAS_DOT11_N_SUPPORT),y)
$(MOD_NAME)-objs += \
	../../common/ba_action.o

#ifdef TXBF_SUPPORT
ifeq ($(HAS_TXBF_SUPPORT),y)
$(MOD_NAME)-objs += \
	../../common/cmm_txbf.o\
	../../common/cmm_txbf_cal.o
endif
#endif // TXBF_SUPPORT //
endif
#endif // DOT11_N_SUPPORT //

#ifdef BG_FT_SUPPORT
ifeq ($(OSABL),NO)
ifeq ($(HAS_BGFP_SUPPORT),y)
$(MOD_NAME)-objs += \
	../../os/linux/br_ftph.o
endif
endif
#endif // BG_FT_SUPPORT //



#ifdef LED_CONTROL_SUPPORT
ifeq ($(HAS_LED_CONTROL_SUPPORT),y)
$(MOD_NAME)-objs += \
	../../common/rt_led.o
endif
#endif // LED_CONTROL_SUPPORT //



#ifdef WSC_INCLUDED
ifeq ($(HAS_WSC),y)
$(MOD_NAME)-objs += \
	../../common/wsc.o\
	../../common/wsc_tlv.o\
	../../common/wsc_ufd.o\
	../../common/crypt_biginteger.o\
	../../common/crypt_dh.o	
endif
ifeq ($(HAS_WSC_V2),y)
$(MOD_NAME)-objs += ../../common/wsc_v2.o
endif
#endif // WSC_INCLUDED //


#ifdef NINTENDO_AP
ifeq ($(HAS_NINTENDO),y)
$(MOD_NAME)-objs += ../../ap/ap_nintendo.o
endif
#endif // NINTENDO_AP //

ifeq ($(HAS_WDS),y)
$(MOD_NAME)-objs += ../../ap/ap_wds.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	../../ap/ap_wds_inf.o
endif
endif

#ifdef APCLI_SUPPORT
ifeq ($(HAS_APCLI),y)
$(MOD_NAME)-objs += \
	../../ap/ap_apcli.o \
	../../ap/apcli_ctrl.o \
	../../ap/apcli_sync.o \
	../../ap/apcli_auth.o \
	../../ap/apcli_assoc.o \
	../../common/cmm_mat.o \
	../../common/cmm_mat_iparp.o \
	../../common/cmm_mat_pppoe.o \
	../../common/cmm_mat_ipv6.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	../../ap/ap_apcli_inf.o
endif
endif
#endif // APCLI_SUPPORT //

ifeq ($(HAS_BLOCK_NET_IF),y)
$(MOD_NAME)-objs += ../../common/netif_block.o
endif

ifeq ($(HAS_IGMP_SNOOP_SUPPORT),y)
$(MOD_NAME)-objs += ../../common/igmp_snoop.o
endif

#ifdef WMM_ACM_SUPPORT
ifeq ($(HAS_WMM_ACM_SUPPORT),y)
$(MOD_NAME)-objs += ../../common/acm_edca.o\
	../../common/acm_comm.o\
	../../common/acm_iocl.o
endif
#endif // WMM_ACM_SUPPORT //

ifeq ($(HAS_QOS_DLS_SUPPORT),y)
$(MOD_NAME)-objs += ../../ap/ap_dls.o
endif

ifeq ($(HAS_IDS_SUPPORT),y)
$(MOD_NAME)-objs += ../../ap/ap_ids.o
endif





#ifdef MESH_SUPPORT
ifeq ($(HAS_MESH_SUPPORT),y)
$(MOD_NAME)-objs += \
	../../common/mesh_bmpkt.o \
	../../common/mesh_ctrl.o \
	../../common/mesh_link_mng.o \
	../../common/mesh_sanity.o \
	../../common/mesh_tlv.o \
	../../common/mesh.o \
	../../common/mesh_forwarding.o \
	../../common/mesh_path_mng.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	../../common/mesh_inf.o
endif
endif
#endif // MESH_SUPPORT //


#ifdef DOT11R_FT_SUPPORT
ifeq ($(HAS_DOT11R_FT_SUPPORT),y)
$(MOD_NAME)-objs += \
	../../common/ft.o\
	../../common/ft_tlv.o\
	../../common/ft_iocl.o\
	../../common/ft_rc.o\
	../../ap/ap_ftkd.o
endif
#endif // DOT11R_FT_SUPPORT //

#ifdef DOT11K_RRM_SUPPORT
ifeq ($(HAS_DOT11K_RRM_SUPPORT),y)
$(MOD_NAME)-objs += \
	../../common/rrm_tlv.o\
	../../common/rrm_sanity.o\
	../../common/rrm.o
endif
#endif // DOT11K_RRM_SUPPORT //

#ifdef DOT11V_WNM_SUPPORT
ifeq ($(HAS_DOT11V_WNM_SUPPORT),y)
$(MOD_NAME)-objs += \
        ../../common/wnm_tlv.o\
        ../../common/wnm.o
endif
#endif // DOT11V_WNM_SUPPORT //

#ifdef DOT11W_PMF_SUPPORT
ifeq ($(HAS_DOT11W_PMF_SUPPORT),y)
$(MOD_NAME)-objs += \
	../../common/pmf.o
endif
#endif // DOT11W_PMF_SUPPORT //

ifeq ($(HAS_CLIENT_WDS_SUPPORT),y)
$(MOD_NAME)-objs += \
	../../common/client_wds.o
endif

ifeq ($(HAS_DFS_SUPPORT),y)
$(MOD_NAME)-objs += \
        ../../common/cmm_dfs.o
endif

ifeq ($(HAS_CS_SUPPORT),y)
$(MOD_NAME)-objs += \
        ../../common/cmm_cs.o
endif

endif
#endif // CONFIG_AP_SUPPORT //


#chip releated
#ifdef RT3050
ifneq ($(findstring 3050,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../common/ee_flash.o\
	../../common/rtmp_swmcu.o\
	../../common/rt_rf.o\
	../../chips/rt305x.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	../../ate/chips/rt305x_ate.o\
	../../ate/common/ate_pci.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	../../os/ecos/rt_pci_rbus.o\
	../../os/ecos/rbus_main_end.o\
	../../os/ecos/rt_rbus_pci_util.o
endif

endif
#endif // RT3050 //

#ifdef RT3052
ifneq ($(findstring 3052,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../common/ee_flash.o\
	../../common/rtmp_swmcu.o\
	../../common/rt_rf.o\
	../../chips/rt305x.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	../../ate/chips/rt305x_ate.o\
	../../ate/common/ate_pci.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	../../os/ecos/rt_pci_rbus.o\
	../../os/ecos/rbus_main_end.o\
	../../os/ecos/rt_rbus_pci_util.o
endif

endif
#endif // RT3052 //

#ifdef RT3350
ifneq ($(findstring 3350,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../common/ee_flash.o\
	../../common/rtmp_swmcu.o\
	../../common/rt_rf.o\
	../../chips/rt305x.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	../../ate/chips/rt305x_ate.o\
	../../ate/common/ate_pci.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	../../os/ecos/rt_pci_rbus.o\
	../../os/ecos/rbus_main_end.o\
	../../os/ecos/rt_rbus_pci_util.o
endif
endif
#endif // RT3350 //

#ifdef RT3352
ifneq ($(findstring 3352,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../common/ee_flash.o\
	../../common/rtmp_swmcu.o\
	../../common/rt_rf.o\
	../../chips/rt305x.o\
	../../chips/rt3352.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	../../ate/chips/rt305x_ate.o\
	../../ate/chips/rt3352_ate.o
	../../ate/common/ate_pci.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	../../os/ecos/rt_pci_rbus.o\
	../../os/ecos/rbus_main_end.o\
	../../os/ecos/rt_rbus_pci_util.o
endif

endif
#endif // RT3352 //

#ifdef RT5350
ifneq ($(findstring 5350,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../common/ee_flash.o\
	../../common/rtmp_swmcu.o\
	../../common/rt_rf.o\
	../../chips/rt305x.o\
	../../chips/rt5350.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	../../ate/chips/rt305x_ate.o\
	../../ate/chips/rt5350_ate.o\
	../../ate/common/ate_pci.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	../../os/ecos/rt_pci_rbus.o\
	../../os/ecos/rbus_main_end.o\
	../../os/ecos/rt_rbus_pci_util.o
endif

endif
#endif // RT5350 //

#ifdef RT6352 // the same MT7620 //
ifeq ($(CHIPSET),6352)
$(MOD_NAME)-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../common/ee_flash.o\
	../../common/rtmp_swmcu.o\
	../../common/rt_rf.o\
	../../common/cmm_loft_cal.o\
	../../common/cmm_rf_cal.o\
	../../chips/rt305x.o\
	../../chips/rt6352.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	../../ate/chips/rt6352_ate.o\
	../../ate/common/ate_pci.o
endif

ifeq ($(HAS_MAC_REPEATER_SUPPORT),y)
$(MOD_NAME)-objs += \
	../../ap/ap_repeater.o
endif

ifeq ($(CONFIG_RT2860V2_AP_MAC_REPEATER),y)
rt2860v2_ap-objs += ../rt2860v2/ap/ap_repeater.o
endif


ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	../../os/ecos/rt_pci_rbus.o\
	../../os/ecos/rbus_main_end.o\
	../../os/ecos/rt_rbus_pci_util.o
endif
endif
#endif // RT6352 //

#ifdef MT7620 // the same RT6352 //
ifeq ($(CHIPSET),7620)
$(MOD_NAME)-objs += \
	../../common/cmm_mac_pci.o\
	../../common/cmm_data_pci.o\
	../../common/ee_flash.o\
	../../common/rtmp_swmcu.o\
	../../common/cmm_loft_cal.o\
	../../common/cmm_rf_cal.o\
	../../common/rt_rf.o\
	../../chips/rt305x.o\
	../../chips/rt6352.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	../../ate/chips/rt6352_ate.o\
	../../ate/common/ate_pci.o
endif

ifeq ($(HAS_MAC_REPEATER_SUPPORT),y)
$(MOD_NAME)-objs += \
	../../ap/ap_repeater.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	../../os/ecos/rt_pci_rbus.o\
	../../os/ecos/rbus_main_end.o\
	../../os/ecos/rt_rbus_pci_util.o
endif
endif
#endif // MT7620 //



# Compiler #
ifeq ($(RT28xx_MODE),AP)
	ALL_OBJS = $($(MOD_NAME)-objs)
endif

EXTRA_CFLAGS = -I../../include -D_KERNEL $(WFLAGS)


all: $(MODULE)

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm -f $(MODULE) $(ALL_OBJS)
	rm -f ../../common/*.o
	rm -f ../../common/*.d
	rm -f ../../os/ecos/*.d
	rm -f ../../os/ecos/.o
	rm -fr ../../os/ecos/.tmp_versions
	rm -f ../../chips/*.o
	rm -f ../../chips/*d
	rm -f ../../ate/chips/*.o
	rm -f ../../ate/chips/*d
	rm -f ../../ate/common/*.o
	rm -f ../../ate/common/*d
	rm -f ../../rate_ctrl/*.o
	rm -f ../../rate_ctrl/*d
ifeq ($(RT28xx_MODE),AP)
	rm -f ../../ap/*.o
	rm -f ../../ap/*.d
else
ifeq ($(RT28xx_MODE),STA)
	rm -f ../../sta/*.o
	rm -f ../../sta/*.d
else
ifeq ($(RT28xx_MODE),APSTA)
	rm -f ../../ap/*.o
	rm -f ../../ap/*.d
	rm -f ../../sta/*.o
	rm -f ../../sta/*d
endif
endif
endif

