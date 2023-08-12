// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.os.Handler;
import android.os.Looper;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.content.Intent;
import android.os.Build;

public class MainActivity extends AppCompatActivity {
    private static MainActivity instance;

    Handler handler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        instance = this;
        handler = new Handler(Looper.getMainLooper());

        Backend.init();

        Backend.log_update();

        findViewById(R.id.reconnect).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Backend.jni_print_clear();
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

        ((TextView)findViewById(R.id.bottomText)).setText(getString(R.string.url) + "\n" +
                "Download location: " + Backend.getDownloads() + "\n" +
                getString(R.string.motd_thing));

        findViewById(R.id.test_suite).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, Tester.class);
                startActivity(intent);
            }
        });
    }

    @Override
    public void onBackPressed() {
        Backend.logLocation = "main";
    }

    public void connectClick(View v) {
        if (Backend.cIsUsingEmulator()) {
            Backend.logLocation = "gallery";
            Intent intent = new Intent(MainActivity.this, Gallery.class);
            startActivity(intent);
            return;
        }
    
        // Socket must be opened on WiFi - otherwise it will prefer cellular
        // TODO: Implement a timeout (If WiFi is disabled)
        Backend.print("Attempting connection...\n");
        ConnectivityManager connectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkRequest.Builder requestBuilder = new NetworkRequest.Builder();
        requestBuilder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        ConnectivityManager.NetworkCallback networkCallback = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                ConnectivityManager.setProcessDefaultNetwork(network);
                if (!Backend.wifi.connect(Backend.FUJI_IP, Backend.FUJI_CMD_PORT, Backend.TIMEOUT)) {
                    Backend.logLocation = "gallery";
                    Intent intent = new Intent(MainActivity.this, Gallery.class);
                    startActivity(intent);
                }
                connectivityManager.unregisterNetworkCallback(this);
            }
        };

        if (Build.VERSION.SDK_INT >= 26) {
            connectivityManager.requestNetwork(requestBuilder.build(), networkCallback, 500);
        } else {
            connectivityManager.requestNetwork(requestBuilder.build(), networkCallback);
        }
    }

    public static MainActivity getInstance() {
        return instance;
    }

    public void setErrorText(String arg) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                TextView error_msg = findViewById(R.id.error_msg);
                error_msg.setText(arg);
            }
        });
    }
}
