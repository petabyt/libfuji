// Basic test suite
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.content.ClipData;
import android.util.Log;
import android.content.Intent;
import android.text.Html;
import android.view.Menu;
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
import android.content.ClipboardManager;
import android.widget.Toast;

import libui.LibU;

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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);
        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle("Regression Testing");

        handler = new Handler(Looper.getMainLooper());

        Backend.cTesterInit(this);

        //connectBluetooth();
        //if (true) return;

        if (Backend.cRouteLogs() == 0) {
            log("Routing logs to memory buffer.");
        }

        ConnectivityManager m = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Backend.fujiConnectToCmd();
                } catch (Exception e) {
                    fail(e.toString());
                    verboseLog = Backend.cEndLogs();
                    return;
                }

                log("Established connection, starting test");

                mainTest(m);

                verboseLog = Backend.cEndLogs();
                log("Hit the copy button to share the verbose log with devs.");
            }
        }).start();
    }

    private String verboseLog = null;
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
        int rc = Backend.cFujiTestSuite(Backend.chosenIP);
        log("Return code: " + rc);
        if (rc != 0) return;

        log("Test completed.");
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        } else if (item.getTitle() == "copy") {
            if (verboseLog != null) {
                ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                ClipData clip = ClipData.newPlainText("Fudge log", verboseLog);
                clipboard.setPrimaryClip(clip);
            } else {
                Toast.makeText(this, "Test not completed yet", Toast.LENGTH_SHORT).show();
            }
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuItem menuItem = menu.add(Menu.NONE, Menu.NONE, Menu.NONE, "copy");
        menuItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        menuItem.setIcon(R.drawable.baseline_content_copy_24);
        return true;
    }
}
