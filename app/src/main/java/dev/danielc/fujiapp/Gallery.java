// Activity with a recycle view, populates with thumbnails - image is downloaded in Viewer activity.
// This does most of the connection initialization (maybe move somewhere else?)
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.PopupWindow;
import android.widget.TextView;

public class Gallery extends AppCompatActivity {
    private static Gallery instance;

    final int GRID_SIZE = 4;
    private int[] objectHandles;
    private RecyclerView recyclerView;
    private ThumbAdapter imageAdapter;
    private ListView listView;
    private PTPList list;

    Handler handler;

    static class ImportPopup {
        PopupWindow window;
        View box;
        int count;
        int length;
        Thread thread;
    }
    static ImportPopup importPopup;

    static void stopDownloading() {
        importPopup.thread.interrupt();
        importPopup.window.dismiss();
        importPopup.thread = null;
        importPopup = null;
    }

    static void downloadingFile() {
        importPopup.count++;
        importPopup.box.post(new Runnable() {
            @Override
            public void run() {
                ((TextView)importPopup.box.findViewById(R.id.import_text)).setText(String.format("Downloading %d/%d", importPopup.count, importPopup.length));
            }
        });
    }

    public void importAll() {
        ((Button)importPopup.box.findViewById(R.id.import_start_stop_button)).setText("Stop");
        importPopup.box.findViewById(R.id.import_start_stop_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                stopDownloading();
            }
        });
        importPopup.thread = new Thread(new Runnable() {
            @Override
            public void run() {
                Backend.cFujiImportFiles(objectHandles);
            }
        });
        importPopup.thread.start();
    }

    public void importPopup() {
        importPopup = new ImportPopup();
        importPopup.count = 0;
        importPopup.length = objectHandles.length;
        LayoutInflater inflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        importPopup.box = inflater.inflate(R.layout.popup_import, null);
        PopupWindow popupWindow = new PopupWindow(importPopup.box, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        importPopup.window = popupWindow;

        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));

        popupWindow.showAtLocation(getWindow().getDecorView().getRootView(), Gravity.CENTER, 0, 0);

        ((Button)importPopup.box.findViewById(R.id.import_start_stop_button)).setText("Start");

        importPopup.box.findViewById(R.id.import_start_stop_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                importAll();
            }
        });
    }

    public static void setLogText(String arg) {
        if (instance == null) return;
        instance.handler.post(new Runnable() {
            @Override
            public void run() {
                TextView tv = instance.findViewById(R.id.gallery_logs);
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

    static void setTitleCamName(String name) {
        if (instance == null) return;
        instance.handler.post(new Runnable() {
            @Override
            public void run() {
                ActionBar actionBar = instance.getSupportActionBar();
                actionBar.setTitle(name);
            }
        });
    }

    void createGallery() {
        recyclerView = new RecyclerView(this);
        recyclerView.setLayoutManager(new GridLayoutManager(this, GRID_SIZE));
        recyclerView.setNestedScrollingEnabled(false);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            recyclerView.setFocusable(View.FOCUSABLE);
        }
        recyclerView.setClickable(true);

        ViewGroup fileView = findViewById(R.id.fileView);
        fileView.addView(recyclerView);

        imageAdapter = new ThumbAdapter(this, objectHandles);

        handler.post(new Runnable() {
            @Override
            public void run() {
                recyclerView.setAdapter(imageAdapter);
                recyclerView.setItemViewCacheSize(50);
                recyclerView.setNestedScrollingEnabled(false);
            }
        });
        imageAdapter.queue.startRequestThread();
    }

    void closeGallery() {
        imageAdapter.queue.stopRequestThread();
        ((ViewGroup)recyclerView.getParent()).removeView(recyclerView);
    }

    void createList() {
        ViewGroup fileView = findViewById(R.id.fileView);
        listView = new ListView(this);
        list = new PTPList(objectHandles, listView);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT
        );
        listView.setLayoutParams(params);
        handler.post(new Runnable() {
            @Override
            public void run() {
                fileView.addView(listView);
                listView.setAdapter(list);
                list.queue.startRequestThread();
            }
        });
    }

    void closeList() {
        list.queue.stopRequestThread();
        ((ViewGroup)listView.getParent()).removeView(listView);
    }

    enum Page {
        FILE_TABLE,
        GALLERY,
        REMOTE,
    };

    void setupPage(Page to) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                if (to == Page.GALLERY) {
                    closeList();
                    createGallery();
                } else if (to == Page.FILE_TABLE) {
                    closeGallery();
                    createList();
                }
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.gallery);

        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(R.string.gallery);
        instance = this;
        handler = new Handler(Looper.getMainLooper());

        if (Backend.cGetKillSwitch()) return;

        findViewById(R.id.import_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                importPopup();
            }
        });

        findViewById(R.id.gallery_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setupPage(Page.GALLERY);
            }
        });

        findViewById(R.id.table_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setupPage(Page.FILE_TABLE);
            }
        });

        findViewById(R.id.remote_button).setEnabled(false);

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

                Frontend.print("Entering image gallery..");
                rc = Backend.cFujiConfigImageGallery();
                if (rc != 0) {
                    fail(rc, "Failed to start image gallery");
                    return;
                }

                objectHandles = Backend.cGetObjectHandles();

                if (objectHandles == null) {
                    Frontend.print(getString(R.string.noImages1));
                    Frontend.print(getString(R.string.getImages2));
                } else if (objectHandles.length == 0) {
                    Frontend.print("No images available.");
                } else {
                    createList();
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
        listView = null;
        list = null;
        imageAdapter = null;
        recyclerView = null;
        instance = null;
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
        }
//        else if (item.getTitle() == "scripts") {
//            Intent intent = new Intent(Gallery.this, Scripts.class);
//            startActivity(intent);
//        }

        return false;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
//        MenuItem scripts = menu.add(Menu.NONE, Menu.NONE, Menu.NONE, "scripts");
//        scripts.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
//        scripts.setIcon(R.drawable.baseline_terminal_24);
        return true;
    }
}

