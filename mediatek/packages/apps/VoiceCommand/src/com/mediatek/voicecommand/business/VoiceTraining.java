/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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
package com.mediatek.voicecommand.business;

import android.os.Bundle;
import android.os.Handler;
import android.util.Log;

import com.mediatek.common.voicecommand.VoiceCommandListener;
import com.mediatek.voicecommand.adapter.IVoiceAdapter;
import com.mediatek.voicecommand.data.DataPackage;
import com.mediatek.voicecommand.mgr.ConfigurationManager;
import com.mediatek.voicecommand.mgr.IMessageDispatcher;
import com.mediatek.voicecommand.mgr.VoiceMessage;
import com.mediatek.voicecommand.service.VoiceCommandManagerStub;

import java.io.File;

public class VoiceTraining extends VoiceCommandBusiness {

    private IVoiceAdapter mJniAdapter;

    public VoiceTraining(IMessageDispatcher dispatcher,
            ConfigurationManager cfgMgr, Handler handler, IVoiceAdapter adapter) {
        super(dispatcher, cfgMgr, handler);
        mJniAdapter = adapter;
        // TODO Auto-generated constructor stub
    }

    @Override
    public int handleSyncVoiceMessage(VoiceMessage message) {
        // TODO Auto-generated method stub
        int errorid = VoiceCommandListener.VOICE_NO_ERROR;
        switch (message.mSubAction) {
        case VoiceCommandListener.ACTION_VOICE_TRAINING_START:
        case VoiceCommandListener.ACTION_VOICE_TRAINING_INTENSITY:
        case VoiceCommandListener.ACTION_VOICE_TRAINING_STOP:
        case VoiceCommandListener.ACTION_VOICE_TRAINING_RESET:
        case VoiceCommandListener.ACTION_VOICE_TRAINING_MODIFY:
        case VoiceCommandListener.ACTION_VOICE_TRAINING_FINISH:
            sendMessageToHandler(message);
            break;
        case VoiceCommandListener.ACTION_VOICE_TRAINING_PSWDFILE:
            getPasswordFilePath(message);
            break;
        default:
            // do nothing because illeage action has been filter in
            // AppDataManager
            break;
        }
        return errorid;
    }

    @Override
    public int handleAsyncVoiceMessage(VoiceMessage message) {
        // TODO Auto-generated method stub
        int errorid = VoiceCommandListener.VOICE_NO_ERROR;
        switch (message.mSubAction) {
        case VoiceCommandListener.ACTION_VOICE_TRAINING_START:
            handleTrainingStart(message);
            break;
        case VoiceCommandListener.ACTION_VOICE_TRAINING_INTENSITY:
            handleTrainingIntensity(message);
            break;
        case VoiceCommandListener.ACTION_VOICE_TRAINING_STOP:
            handleTrainingStop(message);
            break;
        case VoiceCommandListener.ACTION_VOICE_TRAINING_RESET:
            handleTrainingReset(message);
            break;
        case VoiceCommandListener.ACTION_VOICE_TRAINING_MODIFY:
            handleTrainingModify(message);
            break;
        case VoiceCommandListener.ACTION_VOICE_TRAINING_FINISH:
            handleTrainingFinish(message);
            break;
        default:
            break;
        }
        return errorid;
    }

    private int handleTrainingStart(VoiceMessage message) {
        int errorid = VoiceCommandListener.VOICE_NO_ERROR;

        if (message.mExtraData == null) {
            errorid = VoiceCommandListener.VOICE_ERROR_COMMON_INVALIDDATA;
        } else {
            int commandid = message.mExtraData
                    .getInt(VoiceCommandListener.ACTION_EXTRA_SEND_INFO);
            int trainingMode = message.mExtraData
                    .getInt(VoiceCommandListener.ACTION_EXTRA_SEND_INFO1);
            int[] commandMask = message.mExtraData
                    .getIntArray(VoiceCommandListener.ACTION_EXTRA_SEND_INFO2);
            if (commandid < 0) {
                errorid = VoiceCommandListener.VOICE_ERROR_COMMON_INVALIDDATA;
            } else {
                String pwdpath = mCfgMgr.getPasswordFilePath(trainingMode);
                String patternpath = mCfgMgr
                        .getVoiceRecognitionPatternFilePath(trainingMode);
                String featurepath = mCfgMgr.getFeatureFilePath(trainingMode);
                String umbpath = mCfgMgr.getUBMFilePath();
                String wakeupinfoPath = mCfgMgr.getWakeupInfoPath();
                if (pwdpath == null || patternpath == null
                        || featurepath == null || umbpath == null
                        || wakeupinfoPath == null) {
                    Log.i(VoiceCommandManagerStub.TAG,
                            "handleTrainingStart error pwdpath=" + pwdpath
                                    + " patternpath=" + patternpath
                                    + " featurepath=" + featurepath
                                    + " umbpath=" + umbpath
                                    + " wakeupinfoPath =" + wakeupinfoPath);
                    errorid = VoiceCommandListener.VOICE_ERROR_COMMON_SERVICE;
                } else {
                    errorid = mJniAdapter.startVoiceTraining(pwdpath,
                            patternpath, featurepath, umbpath, commandid,
                            commandMask, trainingMode, wakeupinfoPath,
                            message.mPkgName, message.pid);
                }
            }
        }
        sendMessageToApps(message, errorid);

        return errorid;
    }

