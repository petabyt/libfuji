package dev.danielc.fujiapp;

import dev.danielc.common.Camlib;

public class Fuji extends Camlib {

    final static int FUJI_D_REGISTERED = 1;
    final static int FUJI_D_GO_PTP = 2;
    final static int FUJI_D_CANCELED = 3;
    final static int FUJI_D_IO_ERR = 4;
    final static int FUJI_D_OPEN_DENIED = 5;
    final static int FUJI_D_INVALID_NETWORK = 6;
    final static int DISCOVERY_ERROR_THRESHOLD = 5;

    // enum FujiTransport
    final static int FUJI_FEATURE_AUTOSAVE = 1;
    final static int FUJI_FEATURE_WIRELESS_TETHER = 2;
    final static int FUJI_FEATURE_WIRELESS_COMM = 3;
    final static int FUJI_FEATURE_USB_CARD_READER = 4;
    final static int FUJI_FEATURE_USB_TETHER_SHOOT = 5;
    final static int FUJI_FEATURE_RAW_CONV = 6;
}
