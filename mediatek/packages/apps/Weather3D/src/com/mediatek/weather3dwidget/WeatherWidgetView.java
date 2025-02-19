/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

package com.mediatek.weather3dwidget;

import android.content.Context;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.RemoteViews.RemoteView;

import com.mediatek.common.widget.IMtkWidget;

@RemoteView
public class WeatherWidgetView extends FrameLayout implements IMtkWidget{
    private static final String TAG = "W3D/WeatherWidgetView";

    private static final int SCREEN_UNKNOWN = -10000;

    private WeatherView mWeatherView;
    private ImageView mImageView;

    private int mAppWidgetScreen = SCREEN_UNKNOWN;
    private int mAppWidgetId = -1;

    public WeatherWidgetView(Context context) {
        super(context);
        LogUtil.v(TAG, "WeatherWidgetView - 1");
    }

    public WeatherWidgetView(Context context, AttributeSet attrs) {
        super(context, attrs);
        LogUtil.v(TAG, "WeatherWidgetView - 2");
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        LogUtil.i(TAG, "onFinishInflate");

        mWeatherView = (WeatherView)this.findViewWithTag("weather_view");
        mImageView = (ImageView)this.findViewWithTag("snapshot");
        mImageView.setVisibility(View.INVISIBLE);
    }

    public int getPermittedCount() {
        return 1;
    }

    public void setScreen(int screen) {
        mAppWidgetScreen = screen;
    }

    public int getScreen() {
        return mAppWidgetScreen;
    }

    public void setWidgetId(int widgetId) {
        mAppWidgetId = widgetId;
    }

    public int getWidgetId() {
        return mAppWidgetId;
    }
    public void startDrag() {}
    public void stopDrag() {}
    public boolean moveOut(int curScreen) {
        return true;
    }
    public void moveIn(int curScreen) {}
    public void startCovered(int curScreen) {}
    public void stopCovered(int curScreen) {}
    public void onPauseWhenShown(int curScreen) {}
    public void onResumeWhenShown(int curScreen) {}
    public void onSaveInstanceState(Bundle outSate) {}
    public void onRestoreInstanceState(Bundle state) {}
    public void leaveAppwidgetScreen() {}
    public void enterAppwidgetScreen() {}

    private void pause() {
        mImageView.setImageBitmap(mWeatherView.getBitmap());
        mImageView.setVisibility(View.VISIBLE);
        mWeatherView.setVisibility(View.INVISIBLE);
        mWeatherView.pauseRendering();
    }

    private void resume() {
        mWeatherView.resumeRendering();
        mWeatherView.setVisibility(View.VISIBLE);
        mImageView.setVisibility(View.INVISIBLE);
    }

    @Override
    protected void onVisibilityChanged(View changedView, int visibility) {
        LogUtil.v(TAG, "onVisibilityChanged - " + visibility);
        super.onVisibilityChanged(changedView, visibility);
        if (visibility == VISIBLE) {
            resume();
        } else {
            pause();
        }
    }
}

