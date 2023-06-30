// Download the image, button for share and download
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.util.Log;
import androidx.appcompat.app.AppCompatActivity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.Toast;
import android.graphics.Bitmap;
import com.jsibbold.zoomage.ZoomageView;
import org.json.JSONObject;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import org.json.JSONObject;
import android.os.Environment;
import android.content.Intent;
import android.net.Uri;
import android.app.Activity;
import android.content.Context;
import android.os.StrictMode;
import android.widget.ProgressBar;
import android.widget.PopupWindow;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.Gravity;
import android.widget.Button;
import android.view.ViewTreeObserver;

public class Viewer extends AppCompatActivity {
    public static Handler handler = null;
    public static PopupWindow popupWindow = null;
    public static ProgressBar progressBar = null;

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
        if (!directory.exists()) {
            if (!directory.mkdirs()) {
                return;
            }
        }
    }

    String downloadedFilename = null;

    // Must be ran on UI thread
    public void writeFile(String filename, byte[] data) {
        String fujifilm = Backend.getDownloads();
        createDir(fujifilm);

        downloadedFilename = fujifilm + File.separator + filename;
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

        // TODO: Some apps (discord) just sends a raw file (maybe needs lowercase?)
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
                    JSONObject jsonObject = Backend.run("ptp_get_object_info;" + String.valueOf(handle) + ";");
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

                    byte[] file = Backend.cFujiGetFile(handle);

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
                                Bitmap bitmap = BitmapFactory.decodeByteArray(file, 0, file.length);
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
                            Backend.jni_print(e.toString());
                        }
                    });
                }
            }
        });
        thread.start();
    }
}
