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

    public static Bitmap bitmaps[] = null;

    public static int PTP_OF_JPEG = 0x3801;

    public static int transferProgress = 0;

    // Note that all these native functions are synchronized - they can only be called by Java
    // by one thread at a time - necessary for a socket connection
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

    // In order to give the backend access to the static methods, new objects must be made
    public static void init() {
        cInit(new Backend(), new Conn());
    }

    public static String logLocation = "main";

    // debug function for both Java frontend and JNI backend
    private static String basicLog = "";
    public static void jni_print(String arg) {
        Log.d("jni_print", arg);
        basicLog += arg;
        if (logLocation == "main") {
            MainActivity.getInstance().setErrorText(basicLog);
        } else if (logLocation == "gallery") {
            gallery.getInstance().setErrorText(basicLog);
        }
    }

    public static String getDownloads() {
        String mainStorage = Environment.getExternalStorageDirectory().getAbsolutePath();
        String fujifilm = mainStorage + File.separator + "DCIM" + File.separator + "fuji";
        return fujifilm;
    }
}
