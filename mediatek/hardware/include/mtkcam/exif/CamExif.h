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

/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#ifndef _MTK_CAMERA_INC_COMMON_CAMEXIF_CAMEXIF_H_
#define _MTK_CAMERA_INC_COMMON_CAMEXIF_CAMEXIF_H_

//
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
//

using namespace android;

/*******************************************************************************
*
********************************************************************************/
struct exifAPP1Info_s;
struct DEBUG_CMN_INFO_S;
//
class IBaseExif;

/*******************************************************************************
*
********************************************************************************/
enum eDebugExifId {
    //
    ID_ERROR        = 0x00000001,
    //
    ID_EIS          = 0x00000002,
    ID_AAA          = 0x00000004,
    ID_ISP          = 0x00000008,
    ID_CMN          = 0x00000010,
    ID_MF           = 0x00000020,
    ID_N3D          = 0x00000040,
    ID_SENSOR       = 0x00000080,
    ID_RESERVE1     = 0x00000100,
    ID_RESERVE2     = 0x00000200,
    ID_RESERVE3     = 0x00000400,
    ID_SHAD_TABLE   = 0x00001000
    //
};

struct ExifIdMap
{
    typedef MUINT32     VAL_T;
    typedef String8     STR_T;
    //
                        ExifIdMap()
                        : m_vStr2Val()
                        , m_vVal2Str()
                    {
                        m_vStr2Val.clear();
                        m_vVal2Str.clear();
#define _ADD_STRING_VALUE_MAP_(_str_, _val_) \
                        do {\
                            m_vStr2Val.add(String8(_str_), _val_); \
                            m_vVal2Str.add(_val_, String8(_str_)); \
                        } while (0)
                        _ADD_STRING_VALUE_MAP_("ERROR",         ID_ERROR);
                        _ADD_STRING_VALUE_MAP_("AAA",           ID_AAA);
                        _ADD_STRING_VALUE_MAP_("ISP",           ID_ISP);
                        _ADD_STRING_VALUE_MAP_("COMMON",        ID_CMN);
                        _ADD_STRING_VALUE_MAP_("MF",            ID_MF);
                        _ADD_STRING_VALUE_MAP_("N3D",           ID_N3D);
                        _ADD_STRING_VALUE_MAP_("SENSOR",        ID_SENSOR);
                        _ADD_STRING_VALUE_MAP_("EIS",           ID_EIS);
                        _ADD_STRING_VALUE_MAP_("SHAD/RESERVE1", ID_RESERVE1);
                        _ADD_STRING_VALUE_MAP_("RESERVE2",      ID_RESERVE2);
                        _ADD_STRING_VALUE_MAP_("RESERVE3",      ID_RESERVE3);
                        _ADD_STRING_VALUE_MAP_("SHAD_TABLE",    ID_SHAD_TABLE);
#undef _ADD_STRING_VALUE_MAP_
                    }
    virtual             ~ExifIdMap() {}
    //
    virtual VAL_T   valueFor(STR_T const& str) const    { return m_vStr2Val.valueFor(str); }
    virtual STR_T   stringFor(VAL_T const& val) const   { return m_vVal2Str.valueFor(val); }
    //
private:
    DefaultKeyedVector<STR_T, VAL_T>    m_vStr2Val;
    DefaultKeyedVector<VAL_T, STR_T>    m_vVal2Str;
};


/*******************************************************************************
*
********************************************************************************/
struct DbgInfo
{
    mutable MUINT8*     puDbgBuf;
    MUINT32             u4BufSize;

    DbgInfo(MUINT8* _puDbgBuf = NULL, MUINT32 _u4BufSize = 0) : 
        puDbgBuf(_puDbgBuf), u4BufSize(_u4BufSize)
    {
    }
};

/*******************************************************************************
*
********************************************************************************/
struct CamDbgParam
{
    MUINT32    u4ShotMode;      // Shot mode
    MUINT32    u4CamMode;       // Camera mode: Normal, Engineer
    MBOOL      bStereoCam;      // StereoCam feature.

    CamDbgParam(
                MUINT32 _u4ShotMode = 0, 
                MUINT32 _u4CamMode = 0,
                MBOOL _bStereoCam = MFALSE
            )
        : u4ShotMode(_u4ShotMode)
        , u4CamMode(_u4CamMode)
        , bStereoCam(_bStereoCam)
    {
    }
};

/*******************************************************************************
*
********************************************************************************/
struct CamExifParam {

public:     ////    Data Members.
    
    MUINT32     u4GpsIsOn;
    MUINT32     u4GPSAltitude;
    MUINT8      uGPSLatitude[32];
    MUINT8      uGPSLongitude[32];
    MUINT8      uGPSTimeStamp[32];
    MUINT8      uGPSProcessingMethod[64];   //(values of "GPS", "CELLID", "WLAN" or "MANUAL" by the EXIF spec.)
    //
    MUINT32     u4Orientation;
    MUINT32     u4ZoomRatio;   // Digital zoom ratio (x100) For example, 100, 114, and 132 refer to 1.00, 1.14, and 1.32 respectively.
    //
    MUINT32     u4Facing;
    //
    MUINT32     u4ImgIndex;     // The index of continuous shot image(1~n). '0' means single image.
    MUINT32     u4GroupId;      // The group ID for continuous shot.
    MUINT32     u4FocusH;       // Best shot: focus value H
    MUINT32     u4FocusL;       // Best shot: focus value L
    MUINT32     u4RefocusPos;       // Image Refocus: Main sensor position.
    MUINT8      uJpsFileName[32];   // Image Refocus: JPS file name.

public:     ////    Operations.
    CamExifParam()  { ::memset(this, 0, sizeof(CamExifParam)); }

};

