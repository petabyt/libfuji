// Download the image, button for share and download
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp

// TODO: prevent back arrow when downloading image

package dev.danielc.fujiapp;

import android.annotation.SuppressLint;
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

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import com.jsibbold.zoomage.ZoomageView;

import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import javax.microedition.khronos.opengles.GL10;

public class Viewer extends AppCompatActivity {
    public static final String TAG = "viewer";
    public static Handler handler = null;
    public static PopupWindow popupWindow = null;
    public static ProgressBar progressBar = null;

    public static boolean inProgress = false;

    public Bitmap bitmap = null;
    public String filename = null;
    public byte[] file = null;

    // Create a popup - will set popupWindow, will be closed when finished
    public ProgressBar downloadPopup(Activity activity) {
        LayoutInflater inflater = (LayoutInflater) activity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View popupView = inflater.inflate(R.layout.popup_download, null);
        popupWindow = new PopupWindow(popupView, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);

        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));

        popupWindow.showAtLocation(getWindow().getDecorView().getRootView(), Gravity.CENTER, 0, 0);

        return popupView.findViewById(R.id.progress_bar);
    }

    public void createDir(String directoryPath) {
        File directory = new File(directoryPath);
        Backend.print(directoryPath);
        if (!directory.exists()) {
            if (!directory.mkdirs()) {
                return;
            }
        }
    }

    String downloadedFilename = null;

    // Must be ran on UI thread
    public void writeFile(String filename, byte[] data) {
        String saveDir = Backend.getDownloads();
        createDir(saveDir);

        downloadedFilename = saveDir + File.separator + filename;
        File file = new File(downloadedFilename);
        FileOutputStream fos = null;

        try {
            fos = new FileOutputStream(file);
            fos.write(data);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                    Toast.makeText(Viewer.this, "Saved to " + filename, Toast.LENGTH_SHORT).show();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public void toast(String msg) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(Viewer.this, msg, Toast.LENGTH_SHORT).show();
            }
        });
    }

    public void share(String filename, byte[] data) {
        if (downloadedFilename == null) {
            writeFile(filename, data);
        }

        Intent shareIntent = new Intent(Intent.ACTION_SEND);
        shareIntent.setType("image/jpeg");

        Uri imageUri = Uri.parse("file://" + downloadedFilename);
        shareIntent.putExtra(Intent.EXTRA_STREAM, imageUri);

        Intent chooserIntent = Intent.createChooser(shareIntent, "Share image");
        chooserIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        if (chooserIntent.resolveActivity(this.getPackageManager()) != null) {
            this.startActivity(chooserIntent);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_viewer);

        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);

        StrictMode.VmPolicy.Builder builder = new StrictMode.VmPolicy.Builder();
        StrictMode.setVmPolicy(builder.build());

        // This activity must be called with object handle
        Intent intent = getIntent();
        int handle = intent.getIntExtra("handle", 0);

        handler = new Handler(Looper.getMainLooper());

        // Start the popup only when activity 'key' is finished (activity is built and ready to go)
        ViewTreeObserver viewTreeObserver = getWindow().getDecorView().getViewTreeObserver();
        viewTreeObserver.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                getWindow().getDecorView().getViewTreeObserver().removeOnGlobalLayoutListener(this);
                Viewer.progressBar = downloadPopup(Viewer.this);
            }
        });

        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    inProgress = true;
                    Log.d(TAG, "Getting object info");
                    JSONObject jsonObject = Backend.fujiGetUncompressedObjectInfo(handle);

                    filename = jsonObject.getString("filename");
                    int size = jsonObject.getInt("compressedSize");
                    int imgX = jsonObject.getInt("imgWidth");
                    int imgY = jsonObject.getInt("imgHeight");

                    if (filename.endsWith(".MOV")) {
                        inProgress = false;
                        toast("This is a MOV, not supported yet");
                        return;
                    }

                    handler.post(new Runnable() {
                        @SuppressLint({"SetTextI18n", "DefaultLocale"})
                        @Override
                        public void run() {
                            actionBar.setTitle(filename);
                            TextView tv = findViewById(R.id.fileInfo);
                            tv.setText("File size: " + String.format("%.2f", size / 1024.0 / 1024.0)
                                    + "MB\n" + "Dimensions: " + imgX + "x" + imgY);
                        }
                    });

                    file = Backend.cFujiGetFile(handle);

                    if (file == null) {
                        // IO error in downloading
                        throw new Backend.PtpErr(Backend.PTP_IO_ERR);
                    } else if (file.length == 0) {
                        // Runtime error in downloading, no error yet
                        throw new Exception("Error downloading image");
                    }

                    // Scale image to acceptable texture size
                    bitmap = BitmapFactory.decodeByteArray(file, 0, file.length);
                    if (bitmap.getWidth() > GL10.GL_MAX_TEXTURE_SIZE) {
                        float ratio = ((float) bitmap.getHeight()) / ((float) bitmap.getWidth());
                        // Will result in ~11mb tex, can do 4096, but uses 40ish megs, sometimes Android compains about OOM
                        // Might be able to increase for newer Androids
                        Bitmap newBitmap = Bitmap.createScaledBitmap(bitmap,
                            (int)(2048),
                            (int)(2048 * ratio),
                            false
                        );
                        bitmap.recycle();
                        bitmap = newBitmap;
                    }

                    inProgress = false;

                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            Viewer.popupWindow.dismiss();
                            
                            ZoomageView zoomageView = findViewById(R.id.zoom_view);
                            zoomageView.setImageBitmap(bitmap);
                        }
                    });
                } catch (Backend.PtpErr e) {
                    toast("Download IO Error: " + e.rc);
                    Backend.reportError(e.rc, "Download error");
                } catch (Exception e) {
                    toast("Download Error: " + e.toString());
                    Backend.reportError(Backend.PTP_IO_ERR, "Download error: " + e.toString());
                }
            }
        });
        thread.start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        bitmap = null;
        Runtime.getRuntime().gc();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (filename == null || file == null) {
            return true;
        }

        switch (item.getItemId()) {
            case R.id.action_download:
                writeFile(filename, file);
                return true;
            case R.id.action_share:
                share(filename, file);
                return true;
            case android.R.id.home:
                finish();
                return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.viewer, menu);
        return super.onCreateOptionsMenu(menu);
    }
}
