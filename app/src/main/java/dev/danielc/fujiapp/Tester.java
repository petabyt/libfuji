// Basic test suite
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.content.Intent;
import android.text.Html;
import android.widget.TextView;
import android.os.Looper;
import android.app.Activity;
import android.os.Bundle;
import android.app.Activity;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.PopupWindow;
import android.os.Handler;
import android.view.ViewTreeObserver;
import android.widget.ProgressBar;

public class Tester extends AppCompatActivity {
    private Handler handler;

    Bluetooth bt;

    private void connectBluetooth() {
        bt = new Bluetooth();
        Intent intent;
        try {
            intent = bt.getIntent();
        } catch (Exception e) {
            fail("Failed to use bluetooth: " + e.toString());
            return;
        }

        if (intent != null) {
            try {
                startActivityForResult(intent, 1);
            } catch (Exception e) {
                fail("Failed to use bluetooth: permission denied (or bluetooth is off)");
                return;
            }

            // TODO: Finish bluetooth tests
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);

        handler = new Handler(Looper.getMainLooper());

        Backend.cTesterInit(this);

        if (Backend.cRouteLogs(Backend.getLogPath()) == 0) {
            log("Routing logs to " + Backend.getLogPath());
        } else {
            fail("Couldn't route logs to " + Backend.getLogPath() + ", running test anyway");
        }

        connectBluetooth();

        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                mainTest();
            }
        });

        ConnectivityManager connectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkRequest.Builder requestBuilder = new NetworkRequest.Builder();
        requestBuilder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        ConnectivityManager.NetworkCallback networkCallback = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                ConnectivityManager.setProcessDefaultNetwork(network);
                log("Attempting to connect through WiFi: " + Backend.FUJI_IP + ":" + Backend.FUJI_CMD_PORT);
                if (!Conn.connect(Backend.FUJI_IP, Backend.FUJI_CMD_PORT, Backend.TIMEOUT)) {
                    log("Established connection, starting test thread");
                    thread.start();
                } else {
                    fail("Failed to connect to port on WiFi");
                }
                connectivityManager.unregisterNetworkCallback(this);
            }

            @Override
            public void onUnavailable () {
                fail("WiFi is not available, or turned off");
            }
        };

        connectivityManager.requestNetwork(requestBuilder.build(), networkCallback, 500);
    }

    private String currentLogs = "";
    public void log(String str) {
        Log.d("fujiapp-dbg-tester", str);
        handler.post(new Runnable() {
            @Override
            public void run() {
                currentLogs += str + "<br>";
                TextView testerLog = findViewById(R.id.testerLog);
                testerLog.setText(Html.fromHtml(currentLogs));
            }
        });
    }

    public void fail(String str) {
        log("<font color='#EE0000'>[FAIL] " + str + "</font>");
    }

    public void mainTest() {    
        int rc = Backend.cFujiTestSuite();
        log("Return code: " + rc);
        Backend.cEndLogs();
    }
}
