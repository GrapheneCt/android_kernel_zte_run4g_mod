/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
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

package com.mediatek.dm.fumo;

import android.os.Handler;
import android.util.Log;

import com.mediatek.dm.DmConst.TAG;
import com.mediatek.dm.DmService;
import com.mediatek.dm.data.IDmPersistentValues;
import com.redbend.vdm.DownloadDescriptor;
import com.redbend.vdm.fumo.FumoHandler;
import com.redbend.vdm.fumo.VdmFumo;
import com.redbend.vdm.fumo.FumoOperationResult;

public class DmFumoHandler implements FumoHandler {
    private Handler mMsgHandler;

    public DmFumoHandler() {
        mMsgHandler = DmService.getInstance().mHandler;
    }

    // Interface method of fumohandler.
    // Called by engine when dd is downloaded.
    public boolean confirmDownload(DownloadDescriptor dd, VdmFumo fumo) {
        Log.i(TAG.FUMO, "[DmFumoHandler]:confirmDownload");
        mMsgHandler.sendMessage(mMsgHandler.obtainMessage(
                IDmPersistentValues.MSG_NEWVERSIONDETECTED, dd));
        return false;
    }

    // Interface method of fumohandler.
    // Called by engine when dlpkg is downloaded.
    public boolean confirmUpdate(VdmFumo fumo) {
        Log.i(TAG.FUMO, "[DmFumoHandler]:confirmUpdate download finish");
        mMsgHandler.sendMessage(mMsgHandler.obtainMessage(IDmPersistentValues.MSG_DLPKGCOMPLETE));
        return false;
    }

    // Interface method of fumohandler.
    // Request the UA to start the execution of update.
    // Return value is ignored.
    public FumoOperationResult executeUpdate(String updatePkgPath, VdmFumo fumo) {
        Log.i(TAG.FUMO, "[DmFumoHandler]:executeUpdate(" + updatePkgPath);
        // Has to discuss with Hong-rong to provide the interface.
        return null;
    }
}