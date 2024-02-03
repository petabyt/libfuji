// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;

import camlib.SimpleSocket;
import camlib.WiFiComm;
import libui.LibUI;

public class MainActivity extends AppCompatActivity {
    public static MainActivity instance;
    Handler handler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        instance = this;
        handler = new Handler(Looper.getMainLooper());

        LibUI.buttonBackgroundResource = R.drawable.grey_button;
        LibUI.popupDrawableResource = R.drawable.border;

        Backend.init();
        Backend.updateLog();

        findViewById(R.id.reconnect).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Backend.clearPrint();
                connectClick(v);
            }
        });

        findViewById(R.id.help_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, Help.class);
                startActivity(intent);
            }
        });

        TextView bottomText = ((TextView)findViewById(R.id.bottomText));
        bottomText.append(getString(R.string.url) + "\n");
        bottomText.append("Download location: " + Backend.getDownloads() + "\n");
        bottomText.append(getString(R.string.motd_thing) + " " + BuildConfig.VERSION_NAME);

        findViewById(R.id.test_suite).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Backend.cFujiScriptsScreen(MainActivity.this);
            }
        });

        // Require legacy Android write permissions
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
        }

        ConnectivityManager m = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);

        SimpleSocket.setConnectivityManager(m);

        // Idea: Show WiFi status on screen?
        WiFiComm.startNetworkListeners(m);

        //Backend.cFujiScriptsScreen(MainActivity.this);
    }

    public void connectClick(View v) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Backend.fujiConnectToCmd();
                    Backend.print("Connected to the camera");
                    Intent intent = new Intent(MainActivity.this, Gallery.class);
                    startActivity(intent);
                } catch (Exception e) {
                    Backend.print(e.getMessage());
                }
            }
        }).start();
    }

    public void setLogText(String arg) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                TextView tv = findViewById(R.id.error_msg);
                if (tv == null) return;
                tv.setText(arg);
            }
        });
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getTitle() == "open") {

            File[] fileList;
            File file = new File(Backend.getDownloads());
            if (!file.isDirectory()) {
                return super.onOptionsItemSelected(item);
            }

            fileList = file.listFiles();
            String mime = "*/*";
            String path = Backend.getDownloads();
            if (fileList.length != 0) {
                path = fileList[0].getPath();
                mime = "image/*";
            }

            if (Viewer.downloadedFilename != null) {
                path = Viewer.downloadedFilename;
                mime = "image/*";
            }

            Intent intent = new Intent();
            intent.setAction(Intent.ACTION_VIEW);
            intent.setDataAndType(Uri.parse(path), mime);
            startActivity(intent);
        } else if (item.getTitle() == "script") {
            //Backend.cFujiScriptsScreen(MainActivity.this);
            Intent intent = new Intent(MainActivity.this, Tester.class);
            startActivity(intent);
        } else {
            return LibUI.handleOptions(item, false);
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onBackPressed() {
        LibUI.handleBack(false);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuItem menuItem = menu.add(Menu.NONE, Menu.NONE, Menu.NONE, "open");
        menuItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        menuItem.setIcon(R.drawable.baseline_folder_open_24);

        menuItem = menu.add(Menu.NONE, Menu.NONE, Menu.NONE, "script");
        menuItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        menuItem.setIcon(R.drawable.baseline_fact_check_24);

        return super.onCreateOptionsMenu(menu);
    }
}
