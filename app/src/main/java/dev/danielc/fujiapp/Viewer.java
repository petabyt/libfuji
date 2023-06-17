package dev.danielc.fujiapp;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.widget.Toast;
import android.graphics.Bitmap;
import com.jsibbold.zoomage.ZoomageView;


public class Viewer extends AppCompatActivity {

    Handler handler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_viewer);

        Intent intent = getIntent();
        int handle = intent.getIntExtra("handle", 0);

        handler = new Handler(Looper.getMainLooper());

        Toast.makeText(Viewer.this, "Downloading...", Toast.LENGTH_SHORT).show();
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
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