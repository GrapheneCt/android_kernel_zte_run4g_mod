/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2012. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

package com.mediatek.phone.plugin;

import android.content.Context;

import com.mediatek.phone.SIMInfoWrapper;
import com.mediatek.phone.ext.EmergencyDialerExtension;
import com.mediatek.phone.ext.PhoneCallOptionHandlerFactoryExtension;
import com.mediatek.phone.ext.PhoneGlobalsExtension;
import com.mediatek.phone.ext.PhonePluginDefault;
import com.mediatek.phone.ext.SettingsExtension;
import com.mediatek.phone.plugin.calloption.PhoneCallOptionHandlerFactoryOP09Extension;

public class OP09PhonePlugin extends PhonePluginDefault {

    private static final String TAG = "OP09PhonePlugin";

    private Context mPluginContext;

    public OP09PhonePlugin(Context context) {
        super(context);
        mPluginContext = context;
        // init SimInfoWrapper
        SIMInfoWrapper.getDefault().init(context);
    }

    public EmergencyDialerExtension createEmergencyDialerExtension() {
        return new EmergencyDialerOP09Extension(mPluginContext);
    }

    public PhoneCallOptionHandlerFactoryExtension createPhoneCallOptionHandlerFactoryExtension() {
        return new PhoneCallOptionHandlerFactoryOP09Extension(mPluginContext);
    }

    public PhoneGlobalsExtension createPhoneGlobalsExtension() {
        return new OP09PhoneGlobalsExtension(mPluginContext);
    }

    public SettingsExtension createSettingsExtension() {
        return new SettingsOP09Extension(mPluginContext);
    }
}
