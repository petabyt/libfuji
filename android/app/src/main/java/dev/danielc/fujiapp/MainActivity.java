// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.Manifest;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.StrictMode;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.ImageView;
import android.widget.TextView;
import android.provider.Settings;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.InputStream;

import dev.danielc.common.WiFiComm;
import dev.danielc.libui.*;
import 	java.util.concurrent.Semaphore;

public class MainActivity extends AppCompatActivity {
    public static MainActivity instance;
    public Handler handler;
    static WiFiComm wifi;
    public static boolean blockConnect = false;

    @Override
    public void onConfigurationChanged(android.content.res.Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        StrictMode.VmPolicy.Builder builder = new StrictMode.VmPolicy.Builder();
        StrictMode.setVmPolicy(builder.build());

        setContentView(R.layout.activity_main);
        getSupportActionBar().setTitle(getString(R.string.app_name) + " " + BuildConfig.VERSION_NAME);
        Frontend.updateLog();
        instance = this;
        handler = new Handler(Looper.getMainLooper());

        findViewById(R.id.connect_wifi).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (blockConnect || Backend.cGetKillSwitch()) return;
                Frontend.clearPrint();
                connectClick();
            }
        });

        findViewById(R.id.connect_wifi).setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                if (blockConnect || Backend.cGetKillSwitch()) return false;
                Intent intent = new Intent(MainActivity.this, Tester.class);
                startActivity(intent);
                return false;
            }
        });

        findViewById(R.id.feedback_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://discord.gg/DfRptGxGFS"));
                startActivity(browserIntent);
            }
        });

        findViewById(R.id.connect_usb).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (blockConnect || Backend.cGetKillSwitch()) return;
                Frontend.clearPrint();
                connectUSB();
            }
        });

        findViewById(R.id.help_button).setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                Intent intent = new Intent(MainActivity.this, Liveview.class);
                startActivity(intent);
                return false;
            }
        });

        findViewById(R.id.help_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, Help.class);
                startActivity(intent);
            }
        });

        findViewById(R.id.wifi_settings).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent in = new Intent(Settings.ACTION_WIFI_SETTINGS);
                in.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(in);
            }
        });

        findViewById(R.id.discovery_help).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder p = new AlertDialog.Builder(MainActivity.this);
                p.setTitle(R.string.camera_discovery_title);
                p.setMessage(R.string.camera_discovery_help);
                p.setNeutralButton("OK", null);
                p.show();
            }
        });

        wifi = new WiFiComm();

        wifi.onWiFiSelectCancel = new Runnable() {
            @Override
            public void run() {
                Frontend.print("Canceled");
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        findViewById(R.id.wifi_settings).setVisibility(View.VISIBLE);
                    }
                });
            }
        };
        wifi.onWiFiSelectAvailable = new Runnable() {
            @Override
            public void run() {
                Log.d("main", "Selection successful");
                // Time for the network to initialize - not proven needed, but probably good to have
                try {
                    Thread.sleep(1000);
                } catch (Exception e) {

                }
                if (blockConnect || Backend.cGetKillSwitch()) return;
                int rc = tryConnect(3);
                if (rc != 0) {
                    Frontend.print(R.string.connection_failed);
                }
            }
        };

        if (savedInstanceState == null) {
            // Require legacy Android write permissions
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
            }

            LibUI.buttonBackgroundResource = R.drawable.grey_button;
            LibUI.popupDrawableResource = R.drawable.border;
            Backend.init();

            wifi.setConnectivityManager((ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE));
            wifi.startNetworkListeners(this);
        }

        discoveryPopup = new AlertDialog.Builder(this);

        ViewTreeObserver viewTreeObserver = this.getWindow().getDecorView().getViewTreeObserver();
        viewTreeObserver.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                MainActivity.this.getWindow().getDecorView().getViewTreeObserver().removeOnGlobalLayoutListener(this);
                backgroundImage();
            }
        });

        Backend.discoveryThread(MainActivity.this);
    }

    void backgroundImage() {
        try {
            ImageView fl = findViewById(R.id.background_image);
            int width = fl.getWidth();
            int height = fl.getHeight();
            InputStream is = getAssets().open("pexels-thevibrantmachine-3066867.jpg");
            Bitmap bitmap = BitmapFactory.decodeStream(is);
            is.close();

            int bmpWidth = bitmap.getWidth();
            int bmpHeight = bitmap.getHeight();

            float scale = Math.max((float)width / bmpWidth, (float)height / bmpHeight);

            int newWidth = Math.round(bmpWidth * scale);
            int newHeight = Math.round(bmpHeight * scale);

            Bitmap scaledBitmap = Bitmap.createScaledBitmap(bitmap, newWidth, newHeight, true);

            Bitmap croppedBitmap = Bitmap.createBitmap(scaledBitmap, 0, 0, width, height);
            fl.setImageBitmap(croppedBitmap);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    AlertDialog.Builder discoveryPopup;

    void onCameraRegistered(String model, String name, String ip) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
                builder.setTitle("New Camera Registered: " + model);
                builder.setMessage("Use the 'PC AUTO SAVE' feature in your camera to connect to this device.");
                builder.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.cancel();
                    }
                });
                builder.setNeutralButton("Show me how", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.cancel();
                        Intent intent = new Intent(MainActivity.this, Help.class);
                        intent.putExtra("section", "autosave");
                        startActivity(intent);
                    }
                });
                builder.show();
            }
        });
    }

    /// Called by backend
    boolean onReceiveCameraInfo(String model, String name, byte[] struct) {
        Semaphore sem = new Semaphore(1);
        final boolean[] connect = {true};
        handler.post(new Runnable() {
            @Override
            public void run() {
                discoveryPopup.setTitle("Found a camera! (" + model + ")");
                discoveryPopup.setMessage("Do you want to connect?");
                discoveryPopup.show();
                discoveryPopup.setNeutralButton("Yes!", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        try {
                            sem.release();
                        } catch (Exception ignored) {}
                    }
                });
                discoveryPopup.setNegativeButton("No!", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        try {
                            connect[0] = false;
                            sem.release();
                        } catch (Exception ignored) {}
                    }
                });
            }
        });
        try {
            sem.acquire();
        } catch (Exception ignored) {}
        // TODO: return user accepts connection or not
        return connect[0];
    }

    // called by backend
    void onCameraWantsToConnect(String model, String name, byte[] struct) {
        try {
            Frontend.print("Waiting on network...");
            Thread.sleep(500); // Time for network to configure - this can probably be removed
            Backend.cConnectFromDiscovery(struct);
            Backend.cClearKillSwitch();
            handler.post(new Runnable() {
                @Override
                public void run() {
                    findViewById(R.id.wifi_settings).setVisibility(View.GONE);
                }
            });
            Intent intent = new Intent(MainActivity.this, Gallery.class);
            startActivity(intent);
        } catch (Exception e) {
            handler.post(new Runnable() {
                @Override
                public void run() {
                    findViewById(R.id.wifi_settings).setVisibility(View.VISIBLE);
                }
            });
            Frontend.print(e.getMessage());
        }
    }

    public int tryConnect(int extraTmout) {
        blockConnect = true;
        Frontend.print(getString(R.string.connecting));
        int rc = Backend.fujiConnectToCmd(extraTmout);
        blockConnect = false;
        if (rc != 0) return rc;
        Backend.cancelDiscoveryThread();
        Frontend.print("Connection established");
        handler.post(new Runnable() {
            @Override
            public void run() {
                findViewById(R.id.wifi_settings).setVisibility(View.GONE);
            }
        });
        Intent intent = new Intent(MainActivity.this, Gallery.class);
        startActivity(intent);
        return 0;
    }

    public void connectClick() {
        Context ctx = this;
        new Thread(new Runnable() {
            @Override
            public void run() {
                String password = SettingsActivity.getWPA2Password(ctx);
                if (password.length() == 0) password = null;
                int rc = tryConnect(0);
                if (rc != 0) {
                    Frontend.print(R.string.connection_failed);
                    if (wifi.connectToAccessPoint(ctx, password) != 0) {
                        // TODO: This message repeats on every connect attempt, annoying
                        Frontend.print("You must manually connect to the WiFi access point");
                        handler.post(new Runnable() {
                            @Override
                            public void run() {
                                findViewById(R.id.wifi_settings).setVisibility(View.VISIBLE);
                            }
                        });
                    }
                }
            }
        }).start();
    }

    public void connectUSB() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Backend.connectUSB(MainActivity.this);
                    Frontend.print(R.string.connection_failed);
                    Intent intent = new Intent(MainActivity.this, Gallery.class);
                    startActivity(intent);
                } catch (Exception e) {
                    Frontend.print(e.getMessage());
                }
            }
        }).start();
    }

    public static void setLogText(String arg) {
        if (instance == null) return;
       instance.handler.post(new Runnable() {
            @Override
            public void run() {
                TextView tv = instance.findViewById(R.id.error_msg);
                if (tv == null) return;
                tv.setText(arg);
            }
        });
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getTitle() == "open") {
            Intent intent = new Intent(MainActivity.this, FileGallery.class);
            startActivity(intent);
        } else if (item.getTitle() == "settings") {
            Intent intent = new Intent(MainActivity.this, SettingsActivity.class);
            startActivity(intent);
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuItem menuItem = menu.add(Menu.NONE, Menu.NONE, Menu.NONE, "open");
        menuItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        menuItem.setIcon(R.drawable.baseline_folder_open_24);

        menuItem = menu.add(Menu.NONE, Menu.NONE, Menu.NONE, "settings");
        menuItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        menuItem.setIcon(R.drawable.baseline_settings_24);

        return true;
    }
}
