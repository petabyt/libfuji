// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.app.Activity;
import android.os.Handler;
import android.os.Looper;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import androidx.appcompat.app.AppCompatActivity;
import android.widget.Button;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.content.Intent;
import android.widget.PopupWindow;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.Gravity;
import android.os.Build;

public class MainActivity extends AppCompatActivity {
    private static MainActivity instance;

    public void helpPopup(Activity activity) {
        LayoutInflater inflater = (LayoutInflater) activity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View popupView = inflater.inflate(R.layout.popup_help, null);
        PopupWindow popupWindow = new PopupWindow(popupView, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);

        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));

        popupWindow.showAtLocation(getWindow().getDecorView().getRootView(), Gravity.CENTER, 0, 0);

        Button close = (Button) popupView.findViewById(R.id.close);
        close.setOnClickListener(new View.OnClickListener() {
            public void onClick(View popupView) {
                popupWindow.dismiss();
            }
        });
    }

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
                helpPopup(MainActivity.this);
            }
        });

        ((TextView)findViewById(R.id.bottomText)).setText("https://github.com/petabyt/fujiapp\n" +
                "Download location: " + Backend.getDownloads() + "\n" +
                "Beta testing release! Plz report bugs!");

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
        // Socket must be opened on WiFi - otherwise it will prefer cellular
        // TODO: Implement a timeout (If WiFi is disabled)
        Backend.jni_print("Attempting connection...\n");
        ConnectivityManager connectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkRequest.Builder requestBuilder = new NetworkRequest.Builder();
        requestBuilder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        ConnectivityManager.NetworkCallback networkCallback = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                ConnectivityManager.setProcessDefaultNetwork(network);
                if (!Conn.connect(Backend.FUJI_IP, Backend.FUJI_CMD_PORT, Backend.TIMEOUT)) {
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
