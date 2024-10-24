// Custom Java bindings to Fujifilm/camlib
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp

package dev.danielc.fujiapp;
import android.content.Context;
import android.os.PatternMatcher;
import android.util.Log;
import android.os.Environment;
import android.view.View;

import java.io.File;
import java.util.Locale;

import dev.danielc.common.Camlib;
import dev.danielc.common.WiFiComm;

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
    final static int FUJI_FEATURE_WIRELESS_TETHER = 2;
    final static int FUJI_FEATURE_WIRELESS_COMM = 3;
    final static int FUJI_FEATURE_USB_CARD_READER = 4;
    final static int FUJI_FEATURE_USB_TETHER_SHOOT = 5;
    final static int FUJI_FEATURE_RAW_CONV = 6;

    final static PatternMatcher matchAp = new PatternMatcher("FUJIFILM", PatternMatcher.PATTERN_PREFIX);

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

    /** Allocates initial memory */
    public native static void cInit();

    /** IO kill switch is in C/camlib, so we must set it when a connection is established */
    public native static boolean cGetKillSwitch();
    /** Get transport type (enum FujiTransport) */
    public native static int cGetTransport();
    /** Establish connection over USB-OTG */
    public native static int cTryConnectUSB(Context appContext);
    /** establish connection over default wifi network */
    public native static int cTryConnectWiFi(int extraTmout);
    /** Establish connection from information in fuji->info struct */
    public native static int cConnectFromDiscovery();
    /** Setup connection and do init routines according to transport type */
    public native static int cFujiSetup();
    /** Poll for events once */
    public native static int cPtpFujiPing();
    /** Gets a plain list of all object handles on the camera starting from 1 */
    public native static int[] cGetObjectHandles();
    /** Setup and configure settings for image gallery (thumbnails, obj info) */
    public native static int cFujiConfigImageGallery();
    /** GetThumb if possible, otherwise grab exif thumb from file header */
    public native static byte[] cFujiGetThumb(int handle);
    /** Run mass gallery photo importer based on transport */
    public native static int cFujiImportFiles(int[] handles, int filter_mask);

    // For tester only
    public native static int cFujiTestSuite();

    /** Must be called before cFujiGetFile (GetEvent calls should work inbetween) */
    public native static String cFujiBeginDownloadGetObjectInfo(int handle);
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
    static int lastFilenameNum = 0;
    public static String newTetherPhoto() {
        while (true) {
            String filename = getDownloads() + File.separator + "TETHER" + String.format(Locale.US, "%03d", lastFilenameNum) + ".JPG";
            File test = new File(filename);
            if (!test.exists()) {
                return filename;
            } else {
                Log.d("b", "File exists: " + filename);
            }
            lastFilenameNum++;
        }
    }
    public static String getFilePath(String filename) {
        return getDownloads() + File.separator + filename;
    }
}
