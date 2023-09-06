// Activity with a recycle view, populates with thumbnails - image is downloaded in Viewer activity.
// This does most of the connection initialization (maybe move somewhere else?)
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;
import org.json.JSONObject;
import org.json.JSONArray;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.content.Context;
import android.net.ConnectivityManager;
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

        ConnectivityManager m = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);

        findViewById(R.id.disconnectButton).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (Backend.wifi.killSwitch) {
                    Intent intent = new Intent(Gallery.this, MainActivity.class);
                    startActivity(intent);
                } else {
                    Backend.reportError(Backend.PTP_OK, "Graceful disconnect\n");
                }
            }
        });

        recyclerView = findViewById(R.id.galleryView);
        recyclerView.setLayoutManager(new GridLayoutManager(this, 4));

        handler = new Handler(Looper.getMainLooper());

        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                int rc;
                if (Backend.cPtpFujiInit() == 0) {
                    Backend.print("Initialized connection.\n");
                } else {
                    Backend.print("Failed to init socket\n");
                    Backend.reportError(Backend.PTP_IO_ERR, "Graceful disconnect\n");
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
                    Backend.print("Failed to open session.\n");
                    Backend.reportError(Backend.PTP_IO_ERR, "Graceful disconnect\n");
                    return;
                }

                Backend.print("Waiting for device access...\n");
                if (Backend.cPtpFujiWaitUnlocked() == 0) {
                    Backend.print("Gained access to device.\n");
                } else {
                    Backend.print("Failed to gain access to device.");
                    Backend.reportError(Backend.PTP_IO_ERR, "Graceful disconnect\n");
                    return;
                }

                // Camera mode must be set before anything else
                if (Backend.cFujiConfigInitMode() != 0) {
                    Backend.print("Failed to configure mode with the camera.\n");
                    Backend.reportError(Backend.PTP_IO_ERR, "Graceful disconnect\n");
                    return;
                }

                if (Backend.cIsMultipleMode()) {
                    showWarning("Multiple/single import is unsupported, don't expect it to work.");
                } else if (Backend.cIsUntestedMode()) {
                    showWarning("This camera is untested, support is under development.");
                }

                Backend.print("Configuring versions and stuff..\n");
                rc = Backend.cFujiConfigVersion();
                if (rc != 0) {
                    Backend.print("Failed to configure camera versions.\n");
                    Backend.reportError(rc, "Graceful disconnect\n");
                    return;
                }

                // Enter (and exit?) remote mode
                if (Backend.cCameraWantsRemote()) {
                    Backend.print("Entering remote mode..");
                    rc = Backend.cFujiTestStartRemoteSockets();
                    if (rc != 0) {
                        Backend.print("Failed to init remote mode\n");
                        Backend.reportError(rc, "Graceful disconnect\n");
                        return;
                    }

                    if (Backend.wifi.fujiConnectEventAndVideo(m)) {
                        Backend.print("Failed to enter remote mode\n");
                        Backend.reportError(Backend.PTP_RUNTIME_ERR, "Graceful disconnect\n");
                        return;
                    } else {
                        Backend.print("Entered remote mode");
                    }

                    rc = Backend.cFujiTestEndRemoteMode();
                    if (rc != 0) {
                        Backend.print("Failed to exit remote mode");
                        Backend.reportError(rc, "Graceful disconnect\n");
                        return;
                    }
                }

                int[] objectHandles = Backend.cGetObjectHandles();

                if (objectHandles == null) {
                    Backend.print("No JPEG images available.\n");
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
                                Backend.reportError(Backend.PTP_IO_ERR, "Failed to ping camera\n");
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

