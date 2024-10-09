// Download the image, button for share and download
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp

package dev.danielc.fujiapp;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.StrictMode;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.PopupWindow;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.media.MediaScannerConnection;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import com.jsibbold.zoomage.ZoomageView;

import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.Locale;

import javax.microedition.khronos.opengles.GL10;

import dev.danielc.common.WiFiComm;

public class Viewer extends AppCompatActivity {
    public static final String TAG = "viewer";
    public Handler handler = null;
    static WeakReference<PopupWindow> popupWindowInstance = null;

    public Bitmap bitmap = null;
    public String filename = null;
    public byte[] fileByteData = null;
    public boolean notEnoughMemoryToPreview = false;
    public boolean fileIsDownloaded = false;
    public boolean threadIsDone = false;

    private static PopupWindow getPopup() {
        if (popupWindowInstance == null) return null;
        if (popupWindowInstance.get() == null) return null;
        return popupWindowInstance.get();
    }

    static void notifyDownloadSpeed(int mbps) {
        PopupWindow popupWindow = getPopup();
        if (popupWindow == null) return;
        View c = popupWindow.getContentView();
        c.post(new Runnable() {
            @Override
            public void run() {
                if (WiFiComm.isWiFiModuleHandlingTwoConnections(c.getContext()) && mbps < 12) {
                    ((TextView)c.findViewById(R.id.download_warning_text)).setVisibility(View.VISIBLE);
                    ((TextView)c.findViewById(R.id.download_warning_text)).setText(R.string.hint_disconnect_from_main_network);
                }
                ((TextView)c.findViewById(R.id.download_speed)).setText(String.format(Locale.US, "%dmbps", mbps));
            }
        });
    }

    static void notifyDownloadProgress(int percent) {
        PopupWindow popupWindow = getPopup();
        if (popupWindow == null) return;
        View c = popupWindow.getContentView();
        c.post(new Runnable() {
            @Override
            public void run() {
                ((ProgressBar)c.findViewById(R.id.progress_bar)).setProgress(percent);
            }
        });
    }

    void fail(int code, String reason) {
        Backend.reportError(code, reason);
        finish();
    }

    // Create a popup - will set popupWindow, will be closed when finished
    public void downloadPopup(Activity activity) {
        LayoutInflater inflater = (LayoutInflater) activity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View popupView = inflater.inflate(R.layout.popup_download, null);
        PopupWindow popupWindow = new PopupWindow(popupView, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        popupWindow.showAtLocation(getWindow().getDecorView().getRootView(), Gravity.CENTER, 0, 0);
        popupWindowInstance = new WeakReference<>(popupWindow);
    }

    // Notify gallery app that there is a new image
    public void scanImage(String path) {
        Log.d(TAG, "Notifying media scanner of " + path);
        MediaScannerConnection.scanFile(this, new String[] {path}, null, null);
    }

    // Must be ran on UI thread
    public void writeFile() {
        if (fileByteData == null) return;
        String saveDir = Backend.getDownloads();
        String path = saveDir + File.separator + filename;
        File file = new File(path);
        FileOutputStream fos = null;

        try {
            fos = new FileOutputStream(file);
            fos.write(fileByteData);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                    this.scanImage(path);
                    Toast.makeText(Viewer.this, "Saved to " + filename, Toast.LENGTH_SHORT).show();
                    fileIsDownloaded = true;
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public void toast(String msg) {
        if (handler == null) return;
        handler.post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(Viewer.this, msg, Toast.LENGTH_SHORT).show();
            }
        });
    }

    public void share() {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.setDataAndType(Uri.parse("file://" + Backend.getDownloads() + File.separator + filename), "image/jpeg");
        this.startActivity(intent);
    }

    public void downloadAndShare() {
        if (!fileIsDownloaded) {
            writeFile();
        }
        share();
    }

    void downloadFileManually(int handle, int size) {
        String saveDir = Backend.getDownloads();

        String path = saveDir + File.separator + filename;
        int rc = Backend.cFujiDownloadFile(handle, path);
        if (rc != 0) {
            fail(rc, "Failed to download file to storage.");
        }

        fileIsDownloaded = true;
        scanImage(path);

        toast("Saved to " + path);
    }

