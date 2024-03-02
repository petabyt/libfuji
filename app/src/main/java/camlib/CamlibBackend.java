// Basic backend parent class for camlib with JNI
// Copyright Daniel Cook - Apache License
package camlib;

import org.json.JSONObject;

import dev.danielc.fujiapp.Backend;

public class CamlibBackend {
    // Integer error exception - see camlib PTP_ error codes
    public static class PtpErr extends Exception {
        public int rc;
        public PtpErr(int code) {
            rc = code;
        }
    }

    public static final int PTP_OF_JPEG = 0x3801;

    public static final int OPEN_TIMEOUT = 500;
    public static final int TIMEOUT = 1000;

    // camlib error codes
    public static final int PTP_OK = 0;
    public static final int PTP_NO_DEVICE = -1;
    public static final int PTP_NO_PERM = -2;
    public static final int PTP_OPEN_FAIL = -3;
    public static final int PTP_OUT_OF_MEM = -4;
    public static final int PTP_IO_ERR = -5;
    public static final int PTP_RUNTIME_ERR = -6;
    public static final int PTP_UNSUPPORTED = -7;
    public static final int PTP_CHECK_CODE = -8;

    public static String parseErr(int rc) {
        switch (rc) {
            case PTP_NO_DEVICE: return "No device found.";
            case PTP_NO_PERM: return "Invalid permissions.";
            case PTP_OPEN_FAIL: return "Couldn't connect to device.";
            case WiFiComm.NOT_AVAILABLE: return "WiFi not ready yet.";
            case WiFiComm.NOT_CONNECTED: return "WiFi is not connected.";
            case WiFiComm.UNSUPPORTED_SDK: return "Unsupported SDK";
            default: return "Unknown error";
        }
    }

    // camlib supported lv types
    public static final int PTP_LV_NONE = 0;
    public static final int PTP_LV_EOS = 1;
    public static final int PTP_LV_CANON = 2;
    public static final int PTP_LV_ML = 3;

    public native static String cPtpRun(String req);
    public native static byte[] cPtpGetThumb(int handle);
    public native static int cPtpGetPropValue(int code);
    public native static int cPtpOpenSession();
    public native static int cPtpCloseSession();

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
};
