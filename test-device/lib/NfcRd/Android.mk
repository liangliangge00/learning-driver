#
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libNfcRd
LOCAL_SRC_FILES := \
	NfcRd.cpp \
	pn512.cpp \
	pn512_io.cpp \
	NfcTypeA.cpp \
	NfcTypeB.cpp \
	MifareCard.cpp \
	yl0301.cpp 
LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	liblog 
LOCAL_C_INCLUDES += \
	$(JNI_H_INCLUDE) \
	$(call include-path-for, audio-effects) \
	$(JNI_H_INCLUDE)/../include/nativehelper
LOCAL_CFLAGS := -DNDEBUG 
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE:= NfcRd_test
LOCAL_SRC_FILES := \
	test.cpp \
	pn512.cpp \
	pn512_io.cpp \
	NfcTypeA.cpp \
	NfcTypeB.cpp \
	MifareCard.cpp \
	yl0301.cpp 
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	liblog 
include $(BUILD_EXECUTABLE)

$(warning 'LOCAL_C_INCLUDES = $(LOCAL_C_INCLUDES)')
