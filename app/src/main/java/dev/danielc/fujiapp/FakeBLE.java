package dev.danielc.fujiapp;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.AdvertisingSet;
import android.bluetooth.le.AdvertisingSetCallback;
import android.bluetooth.le.AdvertisingSetParameters;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.ParcelUuid;
import android.util.Log;

import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;

import java.util.UUID;

public class FakeBLE {
    static Tester tester;

    public static void log(String str) {
        tester.log(str);
    }

    public static void fail(String str) {
        tester.fail(str);
    }

    public static void start(Tester t) {
        tester = t;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            try {
                advertise();
            } catch (Exception e) {
                fail(e.toString());
                e.printStackTrace();
            }
        } else {
            fail("API is below 26");
        }
    }

    static AdvertisingSetCallback callback;
    static BluetoothLeAdvertiser advertiser;
    static AdvertisingSet advertisingSet;
    static AdvertisingSetParameters.Builder parameters;

    @RequiresApi(api = Build.VERSION_CODES.O)
    static void advertise() {
        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        advertiser = BluetoothAdapter.getDefaultAdapter().getBluetoothLeAdvertiser();

        // Check if all features are supported
        if (!adapter.isLe2MPhySupported()) {
            fail("2M PHY not supported!");
            return;
        }

        if (!adapter.isLeExtendedAdvertisingSupported()) {
            fail("LE Extended Advertising not supported!");
            return;
        }

        ActivityCompat.requestPermissions(tester, new String[]{Manifest.permission.BLUETOOTH_CONNECT}, 1);

        if (ActivityCompat.checkSelfPermission(tester, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            fail("Failed to get connect permissions");
            return;
        }

        adapter.setName("CA2AX-H1-CA2A");

        int maxDataLength = adapter.getLeMaximumAdvertisingDataLength();

        parameters = (new AdvertisingSetParameters.Builder())
                .setLegacyMode(false)
                .setConnectable(true)
                .setInterval(AdvertisingSetParameters.INTERVAL_HIGH)
                .setTxPowerLevel(AdvertisingSetParameters.TX_POWER_MEDIUM)
                .setPrimaryPhy(BluetoothDevice.PHY_LE_1M)
                .setSecondaryPhy(BluetoothDevice.PHY_LE_2M);

        AdvertiseSettings settings = new AdvertiseSettings.Builder()
                .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_POWER)
                .setConnectable(true)
                .setTimeout(0)
                .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_LOW)
                .build();

        callback = new AdvertisingSetCallback() {
            @Override
            public void onAdvertisingSetStarted(AdvertisingSet set, int txPower, int status) {
                log("onAdvertisingSetStarted(): txPower: " + txPower + " , status: " + status);
                advertisingSet = set;
                setupAdvertising();
            }

            @Override
            public void onAdvertisingSetStopped(AdvertisingSet advertisingSet) {
                log("onAdvertisingSetStopped():");
            }
        };

        ActivityCompat.requestPermissions(tester, new String[]{Manifest.permission.BLUETOOTH_ADVERTISE}, 1);

        if (ActivityCompat.checkSelfPermission(tester, Manifest.permission.BLUETOOTH_ADVERTISE) != PackageManager.PERMISSION_GRANTED) {
            fail("Failed to get advertise permissions");
            return;
        }

        final ParcelUuid Service_UUID = ParcelUuid.fromString("6ef0e30d-7308-4458-b62e-f706c692ca77");

        AdvertiseData.Builder dataBuilder = new AdvertiseData.Builder();

        //dataBuilder.addServiceData(Service_UUID, "CA2AX-H1-CA2A".getBytes());
        dataBuilder.addManufacturerData(0x04D8, new byte[]{2, (byte)175, (byte)166, 0, 0});
        dataBuilder.setIncludeDeviceName(false);
        //dataBuilder.addServiceUuid(Service_UUID);

        //advertiser.startAdvertisingSet(parameters.build(), dataBuilder.build(), null, null, null, callback);

        AdvertiseCallback advertiseCallback = new AdvertiseCallback() {
            @Override
            public void onStartSuccess(AdvertiseSettings settingsInEffect) {
                log("advertising start success");
            }
            @Override
            public void onStartFailure(int errorCode) {
                fail("start failure: " + errorCode);
            }
        };

        advertiser.startAdvertising(settings, dataBuilder.build(), advertiseCallback);

    }

    @SuppressLint("MissingPermission")
    @RequiresApi(api = Build.VERSION_CODES.O)
    static void setupAdvertising() {

        advertisingSet.enableAdvertising(true, 0, 0);
        advertisingSet.setAdvertisingParameters(parameters.setTxPowerLevel
                (AdvertisingSetParameters.TX_POWER_LOW).build());

        if (false) {
            // Wait for onAdvertisingDataSet callback...

            // Can also stop and restart the advertising
            advertisingSet.enableAdvertising(false, 0, 0);
            // Wait for onAdvertisingEnabled callback...
            advertisingSet.enableAdvertising(true, 0, 0);
            // Wait for onAdvertisingEnabled callback...

            // Or modify the parameters - i.e. lower the tx power
            advertisingSet.enableAdvertising(false, 0, 0);
            // Wait for onAdvertisingEnabled callback...
            advertisingSet.setAdvertisingParameters(parameters.setTxPowerLevel
                    (AdvertisingSetParameters.TX_POWER_LOW).build());
            // Wait for onAdvertisingParametersUpdated callback...
            advertisingSet.enableAdvertising(true, 0, 0);
            // Wait for onAdvertisingEnabled callback...

            // When done with the advertising:
            //advertiser.stopAdvertisingSet(callback);
        }
    }
}
