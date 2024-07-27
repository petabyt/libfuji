// Custom Java bindings to Fujifilm/camlib
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp

package dev.danielc.fujiapp;
import android.content.Context;
import android.hardware.usb.UsbManager;
import android.util.Log;
import android.os.Environment;
import android.view.View;

import java.io.File;
import org.json.JSONObject;

import camlib.*;

public class Backend extends Camlib {
    static {
        System.loadLibrary("fudge");
    }

    static SimpleUSB usb = new SimpleUSB();

    public static void connectUSB(Context ctx) throws Exception {
        UsbManager man = (UsbManager)ctx.getSystemService(Context.USB_SERVICE);
        usb.getUsbDevices(man);

        Frontend.print("Trying to get permission...");
        usb.waitPermission(ctx);

        for (int i = 0; i < 100; i++) {
            if (usb.havePermission()) {
                Log.d("perm", "Have USB permission");
                continueOpenUSB();
                break;
            }
            try {
                Thread.sleep(50);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    static void continueOpenUSB() {
        try {
            usb.openConnection();
            usb.getInterface();
            usb.getEndpoints();

            cUSBConnectNative(usb);

            cClearKillSwitch();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    static String chosenIP = Backend.FUJI_IP;

    public static int fujiConnectToCmd() {
        return cTryConnectWiFi();
    }

    // Block all communication in UsbComm and WiFiComm
    // Write reason + code, and reconnect popup
    public native static void cReportError(int code, String reason);
    public static void reportError(int code, String reason) {
        if (Backend.cGetKillSwitch()) return;
        discoveryThread(MainActivity.instance);
        Log.d("fudge", reason);
        cReportError(code, reason);
    }

    private static boolean haveInited = false;
    public static void init() {
        if (!haveInited) {
            cInit();
        }
        haveInited = true;
    }

    // Constants
    public static final String FUJI_EMU_IP = "192.168.1.33"; // IP addr of my laptop
    public static final String FUJI_IP = "192.168.0.1";
    public static final int FUJI_CMD_PORT = 55740;

    // IO kill switch is in C/camlib, so we must set it when a connection is established
    public native static void cClearKillSwitch();
    public native static boolean cGetKillSwitch();

    public native static int cUSBConnectNative(SimpleUSB usb);
    public native static int cTryConnectWiFi();
    public native static int cConnectNative(String ip, int port);
    public native static void cInit();
    public native static int cFujiSetup(String ip);
    public native static int cPtpFujiPing();
    public native static int[] cGetObjectHandles();
    public native static int cFujiConfigImageGallery();
    public native static byte[] cFujiGetThumb(int handle);

    // For tester only
    public native static int cFujiTestSuite(String ip);

    // Must be called in order - first one enables compression, second disables compression
    // It must be this way to be as optimized as possible
    public native static String cFujiGetUncompressedObjectInfo(int handle);
    public native static int cFujiGetFile(int handle, byte[] array, int fileSize);
    public native static int cFujiDownloadFile(int handle, String path);
    public native static int cCancelDownload();
    public native static int cSetProgressBarObj(Object progressBar, int size);

    // For test suite only
    public native static void cTesterInit(Tester t);
    public native static int cRouteLogs();
    public native static String cEndLogs();

    public native static View cFujiScriptsScreen(Context ctx);

    final static int DISCOVERY_ERROR_THRESHOLD = 5;
    public native static int cStartDiscovery(Context ctx);
    static Thread discoveryThread = null;
    public static void discoveryThread(Context ctx) {
        if (discoveryThread != null) {
            Log.d("backend", "Discovery thread already running");
        }
        discoveryThread = new Thread(new Runnable() {
            @Override
            public void run() {
                Frontend.discoveryIsActive();
                while (true) {
                    long start_time = System.nanoTime();
                    int rc = Backend.cStartDiscovery(ctx);
                    if (rc < 0) break;
                    long end_time = System.nanoTime();
                    Log.d("backend", "cstartdiscovery: " + ((end_time - start_time) / 1e6));
                    if (((end_time - start_time) / 1e6) < DISCOVERY_ERROR_THRESHOLD) {
                        Frontend.discoveryFailed();
                        break;
                    }
                }
                Backend.discoveryThread = null;
            }
        });
        discoveryThread.start();
        Log.d("backend", "Ending discovery thread");
    }
    public static native void cancelDiscoveryThread();

    // Return directory is guaranteed to exist
    public static String getDownloads() {
        String mainStorage = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES).getPath();
        String fujifilm = mainStorage + File.separator + "fudge";
        File directory = new File(fujifilm);
        if (!directory.exists()) {
            directory.mkdirs();
        }
        return fujifilm;
    }
}
