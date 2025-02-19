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

#ifndef _MTK_HARDWARE_INCLUDE_MTKCAM_MODES_H_
#define _MTK_HARDWARE_INCLUDE_MTKCAM_MODES_H_


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {


/**
 * @enum EAppMode
 * @brief App Mode Enumeration.
 *
 */
enum EAppMode
{
    eAppMode_DefaultMode = 0,                       /*!< Default Mode */
    eAppMode_EngMode,                               /*!< Engineer Mode */
    eAppMode_AtvMode,                               /*!< ATV Mode */
    eAppMode_StereoMode,                            /*!< Stereo Mode */
    eAppMode_VtMode,                                /*!< VT Mode */
    eAppMode_PhotoMode,                             /*!< Photo Mode */
    eAppMode_VideoMode,                             /*!< Video Mode */
    eAppMode_ZsdMode,                               /*!< ZSD Mode */
    eAppMode_FactoryMode,                           /*!< Factory Mode */
};


/**
 * @enum EShotMode
 * @brief Shot Mode Enumeration.
 *
 */
enum EShotMode
{
    eShotMode_NormalShot,                           /*!< Normal Shot */
    eShotMode_ContinuousShot,                       /*!< Continuous Shot Ncc*/
    eShotMode_ContinuousShotCc,                     /*!< Continuous Shot Cc*/
    eShotMode_BestShot,                             /*!< Best Select Shot */
    eShotMode_EvShot,                               /*!< Ev-bracketshot Shot */
    eShotMode_SmileShot,                            /*!< Smile-detection Shot */
    eShotMode_HdrShot,                              /*!< High-dynamic-range Shot */
    eShotMode_AsdShot,                              /*!< Auto-scene-detection Shot */
    eShotMode_ZsdShot,                              /*!< Zero-shutter-delay Shot */
    eShotMode_FaceBeautyShot,                       /*!<  Face-beautifier Shot */
    eShotMode_Mav,                                  /*!< Multi-angle view Shot */
    eShotMode_Autorama,                             /*!< Auto-panorama Shot */
    eShotMode_MultiMotionShot,                      /*!< Multi-motion Shot */
    eShotMode_Panorama3D,                           /*!< Panorama 3D Shot */
    eShotMode_Single3D,                             /*!< Single Camera 3D Shot */
    eShotMode_EngShot,                              /*!< Engineer Mode Shot */
    eShotMode_VideoSnapShot,                        /*!< Video Snap Shot */
};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace NSCam
#endif  //_MTK_HARDWARE_INCLUDE_MTKCAM_MODES_H_

