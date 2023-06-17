package dev.danielc.fujiapp;
import org.json.JSONObject;
import org.json.JSONArray;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

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

        int[] ids = {1, 2, 3};

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
                    Backend.run("ptp_open_session;");
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

                try {
                    Backend.run("ptp_set_property;\"Fuji_Mode\",2;");
                    Backend.run("ptp_set_property;\"Fuji_TransferMode\",2;");
                } catch (Exception e) {
                    Backend.jni_print("Failed to set modes\n");
                    return;
                }

                int[] objectHandles;
                int storageId;

                try {
                    Backend.jni_print("Getting some stuff\n");
                    JSONObject jsonObject = Backend.run("ptp_get_storage_ids;");

                    storageId = jsonObject.getJSONArray("resp").getInt(0);
                    jsonObject = Backend.run("ptp_get_storage_info;" + Integer.toString(storageId));
                    Backend.jni_print(jsonObject.toString() + "\n");

                    jsonObject = Backend.run("ptp_get_object_handles;" + String.valueOf(storageId) + "," + String.valueOf(0) + ", " + String.valueOf(Backend.PTP_OF_JPEG) + ";");
                    JSONArray respArray = jsonObject.getJSONArray("resp");
                    objectHandles = new int[respArray.length()];
                    for (int i = 0; i < respArray.length(); i++) {
                        objectHandles[i] = respArray.getInt(i);
                    }
                } catch (Exception e) {
                    Backend.jni_print("Error getting storage info: " + e.toString());
                    return;
                }

                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        imageAdapter = new ImageAdapter(gallery.this, objectHandles);
                        recyclerView.setAdapter(imageAdapter);
                    }
                });

                while (true) {
                    if (Backend.cPtpFujiPing() == 0) {
                        try {
                            Thread.sleep(1000);
                        } catch (InterruptedException e) {
                            return;
                        }
                    } else {
                        Backend.jni_print("Failed to ping, quitting.");
                        return;
                    }
                }
            }
        });
        thread.start();
    }

//    @Override
//    public void onBackPressed() {
//        Backend.logLocation = "main";
//        Intent intent = new Intent(gallery.this, MainActivity.class);
//        startActivity(intent);
//    }
}