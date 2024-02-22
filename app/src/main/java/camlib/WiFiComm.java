// Basic wifi-priority socket interface for camlib
// Copyright Daniel Cook - Apache License
package camlib;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import android.util.Log;
import java.net.InetSocketAddress;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.ArrayList;

import dev.danielc.fujiapp.Backend;
import libui.LibUI;

public class WiFiComm {
    public static final String TAG = "camlib";

    private static ConnectivityManager cm = null;
    public static void setConnectivityManager(ConnectivityManager cm) {
        WiFiComm.cm = cm;
    }

    static Network wifiDevice = null;

    public static void startNetworkListeners(Context ctx) {
        ConnectivityManager m = (ConnectivityManager)ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkRequest.Builder requestBuilder = new NetworkRequest.Builder();
        requestBuilder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        ConnectivityManager.NetworkCallback networkCallback = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                Log.d(TAG, "Wifi network is available");
                wifiDevice = network;
            }
            @Override
            public void onLost(Network network) {
                Log.e(TAG, "Lost network\n");
                wifiDevice = null;
            }
            @Override
            public void onUnavailable() {
                Log.e(TAG, "Network unavailable\n");
                wifiDevice = null;
            }
        };

        try {
            m.requestNetwork(requestBuilder.build(), networkCallback);
        } catch (Exception e) {
            Intent goToSettings = new Intent(Settings.ACTION_MANAGE_WRITE_SETTINGS);
            goToSettings.setData(Uri.parse("package:" + ctx.getPackageName()));
            ctx.startActivity(goToSettings);
        }
    }

    public static Network getWiFiNetwork() throws Exception {
        NetworkInfo wifiInfo = cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (!wifiInfo.isAvailable()) {
            throw new Exception("WiFi is not available.");
        } else if (!wifiInfo.isConnected()) {
            throw new Exception("Not connected to a WiFi network.");
        }

        if (wifiDevice == null) {
            throw new Exception("Not connected to WiFi.");
        }

        return wifiDevice;
    }

    public static final int NOT_AVAILABLE = -101;
    public static final int NOT_CONNECTED = -102;
    public static final int UNSUPPORTED_SDK = -103;

    public static long getNetworkHandle() {
        NetworkInfo wifiInfo = cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (!wifiInfo.isAvailable()) {
            return NOT_AVAILABLE; // not available
        } else if (!wifiInfo.isConnected()) {
            return NOT_CONNECTED; // not connected
        }

        if (wifiDevice == null) {
            return NOT_CONNECTED;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return wifiDevice.getNetworkHandle();
        } else {
            return UNSUPPORTED_SDK;
        }
    }
}
