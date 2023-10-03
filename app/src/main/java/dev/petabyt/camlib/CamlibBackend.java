// Basic backend parent class for camlib with JNI
// Copyright Daniel Cook - Apache License
package dev.petabyt.camlib;

public class CamlibBackend {
    // Integer error exception - see camlib PTP_ error codes
    public static class PtpErr extends Exception {
        public int rc;
        public PtpErr(int code) {
            rc = code;
        }
    }

    public static boolean connectionIsActive = false;

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

    // camlib supported lv types
    public static final int PTP_LV_NONE = 0;
    public static final int PTP_LV_EOS = 1;
    public static final int PTP_LV_CANON = 2;
    public static final int PTP_LV_ML = 3;
};