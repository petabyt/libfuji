// Basic wifi-priority socket interface for camlib
// Copyright Daniel Cook - Apache License
package camlib;

import static android.net.wifi.WifiManager.STATUS_NETWORK_SUGGESTIONS_SUCCESS;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.MacAddress;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import android.net.NetworkSpecifier;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkSpecifier;
import android.net.wifi.WifiNetworkSuggestion;
import android.os.Build;
import android.os.Handler;
import android.os.PatternMatcher;
import android.provider.Settings;
import android.util.Log;

import java.util.ArrayList;

public class WiFiComm {
    public static final String TAG = "camlib";

    private static ConnectivityManager cm = null;
    public static void setConnectivityManager(ConnectivityManager cm) {
        WiFiComm.cm = cm;
    }

    static Network wifiDevice = null;
    static Network foundWiFiDevice = null;

    public static Handler handler = null;
    public static Runnable onAvailable = null;
    public static Runnable onWiFiSelectAvailable = null;
    public static Runnable onWiFiSelectCancel = null;

    static void run(Runnable r) {
        if (r != null) {
            if (handler == null) {
                r.run();
            } else {
                handler.post(r);
            }
        }
    }

    public static int connectToAccessPoint(Context ctx, String password) {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q) {
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
                    Log.d("wifi", "Network availaable");
                    foundWiFiDevice = network;
                    run(onWiFiSelectAvailable);
                }
                @Override
                public void onUnavailable() {
                    Log.d("wifi", "Network unavailable");
                    foundWiFiDevice = null;
                    run(onWiFiSelectCancel);
                }
            };
            connectivityManager.requestNetwork(request, networkCallback);
        } else {
            return 1;
        }
        return 0;
    }

    public static void startNetworkListeners(Context ctx) {
        ConnectivityManager m = (ConnectivityManager)ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkRequest.Builder requestBuilder = new NetworkRequest.Builder();
        requestBuilder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        ConnectivityManager.NetworkCallback networkCallback = new ConnectivityManager.NetworkCallback() {
            Intent settings = null;
            @Override
            public void onAvailable(Network network) {
                Log.d(TAG, "Wifi network is available");
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

    public static long getNetworkHandle() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            return UNSUPPORTED_SDK;
        }
        NetworkInfo wifiInfo = cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (!wifiInfo.isAvailable()) {
            return NOT_AVAILABLE;
        } else if (!wifiInfo.isConnected()) {
            return NOT_CONNECTED;
        }

        if (wifiDevice != null) {
            return wifiDevice.getNetworkHandle();
        }

        if (foundWiFiDevice != null) {
            return foundWiFiDevice.getNetworkHandle();
        }

        return NOT_AVAILABLE;
    }
}
