// Custom image adapter to show an image from JPEG byte[]
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.Toast;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import android.util.Log;
import android.content.Intent;

import java.util.List;

public class ImageAdapter extends RecyclerView.Adapter<ImageAdapter.ImageViewHolder> {
    private Context context;
    private int[] object_ids;

    public ImageAdapter(Context context, int[] object_ids) {
        this.context = context;
        this.object_ids = object_ids;
    }

    @Override
    public ImageViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        ImageViewHolder holder = ImageViewHolder.inflate(parent);
        int position = holder.getAdapterPosition();
        holder.itemView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(context, Viewer.class);
                intent.putExtra("handle", holder.handle);
                context.startActivity(intent);
            }
        });
        return holder;
    }

    @Override
    public void onBindViewHolder(ImageViewHolder holder, int position) {
        if (position > 100) {
            return;
        }

        holder.handle = object_ids[position];

        new Thread(new Runnable() {
            @Override
            public void run() {
                int id = object_ids[position];
                byte[] jpegByteArray = Backend.cPtpGetThumb(id);
                if (jpegByteArray == null) {
                    return;
                }
                try {
                    Bitmap bitmap = BitmapFactory.decodeByteArray(jpegByteArray, 0, jpegByteArray.length);
                    if (bitmap == null) {
                        Backend.jni_print("Image decode error\n");
                    }
                    holder.itemView.post(new Runnable() {
                        @Override
                        public void run() {
                            holder.image.setImageBitmap(bitmap);
                        }
                    });
                } catch (OutOfMemoryError e) {
                    Backend.jni_print("Out of memory\n");
                }
            }
        }).start();
    }

    @Override
    public int getItemCount() {
        return object_ids.length;
    }

    public static class ImageViewHolder extends RecyclerView.ViewHolder {
        public ImageView image;
        public int handle;

        public void setSource() {

        }

        public ImageViewHolder(@NonNull View itemView) {
            super(itemView);
            image = itemView.findViewById(R.id.imageView);
        }

        public static ImageViewHolder inflate(ViewGroup parent) {
            View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_image, parent, false);
            return new ImageViewHolder(view);
        }
    }
}
