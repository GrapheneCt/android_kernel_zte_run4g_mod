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

package com.mediatek.mms.plugin;

import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.Telephony.Sms;

import com.mediatek.common.featureoption.FeatureOption;
import com.mediatek.encapsulation.MmsLog;
import com.mediatek.mms.ext.DefaultResendSmsExt;

public class Op09ResendSmsExt extends DefaultResendSmsExt {
    private static final String TAG = "Mms/Op09ResendSmsExt";

    public Op09ResendSmsExt(Context context) {
        super(context);
    }

    public void resendMessage(final Context context, final Uri uri, final int simId) {
        MmsLog.d(TAG, "ResendSmsExt.resendMessage");

        new Thread(new Runnable() {
            public void run() {
                // Update the date.
                Long date = System.currentTimeMillis();
                ContentValues values = new ContentValues(1);
                values.put(Sms.DATE, date);
                context.getContentResolver().update(uri, values, null, null);

                // Move the failed message from failed box to queued box.
                boolean isMoved = Sms.moveMessageToFolder(context, uri, Sms.MESSAGE_TYPE_QUEUED, 0);
                MmsLog.d(TAG, "Move message to queued box: " + isMoved);

                // Send Broadcast and handled by the SmsReceiverService
                if (FeatureOption.MTK_GEMINI_SUPPORT) {
                    Intent sentIt = new Intent(RESEND_MESSAGE_ACTION);
                    sentIt.putExtra("simId", simId);
                    context.sendBroadcast(sentIt);
                } else {
                    context.sendBroadcast(new Intent(RESEND_MESSAGE_ACTION));
                }
            }
        }).start();

    }

}
