// Basic test suite
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.content.Intent;
import android.text.Html;
import android.widget.TextView;
import android.os.Looper;
import android.os.Bundle;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.os.Handler;
import android.os.Build;

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

            log("Gained access to bluetooth");

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

        //connectBluetooth();

        ConnectivityManager m = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);

        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                mainTest(m);
                Backend.wifi.close();
                Backend.cEndLogs();
            }
        });

        if (Backend.cIsUsingEmulator()) {
            thread.start();
            return;
        }

        if (Backend.wifi.fujiConnectToCmd(m)) {
            fail("Failed to connect to port on WiFi");
        } else {
            log("Established connection, starting test thread");
            thread.start();
        }
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

    public void mainTest(ConnectivityManager m) {
        int rc = Backend.cFujiTestSuiteSetup();
        log("Return code: " + rc);
        if (rc != 0) return;

        rc = Backend.cFujiTestStartRemoteSockets();
        if (rc != 0) return;

        if (Backend.wifi.fujiConnectEventAndVideo(m)) {
            fail("Failed to accept connections from event and video ports");
            return;
        } else {
            log("Accepted connection from event and video ports");
        }

        rc = Backend.cFujiTestEndRemoteMode();
        if (rc != 0) return;

        rc = Backend.cFujiTestSetupImageGallery();
        if (rc != 0) return;
    }
}