/*******************************************************************************
* (Basic) Camera Exif
********************************************************************************/
class CamExif : public IBaseCamExif
{
public:     ////    Constructor/Destructor
    virtual ~CamExif();
    CamExif();

public:     ////    Interfaces.
    
    virtual MBOOL   init(CamExifParam const& rCamExifParam, CamDbgParam const& rCamDbgParam);
    virtual MBOOL   uninit();

    virtual
    MBOOL
    makeExifApp1(
        MUINT32 const u4ImgWidth,           //  [I] Image Width
        MUINT32 const u4ImgHeight,          //  [I] Image Height
        MUINT32 const u4ThumbSize,          //  [I] Thumb Size
        MUINT8* const puApp1Buf,            //  [O] Pointer to APP1 Buffer
        MUINT32*const pu4App1HeaderSize     //  [O] Pointer to APP1 Header Size
    );

    virtual
    MBOOL
    makeExifApp3(
        MBOOL   const bIsN3dEnable,     //  [I] Native3D(AC) Enable
        MUINT8* const puApp3Buf,        //  [O] Pointer to APP3 Buffer
        MUINT32*const pu4App3Size       //  [O] Pointer to APP3 Size
    );

    virtual
    MBOOL
    makeExifAppn(
        MUINT32 const u4Appn,           //  [I] APPn
        MUINT32 const u4Size,           //  [I] Buffer Size
        MUINT8* const puBuf,            //  [I] Buffer
        MUINT8* const puAppnBuf,        //  [O] Pointer to APPn Buffer
        MUINT32*const pu4AppnSize       //  [O] Pointer to APPn Size
    );

    virtual
    MBOOL
    reserveExifAppn(
        MUINT32 const u4ReserveAppn,    //  [I] APPn
        MUINT32 const u4ReserveSize,    //  [I] Reserve size for APPn
        MUINT8* const puAppnBuf,        //  [O] Pointer to APPn Buffer
        MUINT32*const pu4AppnSize       //  [O] Pointer to APPn Size
    );

    virtual
    MBOOL
    queryExifApp1Info(exifAPP1Info_s*const pexifApp1Info);

    virtual
    MINT32
    determineExifOrientation(
        MUINT32 const   u4DeviceOrientation, 
        MBOOL const     bIsFacing, 
        MBOOL const     bIsFacingFlip = MFALSE
    );

    virtual
    MBOOL
    set3AEXIFInfo(EXIF_INFO_T* p3AEXIFInfo);

    virtual
    MBOOL sendCommand(
                MINT32 cmd,
                MINT32 arg1 = 0,
                MINT32 arg2 = 0,
                MINT32 arg3 = 0);

    virtual
    MBOOL
    appendDebugExif(
        MUINT8* const puAppnBuf,        //  [O] Pointer to APPn Buffer
        MUINT32* const pu4AppnSize,     //  [O] Pointer to APPn Size
        MUINT32 const u4SensorIdx = 0   // [I] Sensor index. default value is 0. (0 means 1st cam, 1 means 2nd cam)
    );

private:
    MUINT32
        mapCapTypeIdx(MUINT32 const u4CapType);

    MUINT32
        mapExpProgramIdx(MUINT32 const u4SceneMode);

    MUINT32
        mapLightSourceIdx(MUINT32 const u4AwbMode);

    MUINT32
        mapMeteringModeIdx(MUINT32 const u4AeMeterMode);

    void
        getCommonDebugInfo(DEBUG_CMN_INFO_S &a_rDebugCommonInfo);

    MBOOL
        getCamDebugInfo(MUINT8* const pDbgInfo, MUINT32 const rDbgSize, MINT32 const dbgModuleID);

    MBOOL appendDebugInfo(
        MINT32 const dbgModuleID,       //  [I] debug module ID
        MINT32 const dbgAppn,           //  [I] APPn
        MUINT8** const ppuAppnBuf,      //  [O] Pointer to APPn Buffer
        MUINT32* const pu4AppnSize      //  [O] Pointer to APPn Size
    );

    MBOOL appendCamDebugInfo(
        MUINT32 const dbgAppn,          // [I] APPn for CAM module
        MUINT8** const puAppnBuf,       //  [O] Pointer to APPn Buffer
        MUINT32*const pu4AppnSize       //  [O] Pointer to APPn Size
    );

protected:  ////    Data Members.
    CamExifParam        mCamExifParam;

private:
    EXIF_INFO_T*        mp3AEXIFInfo;

private:
    CamDbgParam                     mCamDbgParam;
    Vector<DbgInfo>                 mDbgInfo;
    KeyedVector<MUINT32, MUINT32>   mMapModuleID;
    MINT32                          mi4DbgModuleType;

private:
    IBaseExif*          mpBaseExif;
    ExifIdMap*          mpDebugIdMap;
};


#endif  //  _MTK_CAMERA_INC_COMMON_CAMEXIF_CAMEXIF_H_

