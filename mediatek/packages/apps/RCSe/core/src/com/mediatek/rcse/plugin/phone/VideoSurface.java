/*******************************************************************************
 * Software Name : RCS IMS Stack
 *
 * Copyright (C) 2010 France Telecom S.A.
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
 ******************************************************************************/

package com.mediatek.rcse.plugin.phone;

import android.graphics.Bitmap;

/**
 * Video Surface interface
 *
 * @author Deutsche Telekom
 */
public interface VideoSurface {
    /**
     * Set image from a bitmap
     *
     * @param bmp Bitmap
     */
    public void setImage(Bitmap bmp);

    /**
     * Clears the image
     */
    public void clearImage();
}
