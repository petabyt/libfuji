// recyclerview image adapter to load images LIFO style
// Android has an awful archaic system of doing everything so I either have to improvise
// or use the last bloat frameworks.
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.recyclerview.widget.RecyclerView;

import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;

public class FileThumbAdapter extends ThumbAdapter {
    File[] files;
    public FileThumbAdapter(Context context, String directory) {
        super(context);
        File dir = new File(directory);
        files = dir.listFiles();
    }

    void imageClickHandler(ImageViewHolder holder) {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.setDataAndType(Uri.parse(holder.filename), "image/*");
        holder.image.getContext().startActivity(intent);
    }

    @Override
    public int getItemCount() {
        if (files == null) {
            return 0;
        }
        return files.length;
    }

    void queueImage(ImageViewHolder holder, int position) {
        try {
            holder.filename = files[position].getPath();
            FileInputStream fis = new FileInputStream(files[position]);
            byte[] buffer = new byte[(int)files[position].length()];
            fis.read(buffer);
            loadThumb(holder, buffer);
            holder.isLoaded = true;
        } catch (Exception e) {
            invalidThumb(holder.image.getContext(), holder);
        }
    }
}
