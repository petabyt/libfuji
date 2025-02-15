package dev.danielc.fujiapp;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.MenuItem;
import android.view.View;
import android.content.SharedPreferences;
import android.widget.TextView;

public class MySettings extends AppCompatActivity {
    final static String defaultIPAddress = "192.168.0.1";
    final static String defaultPassword = "";
    final static String defaultClientName = "Fudge";

    public static String getWPA2Password() {
        return getSetting(MainActivity.instance, "wpa_password", defaultPassword);
    }

    public static String getClientName() {
        return getSetting(MainActivity.instance, "client_name", defaultClientName);
    }

    public static String getIPAddress() {
        return getSetting(MainActivity.instance, "ip_address", defaultIPAddress);
    }

    static void setSetting(Context ctx, String key, String value) {
        SharedPreferences prefs = ctx.getSharedPreferences(ctx.getPackageName(), MODE_PRIVATE);
        prefs.edit().putString(key, value).apply();
    }
    static void setSetting(Context ctx, String key, int value) {
        SharedPreferences prefs = ctx.getSharedPreferences(ctx.getPackageName(), MODE_PRIVATE);
        prefs.edit().putInt(key, value).apply();
    }
    static String getSetting(Context ctx, String key, String defaultValue) {
        SharedPreferences prefs = ctx.getSharedPreferences(ctx.getPackageName(), MODE_PRIVATE);
        return prefs.getString(key, defaultValue);
    }
    static int getSetting(Context ctx, String key, int defaultValue) {
        SharedPreferences prefs = ctx.getSharedPreferences(ctx.getPackageName(), MODE_PRIVATE);
        return prefs.getInt(key, defaultValue);
    }

    void setupTextView(TextView tv, String key, String defaultValue) {
        tv.setText(MySettings.getSetting(MySettings.this, key, defaultValue));
        tv.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }
            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                MySettings.setSetting(MySettings.this, key, tv.getText().toString());
            }
            @Override
            public void afterTextChanged(Editable editable) {

            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);
        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(R.string.settings);
        setupTextView(findViewById(R.id.ip_address_text), "ip_address", defaultIPAddress);
        setupTextView(findViewById(R.id.wpa_password), "wpa_password", defaultPassword);
        setupTextView(findViewById(R.id.client_name), "client_name", defaultClientName);
        findViewById(R.id.start_discovery).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Backend.discoveryThread(MainActivity.instance);
            }
        });
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