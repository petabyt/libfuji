// Basic wifi-priority socket interface for camlib
// Copyright Daniel Cook - Apache License
package dev.danielc.common;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import android.net.NetworkSpecifier;
import android.net.Uri;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiNetworkSpecifier;
import android.os.Build;
import android.os.Handler;
import android.os.PatternMatcher;
import android.provider.Settings;
import android.util.Log;
import 	android.net.wifi.WifiManager;

import java.lang.reflect.Method;

public class WiFiComm {
    public static final String TAG = "wifi";

    private static ConnectivityManager cm = null;
    public void setConnectivityManager(ConnectivityManager cm) {
        WiFiComm.cm = cm;
    }

    static Network wifiDevice = null;
    static Network foundWiFiDevice = null;

    public Handler handler = null;
    public Runnable onAvailable = null;
    public Runnable onWiFiSelectAvailable = null;
    public Runnable onWiFiSelectCancel = null;
    public boolean blockEvents = false;

    void run(Runnable r) {
        if (blockEvents) return;
        if (r != null) {
            if (handler == null) {
                r.run();
            } else {
                handler.post(r);
            }
        }
    }

    // This will cause 2x slowdown for concurrent connections:
    // https://source.android.com/docs/core/connect/wifi-sta-sta-concurrency#local-only
    public int connectToAccessPoint(Context ctx, String password) {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q) {
            Log.d("wifi", "Connecting: " + password);
            WifiNetworkSpecifier.Builder builder = new WifiNetworkSpecifier.Builder();
            builder.setSsidPattern(new PatternMatcher("FUJIFILM", PatternMatcher.PATTERN_PREFIX));

            if (password != null) {
                builder.setWpa2Passphrase(password);
            }
            NetworkSpecifier specifier = builder.build();

            NetworkRequest request = new NetworkRequest.Builder()
                            .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                            .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                            .setNetworkSpecifier(specifier)
                            .build();

            ConnectivityManager connectivityManager = (ConnectivityManager)ctx.getSystemService(Context.CONNECTIVITY_SERVICE);

            ConnectivityManager.NetworkCallback networkCallback = new ConnectivityManager.NetworkCallback() {
                @Override
                public void onAvailable(Network network) {
                    Log.d("wifi", "Network selected by user: " + network);
                    foundWiFiDevice = network;
                    run(onWiFiSelectAvailable);
                }
                @Override
                public void onUnavailable() {
                    Log.d("wifi", "Network unavailable, not selected by user");
                    foundWiFiDevice = null;
                    run(onWiFiSelectCancel);
                }
            };
            connectivityManager.requestNetwork(request, networkCallback);
            // Disconnect after inactive?
            // connectivityManager.unregisterNetworkCallback(this);
        } else {
            return 1;
        }
        return 0;
    }

    public void startNetworkListeners(Context ctx) {
        ConnectivityManager m = (ConnectivityManager)ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkRequest.Builder requestBuilder = new NetworkRequest.Builder();
        requestBuilder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        //requestBuilder.addTransportType(NetworkCapabilities.TRANSPORT_CELLULAR);

        ConnectivityManager.NetworkCallback networkCallback = new ConnectivityManager.NetworkCallback() {
            Intent settings = null;
            @Override
            public void onAvailable(Network network) {
//                if (m.getNetworkCapabilities(network).hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR)) {
                Log.d(TAG, "Wifi network is available: " + network.getNetworkHandle());
                if (settings != null) {
                    ((Activity)ctx).finish();
                }

                wifiDevice = network;
                run(onAvailable);
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
            @Override
            public void onCapabilitiesChanged(Network network, NetworkCapabilities networkCapabilities) {
                Log.e(TAG, "capabilities changed");
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

    public static final int NOT_AVAILABLE = -101;
    public static final int NOT_CONNECTED = -102;
    public static final int UNSUPPORTED_SDK = -103;

    public static boolean isNetworkValid(Network net) {
        if (cm == null) return false;
        NetworkInfo wifiInfo = cm.getNetworkInfo(net);
        if (net == null) return false;
        if (wifiInfo == null) return false;
        return wifiInfo.isAvailable();
    }

    // If we go through WifiNetworkSpecifier, then the device may be handling two concurrent connections.
    // This is up to a 2x performance hit.
    public static boolean isWiFiModuleHandlingTwoConnections(Context ctx) {
        WifiManager wm = (WifiManager)ctx.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            // 'Query whether or not the device supports concurrent station (STA) connections
            // for local-only connections using WifiNetworkSpecifier.'
            return wm.isStaConcurrencyForLocalOnlyConnectionsSupported();
        }
        // If below 31, then Android supposedly doesn't support concurrent connections at all
        return false;
    }

    public static boolean isHotSpotEnabled(Context ctx) {
        WifiManager wm = (WifiManager)ctx.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        try {
            Method m = wm.getClass().getDeclaredMethod("isWifiApEnabled");
            m.setAccessible(true);
            if ((boolean)m.invoke(wm) == false) {
                return false;
            }
        } catch (Exception e) {
            return false;
        }

        return true;
    }

    @SuppressLint("ObsoleteSdkInt")
    public static long getNetworkHandle() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            return UNSUPPORTED_SDK;
        }

        // Prefer user-selected network
        if (isNetworkValid(foundWiFiDevice)) {
            Log.d("wifi", "Returning found wifi device");
            return foundWiFiDevice.getNetworkHandle();
        }

        if (isNetworkValid(wifiDevice)) {
            Log.d("wifi", "Returning default WiFi");
            return wifiDevice.getNetworkHandle();
        }

        Log.d("wifi", "WiFi network not available");
        return NOT_AVAILABLE;
    }

    public static void openHotSpotSettings(Context ctx) {
        Intent tetherSettings = new Intent();
        tetherSettings.setClassName("com.android.settings", "com.android.settings.TetherSettings");
        ctx.startActivity(tetherSettings);
    }

    public static void openWiFiSettings(Activity ctx) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            ctx.startActivityForResult(new Intent(Settings.Panel.ACTION_WIFI), 0);
        } else {
            ctx.startActivity(new Intent(Settings.ACTION_WIFI_SETTINGS));
        }
    }
}
