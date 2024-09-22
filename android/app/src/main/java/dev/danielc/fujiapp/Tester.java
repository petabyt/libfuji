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
import android.widget.TextView;
import android.os.Looper;
import android.os.Bundle;
import android.content.Context;
import android.net.ConnectivityManager;
import android.os.Handler;

import android.content.ClipboardManager;
import android.widget.Toast;

import java.lang.ref.WeakReference;

public class Tester extends AppCompatActivity {
    private Handler handler;
    public static WeakReference<Tester> ctx = null;

    WiFiComm

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);
        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(getString(R.string.regressiontesting));

        handler = new Handler(Looper.getMainLooper());
        ctx = new WeakReference<>(this);

        if (Backend.cRouteLogs() == 0) {
            log("Routing logs to memory buffer.");
        }

        Context ctx = this;

        findViewById(R.id.open_wifi).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO: Take over WiFi handler from MainActivity, switch back when this activity exits
            }
        });

        new Thread(new Runnable() {
            @Override
            public void run() {
                Frontend.print(getString(R.string.connecting));
                int rc = Backend.fujiConnectToCmd(0);
                if (rc != 0) {
                    fail("WIFI: " + Frontend.parseErr(rc));

                    try {
                        Backend.connectUSB(ctx);
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

    private static String verboseLog = null;
    private static String currentLogs = "";
    public static void log(String str) {
        Tester t = ctx.get();
        if (t == null) return;
        t.handler.post(new Runnable() {
            @Override
            public void run() {
                currentLogs += str + "<br>";
                TextView testerLog = t.findViewById(R.id.testerLog);
                testerLog.setText(Html.fromHtml(currentLogs));
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
