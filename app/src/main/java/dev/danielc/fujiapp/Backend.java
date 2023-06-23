// Java bindings to camlib
package dev.danielc.fujiapp;
import android.graphics.Bitmap;
import android.util.Log;

import org.json.JSONObject;

public class Backend {
    static {
        System.loadLibrary("fujiapp");
    }

    public static Bitmap bitmaps[] = null;

    public static int PTP_OF_JPEG = 0x3801;

    public native synchronized static void cInit(Backend b, Conn c);
    public native synchronized static String cTestFunc();
    public native synchronized static int cPtpFujiInit();
    public native synchronized static int cPtpFujiPing();
    public native synchronized static int cPtpGetPropValue(int code);
    public native synchronized static int cPtpFujiWaitUnlocked();
    public native synchronized static String cPtpRun(String req);
    public native synchronized static byte[] cPtpGetThumb(int handle);
    public native synchronized static byte[] cFujiGetFile(int handle);

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

    public static void init() {
        cInit(new Backend(), new Conn());
    }

    public static String logLocation = "main";

    // debug function for both Java frontend and JNI backend
    private static String basicLog = "";
    public static void jni_print(String arg) {
        Log.d("jni_print", arg);
//        basicLog += arg;
//        if (logLocation == "main") {
//            MainActivity.getInstance().setErrorText(basicLog);
//        } else if (logLocation == "gallery") {
//            gallery.getInstance().setErrorText(basicLog);
//        }
    }
}
