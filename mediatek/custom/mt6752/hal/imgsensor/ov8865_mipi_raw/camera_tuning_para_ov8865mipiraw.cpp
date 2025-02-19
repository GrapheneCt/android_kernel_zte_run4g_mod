#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_ov8865mipiraw.h"                                                    
#include "camera_info_ov8865mipiraw.h"                                                             
#include "camera_custom_AEPlinetable.h"
#include "camera_custom_tsf_tbl.h"

const NVRAM_CAMERA_ISP_PARAM_STRUCT CAMERA_ISP_DEFAULT_VALUE =
{{
    //Version
    Version: NVRAM_CAMERA_PARA_FILE_VERSION,

    //SensorId
    SensorId: SENSOR_ID,
    ISPComm:{
      {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      }
    },
    ISPPca: {
#include INCLUDE_FILENAME_ISP_PCA_PARAM
    },
    ISPRegs:{
#include INCLUDE_FILENAME_ISP_REGS_PARAM
    },
    ISPMfbMixer:{{
      0x01FF0001, // MIX3_CTRL_0
      0x00FF0000, // MIX3_CTRL_1
      0xFFFF0000  // MIX3_SPARE
    }},
    ISPMulitCCM:{
      Poly22:{
        87900,    // i4R_AVG
        18363,    // i4R_STD
        101980,   // i4B_AVG
        20674,    // i4B_STD
        4095,      // i4R_MAX
        512,      // i4R_MIN
        4095,      // i4G_MAX
        512,      // i4G_MIN   
        4095,      // i4B_MAX
        512,      // i4B_MIN
        {  // i4P00[9]
                8897296,-2989626,-787686,-1208954,6829026,-500072,170190,-2770908,7720566
        },
        {  // i4P10[9]
                1867396,-1257886,-609516,-495040,-44440,539480,-147722,392332,-245110
        },
        {  // i4P01[9]
                1628734,-988046,-640704,-716820,-361112,1077932,-114812,-380908,495378
        },
        {  // i4P20[9]
                788014,-983900,196062,-43050,119624,-76574,281758,-1043902,762090
                },
                { // i4P11[9]
                -71500,-689612,761476,243148,119000,-362148,286776,-619070,332618
                },
                { // i4P02[9]
                -631502,130466,501236,302926,68298,-371224,43616,-17274,-25994
                }
      },
      AWBGain:{
        // Strobe
        {
          810,    // i4R
          512,    // i4G
          677    // i4B
        },
        // A
        {
            519,    // i4R
            512,    // i4G
            1450    // i4B
        },
        // TL84
        {
            605,    // i4R
            512,    // i4G
            1172    // i4B
        },
        // CWF
        {
            771,    // i4R
            512,    // i4G
            1293    // i4B
        },
        // D65
        {
            810,    // i4R
            512,    // i4G
            677    // i4B
        },
        // Reserved 1
        {
            512,    // i4R
            512,    // i4G
            512    // i4B
        },
        // Reserved 2
        {
            512,    // i4R
            512,    // i4G
            512    // i4B
        },
        // Reserved 3
        {
            512,    // i4R
            512,    // i4G
            512    // i4B
        }
      },
      Weight:{
        1, // Strobe
        1, // A
        1, // TL84
        1, // CWF
        1, // D65
        1, // Reserved 1
        1, // Reserved 2
        1  // Reserved 3
      }
    },
    bInvokeSmoothCCM: MTRUE
}};

