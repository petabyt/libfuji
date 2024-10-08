// Custom Java bindings to Fujifilm/camlib
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp

package dev.danielc.fujiapp;
import android.content.Context;
import android.hardware.usb.UsbManager;
import android.util.Log;
import android.os.Environment;
import android.view.View;

import java.io.File;

import dev.danielc.common.Camlib;
import dev.danielc.common.SimpleUSB;

public class Backend extends Camlib {
    static {
        System.loadLibrary("fudge");
    }

    // Discovery errors
    final static int FUJI_D_REGISTERED = 1;
    final static int FUJI_D_GO_PTP = 2;
    final static int FUJI_D_CANCELED = 3;
    final static int FUJI_D_IO_ERR = 4;
    final static int FUJI_D_OPEN_DENIED = 5;
    final static int FUJI_D_INVALID_NETWORK = 6;
    final static int DISCOVERY_ERROR_THRESHOLD = 5;

    final static int FUJI_FEATURE_AUTOSAVE = 1;
    final static int FUJI_FEATURE_WIRELESS_COMM = 3;

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
                Thread.sleep(50); // ???
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

    public static int fujiConnectToCmd(int extraTmout) {
        return cTryConnectWiFi(extraTmout);
    }

    // Block all communication in UsbComm and WiFiComm
    // Write reason + code, and reconnect popup
    public native static void cReportError(int code, String reason);
    public static void reportError(int code, String reason) {
        if (Backend.cGetKillSwitch()) return;
        Log.d("fudge", reason);
        cReportError(code, reason);
        discoveryThread(MainActivity.instance);
    }

    public static void reportErrorNonBlock(int code, String reason) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                reportError(code, reason);
            }
        }).start();
    }

    private static boolean haveInited = false;
    public static void init() {
        if (!haveInited) {
            cInit();
        }
        haveInited = true;
    }

    /** IO kill switch is in C/camlib, so we must set it when a connection is established */
    public native static void cClearKillSwitch();
    public native static boolean cGetKillSwitch();
    public native static int cGetTransport();
    public native static int cUSBConnectNative(SimpleUSB usb);
    /** establish connection over default wifi network */
    public native static int cTryConnectWiFi(int extraTmout);
    /** establish connection from the parameters of the discovery struct */
    public native static int cConnectFromDiscovery(byte[] struct);
    public native static void cInit();
    public native static int cFujiSetup();
    public native static int cPtpFujiPing();
    public native static int[] cGetObjectHandles();
    public native static int cFujiConfigImageGallery();
    public native static byte[] cFujiGetThumb(int handle);
    /** Run mass gallery photo importer based on transport */
    public native static int cFujiImportFiles(int[] handles, int filter_mask);

    // For tester only
    public native static int cFujiTestSuite();

    // Must be called in order - first one enables compression, second disables compression
    // It must be this way to be as optimized as possible
    public native static String cFujiGetUncompressedObjectInfo(int handle);
    public native static int cFujiGetFile(int handle, byte[] array, int fileSize);
    public native static int cFujiDownloadFile(int handle, String path);

    // For test suite only
    public native static int cRouteLogs();
    public native static String cEndLogs();

    public native static View cFujiScriptsScreen(Context ctx);

    static Thread discoveryThread = null;
    public static void discoveryThread(Context ctx) {
        if (discoveryThread != null) {
            Log.d("backend", "Discovery thread already running");
            return;
        } else {
            Log.d("discovery", "Discovery starting");
        }
        discoveryThread = new Thread(new Runnable() {
            @Override
            public void run() {
                Frontend.discoveryIsActive();
                while (true) {
                    long start_time = System.nanoTime();
                    int rc = Backend.cStartDiscovery(ctx);
                    if (rc < 0) break;
                    if (rc == FUJI_D_INVALID_NETWORK || rc == FUJI_D_IO_ERR || rc == FUJI_D_OPEN_DENIED) {
                        Log.d("discovery", "error: " + rc);
                        Frontend.discoveryFailed();
                        break;
                    }
                    if (rc == FUJI_D_CANCELED || rc == FUJI_D_GO_PTP) {
                        Log.d("discovery", "Go/cancel");
                        break;
                    }
                    Log.d("discovery", "code: " + rc);
                    long end_time = System.nanoTime();
                    Log.d("discovery", "cstartdiscovery: " + ((end_time - start_time) / 1e6));
                    if (((end_time - start_time) / 1e6) < DISCOVERY_ERROR_THRESHOLD) {
                        Frontend.discoveryFailed();
                        break;
                    }
                }
                Backend.discoveryThread = null;
            }
        });
        discoveryThread.start();
    }
    public native static int cStartDiscovery(Context ctx);
    public static void cancelDiscoveryThread() {
        if (discoveryThread != null) {
            discoveryThread.interrupt();
        }
    }

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
    public static String getFilePath(String filename) {
        return getDownloads() + File.separator + filename;
    }
}
