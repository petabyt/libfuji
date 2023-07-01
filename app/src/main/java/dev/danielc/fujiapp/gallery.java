// Activity with a recycle view, populates with thumbnails - image is downloaded in Viewer activity.
// This does most of the connection initialization (maybe move somewhere else?)
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;
import org.json.JSONObject;
import org.json.JSONArray;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import android.widget.Toast;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.TextView;

public class gallery extends AppCompatActivity {
    private static gallery instance;

    public static gallery getInstance() {
        return instance;
    }

    private RecyclerView recyclerView;
    private ImageAdapter imageAdapter;

    Handler handler;

    public void setErrorText(String arg) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                TextView error_msg = findViewById(R.id.gallery_logs);
                error_msg.setText(arg);
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_gallery);
        instance = this;

        recyclerView = findViewById(R.id.galleryView);
        recyclerView.setLayoutManager(new GridLayoutManager(this, 4));

        handler = new Handler(Looper.getMainLooper());

        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                if (Backend.cPtpFujiInit() == 0) {
                    Backend.jni_print("Successfully initialized command socket\n");
                } else {
                    Backend.jni_print("Failed to init socket\n");
                    return;
                }

                try {
                    Backend.run("ptp_open_session");
                } catch (Exception e) {
                    Backend.jni_print("Failed to open session\n");
                    return;
                }

                try {
                    Backend.jni_print("Fuji_Mode: " + Backend.cPtpGetPropValue(0xdf01) + "\n");
                    Backend.jni_print("Fuji_TransferMode: " + Backend.cPtpGetPropValue(0xdf22) + "\n");
                    Backend.jni_print("Fuji_Unlocked: " + Backend.cPtpGetPropValue(0xdf00) + "\n");
                } catch (Exception e) {
                    Backend.jni_print("Failed to get some access info\n");
                }

                Backend.jni_print("Waiting for device access...\n");
                if (Backend.cPtpFujiWaitUnlocked() == 0) {
                    Backend.jni_print("Gained access to device.\n");
                } else {
                    Backend.jni_print("Failed to gain access to device.");
                    return;
                }

                // Camera mode must be set before anything else
                if (Backend.cFujiConfigFileTransfer() != 0) {
                    Backend.jni_print("Failed to initiate file transfer with camera.\n");
                    return;
                }

                if (Backend.cFujiConfigVersion() != 0) {
                    Backend.jni_print("Failed to configure protocol version.\n");
                    return;
                }

                int[] objectHandles;
                int storageId;

                try {
                    JSONObject jsonObject = Backend.run("ptp_get_storage_ids", new int[]{});
                    storageId = jsonObject.getJSONArray("resp").getInt(0);
                } catch (Exception e) {
                    Backend.jni_print("Failed to detect camera SD card! (" + e.toString() + ")\n");
                    return;
                }

                try {
                    JSONObject jsonObject = Backend.run("ptp_get_object_handles", new int[]{storageId, 0, Backend.PTP_OF_JPEG});

                    JSONArray resp = jsonObject.getJSONArray("resp");
                    objectHandles = new int[resp.length()];
                    for (int i = 0; i < resp.length(); i++) {
                        objectHandles[i] = resp.getInt(i);
                    }
                } catch (Exception e) {
                    Backend.jni_print("Falied to find images on the SD card! (" + e.toString() + ")\n");
                    return;
                }

                if (objectHandles.length == 0) {
                    Backend.jni_print("No JPEG images available. Might figure this out in the future. :)\n");
                } else {
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            imageAdapter = new ImageAdapter(gallery.this, objectHandles);
                            recyclerView.setAdapter(imageAdapter);
                        }
                    });                    
                }

                // Use this thread to ping the camera for events
                while (true) {
                    if (Backend.cPtpFujiPing() == 0) {
                        try {
                            Thread.sleep(1000);
                        } catch (InterruptedException e) {
                            return;
                        }
                    } else {
                        handler.post(new Runnable() {
                        @Override
                            public void run() {
                                Intent intent = new Intent(gallery.this, MainActivity.class);
                                startActivity(intent);
                                // TODO: Choose between these two?
                                Toast.makeText(gallery.this, "Failed to ping, disconnected", Toast.LENGTH_SHORT).show();
                                Backend.jni_print("Disconnected.\n");
                            }
                        });
                        return;
                    }
                }
            }
        });
        thread.start();
    }

    // When back pressed in gallery, do nothing
    @Override
    public void onBackPressed() {
        //super.onBackPressed();
    }
}