const NVRAM_CAMERA_3A_STRUCT CAMERA_3A_NVRAM_DEFAULT_VALUE =
{
    NVRAM_CAMERA_3A_FILE_VERSION, // u4Version
    SENSOR_ID, // SensorId

    // AE NVRAM
    {
        // rDevicesInfo
        {
            1152,    // u4MinGain, 1024 base = 1x
            8192,  // u4MaxGain, 16x
            80,    // u4MiniISOGain, ISOxx  
            128,    // u4GainStepUnit, 1x/8
            25848,    // u4PreExpUnit 
            30,     // u4PreMaxFrameRate
            17353,    // u4VideoExpUnit  
            31,    // u4VideoMaxFrameRate 
            1024,   // u4Video2PreRatio, 1024 base = 1x
            13496,    // u4CapExpUnit 
            30,    // u4CapMaxFrameRate
            1024,   // u4Cap2PreRatio, 1024 base = 1x
            15498,    // u4Video1ExpUnit
            120,    // u4Video1MaxFrameRate
            1024,    // u4Video12PreRatio, 1024 base = 1x
            25848,    // u4Video2ExpUnit
            30,     // u4Video2MaxFrameRate
            1024,   // u4Video22PreRatio, 1024 base = 1x
            25848,    // u4Custom1ExpUnit
            30,     // u4Custom1MaxFrameRate
            1024,    // u4Custom12PreRatio, 1024 base = 1x
            25848,    // u4Custom2ExpUnit
            30,     // u4Custom2MaxFrameRate
            1024,   // u4Custom22PreRatio, 1024 base = 1x
            25848,    // u4Custom3ExpUnit
            30,    // u4Custom3MaxFrameRate
            1024,    // u4Custom32PreRatio, 1024 base = 1x
            25848,    // u4Custom4ExpUnit
            30,     // u4Custom4MaxFrameRate
            1024,   // u4Custom42PreRatio, 1024 base = 1x
            25848,    // u4Custom5ExpUnit
            30,    // u4Custom5MaxFrameRate
            1024,    // u4Custom52PreRatio, 1024 base = 1x
            22,    // u4LensFno, Fno = 2.8
            350    // u4FocusLength_100x                                                                 
        },
        // rHistConfig
        {
            4, // 2,   // u4HistHighThres
            40,  // u4HistLowThres
            2,   // u4MostBrightRatio
            1,   // u4MostDarkRatio
            160, // u4CentralHighBound
            20,  // u4CentralLowBound
            {240, 230, 220, 210, 200}, // u4OverExpThres[AE_CCT_STRENGTH_NUM]
            {62, 70, 82, 108, 141},  // u4HistStretchThres[AE_CCT_STRENGTH_NUM]
            {18, 22, 26, 30, 34}       // u4BlackLightThres[AE_CCT_STRENGTH_NUM]
        },
        // rCCTConfig
        {
            TRUE,            // bEnableBlackLight
            TRUE,            // bEnableHistStretch
            TRUE,           // bEnableAntiOverExposure
            TRUE,            // bEnableTimeLPF
            TRUE,            // bEnableCaptureThres
            TRUE,            // bEnableVideoThres
            TRUE,            // bEnableVideo1Thres
            TRUE,            // bEnableBlackLight                                                        
            TRUE,            // bEnableHistStretch                                                       
            TRUE,           // bEnableAntiOverExposure                                                  
            TRUE,            // bEnableTimeLPF                                                           
            TRUE,            // bEnableCaptureThres                                                      
            TRUE,            // bEnableVideoThres                                                        
            TRUE,            // bEnableStrobeThres
            47,                // u4AETarget
            47,                // u4StrobeAETarget

            50,                // u4InitIndex
            4,                 // u4BackLightWeight
            32,                // u4HistStretchWeight
            4,                 // u4AntiOverExpWeight
            2,                 // u4BlackLightStrengthIndex
            2, // 2,                 // u4HistStretchStrengthIndex                                       
            2,                 // u4AntiOverExpStrengthIndex
            2,                 // u4TimeLPFStrengthIndex
            {1, 3, 5, 7, 8}, // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM]
            90,                // u4InDoorEV = 9.0, 10 base
            -10,    // i4BVOffset delta BV = value/10 
            64,                 // u4PreviewFlareOffset
            64,                 // u4PreviewFlareOffset
            3,                 // u4CaptureFlareThres
            64,                 // u4CaptureFlareOffset
            3,                 // u4CaptureFlareThres
            64,                 // u4VideoFlareOffset
            3,                 // u4VideoFlareThres
            64,                 // u4StrobeFlareOffset                                                   
            3,                 // u4StrobeFlareThres                                                     
            160,                 // u4PrvMaxFlareThres                                                   
            0,                 // u4PrvMinFlareThres
            160,                 // u4VideoMaxFlareThres                                                 
            0,                 // u4VideoMinFlareThres
            18,                // u4FlatnessThres              // 10 base for flatness condition.
            75,    // u4FlatnessStrength
            //rMeteringSpec
            {
                //rHS_Spec
                {
                    TRUE,//bEnableHistStretch           // enable histogram stretch
                    1024,//u4HistStretchWeight          // Histogram weighting value
                                        40,//u4Pcent                      // 1%=10, 0~1000
                                        160,//u4Thd                        // 0~255
                                        75,//u4FlatThd                    // 0~255

                    120,//u4FlatBrightPcent
                    120,//u4FlatDarkPcent
                    //sFlatRatio
                    {
                        1000,  //i4X1
                        1024,  //i4Y1
                        2400, //i4X2
                        0     //i4Y2
                    },
                    TRUE, //bEnableGreyTextEnhance
                    1800, //u4GreyTextFlatStart, > sFlatRatio.i4X1, < sFlatRatio.i4X2
                    {
                        10,     //i4X1
                        1024,   //i4Y1
                        80,     //i4X2
                        0       //i4Y2
                    }
                },
                //rAOE_Spec
                {
                    TRUE, //bEnableAntiOverExposure
                    1024, //u4AntiOverExpWeight
                    10,    //u4Pcent
                    200,  //u4Thd
                    TRUE, //bEnableCOEP
                    1,    //u4COEPcent
                    106,  //u4COEThd
                    0,  // u4BVCompRatio
                    //sCOEYRatio;     // the outer y ratio
                    {
                        23,   //i4X1
                        1024,  //i4Y1
                        47,   //i4X2
                        0     //i4Y2
                    },
                    //sCOEDiffRatio;  // inner/outer y difference ratio
                    {
                        1500, //i4X1
                        0,    //i4Y1
                        2100, //i4X2
                        1024   //i4Y2
                    }
                },
                //rABL_Spec
                {
                    TRUE,//bEnableBlackLigh
                    1024,//u4BackLightWeigh
                    400,//u4Pcent
                    22,//u4Thd,
                    255, // center luminance
                    256, // final target limitation, 256/128 = 2x
                    //sFgBgEVRatio
                    {
                        2200, //i4X1
                        0,    //i4Y1
                        4000, //i4X2
                        1024   //i4Y2
                    },
                    //sBVRatio
                    {
                        3800,//i4X1
                        0,   //i4Y1
                        5000,//i4X2
                        1024  //i4Y2
                    }
                },
                //rNS_Spec
                {
                    TRUE, // bEnableNightScene
                    5,    //u4Pcent
                    170,  //u4Thd
                                        72,   //u4FlatThd
                    200,  //u4BrightTonePcent
                    92, //u4BrightToneThd
                    500,  //u4LowBndPcent
                                        5,    //u4LowBndThdMul, <1024, u4AETarget*u4LowBndThdMul/1024
                    26,    //u4LowBndThdLimit

                    50,  //u4FlatBrightPcent;
                    300,   //u4FlatDarkPcent;
                    //sFlatRatio
                    {
                        1200, //i4X1
                        1024, //i4Y1
                        2400, //i4X2
                        0    //i4Y2
                    },
                    //sBVRatio
                    {
                        -500, //i4X1
                        1024,  //i4Y1
                        3000, //i4X2
                        0     //i4Y2
                    },
                    TRUE, // bEnableNightSkySuppresion
                    //sSkyBVRatio
                    {
                        -4000, //i4X1
                        1024, //i4X2
                        -2000,  //i4Y1
                        0     //i4Y2
                    }
                },
                // rTOUCHFD_Spec
                {
                                40,
                                50,
                                40,
                                50,
                                3,
                                120,
                                80,
                }
            }, //End rMeteringSpec
            // rFlareSpec
            {
                            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                            96,
                            48,
                            0,
                            4,
                            0,
                            1800,
                            0,
            },
            //rAEMoveRatio =
            {
                100, //u4SpeedUpRatio
                100, //u4GlobalRatio
                190, //u4Bright2TargetEnd
                20,   //u4Dark2TargetStart
                90, //u4B2TEnd
                70,  //u4B2TStart
                60,  //u4D2TEnd
                90,  //u4D2TStart
            },

            //rAEVideoMoveRatio =
            {
                100, //u4SpeedUpRatio
                100, //u4GlobalRatio
                150,  //u4Bright2TargetEnd
                20,    //u4Dark2TargetStart
                90, //u4B2TEnd
                50,  //u4B2TStart
                40,  //u4D2TEnd
                90,  //u4D2TStart
            },

            //rAEVideo1MoveRatio =
            {
                100, //u4SpeedUpRatio
                100, //u4GlobalRatio
                150,  //u4Bright2TargetEnd
                20,    //u4Dark2TargetStart
                90, //u4B2TEnd
                50,  //u4B2TStart
                40,  //u4D2TEnd
                90,  //u4D2TStart
            },

            //rAEVideo2MoveRatio =
            {
                100, //u4SpeedUpRatio
                100, //u4GlobalRatio
                150,  //u4Bright2TargetEnd
                20,    //u4Dark2TargetStart
                90, //u4B2TEnd
                50,  //u4B2TStart
                40,  //u4D2TEnd
                90,  //u4D2TStart
            },

            //rAECustom1MoveRatio =
            {
                100, //u4SpeedUpRatio
                100, //u4GlobalRatio
                150,  //u4Bright2TargetEnd
                20,    //u4Dark2TargetStart
                90, //u4B2TEnd
                50,  //u4B2TStart
                40,  //u4D2TEnd
                90,  //u4D2TStart
            },

            //rAECustom2MoveRatio =
            {
                100, //u4SpeedUpRatio
                100, //u4GlobalRatio
                150,  //u4Bright2TargetEnd
                20,    //u4Dark2TargetStart
                90, //u4B2TEnd
                50,  //u4B2TStart
                40,  //u4D2TEnd
                90,  //u4D2TStart
            },

            //rAECustom3MoveRatio =
            {
                100, //u4SpeedUpRatio
                100, //u4GlobalRatio
                150,  //u4Bright2TargetEnd
                20,    //u4Dark2TargetStart
                90, //u4B2TEnd
                50,  //u4B2TStart
                40,  //u4D2TEnd
                90,  //u4D2TStart
            },

            //rAECustom4MoveRatio =
            {
                100, //u4SpeedUpRatio
                100, //u4GlobalRatio
                150,  //u4Bright2TargetEnd
                20,    //u4Dark2TargetStart
                90, //u4B2TEnd
                50,  //u4B2TStart
                40,  //u4D2TEnd
                90,  //u4D2TStart
            },

            //rAECustom5MoveRatio =
            {
                100, //u4SpeedUpRatio
                100, //u4GlobalRatio
                150,  //u4Bright2TargetEnd
                20,    //u4Dark2TargetStart
                90, //u4B2TEnd
                50,  //u4B2TStart
                40,  //u4D2TEnd
                90,  //u4D2TStart
            },

            //rAEFaceMoveRatio =
            {
                100, //u4SpeedUpRatio
                100, //u4GlobalRatio
                150,  //u4Bright2TargetEnd
                20,    //u4Dark2TargetStart
                90, //u4B2TEnd
                40,  //u4B2TStart
                30,  //u4D2TEnd
                90,  //u4D2TStart
            },

            //rAETrackingMoveRatio =
            {
                100, //u4SpeedUpRatio
                100, //u4GlobalRatio
                150,  //u4Bright2TargetEnd
                20,    //u4Dark2TargetStart
                90, //u4B2TEnd
                40,  //u4B2TStart
                30,  //u4D2TEnd
                90,  //u4D2TStart
            },
            //rAEAOENVRAMParam =
            {
                1,      // i4AOEStrengthIdx: 0 / 1 / 2
                130,    // u4BVCompRatio
                {
                    {
                        47,  //u4Y_Target
                        20,  //u4AOE_OE_percent
                        160,  //u4AOE_OEBound
                     10,    //u4AOE_DarkBound
                        950,    //u4AOE_LowlightPrecent
                     1,    //u4AOE_LowlightBound
                        145,    //u4AOESceneLV_L
                        180,    //u4AOESceneLV_H
                        40,    //u4AOE_SWHdrLE_Bound
                    },
                    {
                        47,  //u4Y_Target
                        20,  //u4AOE_OE_percent
                        180,  //u4AOE_OEBound
                        15, //20,    //u4AOE_DarkBound
                        950,    //u4AOE_LowlightPrecent
                        3, //10,    //u4AOE_LowlightBound
                        145,    //u4AOESceneLV_L
                        180,    //u4AOESceneLV_H
                        40,    //u4AOE_SWHdrLE_Bound
                    },
                    {
                        47,  //u4Y_Target
                        20,  //u4AOE_OE_percent
                        200,  //u4AOE_OEBound
                        25,    //u4AOE_DarkBound
                        950,    //u4AOE_LowlightPrecent
                     8,    //u4AOE_LowlightBound
                        145,    //u4AOESceneLV_L
                        180,    //u4AOESceneLV_H
                        40,    //u4AOE_SWHdrLE_Bound
                    }
                }
            }
        }
    },
    // AWB NVRAM
   {	
        {
                // AWB calibration data
                {
		// rUnitGain (unit gain: 1.0 = 512)						
                        {
			0,	// i4R		
			0,	// i4G				
			0	// i4B				
                        },
		// rGoldenGain (golden sample gain: 1.0 = 512)					
                        {
			0,	// i4R				
			0,	// i4G				
			0	// i4B				
                        },
		// rTuningUnitGain (Tuning sample unit gain: 1.0 = 512)						
                        {
			0,	// i4R				
			0,	// i4G				
			0	// i4B				
                        },
                        // rD65Gain (D65 WB gain: 1.0 = 512)
                        {
                    1051,    // D65Gain_R
                    512,    // D65Gain_G
                    705    // D65Gain_B
                        }
                },
                // Original XY coordinate of AWB light source
                {
                        // Strobe
                        {
			0,	// i4X	
			0	// i4Y		
                        },
                        // Horizon
                        {
                    -378,    // OriX_Hor
                    -439    // OriY_Hor
                        },
                        // A
                        {
                    -272,    // OriX_A
                    -460    // OriY_A
                        },
                        // TL84
                        {
                    -146,    // OriX_TL84
                    -409    // OriY_TL84
                        },
                        // CWF
                        {
                    -99,    // OriX_CWF
                    -502    // OriY_CWF
                        },
                        // DNP
                        {
                    2,    // OriX_DNP
                    -438    // OriY_DNP
                        },
                        // D65
                        {
                    147,    // OriX_D65
                    -384    // OriY_D65
                        },
                        // DF
                        {
                    0,    // OriX_DF
                    0    // OriY_DF
                        }
                },
                // Rotated XY coordinate of AWB light source
                {
                        // Strobe
                        {
			0,     // i4X				              
			0	    // i4Y				              
                        },
                        // Horizon
                        {
                    -428,    // RotX_Hor
                    -390    // RotY_Hor
                        },
                        // A
                        {
                    -326,    // RotX_A
                    -423    // RotY_A
                        },
                        // TL84
                        {
                    -194,    // RotX_TL84
                    -388    // RotY_TL84
                        },
                        // CWF
                        {
                    -159,    // RotX_CWF
                    -486    // RotY_CWF
                        },
                        // DNP
                        {
                    -51,    // RotX_DNP
                    -435    // RotY_DNP
                        },
                        // D65
                        {
                    99,    // RotX_D65
                    -399    // RotY_D65
                        },
                        // DF
                        {
                    74,    // RotX_DF
                    -476    // RotY_DF
                        }
                },
                // AWB gain of AWB light source
                {
                        // Strobe
                        {
                    512,    // i4R
			512,	// i4G				          
                    512    // i4B
                        },
                        // Horizon
                        {
                    557,    // AWBGAIN_HOR_R
                    512,    // AWBGAIN_HOR_G
                    1547    // AWBGAIN_HOR_B
                        },
                        // A
                        {
                    661,    // AWBGAIN_A_R
                    512,    // AWBGAIN_A_G
                    1378    // AWBGAIN_A_B
                        },
                        // TL84
                        {
                    731,    // AWBGAIN_TL84_R
                    512,    // AWBGAIN_TL84_G
                    1086    // AWBGAIN_TL84_B
                        },
                        // CWF
                        {
                    883,    // AWBGAIN_CWF_R
                    512,    // AWBGAIN_CWF_G
                    1154    // AWBGAIN_CWF_B
                        },
                        // DNP
                        {
                    929,    // AWBGAIN_DNP_R
                    512,    // AWBGAIN_DNP_G
                    923    // AWBGAIN_DNP_B
                        },
                        // D65
                        {
                    1051,    // AWBGAIN_D65_R
                    512,    // AWBGAIN_D65_G
                    705    // AWBGAIN_D65_B
                        },
                        // DF
                        {
                    512,    // AWBGAIN_DF_R
                    512,    // AWBGAIN_DF_G
                    512    // AWBGAIN_DF_B
                        }
                },
                // Rotation matrix parameter
                {
                7,    // RotationAngle
                254,    // Cos
                31    // Sin
                },
                // Daylight locus parameter
                {
                -165,    // i4SlopeNumerator
                        128    // i4SlopeDenominator
                },
	            // Predictor gain
                {
                        // i4PrefRatio100
                101, // i4PrefRatio100

                        // DaylightLocus_L
                        {
                    1051,    // i4R
                            512, // i4G
                    705    // i4B
                        },
                        // DaylightLocus_H
                        {
                    858,    // i4R
                            512, // i4G
                    915    // i4B
                        },
                        // Temporal General
                        {
                    1051,    // i4R
                            512, // i4G
                    705,    // i4B
                        }
                },
                // AWB light area
                {
                        // Strobe
                        {
                    0,    // i4RightBound
                    0,    // i4LeftBound
                    0,    // i4UpperBound
                    0    // i4LowerBound
                        },
                        // Tungsten
                        {
                    -244,    // TungRightBound
                    -894,    // TungLeftBound
                    -356,    // TungUpperBound
                    -456    // TungLowerBound
                        },
                        // Warm fluorescent
                        {
                    -244,    // WFluoRightBound
                    -894,    // WFluoLeftBound
                    -456,    // WFluoUpperBound
                    -576    // WFluoLowerBound
                        },
                        // Fluorescent
                        {
                    -101,    // FluoRightBound
                    -244,    // FluoLeftBound
                    -323,    // FluoUpperBound
                    -437    // FluoLowerBound
                        },
                        // CWF
                        {
                -101,    // CWFRightBound
                -244,    // CWFLeftBound
                -437,    // CWFUpperBound
                -536    // CWFLowerBound
                        },
                        // Daylight
                        {
                    124,    // DayRightBound
                    -101,    // DayLeftBound
                    -319,    // DayUpperBound
                    -479    // DayLowerBound
                        },
                        // Shade
                        {
                    915,    // ShadeRightBound
                    124,    // ShadeLeftBound
                    -319,    // ShadeUpperBound
                    -479    // ShadeLowerBound
                        },
                        // Daylight Fluorescent
                        {
                    129,    // DFRightBound
                    //-66,    // DFLeftBound
                    -101,
                    -449,    // DFUpperBound
                    -531    // DFLowerBound
                        }
                },
                // PWB light area
                {
                        // Reference area
                        {
                    484,    // PRefRightBound
                    -894,    // PRefLeftBound
                    0,    // PRefUpperBound
                    -576    // PRefLowerBound
                        },
                        // Daylight
                        {
                    149,    // PDayRightBound
                    -101,    // PDayLeftBound
                    -319,    // PDayUpperBound
                    -479    // PDayLowerBound
                        },
                        // Cloudy daylight
                        {
                    249,    // PCloudyRightBound
                    74,    // PCloudyLeftBound
                    -319,    // PCloudyUpperBound
                    -479    // PCloudyLowerBound
                        },
                        // Shade
                        {
                    349,    // PShadeRightBound
                    74,    // PShadeLeftBound
                    -319,    // PShadeUpperBound
                    -479    // PShadeLowerBound
                        },
                        // Twilight
                        {
                    -101,    // PTwiRightBound
                    -261,    // PTwiLeftBound
                    -319,    // PTwiUpperBound
                    -479    // PTwiLowerBound
                        },
                        // Fluorescent
                        {
                    149,    // PFluoRightBound
                    -294,    // PFluoLeftBound
                    -338,    // PFluoUpperBound
                    -536    // PFluoLowerBound
                        },
                        // Warm fluorescent
                        {
                    -226,    // PWFluoRightBound
                    -426,    // PWFluoLeftBound
                    -338,    // PWFluoUpperBound
                    -536    // PWFluoLowerBound
                        },
                        // Incandescent
                        {
                    -226,    // PIncaRightBound
                    -426,    // PIncaLeftBound
                    -319,    // PIncaUpperBound
                    -479    // PIncaLowerBound
                        },
                        // Gray World
                        {
			5000,	// PGWRightBound				            
			-5000,	// PGWLeftBound				            
			5000,	// PGWUpperBound				            
			-5000	// PGWLowerBound				            
                        }
                },
                // PWB default gain
                {
                        // Daylight
                        {
                    962,    // PWB_Day_R
			512,	// PWB_Day_G				                
                    791    // PWB_Day_B
                        },
                        // Cloudy daylight
                        {
                    1131,    // PWB_Cloudy_R
			512,	// PWB_Cloudy_G				              
                    643    // PWB_Cloudy_B
                        },
                        // Shade
                        {
                    1200,    // PWB_Shade_R
			512,	// PWB_Shade_G				              
                    596    // PWB_Shade_B
                        },
                        // Twilight
                        {
                    755,    // PWB_Twi_R
			512,	// PWB_Twi_G				                
                    1077    // PWB_Twi_B
                        },
                        // Fluorescent
                        {
                    909,    // PWB_Fluo_R
			512,	// PWB_Fluo_G				                
                    957    // PWB_Fluo_B
                        },
                        // Warm fluorescent
                        {
                    674,    // PWB_WFluo_R
			512,	// PWB_WFluo_G				              
                    1402    // PWB_WFluo_B
                        },
                        // Incandescent
                        {
                    636,    // PWB_Inca_R
			512,	// PWB_Inca_G				                
                    1341    // PWB_Inca_B
                        },
                        // Gray World
                        {
			512,	// PWB_GW_R				                  
			512,	// PWB_GW_G				                  
			512	// PWB_GW_B				                    
                        }
                },
                // AWB preference color
                {
                        // Tungsten
                        {
                    0,    // TUNG_SLIDER
                    6587    // TUNG_OFFS
                        },
                        // Warm fluorescent
                        {
                    0,    // WFluo_SLIDER
                    5710    // WFluo_OFFS
                        },
                        // Shade
                        {
                    0,    // Shade_SLIDER
                    1343    // Shade_OFFS
                        },
                        // Preference gain: strobe
                        {
			512,	// PRF_STROBE_R				              
			512,	// PRF_STROBE_G				              
			512	// PRF_STROBE_B				                
                        },
                        // Preference gain: tungsten
                        {
			512,	// PRF_TUNG_R				                
			512,	// PRF_TUNG_G				                
			512	// PRF_TUNG_B				                  
                        },
                        // Preference gain: warm fluorescent
                        {
			512,	// PRF_WFluo_R				              
			512,	// PRF_WFluo_G				              
			512	// PRF_WFluo_B				                
                        },
                        // Preference gain: fluorescent
                        {
			512,	// PRF_Fluo_R				                
			512,	// PRF_Fluo_G				                
			512	// PRF_Fluo_B				                  
                        },
                        // Preference gain: CWF
                        {
			512,	// PRF_CWF_R				                
			512,	// PRF_CWF_G				                
			512	// PRF_CWF_B				                  
                        },
                        // Preference gain: daylight
                        {
			512,	// PRF_Day_R				                
			512,	// PRF_Day_G				                
			512	// PRF_Day_B				                  
                        },
                        // Preference gain: shade
                        {
			512,	// PRF_Shade_R				              
			512,	// PRF_Shade_G				              
			512	// PRF_Shade_B				                
                        },
                        // Preference gain: daylight fluorescent
                        {
			512,	// PRF_DF_R				                  
			512,	// PRF_DF_G				                  
			512	// PRF_DF_B				                    
                        }
                },

                // Algorithm Tuning Paramter
                {
                    // AWB Backup Enable
                    0,

                    // AWB LSC Gain
                    {
                        933, //856,        // i4R
                        512, // i4G
                        865, //942         // i4B
                    },
                    {
                        1,      // bEnable
                        6           // i4ScalingFactor: [6] 1~12, [7] 1~6, [8] 1~3, [9] 1~2, [>=10]: 1
                    },
                    {
                            115,    // i4InitLVThr_L
                            155,    // i4InitLVThr_H
                            80      // i4EnqueueLVThr
                    },
                    {
                            65,     // i4Neutral_ParentBlk_Thr
                        { 100, 100, 100, 100, 100, 100, 100, 100, 100, 50,  25,   2,   2,   2,   2,   2,   2,   2,   2}  // (%) i4CWFDF_LUTThr
                    },
                    {
		                { 5,   5,   5,   5,   5,   5,   5,   5,    5,   5,  10,  10,  10,  10,  10,  10,  10,  10,  10},  // (%)
		                { 0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   5,  10,  10,  10,  10,  10,  10,  10,  10},  // (%)
		                { 0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   5,  10,  10,  10,  10,  10,  10,  10,  10},  // (%)
		                { 0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   2,   4,   4,   4,   4,   4,   4,   4},  // (%)
		                { 0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   5,  10,  10,  10,  10,  10,  10,  10,  10},  // (%)
                    },
                    {
                        {
                            1,          // i4Enable
                        140,        // i4LVThr
                            {
                            -15,   // i4Sunset_BoundXr_Thr
                            -435     // i4Sunset_BoundYr_Thr
                            },
                            10,         // i4SunsetCountThr
                            0,          // i4SunsetCountRatio_L
                            171         // i4SunsetCountRatio_H
                        },
                        {
                            1,          // i4Enable
                        115,        // i4LVThr
                            {
                            -158,   // i4BoundXrThr
                            -399     // i4BoundYrThr
                            },
                            128         // i4DaylightProb
                        },
                        {
                            1,          // i4Enable
                        110,        // i4LVThr
                            {
                            -159,   // i4BoundXrThr
                            -486     // i4BoundYrThr
                            },
                            128         // i4DaylightProb
                        },
                        {
                            1,          // i4Enable
                        256,        // i4SpeedRatio
                            {
                            -448,   // i4BoundXrThr
                            143     // i4BoundYrThr
                            }
                        }
                    },
                    {
                        {
                            1,      // Gain Limit Enable
                            691 //717     // Gain ratio
                        },
                        {
                            1,      // Gain Limit Enable
                            768 //832     // Gain ratio
                        }
                    },
                    {
                        { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 70,  30,  20,  10,   0,   0,   0,   0,   0}
                    },
                    {   //LV0    1     2     3      4     5     6     7     8      9      10     11    12   13     14    15   16    17    18
                        {100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  100,  100, 75,  35,   0,   0,  0,   0,   0}, // Strobe
                        {100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  100,  100, 100, 75,  25,   0,  0,   0,   0}, // Tungsten
                        {100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  100,  100, 100, 75,  25,   0,  0,   0,   0}, // Warm fluorescent
                        {100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  100,  100, 100, 50,  25,   0,  0,   0,   0}, // Fluorescent
                        {100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  100,  100, 50,  25,   0,   0,  0,   0,   0}, // CWF
                        {100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  100,  100, 100, 75,  50,  50, 40,  30,   0}, // Daylight
                        {100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  100,  100, 50,  25,   0,   0,  0,   0,   0}, // Shade
                        {100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  100,  100, 50,  25,   0,   0,  0,   0,   0}  // Daylight fluorescent
    		        }
                },
	            {
		            {
			                2300,	// i4CCT[0]
			                2850,	// i4CCT[1]
			                3750,	// i4CCT[2]
			                5100,	// i4CCT[3]
			                6500 	// i4CCT[4]
		            },
		            {
                -527,    // i4RotatedXCoordinate[0]
                -425,    // i4RotatedXCoordinate[1]
                -293,    // i4RotatedXCoordinate[2]
                -150,    // i4RotatedXCoordinate[3]
			                0 	    // i4RotatedXCoordinate[4]
		            }
	            }
        },
                {
                {
                        {
			0,	// i4R		
			0,	// i4G				
			0	// i4B				
                        },
                        {
			0,	// i4R				
			0,	// i4G				
			0	// i4B				
                        },
                        {
			0,	// i4R				
			0,	// i4G				
			0	// i4B				
                        },
                        {
                    1051,    // D65Gain_R
                    512,    // D65Gain_G
                    705    // D65Gain_B
                        }
                },
                {
                        {
			0,	// i4X	
			0	// i4Y		
                        },
                        {
		-394,	// i4X
		-388	// i4Y	
                        },
                        {
			-260,	// i4X	
			-396	    // i4Y	
                        },
                        {
			-63,	// i4X		
			-405	// i4Y		
                        },
                        {
			-65,	// i4X	
			-478	// i4Y	
                        },
                        {
			27,	// i4X		
			-427	// i4Y		
                        },
                        {
			184,	// i4X			
			-379	// i4Y			
                        },
                        {
			147,    // i4X
			-478	// i4Y	
                        }
                },
                {
                        {
			0,     // i4X				              
			0	    // i4Y				              
                        },
                        {
			-408,	// i4X				                  
			-374	// i4Y				                  
                        },
                        {
			-274,	// i4X				                    
			-387	// i4Y				                    
                        },
                        {
			-77,	// i4X				                
			-403	// i4Y				                
                        },
                        {
			-82,	// i4X				                  
			-476	// i4Y				                  
                        },
                        {
			12,	// i4X				                  
			-428	// i4Y				                  
                        },
                        {
			171,	// i4X				                    
			-385	// i4Y				                  
                        },
                        {
			130,	// i4X				                    
			-483	// i4Y				                      
                        }
                },
                {
                        {
			512,	// i4R				          
			512,	// i4G				          
			512	// i4B				            
                        },
                        {
			512,	// i4R				            
			517,	// i4G				            
			1488	// i4B				            
                        },
                        {
                                615,    // i4R
			512,	// i4G				              
			1244	// i4B				              
                        },
                        {
			814,	// i4R				            
			512,	// i4G				            
			964	// i4B				            
                        },
                        {
			895,	// i4R				            
			512,	// i4G				            
			1068	// i4B				            
                        },
                        {
			947,	// i4R				            
			512,	// i4G				            
			880	// i4B				              
                        },
                        {
			1097,	// i4R				            
			512,	// i4G				            
			667	// i4B				              
                        },
                        {
			1192,	// i4R				              
			512,	// i4G				              
			801	// i4B				                
                        }
                },
                {
		2,	// i4RotationAngle					              
		256,	// i4Cos					                      
		9	// i4Sin					                        
                },
                {
		-136,	// i4SlopeNumerator		
                        128    // i4SlopeDenominator
                },
                {
                        101,
                        {
                            1125,    // i4R
                            512, // i4G
                            648     // i4B
                        },
                        {
                            865,    // i4R
                            512, // i4G
                            859     // i4B
                        },
                        {
                            1125,    // i4R
                            512, // i4G
                            648     // i4B
                        }
                },
                {
                        {
			0,	// StrobeRightBound				            
			0,	// StrobeLeftBound				          
			0,	// StrobeUpperBound				          
			0	// StrobeLowerBound				          
                        },
                        {
			-176,	// TungRightBound				            
			-808,	// TungLeftBound				            
			-339,	// TungUpperBound				            
			-395	// TungLowerBound				            
                        },
                        {
			-176,	// WFluoRightBound				          
			-808,	// WFluoLeftBound				            
			-395,	// WFluoUpperBound				          
			-476	// WFluoLowerBound				          
                        },
                        {
			-10,	// FluoRightBound				            
			-176,	// FluoLeftBound				            
			-357,	// FluoUpperBound				            
			-449	// FluoLowerBound				            
                        },
                        {
			3,	// CWFRightBound				            
			-159,	// CWFLeftBound				              
			-449,	// CWFUpperBound				            
			-521	// CWFLowerBound				            
                        },
                        {
			201,	// DayRightBound				              
			-10,	// DayLeftBound				              
			-357,	// DayUpperBound				            
			-449	// DayLowerBound				            
                        },
                        {
			531,	// ShadeRightBound				          
			201,	// ShadeLeftBound				              
			-357,	// ShadeUpperBound				          
			-421	// ShadeLowerBound				          
                        },
                        {
			201,	// DFRightBound				                
			3,	// DFLeftBound				                
			-449,	// DFUpperBound				                
			-521	// DFLowerBound				                  
                        }
                },
                {
                        {
			531,	// PRefRightBound				            
			-808,	// PRefLeftBound				            
			-314,	// PRefUpperBound				            
			-521	// PRefLowerBound				            
                        },
                        {
			226,	// PDayRightBound				            
			-10,	// PDayLeftBound				            
			-357,	// PDayUpperBound				            
			-449	// PDayLowerBound				            
                        },
                        {
			326,	// PCloudyRightBound				        
			151,	// PCloudyLeftBound				            
			-357,	// PCloudyUpperBound				        
			-449	// PCloudyLowerBound				        
                        },
                        {
			426,	// PShadeRightBound				          
			151,	// PShadeLeftBound				          
			-357,	// PShadeUpperBound				          
			-449	// PShadeLowerBound				          
                        },
                        {
			-10,	// PTwiRightBound				            
			-170,	// PTwiLeftBound				            
			-357,	// PTwiUpperBound				            
			-449	// PTwiLowerBound				            
                        },
                        {
			221,	// PFluoRightBound				          
			-182,	// PFluoLeftBound				            
			-335,	// PFluoUpperBound				          
			-526	// PFluoLowerBound				          
                        },
                        {
			-174,	// PWFluoRightBound				          
			-374,	// PWFluoLeftBound				          
			-335,	// PWFluoUpperBound				          
			-526	// PWFluoLowerBound				          
                        },
                        {
			-174,	// PIncaRightBound				          
			-374,	// PIncaLeftBound				            
			-357,	// PIncaUpperBound				          
			-449	// PIncaLowerBound				          
                        },
                        {
			5000,	// PGWRightBound				            
			-5000,	// PGWLeftBound				            
			5000,	// PGWUpperBound				            
			-5000	// PGWLowerBound				            
                        }
                },
                {
                        {
			1036,	// PWB_Day_R				                
			512,	// PWB_Day_G				                
			745	// PWB_Day_B				                  
                        },
                        {
			1228,	// PWB_Cloudy_R				              
			512,	// PWB_Cloudy_G				              
			620	// PWB_Cloudy_B				                
                        },
                        {
			1311,	// PWB_Shade_R				              
			512,	// PWB_Shade_G				              
			578	// PWB_Shade_B				                
                        },
                        {
			800,	// PWB_Twi_R				                
			512,	// PWB_Twi_G				                
			982	// PWB_Twi_B				                
                        },
                        {
			959,	// PWB_Fluo_R				                
			512,	// PWB_Fluo_G				                
			874	// PWB_Fluo_B				                  
                        },
                        {
			654,	// PWB_WFluo_R				              
			512,	// PWB_WFluo_G				              
			1317	// PWB_WFluo_B				              
                        },
                        {
			630,	// PWB_Inca_R				                
			512,	// PWB_Inca_G				                
			1271	// PWB_Inca_B				                
                        },
                        {
			512,	// PWB_GW_R				                  
			512,	// PWB_GW_G				                  
			512	// PWB_GW_B				                    
                        }
                },
                {
                        {
			100,	// TUNG_SLIDER				                
			3730	// TUNG_OFFS				                
                        },
                        {
			100,	// WFluo_SLIDER				                
			3730	// WFluo_OFFS				                
                        },
                        {
			50,	// Shade_SLIDER				                
			909	// Shade_OFFS				                  
                        },
                        {
			512,	// PRF_STROBE_R				              
			512,	// PRF_STROBE_G				              
			512	// PRF_STROBE_B				                
                        },
                        {
			512,	// PRF_TUNG_R				                
			512,	// PRF_TUNG_G				                
			512	// PRF_TUNG_B				                  
                        },
                        {
			512,	// PRF_WFluo_R				              
			512,	// PRF_WFluo_G				              
			512	// PRF_WFluo_B				                
                        },
                        {
			512,	// PRF_Fluo_R				                
			512,	// PRF_Fluo_G				                
			512	// PRF_Fluo_B				                  
                        },
                        {
			512,	// PRF_CWF_R				                
			512,	// PRF_CWF_G				                
			512	// PRF_CWF_B				                  
                        },
                        {
			512,	// PRF_Day_R				                
			512,	// PRF_Day_G				                
			512	// PRF_Day_B				                  
                        },
                        {
			512,	// PRF_Shade_R				              
			512,	// PRF_Shade_G				              
			512	// PRF_Shade_B				                
                        },
                        {
			512,	// PRF_DF_R				                  
			512,	// PRF_DF_G				                  
			512	// PRF_DF_B				                    
                        }
                },
                {
                    0,
                    {
                        865,        // i4R
                        512, // i4G
                        859         // i4B
                    },
                    // Parent block weight parameter
                    {
                        1,      // bEnable
                        6           // i4ScalingFactor: [6] 1~12, [7] 1~6, [8] 1~3, [9] 1~2, [>=10]: 1
                    },
                    // AWB LV threshold for predictor
                    {
                            100,    // i4InitLVThr_L
                            140,    // i4InitLVThr_H
                            80      // i4EnqueueLVThr
                    },
                    // AWB number threshold for temporal predictor
                    {
                            65,     // i4Neutral_ParentBlk_Thr
                        //LV0   1    2    3    4    5    6    7    8    9    10   11   12   13   14   15   16   17   18
                        {  100,   100,   100,   100,   100,   100,   100,   100,   50,   25,   2,   2,   2,   2,   2,   2,   2,   2,   2}  // (%) i4CWFDF_LUTThr
                    },
                    // AWB light neutral noise reduction for outdoor
                    {
                        //LV0  1    2    3    4    5    6    7    8    9    10   11   12   13   14   15   16   17   18
                        // Non neutral
		                { 5,  5,  5,  5,  5,  5,  5,  5,  5,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10},  // (%)
		                // Flurescent
		                { 0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  10,  10,  10,  10,  10,  10,  10,  10,  10},  // (%)
		                // CWF
		                { 0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  10,  10,  10,  10,  10,  10,  10,  10,  10},  // (%)
		                // Daylight
		                { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  4,  4,  4,  4,  4,  4,  4,  4},  // (%)
		                // DF
		                { 0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  10,  10,  10,  10,  10,  10,  10,  10,  10},  // (%)
                    },
                    // AWB feature detection
                    {
                        // Sunset Prop
                        {
                            1,          // i4Enable
                            130,        // i4LVThr
                            {
                                23,   // i4Sunset_BoundXr_Thr
                                -428     // i4Sunset_BoundYr_Thr
                            },
                            10,         // i4SunsetCountThr
                            0,          // i4SunsetCountRatio_L
                            171         // i4SunsetCountRatio_H
                        },

                        // Shade F Detection
                        {
                            1,          // i4Enable
                            105,        // i4LVThr
                            {
                                -55,   // i4BoundXrThr
                                -385    // i4BoundYrThr
                            },
                            128         // i4DaylightProb
                        },

                        // Shade CWF Detection
                        {
                            1,          // i4Enable
                            95,         // i4LVThr
                            {
                                -82,   // i4BoundXrThr
                                -476    // i4BoundYrThr
                            },
                            128         // i4DaylightProb
                        },

                        // Low CCT
                        {
                            1,          // i4Enable
                            512,       // i4SpeedRatio
                            {
                               -458,       // i4BoundXrThr
                               188         // i4BoundYrThr
                            }
                        }
                    },

                    // AWB Gain Limit
                    {
                        // rNormalLowCCT
                        {
                            1,      // Gain Limit Enable
                        717     // Gain ratio
                        },
                        // rPrefLowCCT
                        {
                            1,      // Gain Limit Enable
                        832     // Gain ratio
                        }
                    },

                    // AWB non-neutral probability for spatial and temporal weighting look-up table (Max: 100; Min: 0)
                    {
                        //LV0   1    2    3    4    5    6    7    8    9    10   11   12   13   14   15   16   17   18
		                { 100,  100,  100,  100,  100,  100,  100,  100,  100,  70,  30,  20,  10,  0,  0,  0,  0,  0,  0}
                    },

                    // AWB daylight locus probability look-up table (Max: 100; Min: 0)
                    {   //LV0    1     2     3      4     5     6     7     8      9      10     11    12   13     14    15   16    17    18
		                { 100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  50,  0,  0,  0,  0,  0,  0,  0}, // Strobe
		                { 100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  75,  25,  0,  0,  0,  0,  0}, // Tungsten
		                { 100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  75,  25,  0,  0,  0,  0,  0}, // Warm fluorescent
		                { 100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  50,  25,  0,  0,  0,  0,  0}, // Fluorescent
		                { 100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  50,  25,  0,  0,  0,  0,  0,  0}, // CWF
		                { 100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  75,  50,  50,  40,  30,  0,  0}, // Daylight
		                { 100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  50,  25,  0,  0,  0,  0,  0,  0}, // Shade
		                { 100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,  50,  25,  0,  0,  0,  0,  0,  0} // Daylight fluorescent
    		        }
                },

                // CCT estimation
                {
                        // CCT
                        {
			2300,	// CCT0				                      
			2850,	// CCT1				                      
			3750,	// CCT2				                      
			5100,	// CCT3				                      
			6500     // CCT4				                      
		            },
                        // Rotated X coordinate
                        {
			-579,	// RotatedXCoordinate0				      
			-445,	// RotatedXCoordinate1				      
			-248,	// RotatedXCoordinate2				      
			-159,	// RotatedXCoordinate3				      
			0  // RotatedXCoordinate4				        
		            }
	            }
        }
        },
    // Flash AWB NVRAM
    {
#include INCLUDE_FILENAME_FLASH_AWB_PARA
    },

	{0}  //FIXED                                  
};

