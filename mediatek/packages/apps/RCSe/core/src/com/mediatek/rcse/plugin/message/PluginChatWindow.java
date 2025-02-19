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

package com.mediatek.rcse.plugin.message;

import org.gsma.joyn.chat.ChatMessage;

import android.content.Intent;
import android.net.Uri;
import android.os.RemoteException;
import android.telephony.TelephonyManager;

import com.mediatek.rcse.api.Logger;
import com.mediatek.rcse.service.binder.IRemoteChatWindow;
import com.mediatek.rcse.service.binder.IRemoteReceivedChatMessage;
import com.mediatek.rcse.service.binder.IRemoteSentChatMessage;

import com.orangelabs.rcs.platform.AndroidFactory;

/**
 * Plugin Group Chat Window
 */
public class PluginChatWindow extends IRemoteChatWindow.Stub {

    private static final String TAG = "PluginChatWindow";
    private IpMessageManager mMessageManager;
    public String msubject = "Group Chat";
    public boolean mNewGroupCreate = false;

    public boolean ismNewGroupCreate() {
		return mNewGroupCreate;
	}
	public void setmNewGroupCreate(boolean mNewGroupCreate) {
		this.mNewGroupCreate = mNewGroupCreate;
	}
    public PluginChatWindow(IpMessageManager messageManager) {
        mMessageManager = messageManager;
    }
    @Override
    public void addLoadHistoryHeader(boolean showLoader) throws RemoteException {
        Logger.v(TAG, "addLoadHistoryHeader(), showLoader = " + showLoader);
    }

    @Override
    public IRemoteReceivedChatMessage addReceivedMessage(ChatMessage message, boolean isRead) throws RemoteException {
        Logger.v(TAG, "addReceivedMessage(), message = " + message + "isRead = " + isRead);
        PluginReceivedChatMessage receivedChatMessage = new PluginReceivedChatMessage(message,
                isRead);
        return receivedChatMessage;
    }

    @Override
    public IRemoteSentChatMessage addSentMessage(ChatMessage message, int messageTag) throws RemoteException {
        Logger.v(TAG, "addSentMessage(), message = " + message + " messageTag = " + messageTag);
        if (messageTag == 0) {
            final Uri uri = Uri.fromParts("smsto", message.getContact(), null);
            Intent intent = new Intent(TelephonyManager.ACTION_RESPOND_VIA_MESSAGE, uri);
            intent.putExtra(Intent.EXTRA_TEXT, message.getMessage());
            intent.putExtra("showUI", false);
            AndroidFactory.getApplicationContext().startService(intent);
            return null;
        }
        PluginSentChatMessage sentChatMessage ;
        if(ismNewGroupCreate())
        {
        	mNewGroupCreate = false;
        	sentChatMessage = new PluginSentChatMessage(mMessageManager, message, messageTag,msubject);
        }
        else        	
        	sentChatMessage = new PluginSentChatMessage(mMessageManager, message, messageTag);
        return sentChatMessage;
    }

    @Override
    public void removeAllMessages() throws RemoteException {
        Logger.v(TAG, "removeAllMessages()");
    }

    @Override
    public void updateAllMsgAsRead() throws RemoteException {
        Logger.v(TAG, "updateAllMsgAsRead()");

    }

	public void addgroupSubject(String subject) {
		msubject = subject;
		Logger.v(TAG, "addgroupSubject() PluginChatWindow");
	}
    

    @Override
    public IRemoteSentChatMessage getSentChatMessage(String messageId) throws RemoteException {
        Logger.v(TAG, "getSentChatMessage() messageId =" + messageId);
        return null;
    }

}