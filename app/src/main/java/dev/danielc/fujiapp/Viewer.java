// Download the image, button for share and download
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp

// TODO: prevent back arrow when downloading image

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
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.PopupWindow;
import android.widget.ProgressBar;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.jsibbold.zoomage.ZoomageView;

import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import javax.microedition.khronos.opengles.GL10;

public class Viewer extends AppCompatActivity {
    public static Handler handler = null;
    public static PopupWindow popupWindow = null;
    public static ProgressBar progressBar = null;

    public static boolean inProgress = false;

    public Bitmap bitmap = null;

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

        StrictMode.VmPolicy.Builder builder = new StrictMode.VmPolicy.Builder();
        StrictMode.setVmPolicy(builder.build());

        // This activity must be called with object handle
        Intent intent = getIntent();
        int handle = intent.getIntExtra("handle", 0);

        handler = new Handler(Looper.getMainLooper());

        // Start the popup only when activity 'key' is finished
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
                    JSONObject jsonObject = Backend.run("ptp_get_object_info", new int[]{handle});
                    if (jsonObject == null) {
                        handler.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(Viewer.this, "Failed to get file info", Toast.LENGTH_SHORT).show();
                            }
                        });
                        return;
                    }

                    String filename = jsonObject.getJSONObject("resp").getString("filename");

                    inProgress = true;
                    byte[] file = Backend.cFujiGetFile(handle);

                    bitmap = BitmapFactory.decodeByteArray(file, 0, file.length);
                    if (bitmap.getWidth() > GL10.GL_MAX_TEXTURE_SIZE) {
                        float ratio = ((float) bitmap.getHeight()) / ((float) bitmap.getWidth());
                        bitmap = Bitmap.createScaledBitmap(bitmap,
                                (int)(4096),
                                (int)((4096) * ratio),
                                false);
                    }

                    Runtime.getRuntime().gc();

                    inProgress = false;

                    handler.post(new Runnable() {
                    @Override
                        public void run() {
                            Viewer.popupWindow.dismiss();
                        }
                    });

                    if (file == null) {
                        handler.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(Viewer.this, "Failed to download", Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        handler.post(new Runnable() {
                            @Override
                            public void run() {
                                ZoomageView zoomageView = findViewById(R.id.zoom_view);
                                zoomageView.setImageBitmap(bitmap);

                                findViewById(R.id.download_button).setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        writeFile(filename, file);
                                    }
                                });

                                findViewById(R.id.share_button).setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        share(filename, file);
                                    }
                                });
                            }
                        });
                    }
                } catch (Exception e) {
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(Viewer.this, "Exception in download", Toast.LENGTH_SHORT).show();
                            Backend.print(e.toString());
                        }
                    });
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
}
