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

        NetworkRequest.Builder requestBuilder = new NetworkRequest.Builder();
        requestBuilder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        ConnectivityManager.NetworkCallback networkCallback = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                Log.d(TAG, "Wifi available");
                ConnectivityManager.setProcessDefaultNetwork(network);
                try {
                    // Create and connect to socket
                    currentNetworkCallbackSocket = new Socket(ip, port);
                    currentNetworkCallbackSocket.setKeepAlive(true);
                    currentNetworkCallbackSocket.setTcpNoDelay(true);
                    currentNetworkCallbackSocket.setReuseAddress(true);
                }  catch (SocketTimeoutException e) {
                    failReason = "Connection timed out";
                    Log.e(TAG, e.toString());
                    currentNetworkCallbackDone = -1;
                } catch (Exception e) {
                    failReason = "Failed to connect to the camera";
                    Log.e(TAG, e.toString());
                    currentNetworkCallbackDone = -1;
                }
                if (currentNetworkCallbackDone != -1) {
                    currentNetworkCallbackDone = 1;
                }
                connectivityManager.unregisterNetworkCallback(this);
            }
            @Override
            public void onLost(Network network) {
                Log.e(TAG, "Lost network\n");
            }
        };

        if (Build.VERSION.SDK_INT >= 26) {
            connectivityManager.requestNetwork(requestBuilder.build(), networkCallback, CamlibBackend.OPEN_TIMEOUT);
        } else {
            connectivityManager.requestNetwork(requestBuilder.build(), networkCallback);
        }
        Log.d(TAG, "Requested wifi network usage");

        // Low tech solution for async execution
        int waits = 0;
        while (true) {
            // If network is not provided within 1s, assume WiFi is disabled
            if (waits > 1000) {
                failReason = "Not connected to Fuji WiFi\n";
                return null;
            }

            if (currentNetworkCallbackDone == 1) {
                return currentNetworkCallbackSocket;
            } else if (currentNetworkCallbackDone == -1) {
                return null;
            }

            try {
                Thread.sleep(1);
                waits++;
            } catch (Exception e) {
                Log.e(TAG, "Sleep fail (???)");
            }
        }
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
