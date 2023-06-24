// Native Java interface for JNI to use sockets - socket() doesn't work for some reason (probably thread issues)
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.util.Log;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.SocketTimeoutException;
import android.util.Log;

public class Conn {
    private static Socket socket;
    private static InputStream inputStream;
    private static OutputStream outputStream;

    public static boolean connect(String ipAddress, int port, int timeout) {
        try {
            socket = new Socket();
            socket.connect(new java.net.InetSocketAddress(ipAddress, port), timeout);
            socket.setSoTimeout(timeout);
            inputStream = socket.getInputStream();
            outputStream = socket.getOutputStream();
            return false;
        } catch (SocketTimeoutException e) {
            Backend.jni_print("Connection timed out.\n");
        } catch (IOException e) {
            Backend.jni_print("Error connecting to the server: " + e.getMessage() + "\n");
        }
        return true;
    }

    public static int write(byte[] data) {
        try {
            outputStream.write(data);
            outputStream.flush();
            return data.length;
        } catch (IOException e) {
            Backend.jni_print("Error writing to the server: " + e.getMessage() + "\n");
            return -1;
        }
    }

    public static int read(byte[] buffer, int length) {
        int read = 0;
        while (true) {
            try {
                int rc = inputStream.read(buffer, read, length - read);
                if (rc == -1) return rc;
                read += rc;
                if (read == length) return read;

                final int progress = (int)((double)read / (double)length * 100.0);
                if (Viewer.handler == null) continue;
                Viewer.handler.post(new Runnable() {
                    @Override
                    public void run() {
                        Viewer.progressBar.setProgress(progress);
                    }
                });
            } catch (IOException e) {
                Backend.jni_print("Error reading " + length + " bytes: " + e.getMessage() + "\n");
                return -1;
            }
        }
    }

    public static synchronized void close() {
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
            Backend.jni_print("Error closing the socket: " + e.getMessage() + "\n");
        }
    }
}
