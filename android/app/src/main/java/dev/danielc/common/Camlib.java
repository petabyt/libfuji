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

    public static final int PTP_OF_Undefined = 0x3000;
    public static final int PTP_OF_Folder = 0x3001;
    public static final int PTP_OF_MOV = 0x300d;
    public static final int PTP_OF_JPEG = 0x3801;
    public static final int PTP_OF_RAW = 0xb103;

    // ptp_ error codes
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

    // Supported lv types
    public static final int PTP_LV_NONE = 0;
    public static final int PTP_LV_EOS = 1;
    public static final int PTP_LV_CANON = 2;
    public static final int PTP_LV_ML = 3;

    public native static byte[] cPtpGetThumb(int handle);
    public native static int cPtpGetPropValue(int code);
    public native static int cPtpOpenSession();
    public native static int cPtpCloseSession();
    public native static JSONObject cGetObjectInfo(int handle);

    /// @brief Initialize object service on ptpruntime
    public native static void cPtpObjectServiceStart(int[] handles);
    /// @brief Get objectinfo from object handle
    public native static JSONObject cPtpObjectServiceGet(int handle);
    /// @brief Get objectinfo (after being sorted) in the array given to cPtpObjectServiceStart
    public native static JSONObject cPtpObjectServiceGetIndex(int index);
    /// @brief Get an array of all known objectinfos
    public native static JSONObject[] cPtpObjectServiceGetFilled();
    /// @brief Step the object info service once, perform one operation
    public native static int cPtpObjectServiceStep();
    /// @brief Bump an object handle to a higher priority - so it will be handled immediately by cPtpObjectServiceStep
    public native static void cPtpObjectServiceAddPriority(int handle);

    public final static int PTP_SELET_JPEG = 1;
    public final static int PTP_SELET_RAW = 2;
    public final static int PTP_SELET_MOV = 3;

    // Object service bitmask options
    public final static int PTP_SORT_BY_OLDEST = 1;
    public final static int PTP_SORT_BY_NEWEST = 2;
    public final static int PTP_SORT_BY_ALPHA_A_Z = 3;
    public final static int PTP_SORT_BY_ALPHA_Z_A = 4;
    public final static int PTP_SORT_BY_JPEGS_FIRST = 5;
    public final static int PTP_SORT_BY_MOVS_FIRST = 6;
    public final static int PTP_SORT_BY_RAWS_FIRST = 7;
};
