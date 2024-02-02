// WIP util lib
package libui;

import android.Manifest;
import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.widget.Toast;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Scanner;

public class LibU {
    public static void createDir(String directoryPath) {
        File directory = new File(directoryPath);
        if (!directory.exists()) {
            directory.mkdirs();
        }
    }

    public static void toast(Context ctx, String arg) {
        Toast.makeText(ctx, arg, Toast.LENGTH_SHORT).show();
    }

    public static void shareJpeg(Context ctx, String path, String action) {
        Intent shareIntent = new Intent(Intent.ACTION_SEND);
        shareIntent.setType("image/jpeg");

        Uri imageUri = Uri.parse("file://" + path);
        shareIntent.putExtra(Intent.EXTRA_STREAM, imageUri);

        Intent chooserIntent = Intent.createChooser(shareIntent, action);
        chooserIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        if (chooserIntent.resolveActivity(ctx.getPackageManager()) != null) {
            ctx.startActivity(chooserIntent);
        }
    }

    public static void writeFile(String path, byte[] data) throws Exception {
        File file = new File(path);
        FileOutputStream fos = null;

        try {
            fos = new FileOutputStream(file);
            fos.write(data);
        } catch (IOException e) {
            e.printStackTrace();
            throw e;
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                    throw e;
                }
            }
        }
    }

    public void getFilePermissions(Activity ctx) {
        // Require legacy Android write permissions
        if (ContextCompat.checkSelfPermission(ctx, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(ctx, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
        }
    }

    public static String getDCIM() {
        String mainStorage = Environment.getExternalStorageDirectory().getAbsolutePath();
        return mainStorage + File.separator + "DCIM" + File.separator;
    }

    public static String getDocuments() {
        String mainStorage = Environment.getExternalStorageDirectory().getAbsolutePath();
        return mainStorage + File.separator + "Documents" + File.separator;
    }

    public static JSONObject getJSONSettings(Activity ctx, String key) throws Exception {
        SharedPreferences prefs = ctx.getSharedPreferences(ctx.getPackageName(), Context.MODE_PRIVATE);

        String value = prefs.getString(ctx.getPackageName() + "." + key, null);
        if (value == null) return null;

        return new JSONObject(value);
    }

    public static void storeJSONSettings(Activity ctx, String key, String value) throws Exception {
        SharedPreferences prefs = ctx.getSharedPreferences(ctx.getPackageName(), Context.MODE_PRIVATE);
        prefs.edit().putString(ctx.getPackageName() + "." + key, value).apply();
    }

    public static byte[] readFileFromAssets(Context ctx, String file) throws Exception {
        try {
            InputStream inputStream = ctx.getAssets().open(file);
            byte buffer[] = new byte[inputStream.available()];
            inputStream.read(buffer);
            inputStream.close();
            return buffer;
        } catch (IOException e) {
            e.printStackTrace();
            throw e;
        }
    }
}