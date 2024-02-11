// Custom Java bindings to Fujifilm/camlib
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp

package dev.danielc.fujiapp;
import android.content.Context;
import android.hardware.usb.UsbManager;
import android.util.Log;
import android.os.Environment;
import java.io.File;
import org.json.JSONObject;
import java.util.Arrays;

import camlib.*;

public class Backend extends CamlibBackend {
    static {
        System.loadLibrary("fujiapp");
    }

    static SimpleUSB usb = new SimpleUSB();

    public static void connectUSB(Context ctx) throws Exception {
        UsbManager man = (UsbManager)ctx.getSystemService(Context.USB_SERVICE);
        usb.getUsbDevices(man);
        usb.openConnection();
        usb.getInterface();
        usb.getEndpoints();
    }

    static String chosenIP = Backend.FUJI_IP;

    public static void fujiConnectToCmd() throws Exception {
        Backend.print("Connecting...");

        int rc = cConnectNative(Backend.FUJI_IP, Backend.FUJI_CMD_PORT);
        if (rc != 0) {
            if (BuildConfig.DEBUG) {
                Backend.print("Trying emulator IP..");
                rc = cConnectNative(Backend.FUJI_EMU_IP, Backend.FUJI_CMD_PORT);
                chosenIP = Backend.FUJI_EMU_IP;
            }
        }

        if (rc == 0) {
            cClearKillSwitch();
        } else {
            throw new Exception(parseErr(rc));
        }
    }

    // Block all communication in UsbComm and WiFiComm
    // Write reason + code, and reconnect popup
    public native static void cReportError(int code, String reason);
    public static void reportError(int code, String reason) {
        if (Backend.cGetKillSwitch()) return;
        Log.d("fudge", reason);
        cReportError(code, reason);
    }

    // In order to give the backend access to the static methods, new objects must be made
    private static boolean haveInited = false;
    public static void init() {
        if (haveInited == false) {
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

    public native static int cConnectNative(String ip, int port);
    public native static void cInit();
    public native static int cFujiSetup(String ip);
    public native static int cPtpFujiPing();
    public native static int[] cGetObjectHandles();
    public native static int cFujiConfigImageGallery();

    // For tester only
    public native static int cFujiTestSuite(String ip);

    // Must be called in order - first one enables compression, second disables compression
    // It must be this way to be as optimized as possible
    public native static String cFujiGetUncompressedObjectInfo(int handle);
    public native static int cFujiGetFile(int handle, byte[] array, int fileSize);
    public native static int cFujiDownloadFile(int handle, String path);

    // For test suite only
    public native static void cTesterInit(Tester t);
    public native static int cRouteLogs();
    public native static String cEndLogs();

    public native static int cSetProgressBarObj(Object progressBar, int size);

    public native static void cFujiScriptsScreen(Context ctx);

    public static JSONObject fujiGetUncompressedObjectInfo(int handle) throws Exception {
        try {
            String resp = cFujiGetUncompressedObjectInfo(handle);
            if (resp == null) throw new Exception("Failed to get obj info");
            return new JSONObject(resp);
        } catch (Exception e) {
            throw e;
        }
    }

    // C/Java -> async UI logging
    final static int MAX_LOG_LINES = 5;

    public static void clearPrint() {
        basicLog = "";
        updateLog();
    }

    // debug function for both Java frontend and JNI backend
    private static String basicLog = "";
    public static void print(String arg) {
        Log.d("fudge", arg);

        basicLog += arg + "\n";

        String[] lines = basicLog.split("\n");
        if (lines.length > MAX_LOG_LINES) {
            basicLog = String.join("\n", Arrays.copyOfRange(lines, 1, lines.length)) + "\n";
        }

        updateLog();
    }

    public static void updateLog() {
        if (MainActivity.instance != null) {
            MainActivity.instance.setLogText(basicLog);
        }
        if (Gallery.instance != null) {
            Gallery.instance.setLogText(basicLog);
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

    public static void sendTextUpdate(String key, String value) {
        switch (key) {
            case "cam_name":
                if (Gallery.instance == null) return;
                Gallery.instance.setTitleCamName(value);
                return;
            default:
                Log.d("fudge", "Unknown update key " + key);
                return;
        }
    }
}
