// Basic backend parent class for camlib with JNI
// Copyright Daniel Cook - Apache License
package dev.danielc.common;

import org.json.JSONObject;

public class Camlib {
    // Integer error exception - see camlib PTP_ error codes
    public static class PtpErr extends Exception {
        public int rc;
        public PtpErr(int code) {
            rc = code;
        }
    }

    public static final int PTP_OF_JPEG = 0x3801;

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
    public static final int PTP_CANCELED = -9;

    // camlib supported lv types
    public static final int PTP_LV_NONE = 0;
    public static final int PTP_LV_EOS = 1;
    public static final int PTP_LV_CANON = 2;
    public static final int PTP_LV_ML = 3;

    public native static byte[] cPtpGetThumb(int handle);
    public native static int cPtpGetPropValue(int code);
    public native static int cPtpOpenSession();
    public native static int cPtpCloseSession();
    public native static String cGetObjectInfo(int handle);
    public static JSONObject getObjectInfo(int handle) throws Exception {
        String x = cGetObjectInfo(handle);
        if (x == null) return null;
        return new JSONObject(x);
    }

    /// @brief Initialize object service on ptpruntime
    public native static void cPtpObjectServiceStart(int[] handles);
    public native static JSONObject cPtpObjectServiceGet(int handle);
    public native static JSONObject cPtpObjectServiceGetIndex(int index);
    public native static JSONObject[] cPtpObjectServiceGetFilled();
    public native static int cPtpObjectServiceStep();
    public native static void cPtpObjectServiceAddPriority(int handle);

    public final static int PTP_SELET_JPEG = 1 << 0;
    public final static int PTP_SELET_RAW = 1 << 1;
    public final static int PTP_SELET_MOV = 1 << 2;
    public final static int PTP_SORT_NEWEST = 1 << 3;
    public final static int PTP_SORT_OLDEST = 1 << 4;
    public final static int PTP_SORT_LARGEST = 1 << 5;
    public final static int PTP_SORT_SMALLEST = 1 << 6;

};
