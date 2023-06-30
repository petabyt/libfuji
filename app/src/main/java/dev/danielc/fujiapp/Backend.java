// Custom Java bindings to camlib
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;
import android.graphics.Bitmap;
import android.util.Log;
import android.os.Environment;
import java.io.File;
import org.json.JSONObject;

public class Backend {
    static {
        System.loadLibrary("fujiapp");
    }

    // In order to give the backend access to the static methods, new objects must be made
    public static void init() {
        cInit(new Backend(), new Conn());
    }

    // Clear entire backend for a new connection
    public static void clear() {
        // TODO: clear connection state? don't want to reinit connection in some places maybe.
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
    public native synchronized static String cTestFunc();
    public native synchronized static int cPtpFujiInit();
    public native synchronized static int cPtpFujiPing();
    public native synchronized static int cPtpGetPropValue(int code);
    public native synchronized static int cPtpFujiWaitUnlocked();
    public native synchronized static String cPtpRun(String req);
    public native synchronized static byte[] cPtpGetThumb(int handle);
    public native synchronized static byte[] cFujiGetFile(int handle);

    // This runs a text binding request from camlib - see docs/
    public static JSONObject run(String req) throws Exception {
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

    // JNI -> UI log communication

    public static String logLocation = "main";

    public static void jni_print_clear() {
        basicLog = "";
        MainActivity.getInstance().setErrorText("");
    }

    // debug function for both Java frontend and JNI backend
    private static String basicLog = "";
    public static void jni_print(String arg) {
        Log.d("jni_print", arg);
        basicLog += arg;
        if (MainActivity.getInstance() != null) {
            MainActivity.getInstance().setErrorText(basicLog);
        }
        if (gallery.getInstance() != null) {
            gallery.getInstance().setErrorText(basicLog);
        }
    }

    public static String getDownloads() {
        String mainStorage = Environment.getExternalStorageDirectory().getAbsolutePath();
        String fujifilm = mainStorage + File.separator + "DCIM" + File.separator + "fuji";
        return fujifilm;
    }
}
