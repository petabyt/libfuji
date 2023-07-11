// Java class to connect to Fujiiflm's Bluetooth, and enable wifi
// Copyright 2023 fujiapp by Daniel C et al - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;
import android.util.Log;
import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;

public class Bluetooth {
    private BluetoothAdapter adapter;

    private BluetoothDevice dev;
    public BluetoothGatt gatt;

    private String FUJI_BLE_DEVICE_INFO = "91F1DE68-DFF6-466E-8B65-FF13B0F16FB8";
    private String FUJI_WIFI_LAUNCH_REQUEST = "FB15C357-364F-49D3-B5C5-1E32C0DED038";
    private String FUJI_GET_WIFI_STATE = "FB15C357-364F-49D3-B5C5-1E32C0DED038";
    private String FUJI_GET_BLE_VERSION = "389363E4-712E-4CF2-A72E-BFCF7FB6ADC1";
    private String FUJI_GET_SSID = "BF6DC9CF-3606-4EC9-A4C8-D77576E93EA4";

    public Intent getIntent() throws Exception {
        try {
            adapter = BluetoothAdapter.getDefaultAdapter();
        } catch (Exception e) {
            throw new Exception("Failed to get bluetooth adapter");
        }

        try {
            if (!adapter.isEnabled()) {
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                // Needs to be run in activity
                return enableBtIntent;
            }
        } catch (Exception e) {
            throw new Exception("Failed to get bluetooth request intent");
        }

        throw new Exception("Bluetooth adapter is disabled");
    }

    public void readCharacteristic() {

    }

// BluetoothGattService service = bluetoothGatt.getService(UUID.fromString(BTManagerService.SERVICE_FF_CONNECTED_DEVICE_INFORMATION));
}
