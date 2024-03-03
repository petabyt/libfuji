// Activity with a recycle view, populates with thumbnails - image is downloaded in Viewer activity.
// This does most of the connection initialization (maybe move somewhere else?)
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
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

    void setTitleCamName(String name) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                ActionBar actionBar = getSupportActionBar();
                actionBar.setTitle(getString(R.string.gallery) + ": " + name);
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_gallery);
        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(getString(R.string.gallery));
        instance = this;

        ConnectivityManager m = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);

        recyclerView = findViewById(R.id.galleryView);
        recyclerView.setLayoutManager(new GridLayoutManager(this, GRID_SIZE));
        recyclerView.setNestedScrollingEnabled(false);

        handler = new Handler(Looper.getMainLooper());

        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                int rc = Backend.cFujiSetup(Backend.chosenIP);
                if (rc != 0) {
                    fail(rc, "Setup error");
                    return;
                }

                Backend.print("Entering image gallery..");
                rc = Backend.cFujiConfigImageGallery();
                if (rc != 0) {
                    fail(rc, "Failed to start image gallery");
                    return;
                }

                int[] objectHandles = Backend.cGetObjectHandles();

                if (objectHandles == null) {
                    Backend.print(getString(R.string.noImages1));
                    Backend.print(getString(R.string.getImages2));
                } else if (objectHandles.length == 0) {
                    Backend.print("No images available.");
                } else {
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            imageAdapter = new ImageAdapter(Gallery.this, objectHandles);
                            imageAdapter.recyclerView = recyclerView;
                            recyclerView.setAdapter(imageAdapter);
                            recyclerView.setItemViewCacheSize(50);
                            recyclerView.setNestedScrollingEnabled(false);
                        }
                    });

                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            ImageAdapter.requestThread();
                        }
                    }).start();
                }

                // After init, use this thread to ping the camera for events
                while (true) {
                    if (Backend.cPtpFujiPing() == 0) {
                        try {
                            Thread.sleep(1000);
                        } catch (InterruptedException e) {}
                    } else {
                        fail(Backend.PTP_IO_ERR, "Failed to ping camera");
                        return;
                    }
                }
            }
        });
        thread.start();
    }

    @Override
    public void onDestroy() {
        imageAdapter = null;
        recyclerView = null;
        super.onDestroy();
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
            Intent intent = new Intent(Gallery.this, Scripts.class);
            startActivity(intent);
        }

        return false;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuItem menuItem = menu.add(Menu.NONE, Menu.NONE, Menu.NONE, "scripts");
        menuItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        menuItem.setIcon(R.drawable.baseline_terminal_24);

        return true;
    }
}

