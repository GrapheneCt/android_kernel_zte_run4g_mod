/*
 * Copyright (C) 2008 The Android Open Source Project
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
 
#ifndef __HWMSEN_CHIP_INFO_H__ 
#define __HWMSEN_CHIP_INFO_H__

typedef enum SENSOR_NUM_DEF
{
	 SONSER_UNSUPPORTED = -1,    
	 
	#ifdef CUSTOM_KERNEL_ACCELEROMETER
		ACCELEROMETER_NUM,
	#endif
	
	#ifdef CUSTOM_KERNEL_MAGNETOMETER
		MAGNETOMETER_NUM,
		ORIENTATION_NUM ,
	#endif
	
	#ifdef CUSTOM_KERNEL_ALSPS
		ALS_NUM,
		PS_NUM,
	#endif
	
	#ifdef CUSTOM_KERNEL_GYROSCOPE
		GYROSCOPE_NUM,
	#endif
	
	#ifdef CUSTOM_KERNEL_BAROMETER
		PRESSURE_NUM,
	#endif
	
	#ifdef CUSTOM_KERNEL_TEMPURATURE
		TEMPURATURE_NUM,
	#endif

	#ifdef CUSTOM_KERNEL_STEP_COUNTER
		STEP_COUNTER_NUM,
		STEP_DETECTOR_NUM,
	#endif

	#ifdef CUSTOM_KERNEL_SIGNIFICANT_MOTION_SENSOR
		STEP_SIGNIFICANT_MOTION_NUM,
	#endif

	#ifdef CUSTOM_KERNEL_PEDOMETER
		PEDOMETER_NUM,
	#endif

	#ifdef CUSTOM_KERNEL_IN_POCKET_SENSOR
		IN_POCKET_NUM,
	#endif

	#ifdef CUSTOM_KERNEL_ACTIVITY_SENSOR
		ACTIVITY_NUM,
	#endif
	
	#ifdef CUSTOM_KERNEL_PICK_UP_SENSOR
		PICK_UP_NUM,
	#endif

	#ifdef CUSTOM_KERNEL_FACE_DOWN_SENSOR
		FACE_DOWN_NUM,
	#endif

	#ifdef CUSTOM_KERNEL_SHAKE_SENSOR
		SHAKE_NUM,
	#endif
	
	SENSORS_NUM
	
}SENSOR_NUM_DEF;

#define MAX_NUM_SENSOR      (SENSORS_NUM)

#endif