    private int handleTrainingStop(VoiceMessage message) {
        mJniAdapter.stopVoiceTraining(message.mPkgName, message.pid);

        message.mExtraData = DataPackage.packageSuccessResult();

        mDispatcher.dispatchMessageUp(message);

        return VoiceCommandListener.VOICE_NO_ERROR;
    }

    private int handleTrainingIntensity(VoiceMessage message) {

        int intensity = mJniAdapter.getNativeIntensity();
        message.mExtraData = DataPackage.packageResultInfo(
                VoiceCommandListener.ACTION_EXTRA_RESULT_SUCCESS, intensity, 0);

        mDispatcher.dispatchMessageUp(message);
        return VoiceCommandListener.VOICE_NO_ERROR;
    }

    private int getPasswordFilePath(VoiceMessage message) {
        int errorid = VoiceCommandListener.VOICE_NO_ERROR;
        Bundle bundle = null;
        if (message.mExtraData == null) {
            errorid = VoiceCommandListener.VOICE_ERROR_COMMON_INVALIDDATA;
            bundle = DataPackage.packageErrorResult(errorid);
        } else {
            int commandid = message.mExtraData
                    .getInt(VoiceCommandListener.ACTION_EXTRA_SEND_INFO);
            int trainingMode = message.mExtraData
                    .getInt(VoiceCommandListener.ACTION_EXTRA_SEND_INFO1);
            String passwordpath = mCfgMgr.getPasswordFilePath(trainingMode);
            if (commandid < 0 || passwordpath == null) {
                errorid = VoiceCommandListener.VOICE_ERROR_COMMON_INVALIDDATA;
                bundle = DataPackage.packageErrorResult(errorid);
            } else {
                String path = passwordpath + commandid + ".dat";
                File file = new File(path);
                if (file.exists()) {
                    bundle = DataPackage.packageResultInfo(
                            VoiceCommandListener.ACTION_EXTRA_RESULT_SUCCESS,
                            path, null);
                } else {
                    errorid = VoiceCommandListener.VOICE_ERROR_COMMON_INVALIDDATA;
                    bundle = DataPackage.packageErrorResult(errorid);
                }
            }
        }
        message.mExtraData = bundle;
        mDispatcher.dispatchMessageUp(message);

        return errorid;
    }

    private int handleTrainingReset(VoiceMessage message) {
        int errorid = VoiceCommandListener.VOICE_NO_ERROR;

        if (message.mExtraData == null) {
            errorid = VoiceCommandListener.VOICE_ERROR_COMMON_INVALIDDATA;
        } else {
            int commandid = message.mExtraData
                    .getInt(VoiceCommandListener.ACTION_EXTRA_SEND_INFO);
            int trainingMode = message.mExtraData
                    .getInt(VoiceCommandListener.ACTION_EXTRA_SEND_INFO1);
            if (commandid < 0) {
                errorid = VoiceCommandListener.VOICE_ERROR_COMMON_INVALIDDATA;
            } else {
                String pwdpath = mCfgMgr.getPasswordFilePath(trainingMode);
                String patternpath = mCfgMgr
                        .getVoiceRecognitionPatternFilePath(trainingMode);
                String featurepath = mCfgMgr.getFeatureFilePath(trainingMode);

                if (pwdpath == null || patternpath == null
                        || featurepath == null) {
                    Log.i(VoiceCommandManagerStub.TAG,
                            "handleTrainingReset error pwdpath=" + pwdpath
                                    + " patternpath=" + patternpath
                                    + " featurepath=" + featurepath);
                    errorid = VoiceCommandListener.VOICE_ERROR_COMMON_SERVICE;
                } else {
                    errorid = mJniAdapter.resetVoiceTraining(pwdpath,
                            patternpath, featurepath, commandid);
                }
            }
        }
        sendMessageToApps(message, errorid);
        return errorid;
    }

    private int handleTrainingModify(VoiceMessage message) {
        int errorid = VoiceCommandListener.VOICE_NO_ERROR;

        if (message.mExtraData == null) {
            errorid = VoiceCommandListener.VOICE_ERROR_COMMON_INVALIDDATA;
        } else {
            int commandid = message.mExtraData
                    .getInt(VoiceCommandListener.ACTION_EXTRA_SEND_INFO);
            int trainingMode = message.mExtraData
                    .getInt(VoiceCommandListener.ACTION_EXTRA_SEND_INFO1);
            if (commandid < 0) {
                errorid = VoiceCommandListener.VOICE_ERROR_COMMON_INVALIDDATA;
            } else {
                String pwdpath = mCfgMgr.getPasswordFilePath(trainingMode);
                String patternpath = mCfgMgr
                        .getVoiceRecognitionPatternFilePath(trainingMode);
                String featurepath = mCfgMgr.getFeatureFilePath(trainingMode);

                if (pwdpath == null || patternpath == null
                        || featurepath == null) {
                    Log.i(VoiceCommandManagerStub.TAG,
                            "handleTrainingModify error pwdpath=" + pwdpath
                                    + " patternpath=" + patternpath
                                    + " featurepath=" + featurepath);
                    errorid = VoiceCommandListener.VOICE_ERROR_COMMON_SERVICE;
                } else {
                    errorid = mJniAdapter.modifyVoiceTraining(pwdpath,
                            patternpath, featurepath, commandid);
                }
            }
        }
        sendMessageToApps(message, errorid);
        return errorid;
    }

    private int handleTrainingFinish(VoiceMessage message) {
        int errorid = mJniAdapter.finishVoiceTraining(message.mPkgName,
                message.pid);
        sendMessageToApps(message, errorid);
        return errorid;
    }

}
