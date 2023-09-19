// Native Java interface for JNI to use sockets - socket() doesn't work for some reason (probably thread issues)
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.os.Build;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.SocketTimeoutException;

public class WiFiComm {
    private static final String TAG = "wificomm";
    private Socket socket = null;
    private InputStream inputStream = null;
    private OutputStream outputStream = null;

    public boolean killSwitch = true;

    // Request to open a socket over WiFi - LTE connection is often preferred by Android,
    // since Fuji's IP is 192.168.0.1
    // Don't run this in the UI thread, and don't run more than once at a time
    static int currentNetworkCallbackDone = 0;
    static Socket currentNetworkCallbackSocket = null;
    public static Socket connectWiFiSocket(ConnectivityManager connectivityManager, String ip, int port) {
        currentNetworkCallbackSocket = null;
        currentNetworkCallbackDone = 0;

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
                    Backend.print("Connection timed out\n");
                    Log.e(TAG, e.toString());
                    currentNetworkCallbackDone = -1;
                } catch (Exception e) {
                    Backend.print("Failed to connect to the camera\n");
                    Log.e(TAG, e.toString());
                    currentNetworkCallbackDone = -1;
                }
                if (currentNetworkCallbackDone != -1) {
                    currentNetworkCallbackDone = 1;
                }
                connectivityManager.unregisterNetworkCallback(this);
            }
        };

        if (Build.VERSION.SDK_INT >= 26) {
            connectivityManager.requestNetwork(requestBuilder.build(), networkCallback, Backend.OPEN_TIMEOUT);
        } else {
            connectivityManager.requestNetwork(requestBuilder.build(), networkCallback);
        }
        Log.d(TAG, "Requested wifi network usage");

        // Low tech solution for async execution
        int waits = 0;
        while (true) {
            // If network is not provided within 1s, assume WiFi is disabled
            if (waits > 1000) {
                Backend.print("Not connected to Fuji WiFi\n");
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

    private Socket cmdSocket = null;
    private InputStream cmdInputStream = null;
    private OutputStream cmdOutputStream = null;
    public boolean fujiConnectToCmd(ConnectivityManager m) {
        cmdSocket = connectWiFiSocket(m, Backend.FUJI_IP, Backend.FUJI_CMD_PORT);
        if (cmdSocket == null) {
            return true;
        }

        try {
            cmdSocket.setSoTimeout(Backend.TIMEOUT);
            cmdInputStream = cmdSocket.getInputStream();
            cmdOutputStream = cmdSocket.getOutputStream();
            killSwitch = false;
            Log.d(TAG, "Successful connection established");
        } catch (Exception e) {
            // TODO: Handle
            return true;
        }

        return false;
    }

    private Socket eventSocket = null;
    private Socket videoSocket = null;
    public boolean fujiConnectEventAndVideo(ConnectivityManager m) {
        eventSocket = connectWiFiSocket(m, Backend.FUJI_IP, Backend.FUJI_EVENT_PORT);
        if (eventSocket == null) {
            return true;
        }

        videoSocket = connectWiFiSocket(m, Backend.FUJI_IP, Backend.FUJI_VIDEO_PORT);
        if (videoSocket == null) {
            return true;
        }

        return false;
    }

    public int cmdWrite(byte[] data) {
        if (killSwitch) {
            Log.d(TAG, "kill switch on, breaking request");
            return -1;
        }
        try {
            cmdOutputStream.write(data);
            cmdOutputStream.flush();
            return data.length;
        } catch (IOException e) {
            Backend.print("Error writing to the server: " + e.getMessage() + "\n");
            return -1;
        }
    }

    public int cmdRead(byte[] buffer, int length) {
        int read = 0;
        while (true) {
            try {
                if (killSwitch) return -1;

                int rc = cmdInputStream.read(buffer, read, length - read);
                if (rc == -1) return rc;
                read += rc;
                if (read == length) return read;

                // Post progress percentage to progressBar
                final int progress = (int)((double)read / (double)length * 100.0);
                if (Viewer.handler == null) continue;
                Viewer.handler.post(new Runnable() {
                    @Override
                    public void run() {
                        Viewer.progressBar.setProgress(progress);
                    }
                });
            } catch (IOException e) {
                Backend.print("Error reading " + length + " bytes: " + e.getMessage() + "\n");
                return -1;
            }
        }
    }

    public int generic_write(byte data[]) {
        if (killSwitch) return -1;
        try {
            cmdOutputStream.write(data);
            cmdOutputStream.flush();
            return data.length;
        } catch (IOException e) {
            Backend.print("Error writing to the server: " + e.getMessage() + "\n");
            return -1;
        }
    }

    public int genericWrite(OutputStream output, byte[] data) {
        if (killSwitch) return -1;
        try {
            output.write(data);
            output.flush();
            return data.length;
        } catch (IOException e) {
            Backend.print("Error writing to the server: " + e.getMessage() + "\n");
            return -1;
        }
    }

    public int genericRead(InputStream input, byte[] buffer, int length) {
        int read = 0;
        while (true) {
            try {
                if (killSwitch) return -1;

                int rc = input.read(buffer, read, length - read);
                if (rc == -1) return rc;
                read += rc;
                if (read == length) return read;
            } catch (IOException e) {
                Backend.print("Error reading " + length + " bytes: " + e.getMessage() + "\n");
                return -1;
            }
        }
    }

    public synchronized void close() {
        killSwitch = true;

        try {
            // Suck the remaining bytes out of socket
            byte[] remaining = new byte[100];
            cmdInputStream.read(remaining);
        } catch (Exception e) {
            // I don't care
        }

        try {
            Log.e("fudge", "Closing sockets");

            if (cmdSocket != null) {
                cmdSocket.close();
            }
            if (cmdInputStream != null) {
                cmdInputStream.close();
            }
            if (cmdOutputStream != null) {
                cmdOutputStream.close();
            }
        } catch (IOException e) {
            Backend.print("Error closing the socket: " + e.getMessage() + "\n");
        }
    }
}
