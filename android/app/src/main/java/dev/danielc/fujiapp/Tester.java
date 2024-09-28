// Basic test suite
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.content.ClipData;
import android.text.Html;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ScrollView;
import android.widget.TextView;
import android.os.Looper;
import android.os.Bundle;
import android.content.Context;
import android.os.Handler;

import android.content.ClipboardManager;
import android.widget.Toast;

import java.lang.ref.WeakReference;

import dev.danielc.common.WiFiComm;

public class Tester extends AppCompatActivity {
    private Handler handler;
    public static WeakReference<Tester> ctx = null;
    private static String verboseLog = null;
    private static String currentLogs = "";

    @Override
    protected void onDestroy() {
        super.onDestroy();
        MainActivity.wifi.blockEvents = false;
        ctx.clear();
        handler = null;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);
        currentLogs = "";
        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(getString(R.string.regressiontesting));

        handler = new Handler(Looper.getMainLooper());
        ctx = new WeakReference<>(this);

        MainActivity.wifi.blockEvents = true;
        WiFiComm wifi = new WiFiComm();
        wifi.onWiFiSelectAvailable = new Runnable() {
            @Override
            public void run() {
                tryConnect();
            }
        };

        log("(The tester is only designed for WiFi pairing. PC AutoSave or Wireless Tether Shoot is not working here.)");

        if (Backend.cRouteLogs() == 0) {
            log("Routing logs to memory buffer.");
        }

        findViewById(R.id.tester_select_wifi).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String password = SettingsActivity.getWPA2Password(Tester.this);
                if (password.length() == 0) password = null;
                if (wifi.connectToAccessPoint(Tester.this, password) != 0) {
                    fail("Didn't select access point.");
                }
            }
        });
        findViewById(R.id.tester_connect).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                tryConnect();
            }
        });
    }

    void tryConnect() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                log(getString(R.string.connecting));
                int rc = Backend.fujiConnectToCmd(0);
                if (rc != 0) {
                    fail("Failed to connect to cmd: " + Frontend.parseErr(rc));

                    try {
                        Backend.connectUSB(Tester.this);
                    } catch (Exception e2) {
                        fail("USB: " + e2.toString());
                        verboseLog = Backend.cEndLogs();
                        return;
                    }
                }

                log("Established connection, starting test");

                mainTest();

                verboseLog = Backend.cEndLogs();
                log("Hit the copy button to share the verbose log with devs.");
            }
        }).start();
    }

    public static void log(String str) {
        Tester t = ctx.get();
        if (t == null) return;
        t.handler.post(new Runnable() {
            @Override
            public void run() {
                currentLogs += str + "<br>";
                TextView testerLog = t.findViewById(R.id.testerLog);
                testerLog.setText(Html.fromHtml(currentLogs));
                ScrollView scroll = t.findViewById(R.id.tester_scroll);
                scroll.fullScroll(View.FOCUS_DOWN);
            }
        });
    }

    public static void fail(String str) {
        log("<font color='#EE0000'>[FAIL] " + str + "</font>");
    }

    public void mainTest() {
        int rc = Backend.cFujiTestSuite();
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
