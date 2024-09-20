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
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Message;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.PopupWindow;
import android.widget.ProgressBar;
import android.widget.Switch;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;

import java.lang.ref.WeakReference;

public class Gallery extends AppCompatActivity {
    private static WeakReference<Gallery> instance = null;
    static Gallery getInstance() {
        if (instance == null) return null;
        if (instance.get() == null) return null;
        return instance.get();
    }
    private static WeakReference<ImportPopup> importPopup = null;
    static ImportPopup getImportPopup() {
        if (importPopup == null) return null;
        if (importPopup.get() == null) return null;
        return importPopup.get();
    }

    final int GRID_SIZE = 4;
    private int[] objectHandles;
    private RecyclerView recyclerView;
    private ThumbAdapter imageAdapter;
    private ListView listView;
    private ObjectInfoAdapter list;

    Handler handler;

    static class ImportPopup {
        PopupWindow window;
        View box;
        int count;
        int length;
        Thread thread;
    }

    static void stopDownloading() {
        if (getImportPopup() == null) return;
        getImportPopup().thread.interrupt();
    }

    static void downloadingFile(JSONObject oi) {
        if (getImportPopup() == null) return;
        getImportPopup().box.post(new Runnable() {
            @Override
            public void run() {
                try {
                    ((TextView)getImportPopup().box.findViewById(R.id.import_text)).setText(String.format("Downloading %s", oi.getString("filename"), getImportPopup().length));
                } catch (JSONException e) {
                    throw new RuntimeException(e);
                }
            }
        });
    }

    static void downloadedFile(String filepath) {
        ImportPopup ip = getImportPopup();
        if (ip == null) return;
        ip.count++;
        ip.box.post(new Runnable() {
            @Override
            public void run() {
                ((ProgressBar)ip.box.findViewById(R.id.import_progress)).setProgress(ip.count * 100 / ip.length);
            }
        });
    }

    public void importAll() {
        ImportPopup ip = getImportPopup();
        if (ip == null) return;
        ((Button)ip.box.findViewById(R.id.import_start_stop_button)).setText(R.string.stop_importing);
        ip.box.findViewById(R.id.import_start_stop_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                stopDownloading();
            }
        });
        int mask = 0;
        if (((Switch)ip.box.findViewById(R.id.import_switch_raws)).isChecked()) mask |= Backend.PTP_SELET_RAW;
        if (((Switch)ip.box.findViewById(R.id.import_switch_movs)).isChecked()) mask |= Backend.PTP_SELET_MOV;
        if (((Switch)ip.box.findViewById(R.id.import_switch_jpegs)).isChecked()) mask |= Backend.PTP_SELET_JPEG;
        int finalMask = mask;
        ip.thread = new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d("TAG", "isChecked: " + finalMask);
                int rc = Backend.cFujiImportFiles(objectHandles, finalMask);
                if (rc != 0) {
                    fail(rc, "Failed to import files");
                }
                ip.box.post(new Runnable() {
                    @Override
                    public void run() {
                        ImportPopup ip = getImportPopup();
                        if (ip == null) return;
                        ip.window.dismiss();
                        ip.thread = null;
                        importPopup.clear();
                    }
                });
            }
        });
        ip.thread.start();
    }

    public void importPopup() {
        ImportPopup ip = new ImportPopup();
        importPopup = new WeakReference<>(ip);
        ip.count = 0;
        ip.length = objectHandles.length;
        LayoutInflater inflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        ip.box = inflater.inflate(R.layout.popup_import, null);
        PopupWindow popupWindow = new PopupWindow(ip.box, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        ip.window = popupWindow;

        popupWindow.setOutsideTouchable(true);
        popupWindow.setFocusable(true);
        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.BLACK));

        popupWindow.showAtLocation(getWindow().getDecorView().getRootView(), Gravity.CENTER, 0, 0);

        ((Button)ip.box.findViewById(R.id.import_start_stop_button)).setText("Start");

        ip.box.findViewById(R.id.import_start_stop_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                importAll();
            }
        });
    }

    public static void setLogText(String arg) {
        if (getInstance() == null) return;
        getInstance().handler.post(new Runnable() {
            @Override
            public void run() {
                if (getInstance() == null) return;
                TextView tv = getInstance().findViewById(R.id.gallery_logs);
                if (tv == null) return;
                tv.setText(arg);
            }
        });
    }

    void fail(int code, String reason) {
        Backend.reportError(code, reason);
        finish();
    }

    static void setTitleCamName(String name) {
        if (getInstance() == null) return;
        getInstance().handler.post(new Runnable() {
            @Override
            public void run() {
                if (getInstance() == null) return;
                ActionBar actionBar = getInstance().getSupportActionBar();
                actionBar.setTitle(name);
            }
        });
    }

    // Called by JNI
    static void objectServiceUpdated(JSONObject[] handles) {
        if (getInstance() == null) return;
        if (getInstance().list == null) return;
        getInstance().list.updateList(handles);
    }

    static void pauseAll() {
        Gallery ctx = getInstance(); if (ctx == null) return;
        if (ctx.imageAdapter != null) {
            ctx.imageAdapter.queue.pause();
        }
        if (ctx.list != null) {
            ctx.list.queue.pause();
        }
    }

    static void resumeAll() {
        Gallery ctx = getInstance(); if (ctx == null) return;
        if (ctx.imageAdapter != null)
            ctx.imageAdapter.queue.start();
        if (ctx.list != null)
            ctx.list.queue.start();
    }

    void createGallery() {
        recyclerView = new RecyclerView(this);
        recyclerView.setLayoutManager(new GridLayoutManager(this, GRID_SIZE));

        imageAdapter = new ThumbAdapter(this, objectHandles);

        handler.post(new Runnable() {
            @Override
            public void run() {
                ViewGroup fileView = findViewById(R.id.fileView);
                fileView.addView(recyclerView);

                recyclerView.setAdapter(imageAdapter);
                recyclerView.setItemViewCacheSize(50);
                recyclerView.setNestedScrollingEnabled(false);
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    recyclerView.setFocusable(View.FOCUSABLE);
                }
                recyclerView.setClickable(true);
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
        list = new ObjectInfoAdapter(objectHandles, listView);
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

    Page current = null;
    void setupPage(Page to) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                if (to == Page.GALLERY && current != Page.GALLERY) {
                    if (current != null) closeList();
                    createGallery();
                    current = Page.GALLERY;
                } else if (to == Page.FILE_TABLE && current != Page.FILE_TABLE) {
                    if (current != null) closeGallery();
                    createList();
                    current = Page.FILE_TABLE;
                }
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.gallery);

        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(R.string.gallery);
        instance = new WeakReference<>(this);
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
        if (savedInstanceState == null) {
            startGalleryThread();
        }
    }

    void startGalleryThread() {
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                int rc = Backend.cFujiSetup();
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
                    Backend.cPtpObjectServiceStart(objectHandles);
                    int f = Backend.cGetTransport();
                    if (f == Backend.FUJI_FEATURE_AUTOSAVE) {
                        setupPage(Page.FILE_TABLE);
                    } else {
                        setupPage(Page.GALLERY);
                    }
                }

                // After init, use this thread to ping the camera for events
                while (true) {
                    if (Backend.cPtpFujiPing() == 0) {
                        try {
                            Thread.sleep(1000);
                        } catch (InterruptedException ignored) {}
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
        importPopup = null;
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
        return false;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        return true;
    }
}

