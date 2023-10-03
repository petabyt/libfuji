// Basic test suite
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.content.Intent;
import android.text.Html;
import android.view.MenuItem;
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

import java.net.Socket;

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
                fail("Failed to use bluetooth: permission denied (or bluetooth is off)" + e);
                return;
            }

            log("Gained access to bluetooth");

            // TODO: Finish bluetooth tests
        } else {
            return;
        }

        bt.getConnectedDevice();
    }

    Socket testSock = null;
    void socketTest(ConnectivityManager m) {
        try {
            NetworkRequest.Builder requestBuilder = new NetworkRequest.Builder();
            requestBuilder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
            ConnectivityManager.NetworkCallback networkCallback = new ConnectivityManager.NetworkCallback() {
                @Override
                public void onAvailable(Network network) {
                    Log.e("sad", "Wifi available");
                    ConnectivityManager.setProcessDefaultNetwork(network);

                    try {
                        Socket s = new Socket(Backend.FUJI_IP, Backend.FUJI_CMD_PORT);
                        s.setKeepAlive(true);
                        s.setTcpNoDelay(true);
                        s.setReuseAddress(true);
                        testSock = s;
                        log("Success socket");
                    } catch (Exception e) {
                        fail(e.toString());
                    }

                    m.unregisterNetworkCallback(this);
                }
            };

            if (Build.VERSION.SDK_INT >= 26) {
                m.requestNetwork(requestBuilder.build(), networkCallback, Backend.OPEN_TIMEOUT);
            } else {
                m.requestNetwork(requestBuilder.build(), networkCallback);
            }
        } catch (Exception e) {
            fail(e.toString());
        }

        try {
            Thread.sleep(1000);
            if (testSock != null) {
                byte[] data = {12, 12, 12, 12};
                testSock.getOutputStream().write(data);
                testSock.getOutputStream().flush();

                testSock.close();
            }
        } catch (Exception e) {
            fail(e.toString());
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);
        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);

        handler = new Handler(Looper.getMainLooper());

        Backend.cTesterInit(this);

        if (Backend.cRouteLogs(Backend.getLogPath()) == 0) {
            log("Routing logs to " + Backend.getLogPath());
        } else {
            fail("Couldn't route logs to " + Backend.getLogPath() + ", running test anyway");
        }

        //connectBluetooth();

        ConnectivityManager m = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);

        new Thread(new Runnable() {
            @Override
            public void run() {
                if (!Backend.cIsUsingEmulator()) {
                    if (Backend.wifi.fujiConnectToCmd(m)) {
                        log(Backend.wifi.failReason);
                        fail("Failed to connect to port on WiFi");
                        return;
                    }
                }

                log("Established connection, starting test thread");

                mainTest(m);
                Backend.wifi.close();
                Backend.cEndLogs();
            }
        }).start();
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

        if (Backend.cCameraWantsRemote()) {
            rc = Backend.cFujiTestStartRemoteSockets();
            if (rc != 0) return;

            if (Backend.wifi.fujiConnectEventAndVideo(m)) {
                fail("Failed to accept connections from event and video ports");
                return;
            } else {
                log("Accepted connection from event and video ports");
            }

            rc = Backend.cFujiEndRemoteMode();
            if (rc != 0) return;
        }

        rc = Backend.cFujiTestSetupImageGallery();
        if (rc != 0) return;

        try {
            Camera.closeSession();
        } catch (Exception e) {
            fail("Failed to close session");
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
