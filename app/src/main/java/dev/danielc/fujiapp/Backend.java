// Custom Java bindings to Fujifilm/camlib
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp

package dev.danielc.fujiapp;
import android.content.Context;
import android.net.ConnectivityManager;
import android.util.Log;
import android.os.Environment;
import java.io.File;
import org.json.JSONObject;
import java.util.Arrays;
import android.content.Context;
import android.content.Intent;
import camlib.*;

public class Backend extends CamlibBackend {
    static {
        System.loadLibrary("fujiapp");
    }

    static SimpleSocket cmdSocket = new SimpleSocket();
    static SimpleSocket eventSocket = new SimpleSocket();
    static SimpleSocket videoSocket = new SimpleSocket();

    public static void fujiConnectToCmd() throws Exception {
        Backend.print("Connecting...");

        try {
            cmdSocket.connectWiFi(Backend.FUJI_IP, Backend.FUJI_CMD_PORT);
            cClearKillSwitch();
        } catch (Exception e) {
            if (BuildConfig.DEBUG) {
                Backend.print("Trying emulator IP");
                try {
                    cmdSocket.connectWiFi(Backend.FUJI_EMU_IP, Backend.FUJI_CMD_PORT);
                    cClearKillSwitch();
                } catch (Exception e2) {
                    throw e2;
                }
            } else {
                throw e;
            }
        }
    }

    public static void fujiConnectEventAndVideo() throws Exception {
        String ip = cmdSocket.ip;
        try {
            eventSocket.connectWiFi(ip, Backend.FUJI_EVENT_PORT);
        } catch (Exception e) {
            Backend.print("Failed to connect to event socket: " + e.toString());
            throw e;
        }

        try {
            videoSocket.connectWiFi(ip, Backend.FUJI_VIDEO_PORT);
        } catch (Exception e) {
            Backend.print("Failed to connect to video socket: " + e.toString());
            throw e;
        }
    }

    // Block all communication in UsbComm and WiFiComm
    // Write reason + code, and reconnect popup
    public native static void cReportError(int code, String reason);
    public static void reportError(int code, String reason) {
        cReportError(code, reason);
    }
    public static void exitToMain(Context ctx) {
        // Kill switch is operated by camlib, so checking socket kill switch will do for now
        if (!Backend.cmdSocket.alive) return;

        Intent intent = new Intent(ctx, MainActivity.class);
        ctx.startActivity(intent);
    }

    // In order to give the backend access to the static methods, new objects must be made
    private static boolean haveInited = false;
    public static void init() {
        if (haveInited == false) {
            cInit(new Backend(), cmdSocket);
        }
        haveInited = true;
    }

    // Clear entire backend for a new connection
    public static void clear() {
        //wifi.connection = WiFiComm.Status.OFF;
    }

    // Constants
    public static final String FUJI_EMU_IP = "192.168.1.33"; // IP addr of my laptop
    public static final String FUJI_IP = "192.168.0.1";
    public static final int FUJI_CMD_PORT = 55740;
    public static final int FUJI_EVENT_PORT = 55741;
    public static final int FUJI_VIDEO_PORT = 55742;
    public static final int OPEN_TIMEOUT = 1000;

    // IO kill switch is in C/camlib, so we must set it when a connection is established
    public native static void cClearKillSwitch();

    // Note: 'synchronized' means only one of these methods can be used at time -
    // java's version of a mutex
    public native static void cInit(Backend b, SimpleSocket c);
    public native static int cPtpFujiInit();
    public native static int cPtpFujiPing();
    //public native static int cPtpGetPropValue(int code);
    public native static int cPtpFujiWaitUnlocked();
    public native static int cFujiConfigVersion();
    public native static int cFujiConfigInitMode();
    public native static boolean cIsMultipleMode();
    public native static boolean cIsUntestedMode();
    public native static boolean cCameraWantsRemote();
    public native static int[] cGetObjectHandles();
    public native static int cFujiEndRemoteMode();
    public native static int cFujiConfigImageGallery();
    public native static int cFujiDownloadMultiple();

    // For tester only
    public native static int cFujiTestStartRemoteSockets();

    // Must be called in order - first one enables compression, second disables compression
    // This is a cheap fix for now, will be fixed in the next refactoring
    public native static String cFujiGetUncompressedObjectInfo(int handle);
    public native static byte[] cFujiGetFile(int handle);

    // For test suite only
    public native static void cTesterInit(Tester t);
    //public native static String cTestFunc();
    public native static int cFujiTestSetupImageGallery();
    //public native static int cTestStuff();
    public native static int cFujiTestSuiteSetup();

    // Enable disable verbose logging to file
    public native static int cRouteLogs(String filename);
    public native static String cEndLogs();

    public native static int cFujiScriptsScreen(Context ctx);

    public native static int cSetProgressBar(Object progressBar);

    // Runs a request with integer parameters
    public static JSONObject run(String req, int[] arr) throws Exception {
        // Build camlib request string (see docs/)
        req += ";";
        for (int i = 0; i < arr.length; i++) {
            req += String.valueOf(arr[i]);
            if (i != arr.length - 1) {
                req += ",";
            }
        }
        req += ";";

        String resp = cPtpRun(req);
        try {
            JSONObject jsonObject = new JSONObject(resp);
            if (jsonObject.getInt("error") != 0) {
                Backend.print("Non zero error: " + Integer.toString(jsonObject.getInt("error")));
                throw new Exception("Error code");
            }

            return jsonObject;
        } catch (Exception e) {
            throw e;
        }
    }

    public static JSONObject run(String req) throws Exception {
        return run(req, new int[]{});
    }

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

    public static String getDownloads() {
        String mainStorage = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES).getPath();
        String fujifilm = mainStorage + File.separator + "fudge";
        return fujifilm;
    }

    public static String getLogPath() {
        String mainStorage = Environment.getExternalStorageDirectory().getAbsolutePath();
        String fujifilm = mainStorage + File.separator + "Documents" + File.separator + "fujiapp.txt";
        return fujifilm;
    }
}
