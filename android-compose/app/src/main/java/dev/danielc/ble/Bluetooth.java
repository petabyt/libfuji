package dev.danielc.ble;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothProfile;
import android.content.Intent;
import java.util.Set;
import android.content.Context;
import android.content.pm.PackageManager;
import android.util.Log;

import androidx.core.app.ActivityCompat;

public class Bluetooth {
    public static final String TAG = "ble";
    Context context;
    private BluetoothAdapter adapter;

    private BluetoothDevice dev;
    public BluetoothGatt gatt;
    MyBluetoothGattCallback callback;

    public class MyBluetoothGattCallback extends BluetoothGattCallback {
        @Override
        @SuppressLint("MissingPermission")
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (newState==BluetoothProfile.STATE_CONNECTED) {
                // Bluetooth connected, you can now discover services
                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT)!=PackageManager.PERMISSION_GRANTED) {
                    return;
                }
                gatt.discoverServices();
            } else if (newState==BluetoothProfile.STATE_DISCONNECTED) {
                // Bluetooth disconnected, clean up resources if needed
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status==BluetoothGatt.GATT_SUCCESS) {
                // Services discovered, you can now read/write characteristics or enable notifications

                // Example: Reading a characteristic
//                BluetoothGattService service = gatt.getService(UUID.fromString(XX));
//                BluetoothGattCharacteristic characteristic = service.getCharacteristic(UUID.fromString(XX));
//                gatt.readCharacteristic(characteristic);
//
//                // Example: Enabling notifications for a characteristic
//                BluetoothGattCharacteristic notifyCharacteristic = service.getCharacteristic(UUID.fromString(XX));
//                gatt.setCharacteristicNotification(notifyCharacteristic, true);
//
//                // Example: Writing to a characteristic
//                BluetoothGattCharacteristic writeCharacteristic = service.getCharacteristic(UUID.fromString(XX));
//                byte[] dataToWrite = {0x01, 0x02}; // Sample data to write
//                writeCharacteristic.setValue(dataToWrite);
//                gatt.writeCharacteristic(writeCharacteristic);
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            if (status==BluetoothGatt.GATT_SUCCESS) {
                // Characteristic read successfully, handle the data here
                byte[] data = characteristic.getValue();
                // Do something with the data
            }
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            if (status==BluetoothGatt.GATT_SUCCESS) {
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
            if (status==BluetoothGatt.GATT_SUCCESS) {
                // Descriptor write successful, if you've enabled notifications, this callback will be called
            }
        }
    }

    // Return an Intent that is to be run by the caller
    public Intent getIntent() throws Exception {
        try {
            adapter = BluetoothAdapter.getDefaultAdapter();
        } catch (Exception e) {
            throw new Exception("Failed to get bluetooth adapter");
        }

        try {
            if (adapter.isEnabled()) {
                return new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            } else {
                throw new Exception("Bluetooth adapter is disabled");
            }
        } catch (Exception e) {
            throw new Exception("Failed to get bluetooth request intent");
        }
    }

    @SuppressLint("MissingPermission")
    public BluetoothDevice getConnectedDevice() {
        Set<BluetoothDevice> bondedDevices = adapter.getBondedDevices();
        if (bondedDevices!=null) {
            for (BluetoothDevice device : bondedDevices) {
                Log.d(TAG, String.format("Currently connected to %s", device.getName()));
            }
        }
        return null; // No connected device found
    }

    @SuppressLint("MissingPermission")
    public void connectGATT(Context c) throws Exception {
        context = c;
        if (dev != null) {
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT)!=PackageManager.PERMISSION_GRANTED) {
                throw new Exception("Bluetooth permission denied");
            }
            gatt = dev.connectGatt(context, false, callback);
        }
    }
}
