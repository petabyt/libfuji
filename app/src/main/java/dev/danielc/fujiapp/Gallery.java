// Activity with a recycle view, populates with thumbnails - image is downloaded in Viewer activity.
// This does most of the connection initialization (maybe move somewhere else?)
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.content.Context;
import android.net.ConnectivityManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.TextView;

import libui.LibUI;

public class Gallery extends AppCompatActivity {
    public static Gallery instance;

    final int GRID_SIZE = 4;

    private RecyclerView recyclerView;
    private ImageAdapter imageAdapter;

    Handler handler;

    public void setLogText(String arg) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                TextView tv = findViewById(R.id.gallery_logs);
                if (tv == null) return;
                tv.setText(arg);
            }
        });
    }

    void showWarning(String text) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                TextView warn_msg = findViewById(R.id.bottomDialogText);
                warn_msg.setText(text);

                View b = findViewById(R.id.bottomDialog);
                b.setVisibility(View.VISIBLE);
            }
        });

        // Give time for the warning to show and warn the user (in case it breaks)
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            return;
        }
    }

    void fail(int code, String reason) {
        if (Backend.cGetKillSwitch()) return;
        Backend.reportError(code, reason);
        handler.post(new Runnable() {
            @Override
            public void run() {
                Gallery.this.finish();
            }
        });
    }

    @Override
    protected void onResume() {
        LibUI.start(this);
        super.onResume();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_gallery);
        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle("Gallery");
        instance = this;
        LibUI.start(this);

        ConnectivityManager m = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);

        recyclerView = findViewById(R.id.galleryView);
        recyclerView.setLayoutManager(new GridLayoutManager(this, GRID_SIZE));

        handler = new Handler(Looper.getMainLooper());

        // If kill switch off, invalid state, finish()

        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                int rc;
                if (Backend.cPtpFujiInit() == 0) {
                    Backend.print("Initialized connection.");
                } else {
                    fail(Backend.PTP_IO_ERR, "Failed to init socket");
                    return;
                }

                String camName = Backend.cPtpFujiGetName();
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        actionBar.setTitle("Gallery: " + camName);
                    }
                });

                // Fuji cameras require delay after init
                try {
                    Thread.sleep(500);
                } catch (Exception e) {
                    return;
                }

                try {
                    Backend.cPtpOpenSession();
                } catch (Exception e) {
                    fail(Backend.PTP_IO_ERR, "Failed to open session.");
                    return;
                }

                Backend.print("Waiting for device access...");
                if (Backend.cPtpFujiWaitUnlocked() == 0) {
                    Backend.print("Gained access to device.");
                } else {
                    fail(Backend.PTP_IO_ERR, "Failed to gain access to device.");
                    return;
                }

                // Camera mode must be set before anything else
                if (Backend.cFujiConfigInitMode() != 0) {
                    fail(Backend.PTP_IO_ERR, "Failed to configure mode with the camera.");
                    return;
                }

                if (Backend.cIsMultipleMode()) {
                    showWarning("View multiple mode in development");
                    rc = Backend.cFujiDownloadMultiple();
                    if (rc != 0) {
                        fail(rc, "Error importing images");
                        return;
                    }
                    Backend.print("Check your file manager app/gallery.");
                    return;
                }

                if (Backend.cIsUntestedMode()) {
                    showWarning("Support for this camera is under development.");
                }

                Backend.print("Configuring versions and stuff..");
                rc = Backend.cFujiConfigVersion();
                if (rc != 0) {
                    fail(rc, "Failed to configure camera versions.");
                    return;
                }

                // Enter and 'exit' remote mode
                if (Backend.cCameraWantsRemote()) {
                    Backend.print("Entering remote mode..");
                    rc = Backend.cFujiTestStartRemoteSockets();
                    if (rc != 0) {
                        fail(rc, "Failed to init remote mode");
                        return;
                    }

                    try {
                        Backend.fujiConnectEventAndVideo();
                    } catch (Exception e) {
                        fail(Backend.PTP_RUNTIME_ERR, "Failed to enter remote mode");
                        return;
                    }

                    rc = Backend.cFujiEndRemoteMode();
                    if (rc != 0) {
                        fail(rc, "Failed to exit remote mode");
                        return;
                    }
                }

                Backend.print("Entering image gallery..");
                rc = Backend.cFujiConfigImageGallery();
                if (rc != 0) {
                    fail(rc, "Failed to start image gallery");
                    return;
                }

                int[] objectHandles = Backend.cGetObjectHandles();

                if (objectHandles == null) {
                    Backend.print("No JPEG images available.");
                    Backend.print("Maybe you only have RAW files? Fuji doesn't let us view RAW over WiFi :(");
                } else {
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            imageAdapter = new ImageAdapter(Gallery.this, objectHandles);
                            recyclerView.setAdapter(imageAdapter);
                            recyclerView.setItemViewCacheSize(50);
                            recyclerView.setNestedScrollingEnabled(false);
                        }
                    });
                }

                // After init, use this thread to ping the camera for events
                while (true) {
                    if (Backend.cPtpFujiPing() == 0) {
                        try {
                            Thread.sleep(1000);
                        } catch (InterruptedException e) {
                            return;
                        }
                    } else {
                        fail(Backend.PTP_IO_ERR, "Failed to ping camera");
                        return;
                    }
                }
            }
        });
        thread.start();
    }

    // When back pressed in gallery, do nothing
    @Override
    public void onBackPressed() {
        if (LibUI.handleBack(true)) {
            Backend.reportError(0, "Quitting");
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            if (LibUI.handleOptions(item, true)) {
                Backend.reportError(0, "Quitting");
                return true;
            }
        } else if (item.getTitle() == "scripts") {
            Backend.cFujiScriptsScreen(this);
        }

        return false;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuItem menuItem = menu.add(Menu.NONE, Menu.NONE, Menu.NONE, "scripts");
        menuItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        menuItem.setIcon(R.drawable.baseline_terminal_24);

        return LibUI.handleMenu(menu);
    }
}