#include INCLUDE_FILENAME_ISP_LSC_PARAM
//};  //  namespace
const CAMERA_TSF_TBL_STRUCT CAMERA_TSF_DEFAULT_VALUE =
{
    {
        1,  // isTsfEn
        2,  // tsfCtIdx
        {20, 2000, -110, -110, 512, 512, 512, 0}    // rAWBInput[8]
    },
#include INCLUDE_FILENAME_TSF_PARA
#include INCLUDE_FILENAME_TSF_DATA
};
const NVRAM_CAMERA_FEATURE_STRUCT CAMERA_FEATURE_DEFAULT_VALUE =
{
#include INCLUDE_FILENAME_FEATURE_PARA
};

typedef NSFeature::RAWSensorInfo<SENSOR_ID> SensorInfoSingleton_T;


namespace NSFeature {
  template <>
  UINT32
  SensorInfoSingleton_T::
  impGetDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, VOID*const pDataBuf, UINT32 const size) const
  {
    UINT32 dataSize[CAMERA_DATA_TYPE_NUM] = {sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT),
        sizeof(NVRAM_CAMERA_3A_STRUCT),
        sizeof(NVRAM_CAMERA_SHADING_STRUCT),
        sizeof(NVRAM_LENS_PARA_STRUCT),
        sizeof(AE_PLINETABLE_T),
        0,
        sizeof(CAMERA_TSF_TBL_STRUCT),
        0,
        sizeof(NVRAM_CAMERA_FEATURE_STRUCT)
    };

    if (CameraDataType > CAMERA_NVRAM_DATA_FEATURE || NULL == pDataBuf || (size < dataSize[CameraDataType]))
    {
      return 1;
    }

    switch(CameraDataType)
    {
      case CAMERA_NVRAM_DATA_ISP:
        memcpy(pDataBuf,&CAMERA_ISP_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT));
        break;
      case CAMERA_NVRAM_DATA_3A:
        memcpy(pDataBuf,&CAMERA_3A_NVRAM_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_3A_STRUCT));
        break;
      case CAMERA_NVRAM_DATA_SHADING:
        memcpy(pDataBuf,&CAMERA_SHADING_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_SHADING_STRUCT));
        break;
      case CAMERA_DATA_AE_PLINETABLE:
        memcpy(pDataBuf,&g_PlineTableMapping,sizeof(AE_PLINETABLE_T));
        break;
      case CAMERA_DATA_TSF_TABLE:
        memcpy(pDataBuf,&CAMERA_TSF_DEFAULT_VALUE,sizeof(CAMERA_TSF_TBL_STRUCT));
        break;
      case CAMERA_NVRAM_DATA_FEATURE:
        memcpy(pDataBuf,&CAMERA_FEATURE_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_FEATURE_STRUCT));
        break;
      default:
        break;
    }
    return 0;
  }};  //  NSFeature
