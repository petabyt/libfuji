package dev.danielc.fujiapp;

import android.media.MediaScannerConnection;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.IdRes;
import androidx.annotation.StringRes;

import org.json.JSONObject;

import java.util.Arrays;

import dev.danielc.common.Camlib;
import dev.danielc.common.WiFiComm;

public class Frontend {
    public static String parseErr(int rc) {
        switch (rc) {
            case Camlib.PTP_NO_DEVICE: return "No device found.";
            case Camlib.PTP_NO_PERM: return "Invalid permissions.";
            case Camlib.PTP_OPEN_FAIL: return "Couldn't connect to device.";
            case WiFiComm.NOT_AVAILABLE: return "WiFi not ready yet.";
            case WiFiComm.NOT_CONNECTED: return "WiFi is not connected. Wait a few seconds or check your settings.";
            case WiFiComm.UNSUPPORTED_SDK: return "Unsupported SDK";
            default: return "Unknown error";
        }
    }

    public static void discoveryIsActive() {
        MainActivity.instance.handler.post(new Runnable() {
            @Override
            public void run() {
                MainActivity.instance.findViewById(R.id.discoveryProgressBar).setVisibility(View.VISIBLE);
                TextView tv = MainActivity.instance.findViewById(R.id.discoveryMessage);
                tv.setVisibility(View.GONE);
            }
        });
    }

    public static void discoveryFailed() {
        MainActivity.instance.handler.post(new Runnable() {
            @Override
            public void run() {
                MainActivity.instance.findViewById(R.id.discoveryProgressBar).setVisibility(View.GONE);
                TextView tv = MainActivity.instance.findViewById(R.id.discoveryMessage);
                tv.setVisibility(View.VISIBLE);
                tv.setText(R.string.discovery_failed);
            }
        });
    }

    public static void discoveryWaitWifi() {
        MainActivity.instance.handler.post(new Runnable() {
            @Override
            public void run() {
                MainActivity.instance.findViewById(R.id.discoveryProgressBar).setVisibility(View.GONE);
                TextView tv = MainActivity.instance.findViewById(R.id.discoveryMessage);
                tv.setVisibility(View.VISIBLE);
                tv.setText(R.string.waiting_on_wifi);
            }
        });
    }

    public static String formatFilesize(int size) {
        return String.format("%.2fMB\n", size / 1024.0 / 1024.0);
    }

    final static int MAX_LOG_LINES = 3;

    // debug function for both Java frontend and JNI backend
    private static String basicLog = "";

    public static String getString(@StringRes int res) {
        return MainActivity.instance.getString(res);
    }

    public static void clearPrint() {
        basicLog = "";
        updateLog();
    }

    public static void print(String arg) {
        Log.d("fudge", arg);

        basicLog += arg + "\n";

        String[] lines = basicLog.split("\n");
        if (lines.length > MAX_LOG_LINES) {
            basicLog = String.join("\n", Arrays.copyOfRange(lines, 1, lines.length)) + "\n";
        }

        updateLog();
    }

    static void downloadingFile(JSONObject info) {
        Gallery.downloadingFile(info);
    }

    static void downloadedFile(String path) {
        Gallery.downloadedFile(path);
        MediaScannerConnection.scanFile(MainActivity.instance, new String[] {path}, null, null);
    }

    public static void print(@StringRes int resID) {
        print(getString(resID));
    }

    public static void updateLog() {
        MainActivity.setLogText(basicLog.trim());
        Gallery.setLogText(basicLog.trim());
    }

    public static void sendCamName(String value) {
        Gallery.setTitleCamName(value);
    }
    public static void notifyDownloadProgress(int percent) {Viewer.notifyDownloadProgress(percent);}
    public static void notifyDownloadSpeed(int mbps) {Viewer.notifyDownloadSpeed(mbps);}
    public static void onCameraRegistered(String model, String name, String ip) { MainActivity.instance.onCameraRegistered(model, name, ip); }
    public static void onCameraWantsToConnect(String model, String name, String ip) { MainActivity.instance.onCameraWantsToConnect(model, name); }
}
