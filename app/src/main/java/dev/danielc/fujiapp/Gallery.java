// Activity with a recycle view, populates with thumbnails - image is downloaded in Viewer activity.
// This does most of the connection initialization (maybe move somewhere else?)
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;
import org.json.JSONObject;
import org.json.JSONArray;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.view.View;
import android.widget.Toast;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.TextView;

public class Gallery extends AppCompatActivity {
    private static Gallery instance;

    public static Gallery getInstance() {
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

    void showWarning(String text) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                TextView warn_msg = findViewById(R.id.bottomDialogText);
                warn_msg.setText(text);

                View b = findViewById(R.id.bottomDialog);
                b.setVisibility(View.VISIBLE);
            }
        });

        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            return;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_gallery);
        instance = this;

        findViewById(R.id.blowup).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Thread testThread = new Thread(new Runnable() {
                    @Override
                    public void run() {
                        Backend.jni_print("Attempted to detonate internal camera time bomb: " + Backend.cTestStuff());
                    }
                });
                testThread.start();
            }
        });

        recyclerView = findViewById(R.id.galleryView);
        recyclerView.setLayoutManager(new GridLayoutManager(this, 4));

        handler = new Handler(Looper.getMainLooper());

        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                if (Backend.cPtpFujiInit() == 0) {
                    Backend.jni_print("Initialized connection.\n");
                } else {
                    Backend.jni_print("Failed to init socket\n");
                    return;
                }

                // Sleep because Fuji camera is sensitive
                try {
                    Thread.sleep(500);
                } catch (Exception e) {
                    return;
                }

                try {
                    Camera.openSession();
                } catch (Exception e) {
                    Backend.jni_print("Failed to open session.\n");
                    return;
                }

                Backend.jni_print("Waiting for device access...\n");
                if (Backend.cPtpFujiWaitUnlocked() == 0) {
                    Backend.jni_print("Gained access to device.\n");
                } else {
                    Backend.jni_print("Failed to gain access to device.");
                    return;
                }

                // Camera mode must be set before anything else
                if (Backend.cFujiConfigInitMode() != 0) {
                    Backend.jni_print("Failed to configure mode with the camera.\n");
                    return;
                }

                if (Backend.cIsMultipleMode()) {
                    showWarning("Multiple/single import is unsupported, don't expect it to work.");
                } else if (Backend.cIsUntestedMode()) {
                    showWarning("This camera is untested, don't expect it to work.");
                }

                if (Backend.cFujiConfigVersion() != 0) {
                    Backend.jni_print("Failed to configure camera function version.\n");
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
                            imageAdapter = new ImageAdapter(Gallery.this, objectHandles);
                            recyclerView.setAdapter(imageAdapter);
                        }
                    });
                }

                // After init, use this thread to ping the camera for events
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
                                Intent intent = new Intent(Gallery.this, MainActivity.class);
                                startActivity(intent);
                                // TODO: Choose between these two?
                                Toast.makeText(Gallery.this, "Failed to ping, disconnected", Toast.LENGTH_SHORT).show();
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
        // TODO: Press again to terminate connection
    }
}

