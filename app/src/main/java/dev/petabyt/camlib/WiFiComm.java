// Basic wifi-priority socket interface for camlib
// Copyright Daniel Cook - Apache License
package dev.petabyt.camlib;

import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import android.os.Build;
import android.util.Log;
import java.net.InetSocketAddress;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.SocketTimeoutException;

public class WiFiComm {
    public static final String TAG = "camlib";
    private Socket socket;
    private InputStream inputStream;
    private OutputStream outputStream;

    public static String failReason = null;

    public boolean killSwitch = true;

    static int currentNetworkCallbackDone = 0;
    static Socket currentNetworkCallbackSocket = null;

    static Network wifiDevice = null;

    static Socket tryConnectToSocket(Network net, String ip, int port) {
        failReason = "None yet";
        Socket sock;
        try {
            // Create and connect to socket
            sock = new Socket();

            // Bind socket to the network device we selected
            net.bindSocket(sock);
            
            //sock.setKeepAlive(true);
            sock.setTcpNoDelay(true);
            sock.setReuseAddress(true);

            sock.connect(new InetSocketAddress(ip, port), 1000);
        } catch (SocketTimeoutException e) {
            failReason = "Connection timed out";
            Log.d(TAG, e.toString());
            currentNetworkCallbackDone = -1;
            return null;
        } catch (Exception e) {
            failReason = "Failed to connect.";
            Log.d(TAG, e.toString());
            currentNetworkCallbackDone = -1;
            return null;
        }

        return sock;
    }

    public static void startNetworkListeners(ConnectivityManager connectivityManager) {
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

        connectivityManager.requestNetwork(requestBuilder.build(), networkCallback);
    }

    public static Socket connectWiFiSocket(ConnectivityManager connectivityManager, String ip, int port) {
        currentNetworkCallbackSocket = null;
        currentNetworkCallbackDone = 0;

        NetworkInfo wifiInfo = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (!wifiInfo.isAvailable()) {
            failReason = "WiFi is not available.";
            return null;
        } else if (!wifiInfo.isConnected()) {
            failReason = "Not connected to a WiFi network.";
            return null;
        }

        if (wifiDevice == null) {
            failReason = "Not connected to WiFi.";
            return null;
        }

        return tryConnectToSocket(wifiDevice, ip, port);
    }

    public boolean connect(String ipAddress, int port, int timeout) {
        try {
            socket = new Socket();
            socket.connect(new java.net.InetSocketAddress(ipAddress, port), timeout);
            socket.setSoTimeout(timeout);
            inputStream = socket.getInputStream();
            outputStream = socket.getOutputStream();
            killSwitch = false;
            return false;
        } catch (SocketTimeoutException e) {
            failReason = "No connection found.";
        } catch (IOException e) {
            failReason = "Error connecting to the server: " + e.getMessage();
        }
        return true;
    }

    public int write(byte[] data, int length) {
        if (killSwitch) return -1;
        try {
            outputStream.write(data);
            outputStream.flush();
            return data.length;
        } catch (IOException e) {
            failReason = "Error writing to the server: " + e.getMessage();
            return -1;
        }
    }

    public int read(byte[] buffer, int length) {
        int read = 0;
        while (true) {
            try {
                if (killSwitch) return -1;

                int rc = inputStream.read(buffer, read, length - read);
                if (rc == -1) return rc;
                read += rc;
                if (read == length) return read;

                // Post progress percentage to progressBar
//                final int progress = (int)((double)read / (double)length * 100.0);
//                if (Viewer.handler == null) continue;
//                Viewer.handler.post(new Runnable() {
//                    @Override
//                    public void run() {
//                        Viewer.progressBar.setProgress(progress);
//                    }
//                });
            } catch (IOException e) {
                failReason = "Error reading " + length + " bytes: " + e.getMessage();
                return -1;
            }
        }
    }

    public synchronized void close() {
        killSwitch = true;
        try {
            if (inputStream != null) {
                inputStream.close();
            }
            if (outputStream != null) {
                outputStream.close();
            }
            if (socket != null) {
                socket.close();
            }
        } catch (IOException e) {
            failReason = "Error closing the socket: " + e.getMessage();
        }
    }
}
