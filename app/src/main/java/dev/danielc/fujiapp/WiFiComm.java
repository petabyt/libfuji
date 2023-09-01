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
    private Socket socket = null;
    private InputStream inputStream = null;
    private OutputStream outputStream = null;

    public boolean killSwitch = true;

    // Don't run this in the UI thread, and don't run more than once at a time
    static int currentNetworkCallbackDone = 0;
    static Socket currentNetworkCallbackSocket = null;
    public static Socket connectWiFiSocket(ConnectivityManager connectivityManager, String ip, int port) {
        NetworkRequest.Builder requestBuilder = new NetworkRequest.Builder();
        requestBuilder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        ConnectivityManager.NetworkCallback networkCallback = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                ConnectivityManager.setProcessDefaultNetwork(network);
                currentNetworkCallbackSocket = new Socket();
                try {
                    currentNetworkCallbackSocket.connect(new java.net.InetSocketAddress(ip, port), Backend.TIMEOUT);
                }  catch (SocketTimeoutException e) {
                    Backend.print("Connection timed out\n");
                    currentNetworkCallbackDone = -1;
                } catch (IOException e) {
                    Backend.print("Failed to connect to the camera\n");
                    currentNetworkCallbackDone = -1;
                }
                if (currentNetworkCallbackDone != -1) {
                    currentNetworkCallbackDone = 1;
                }
                connectivityManager.unregisterNetworkCallback(this);
            }
        };

        if (Build.VERSION.SDK_INT >= 26) {
            connectivityManager.requestNetwork(requestBuilder.build(), networkCallback, 500);
        } else {
            connectivityManager.requestNetwork(requestBuilder.build(), networkCallback);
        }

        // Low tech solution for async execution
        while (true) {
            if (currentNetworkCallbackDone == 1) {
                return currentNetworkCallbackSocket;
            } else if (currentNetworkCallbackDone == -1) {
                return null;
            }

            try {
                Thread.sleep(5);
            } catch (Exception e) {
                // TODO: Handle
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
        } catch (Exception e) {
            // TODO: Handle
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
            Log.e("fudge", "Closing sockets");
            if (cmdInputStream != null) {
                cmdInputStream.close();
            }
            if (cmdOutputStream != null) {
                cmdOutputStream.close();
            }
            if (cmdSocket != null) {
                cmdSocket.close();
            }
        } catch (IOException e) {
            Backend.print("Error closing the socket: " + e.getMessage() + "\n");
        }
    }
}
