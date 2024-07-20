// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.Manifest;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.widget.TextView;
import android.provider.Settings;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;
import java.io.InputStream;

import camlib.WiFiComm;
import dev.danielc.libui.*;

public class MainActivity extends AppCompatActivity {
    public static MainActivity instance;
    Handler handler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        instance = this;
        handler = new Handler(Looper.getMainLooper());

        getSupportActionBar().setTitle("Fudge " + BuildConfig.VERSION_NAME);

        Backend.init();
        Backend.updateLog();

        LibUI.buttonBackgroundResource = R.drawable.grey_button;
        LibUI.popupDrawableResource = R.drawable.border;

        findViewById(R.id.connect_wifi).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Backend.clearPrint();
                connectClick();
            }
        });

        findViewById(R.id.connect_wifi).setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                Intent intent = new Intent(MainActivity.this, Tester.class);
                startActivity(intent);
                return false;
            }
        });

        findViewById(R.id.connect_usb).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Backend.clearPrint();
                connectUSB();
            }
        });

//        findViewById(R.id.plugins).setOnClickListener(new View.OnClickListener() {
//            @Override
//            public void onClick(View v) {
//                Intent intent = new Intent(MainActivity.this, Scripts.class);
//                startActivity(intent);
//            }
//        });
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

//        findViewById(R.id.plugins).setOnClickListener(new View.OnClickListener() {
//            @Override
//            public void onClick(View v) {
//                Intent intent = new Intent(MainActivity.this, Scripts.class);
//                startActivity(intent);
//            }
//        });

        // Require legacy Android write permissions
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
        }

        WiFiComm.setConnectivityManager((ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE));

        // Idea: Show WiFi status on screen?
        // Show/hide discovery message
        WiFiComm.startNetworkListeners(this);
        Context ctx = this;
        WiFiComm.onAvailable = new Runnable() {
            @Override
            public void run() {
                Backend.discoveryThread(ctx);
            }
        };

        discoveryPopup = new AlertDialog.Builder(MainActivity.this);

        try {
            // https://www.pexels.com/photo/photo-of-vehicle-on-asphalt-road-3066867/
            InputStream is = getAssets().open("pexels-thevibrantmachine-3066867.jpg");
            int size = is.available();
            byte[] buffer = new byte[size];
            is.read(buffer);
            is.close();
            Bitmap bitmap = BitmapFactory.decodeByteArray(buffer, 0, buffer.length, null);
            Bitmap resizedBmp = Bitmap.createBitmap(bitmap, 0, 0, 1080*3, bitmap.getHeight());
            ImageView fl = findViewById(R.id.imageView2);
            fl.setImageBitmap(resizedBmp);
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

    void onReceiveCameraInfo(String model, String name, String ip) {
        discoveryPopup.setTitle("Found a camera! (" + model + ")");
        discoveryPopup.setMessage("Do you want to connect?");
        discoveryPopup.show();
//        return bool accept connection or not
    }

    void onCameraWantsToConnect(String model, String name, String ip, int port) {
        try {
            Thread.sleep(1000);
            Backend.chosenIP = ip;
            Backend.cConnectNative(ip, Backend.FUJI_CMD_PORT);
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
            Backend.print(e.getMessage());
        }
    }

    public int tryConnect() {
        int rc = Backend.fujiConnectToCmd();
        if (rc != 0) return rc;
        Backend.print("Connection established");
        Backend.cancelDiscoveryThread();
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
        WiFiComm.onWiFiSelectCancel = new Runnable() {
            @Override
            public void run() {
                Backend.print("Selection canceled");
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        findViewById(R.id.wifi_settings).setVisibility(View.VISIBLE);
                    }
                });
            }
        };
        WiFiComm.onWiFiSelectAvailable = new Runnable() {
            @Override
            public void run() {
                try {
                    Backend.fujiConnectToCmd();
                } catch (Exception e) {
                    Backend.print(e.getMessage());
                }
            }
        };
        Context ctx = this;
        new Thread(new Runnable() {
            @Override
            public void run() {
                int rc = tryConnect();
                if (rc != 0) {
                    if (WiFiComm.connectToAccessPoint(ctx, null) != 0) {
                        Backend.print("You must manually connect to the WiFi access point");
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
                    Backend.print("Connection established");
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

    void openFiles() {
        // TODO: this sucks
        File[] fileList;
        File file = new File(Backend.getDownloads());
        if (!file.isDirectory()) {
            return;
        }

        fileList = file.listFiles();
        String mime = "*/*";
        String path = Backend.getDownloads();
        if (fileList.length != 0) {
            path = fileList[0].getPath();
            mime = "image/*";
        }

//        if (Viewer.filename != null) {
//            path = Viewer.filename;
//            mime = "image/*";
//        }

        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.setDataAndType(Uri.parse(path), mime);
        startActivity(intent);
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getTitle() == "open") {
            openFiles();
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