    ActionBar actionBar;
    Thread thread = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Gallery.pauseAll();
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_viewer);

        actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);

        // ???
        StrictMode.VmPolicy.Builder builder = new StrictMode.VmPolicy.Builder();
        StrictMode.setVmPolicy(builder.build());

        // This activity must be called with object handle
        Intent intent = getIntent();
        int handle = intent.getIntExtra("handle", 0);

        handler = new Handler(Looper.getMainLooper());

        ViewTreeObserver viewTreeObserver = getWindow().getDecorView().getViewTreeObserver();
        viewTreeObserver.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                getWindow().getDecorView().getViewTreeObserver().removeOnGlobalLayoutListener(this);
                downloadPopup(Viewer.this);
                thread = new Thread(new Runnable() {
                    @Override
                    public void run() {
                        loadThumb(handle);
                    }
                });
                thread.start();
            }
        });
    }

    private void loadThumb(int handle) {
        try {
            String t = Backend.cFujiBeginDownloadGetObjectInfo(handle);
            if (t == null) {
                fail(Backend.PTP_IO_ERR, "Failed to get object info");
                return;
            }
            JSONObject jsonObject = new JSONObject(t);

            if (Thread.interrupted()) { return; }

            filename = jsonObject.getString("filename");
            int size = jsonObject.getInt("compressedSize");
            int imgX = jsonObject.getInt("imgWidth");
            int imgY = jsonObject.getInt("imgHeight");

            if (filename.endsWith(".MOV")) {
                toast("MOV playback not supported yet");
                threadIsDone = true;
                return;
            }

            handler.post(new Runnable() {
                @Override
                public void run() {
                    actionBar.setTitle(filename);
                    TextView tv = findViewById(R.id.fileInfo);
                    tv.setText(String.format(getString(R.string.filesize) + ": %.2fMB\n", size / 1024.0 / 1024.0));
                    tv.append(String.format(getString(R.string.dimensions) + ": %dx%d\n", imgX, imgY));
                }
            });

            try {
                fileByteData = new byte[size];
            } catch (OutOfMemoryError e) {
                toast("Not enough memory to preview file");
                notEnoughMemoryToPreview = true;
                downloadFileManually(handle, size);
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        PopupWindow popupWindow = getPopup();
                        if (popupWindow == null) return;
                        popupWindow.dismiss();
                        threadIsDone = true;
                    }
                });
                return;
            }

            if (Thread.interrupted()) { return; }

            int rc = Backend.cFujiGetFile(handle, fileByteData, size);
            if (rc == Backend.PTP_CHECK_CODE) {
                toast("Can't download this file");
                finish();
                threadIsDone = true;
                return;
            } else if (rc == Backend.PTP_CANCELED) {
                toast("Canceled");
                finish();
                threadIsDone = true;
                return;
            } else if (rc != 0) {
                fail(Backend.PTP_IO_ERR, "Failed to download image");
                threadIsDone = true;
                return;
            }

            if (Thread.interrupted()) { return; }

            BitmapFactory.Options options = new BitmapFactory.Options();
            if (imgX > GL10.GL_MAX_TEXTURE_SIZE) {
                Log.d("viewer", "Scaling image 2/2/2");
                options.inSampleSize = 2;
                options.inDensity = 2;
                options.inTargetDensity = 2;
                options.inScaled = true;
            }
//            if (size > 15000000) {
//                Log.d("viewer", "Scaling image 8/8/4");
//                options.inSampleSize = 8;
//                options.inDensity = 8;
//                options.inTargetDensity = 4;
//                options.inScaled = true;
//            }

            if (Thread.interrupted()) { return; }

            bitmap = BitmapFactory.decodeByteArray(fileByteData, 0, fileByteData.length, options);

            // Resizing didn't go as expected, we need to scale the bitmap again
            if (bitmap.getWidth() > GL10.GL_MAX_TEXTURE_SIZE) {
                Log.d("viewer", "Image is still too large, downsizing");
                float ratio = ((float) bitmap.getHeight()) / ((float) bitmap.getWidth());
                Bitmap newBitmap = Bitmap.createScaledBitmap(bitmap, (int)(2048), (int)(2048 * ratio), false);
                bitmap.recycle();
                bitmap = newBitmap;
            }

            if (Thread.interrupted()) { return; }

            handler.post(new Runnable() {
                @Override
                public void run() {
                    PopupWindow popupWindow = getPopup();
                    if (popupWindow == null) return;
                    popupWindow.dismiss();
                    ZoomageView zoomageView = findViewById(R.id.zoom_view);
                    zoomageView.setImageBitmap(bitmap);
                    threadIsDone = true;
                }
            });
        } catch (Exception e) {
            e.printStackTrace();
            fail(Backend.PTP_IO_ERR, e.getMessage());
        }
    }

    @Override
    protected void onDestroy() {
        actionBar = null;
        if (bitmap != null) {
            bitmap.recycle();
            bitmap = null;
        }
        PopupWindow popupWindow = getPopup();
        if (popupWindow == null) return;
        popupWindow.dismiss();
        popupWindowInstance.clear();
        //Runtime.getRuntime().gc();
        handler = null;
        super.onDestroy();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_download:
                if (notEnoughMemoryToPreview) {
                    toast(getString(R.string.alreadydownloaded));
                } else {
                    if (!threadIsDone) return true;
                    writeFile();
                }
                return true;
            case R.id.action_share:
                if (notEnoughMemoryToPreview) {
                    share();
                } else {
                    downloadAndShare();
                }
                return true;
            case android.R.id.home:
                thread.interrupt();
                Gallery.resumeAll();
                finish();
                return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onBackPressed() {
        thread.interrupt();
        Gallery.resumeAll();
        super.onBackPressed();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.viewer, menu);
        return super.onCreateOptionsMenu(menu);
    }
}
