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
package com.mediatek.rcse.service.binder;


import org.gsma.joyn.chat.ChatMessage;
import com.mediatek.rcse.service.binder.IRemoteReceivedChatMessage;
import com.mediatek.rcse.service.binder.IRemoteSentChatMessage;
import com.mediatek.rcse.service.binder.FileStructForBinder;
import com.mediatek.rcse.service.binder.IRemoteFileTransfer;

/**
 * This interface defines one type of chat window that used for a 1-2-1 chat
 */
interface IRemoteOne2OneChatWindow {

    /**
     * Set whether the file transfer function should be enabled
     * 
     * @param isEnable Enable or disable the file transfer function
     */
    void setFileTransferEnable(int reason);

    /**
     * Set whether the contact is composing
     * 
     * @param isComposing Whether this contact is composing
     */
    void setIsComposing(boolean isComposing);

    /**
     * Set whether the remote contact is offline
     * 
     * @param isOffline Whether need to show the offline reminder
     */
    void setRemoteOfflineReminder(boolean isOffline);

    /**
     * Add a sent file transfer into VIEW module
     * 
     * @param file The file to be sent
     * @return A IFileTransfer to MODEL module
     */
    IRemoteFileTransfer addSentFileTransfer(in FileStructForBinder file);

    /**
     * Add a received file transfer into VIEW module
     * 
     * @param file The file received
     * @return A IFileTransfer to MODEL module
     */
    IRemoteFileTransfer addReceivedFileTransfer(in FileStructForBinder file, boolean isAutoAccept);
    
    /**
     * Add a received message into this chat window.
     * 
     * @param message The message to add.
     * @param isRead identify if the message is read.
     * @return IReceivedChatMessage converted from ChatMessage.
     */
    IRemoteReceivedChatMessage addReceivedMessage(in ChatMessage message, boolean isRead);

    /**
     * Add a received message into this chat window.
     * 
     * @param message The message to add.
     * @param messageTag The tag of this message
     * @return ISentChatMessage converted from ChatMessage.
     */
    IRemoteSentChatMessage addSentMessage(in ChatMessage message, int messageTag);

    /**
     * Remove all messages from this chat window.
     */
    void removeAllMessages();

    /**
     * Get a single piece of message interface in this chat window
     * 
     * @param index The index of the message you want to get
     * @return IChatMessage if the message of the index exists, otherwise
     *         null
     */
    IRemoteSentChatMessage getSentChatMessage(String messageId);

    /**
     * add load history view.
     */
    void addLoadHistoryHeader(boolean showLoader);

    /**
     * update all the message as read.
     */
    void updateAllMsgAsRead();
}
