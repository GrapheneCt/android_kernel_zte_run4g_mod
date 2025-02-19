/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.mediatek.cta.camera;

import android.app.Activity;
import android.app.KeyguardManager;
import android.content.Context;
import android.content.Intent;
import android.hardware.Camera;
import android.os.Bundle;
import android.util.Log;


/**
 * Superclass of Camera and VideoCamera activities.
 */
public abstract class ActivityBase extends Activity {
    public static final String TAG = "ActivityBase";
    protected Camera mCameraDevice;
    private int mResultCodeForTesting;
    private boolean mOnResumePending;
    private Intent mResultDataForTesting;

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        Log.v(TAG, "onWindowFocusChanged.hasFocus=" + hasFocus + ".mOnResumePending=" + mOnResumePending);
        if (hasFocus && mOnResumePending) {
            doOnResume();
            mOnResumePending = false;
        }
    }

    @Override
    public boolean onSearchRequested() {
        return false;
    }

    @Override
    protected void onResume() {
        super.onResume();
        // Don't grab the camera if in use by lockscreen. For example, face
        // unlock may be using the camera. Camera may be already opened in
        // onCreate. doOnResume should continue if mCameraDevice != null.
        // Suppose camera app is in the foreground. If users turn off and turn
        // on the screen very fast, camera app can still have the focus when the
        // lock screen shows up. The keyguard takes input focus, so the caemra
        // app will lose focus when it is displayed.
        Log.v(TAG, "onResume. hasWindowFocus()=" + hasWindowFocus());
        if (mCameraDevice == null && isKeyguardLocked()) {
            Log.v(TAG, "onResume. mOnResumePending=true");
            mOnResumePending = true;
        } else {
            Log.v(TAG, "onResume. mOnResumePending=false");
            doOnResume();
            mOnResumePending = false;
        }
    }

    // Put the code of onResume in this method.
    protected abstract void doOnResume();

    @Override
    protected void onPause() {
        Log.v(TAG, "onPause");
        super.onPause();
        mOnResumePending = false;
    }

    private boolean isKeyguardLocked() {
        KeyguardManager kgm = (KeyguardManager) getSystemService(Context.KEYGUARD_SERVICE);
        if (kgm != null) {
            Log.v(TAG,
                    "kgm.isKeyguardLocked()=" + kgm.isKeyguardLocked() + ". kgm.isKeyguardSecure()="
                            + kgm.isKeyguardSecure());
        }
        // isKeyguardSecure excludes the slide lock case.
        return (kgm != null) && kgm.isKeyguardLocked() && kgm.isKeyguardSecure();
    }
}
