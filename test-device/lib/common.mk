ifndef TARGET_VENDOR
TARGET_VENDOR := telpo
endif

#$(call inherit-product, device/qcom/msm8953_64/msm8953_64.mk) 
#$(call inherit-product-if-exists, device/telpo/common/version.mk)


#----------------------------------------------------------------------
# SD450 AUDIO SPK
#----------------------------------------------------------------------
#SD450_AUDIO := true
#$(warning "---SD450_AUDIO= $(SD450_AUDIO)---")

#----------------------------------------------------------------------
# Sepolicy
#----------------------------------------------------------------------
BOARD_SEPOLICY_DIRS += device/telpo/sepolicy

#----------------------------------------------------------------------
# Common Linux Kernel modules
#----------------------------------------------------------------------
TELPO_COMMON_DLKM := gpio-ctrl.ko
TELPO_COMMON_DLKM += switch_gpios.ko
TELPO_COMMON_DLKM += wiegand.ko
TELPO_COMMON_DLKM += pn512_i2c.ko

#----------------------------------------------------------------------
# Service
#----------------------------------------------------------------------
TELPO_COMMON_SERVICE += get_mac 
TELPO_COMMON_SERVICE += set_atvalue 
TELPO_COMMON_SERVICE += ext_wdg      #添加外部看门狗

#----------------------------------------------------------------------
# Rootdir
#----------------------------------------------------------------------
TELPO_COMMON_INIT := init.telpo.rc
TELPO_COMMON_INIT += init.telpo.boot.sh
TELPO_COMMON_INIT += init.telpo.bootcomplete.sh


#----------------------------------------------------------------------
# App
#----------------------------------------------------------------------
TELPO_COMMON_APP := Bing_Search
TELPO_COMMON_APP += google.pinyin
TELPO_COMMON_APP += UpdateService

#----------------------------------------------------------------------
# JNI lib
#----------------------------------------------------------------------
TELPO_COMMON_JNI := libfaceutil
TELPO_COMMON_JNI += libNfcRd
#


PRODUCT_PACKAGES += $(TELPO_COMMON_DLKM)
PRODUCT_PACKAGES += $(TELPO_COMMON_SERVICE)
PRODUCT_PACKAGES += $(TELPO_COMMON_INIT)
PRODUCT_PACKAGES += $(TELPO_COMMON_APP)
PRODUCT_PACKAGES += $(TELPO_COMMON_JNI)

#----------------------------------------------------------------------
# Property overrides
#----------------------------------------------------------------------
# adb tcp debug
PRODUCT_PROPERTY_OVERRIDES += service.adb.tcp.port=5555

# telpo system version property
PRODUCT_PROPERTY_OVERRIDES += ro.telpo.version=1.0.$(TELPO_BUILD_NUM)_$(TELPO_BUILD_DATA)

# 固件版本号，对接OTAServices
PRODUCT_PROPERTY_OVERRIDES += ro.product.version=1.0.$(TELPO_BUILD_NUM)
PRODUCT_PROPERTY_OVERRIDES += ro.radio.platform=msm8953

#USB开机默认为OTG模式
PRODUCT_PROPERTY_OVERRIDES += persist.usb.otg=1	

