/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.mediatek.blemanager.common;

import android.bluetooth.BluetoothDevice;

/**
 * BluetoothCallback provides a callback interface for the settings
 * UI to receive events from {@link BluetoothLEEventManager}.
 */
public class BluetoothCallback {
//    void onBluetoothStateChanged(int bluetoothState);
//    void onScanningStateChanged(boolean started);
//    void onDeviceAdded(CachedBluetoothDevice cachedDevice);
//    void onDeviceDeleted(CachedBluetoothDevice cachedDevice);
//    void onDeviceBondStateChanged(CachedBluetoothDevice cachedDevice, int bondState);

    public interface BluetoothAdapterState {
        void onBluetoothStateChanged(int state);
        void onBluetoothScanningStateChanged(boolean started);
    }
    
    public interface BluetoothLEDeviceScanned {
        void onScannedBleDeviceAdded(BluetoothDevice device);
        void onScannedBleDeviceRemoved(BluetoothDevice device);
    }

}
