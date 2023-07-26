// Custom Java bindings to camlib
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp

package dev.danielc.fujiapp;
import android.util.Log;
import android.os.Environment;
import java.io.File;
import org.json.JSONObject;
import java.util.Arrays;

public class Backend {
    static {
        System.loadLibrary("fujiapp");
    }

    // In order to give the backend access to the static methods, new objects must be made
    private static boolean haveInited = false;
    public static void init() {
        if (haveInited == false) {
            cInit(new Backend(), new Conn());
        }
        haveInited = true;
    }

    // Clear entire backend for a new connection
    public static void clear() {
        Conn.connection = Conn.Status.OFF;
    }

    // Constants
    public static final String FUJI_IP = "192.168.0.1";
    public static final int FUJI_CMD_PORT = 55740;
    public static final int TIMEOUT = 1000;
    public static final int PTP_OF_JPEG = 0x3801;

    //public static Bitmap bitmaps[] = null;

    // Note that all these native functions are synchronized (they can only be called by Java
    // by one thread at a time - necessary for a socket connection)
    public native synchronized static void cInit(Backend b, Conn c);
    public native synchronized static void cTesterInit(Tester t);
    public native synchronized static String cTestFunc();
    public native synchronized static int cPtpFujiInit();
    public native synchronized static int cPtpFujiPing();
    public native synchronized static int cPtpGetPropValue(int code);
    public native synchronized static int cPtpFujiWaitUnlocked();
    public native synchronized static int cFujiConfigVersion();
    public native synchronized static int cFujiConfigInitMode();
    public native synchronized static String cPtpRun(String req);
    public native synchronized static byte[] cPtpGetThumb(int handle);
    public native synchronized static byte[] cFujiGetFile(int handle);
    public native synchronized static boolean cIsMultipleMode();
    public native synchronized static int cTestStuff();

    // Enable disable verbose logging to file
    public native synchronized static int cRouteLogs(String filename);
    public native synchronized static void cEndLogs();

    public native synchronized static int cFujiTestSuite();

    // Runs a request with integer parameters
    public static JSONObject run(String req, int[] arr) throws Exception {
        if (Conn.connection == Conn.Status.OFF) {
            throw new Exception("Connection closed.");
        }
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
                Backend.jni_print("Non zero error: " + Integer.toString(jsonObject.getInt("error")) + "\n");
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

    public static void pingUntilDisconnect() {
        // TODO: good idea?
    }

    // JNI -> UI log communication

    public static String logLocation = "main";

    public static void jni_print_clear() {
        basicLog = "";
        MainActivity.getInstance().setErrorText("");
    }

    // debug function for both Java frontend and JNI backend
    private static String basicLog = "";
    public static void jni_print(String arg) {
        Log.d("fujiapp-dbg", arg);
        basicLog += arg;

        String[] lines = basicLog.split("\n");
        if (lines.length > 5) {
            basicLog = String.join("\n", Arrays.copyOfRange(lines, 1, lines.length)) + "\n";
        }

        log_update();
    }

    public static void log_update() {
        if (MainActivity.getInstance() != null) {
            MainActivity.getInstance().setErrorText(basicLog);
        }
        if (Gallery.getInstance() != null) {
            Gallery.getInstance().setErrorText(basicLog);
        }
    }

    public static String getDownloads() {
        String mainStorage = Environment.getExternalStorageDirectory().getAbsolutePath();
        String fujifilm = mainStorage + File.separator + "DCIM" + File.separator + "fuji";
        return fujifilm;
    }

    public static String getLogPath() {
        String mainStorage = Environment.getExternalStorageDirectory().getAbsolutePath();
        String fujifilm = mainStorage + File.separator + "Documents" + File.separator + "fujiapp.txt";
        return fujifilm;
    }
}

