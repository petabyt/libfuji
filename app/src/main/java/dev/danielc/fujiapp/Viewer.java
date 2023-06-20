package dev.danielc.fujiapp;

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
import android.os.StrictMode;

public class Viewer extends AppCompatActivity {
    Handler handler;

    public void createDir(String directoryPath) {
        File directory = new File(directoryPath);

        if (!directory.exists()) {
            boolean created = directory.mkdirs();
            if (!created) {
                return;
            }
        }
    }

    String downloadedFile = null;

    public void writeFile(String filename, byte[] data) {
        String mainStorage = Environment.getExternalStorageDirectory().getAbsolutePath();
        String fujifilm = mainStorage + File.separator + "DCIM" + File.separator + "fuji";

        createDir(fujifilm);

        downloadedFile = fujifilm + File.separator + filename;
        File file = new File(downloadedFile);
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
        if (downloadedFile == null) {
            writeFile(filename, data);
        }

        Intent shareIntent = new Intent(Intent.ACTION_SEND);
        shareIntent.setType("image/jpeg");

        Uri imageUri = Uri.parse("file://" + downloadedFile);
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

        Intent intent = getIntent();
        int handle = intent.getIntExtra("handle", 0);

        handler = new Handler(Looper.getMainLooper());

        Toast.makeText(Viewer.this, "Downloading...", Toast.LENGTH_SHORT).show();
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
                                Toast.makeText(Viewer.this, "Loading the image", Toast.LENGTH_SHORT).show();
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
                        }
                    });
                }
            }
        });
        thread.start();
    }

//    @Override
//    public void onBackPressed() {
//        Backend.logLocation = "gallery";
//        Intent intent = new Intent(this, gallery.class);
//        this.startActivity(intent);
//    }
}