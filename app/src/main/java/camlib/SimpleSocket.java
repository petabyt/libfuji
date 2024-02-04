package camlib;

import android.net.ConnectivityManager;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

import dev.danielc.fujiapp.Backend;

public class SimpleSocket {
    private static ConnectivityManager m = null;
    public static void setConnectivityManager(ConnectivityManager m) {
        SimpleSocket.m = m;
    }

    public int port;
    public String ip;
    public boolean alive;

    int timeout = 2000;

    Socket socket;
    InputStream inputStream;
    OutputStream outputStream;

    public String failReason;

    private byte[] buffer = null;
    private int bufferSize = 512 * 10;

    public SimpleSocket() {
        this.buffer = new byte[bufferSize];
    }

    public Object getBuffer() {
        return this.buffer;
    }

    public int getBufferSize() {
        return this.bufferSize;
    }

    public void connectWiFi(String ip, int port) throws Exception {
        Socket s;
        try {
            s = WiFiComm.connectWiFiSocket(m, ip, port);
        } catch (Exception e) {
            throw e;
        }

        this.ip = ip;
        this.port = port;

        s.setSoTimeout(timeout);
        this.inputStream = s.getInputStream();
        this.outputStream = s.getOutputStream();

        alive = true;
    }

    public int read(int size) {
        try {
            return inputStream.read(buffer, 0, size);
        } catch (IOException e) {
            failReason = e.toString();
            //alive = false;
            return -1;
        }
    }

    public int write(int size) {
        try {
            outputStream.write(buffer, 0, size);
            outputStream.flush();
            return size;
        } catch (IOException e) {
            failReason = e.toString();
            //alive = false;
            return -1;
        }
    }

    public void close() {
        alive = false;
        try {
            // Suck the remaining bytes out of socket
            byte[] remaining = new byte[100];
            inputStream.read(remaining);
        } catch (Exception e) {
            // I don't care
        }

        try {
            if (socket != null) {
                socket.close();
            }
            if (inputStream != null) {
                inputStream.close();
            }
            if (outputStream != null) {
                outputStream.close();
            }
        } catch (IOException e) {
            Backend.print("Socket close error: " + e.getMessage());
        }
    }
}
