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
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.TextView;

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

    void fail(int code, String reason) {
        Backend.reportError(code, reason);
        finish();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    void setTitleCamName(String name) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                ActionBar actionBar = getSupportActionBar();
                actionBar.setTitle(name);
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.gallery);

        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(getString(R.string.gallery));
        instance = this;

        ConnectivityManager m = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);

        recyclerView = findViewById(R.id.galleryView);
        recyclerView.setLayoutManager(new GridLayoutManager(this, GRID_SIZE));
        recyclerView.setNestedScrollingEnabled(false);

        handler = new Handler(Looper.getMainLooper());
        if (Backend.cGetKillSwitch()) return;

        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                int rc = Backend.cFujiSetup(Backend.chosenIP);
                if (rc != 0) {
                    fail(rc, "Setup error");
                    return;
                }

                // SINGLE/MULTIPLE downloader fuji will gracefully kill connection (done in cFujiSetup)
                if (Backend.cGetKillSwitch()) return;

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
        instance = null;
        imageAdapter = null;
        recyclerView = null;
        super.onDestroy();
    }

    // When back pressed in gallery, do nothing
    @Override
    public void onBackPressed() {
        Backend.reportError(0, "Quitting");
        super.onBackPressed();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            fail(0, "Quitting");
            return true;
        } else if (item.getTitle() == "scripts") {
            Intent intent = new Intent(Gallery.this, Scripts.class);
            startActivity(intent);
        }

        return false;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuItem scripts = menu.add(Menu.NONE, Menu.NONE, Menu.NONE, "scripts");
        scripts.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        scripts.setIcon(R.drawable.baseline_terminal_24);
        return true;
    }
}

