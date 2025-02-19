# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.

# MediaTek Inc. (C) 2010. All rights reserved.
#
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
# RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
# AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
#
# The following software/firmware and/or related documentation ("MediaTek Software")
# have been modified by MediaTek Inc. All revisions are subject to any receiver's
# applicable license agreements with MediaTek Inc.


# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    ril_pupack.cpp

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libbinder \
    libcutils \
    libhardware_legacy

ifeq ($(TELEPHONY_DFOSET),yes)
LOCAL_SHARED_LIBRARIES += libdfo
endif  
LOCAL_C_INCLUDES := $(KERNEL_HEADERS) $(LOCAL_PATH)/../../../external/dfo/featured

LOCAL_CFLAGS := -DMTK_RIL

ifeq ($(GEMINI),yes)	
  LOCAL_CFLAGS := -D_GNU_SOURCE -DMTK_RIL -DMTK_GEMINI -D__CCMNI_SUPPORT__
else
  LOCAL_CFLAGS := -D_GNU_SOURCE -DMTK_RIL -D__CCMNI_SUPPORT__

  ifneq ($(MTK_SHARE_MODEM_SUPPORT),1)
        LOCAL_CFLAGS += -DMTK_GEMINI     
  endif
endif
LOCAL_CFLAGS += -D__ATCI_CHANNEL_SUPPORT__

LOCAL_MODULE:= libutilrilmtk

LOCAL_LDLIBS += -lpthread

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)


# For RdoServD which needs a static library
# =========================================
ifneq ($(ANDROID_BIONIC_TRANSITION),)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    ril_pupack.cpp

LOCAL_STATIC_LIBRARIES := \
    libutils_static \
    libcutils

ifeq ($(TELEPHONY_DFOSET),yes)
LOCAL_SHARED_LIBRARIES += libdfo
endif  
LOCAL_C_INCLUDES := $(KERNEL_HEADERS) $(LOCAL_PATH)/../../../external/dfo/featured

LOCAL_CFLAGS := -DMTK_RIL

ifeq ($(GEMINI),yes)	
  LOCAL_CFLAGS := -D_GNU_SOURCE -DMTK_RIL -DMTK_GEMINI -D__CCMNI_SUPPORT__
else
  LOCAL_CFLAGS := -D_GNU_SOURCE -DMTK_RIL -D__CCMNI_SUPPORT__

  ifneq ($(MTK_SHARE_MODEM_SUPPORT),1)
        LOCAL_CFLAGS += -DMTK_GEMINI     
  endif
endif
LOCAL_CFLAGS += -D__ATCI_CHANNEL_SUPPORT__

LOCAL_MODULE:= libutilrilmtk_static

LOCAL_LDLIBS += -lpthread

include $(BUILD_STATIC_LIBRARY)

# SGLTE project
#======================
ifeq ($(MTK_LTE_DC_SUPPORT),yes)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    ril_pupack.cpp

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libbinder \
    libcutils \
    libhardware_legacy

ifeq ($(TELEPHONY_DFOSET),yes)
LOCAL_SHARED_LIBRARIES += libdfo
endif  
LOCAL_C_INCLUDES := $(KERNEL_HEADERS) $(LOCAL_PATH)/../../../external/dfo/featured

LOCAL_CFLAGS := -DMTK_RIL
LOCAL_CFLAGS += -DMTK_CSFB_IN_MMDC_PROJECT
LOCAL_CFLAGS += -UMTK_LTE_DC_SUPPORT
LOCAL_CFLAGS += -D__ATCI_CHANNEL_SUPPORT__

LOCAL_MODULE:= libutilrilmtk-s

LOCAL_LDLIBS += -lpthread

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
endif # MTK_LTE_DC_SUPPORT
endif # ANDROID_BIONIC_TRANSITION
