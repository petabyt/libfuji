// recyclerview image adapter to load images LIFO style
// Android has an awful archaic system of doing everything so I either have to improvise
// or use the last bloat frameworks.
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import java.io.File;

import dev.danielc.common.Exif;
import dev.danielc.views.ThumbAdapter;

public class FileThumbAdapter extends ThumbAdapter {
    File[] files;
    public FileThumbAdapter(Context context, String directory) {
        super(context);
        File dir = new File(directory);
        files = dir.listFiles();
    }

    public void imageClickHandler(ImageViewHolder holder) {
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

    @Override
    public void queueImage(ImageViewHolder holder, int position) {
        holder.image.post(new Runnable() {
            @Override
            public void run() {
                try {
                    holder.filename = files[position].getPath();
                    byte[] buffer = Exif.getThumbnail(holder.filename);
                    if (buffer == null) invalidThumb(holder.image.getContext(), holder);
                    loadThumb(holder, buffer);
                    holder.isLoaded = true;
                } catch (Exception e) {
                    invalidThumb(holder.image.getContext(), holder);
                }
            }
        });
    }

    @Override
    public void cancelRequest(ImageViewHolder holder) {}
}
