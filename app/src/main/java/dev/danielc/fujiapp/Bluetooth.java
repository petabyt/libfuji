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
import android.bluetooth.BluetoothProfile;
import android.content.Intent;
import 	java.util.UUID;
import 	java.util.Set;
import android.content.Context;

public class Bluetooth {
    private BluetoothAdapter adapter;

    private BluetoothDevice dev;
    public BluetoothGatt gatt;

    private String FUJI_DEVICE_INFO = "91F1DE68-DFF6-466E-8B65-FF13B0F16FB8";
    private String FUJI_WIFI_LAUNCH_REQUEST = "FB15C357-364F-49D3-B5C5-1E32C0DED038";
    private String FUJI_GET_WIFI_STATE = "FB15C357-364F-49D3-B5C5-1E32C0DED038";
    private String FUJI_GET_BLE_VERSION = "389363E4-712E-4CF2-A72E-BFCF7FB6ADC1";
    private String FUJI_GET_SSID = "BF6DC9CF-3606-4EC9-A4C8-D77576E93EA4";

    private final BluetoothGattCallback callback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                // Bluetooth connected, you can now discover services
                gatt.discoverServices();
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                // Bluetooth disconnected, clean up resources if needed
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                // Services discovered, you can now read/write characteristics or enable notifications

                // Example: Reading a characteristic
                BluetoothGattService service = gatt.getService(UUID.fromString(FUJI_DEVICE_INFO));
                BluetoothGattCharacteristic characteristic = service.getCharacteristic(UUID.fromString(FUJI_GET_BLE_VERSION));
                gatt.readCharacteristic(characteristic);

                // Example: Enabling notifications for a characteristic
                BluetoothGattCharacteristic notifyCharacteristic = service.getCharacteristic(UUID.fromString(FUJI_WIFI_LAUNCH_REQUEST));
                gatt.setCharacteristicNotification(notifyCharacteristic, true);

                // Example: Writing to a characteristic
                BluetoothGattCharacteristic writeCharacteristic = service.getCharacteristic(UUID.fromString(FUJI_GET_WIFI_STATE));
                byte[] dataToWrite = {0x01, 0x02}; // Sample data to write
                writeCharacteristic.setValue(dataToWrite);
                gatt.writeCharacteristic(writeCharacteristic);
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                // Characteristic read successfully, handle the data here
                byte[] data = characteristic.getValue();
                // Do something with the data
            }
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                // Characteristic write successful
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            // Characteristic notification or indication received, handle the updated value here
            byte[] data = characteristic.getValue();
            // Do something with the updated data
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                // Descriptor write successful, if you've enabled notifications, this callback will be called
            }
        }

        // Implement other callback methods as needed for onDescriptorRead, onMtuChanged, etc.
    };

    // Return an Intent that is to be run by the caller
    public Intent getIntent() throws Exception {
        try {
            adapter = BluetoothAdapter.getDefaultAdapter();
        } catch (Exception e) {
            throw new Exception("Failed to get bluetooth adapter");
        }

        try {
            if (!adapter.isEnabled()) {
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                return enableBtIntent;
            }
        } catch (Exception e) {
            throw new Exception("Failed to get bluetooth request intent");
        }

        throw new Exception("Bluetooth adapter is disabled");
    }

    public BluetoothDevice getConnectedDevice() {
        Set<BluetoothDevice> bondedDevices = adapter.getBondedDevices();
        if (bondedDevices != null) {
            for (BluetoothDevice device : bondedDevices) {
                Backend.jni_print("Currently connected to " + device.getName());
            }
        }
        return null; // No connected device found
    }

    public void connectGATT(Context context) {
        if (dev != null) {
            gatt = dev.connectGatt(context, false, callback);
        }
    }
}
