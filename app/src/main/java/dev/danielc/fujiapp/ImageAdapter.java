// Custom image adapter to download and show an image from byte[]
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

public class ImageAdapter extends RecyclerView.Adapter<ImageAdapter.ImageViewHolder> {
    private Context context;
    private int[] object_ids;

    public ImageAdapter(Context context, int[] object_ids) {
        this.context = context;
        this.object_ids = object_ids;
    }

    // Set up click event - navigate to viewer and send object handle
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

    // When image is scrolled into view, it will be downloaded TODO: store images in a 10mb(?) stack
    // to prevent re-downloading (slow)
    @Override
    public void onBindViewHolder(ImageViewHolder holder, int position) {
        Log.d("adapt", "Creation image");
        holder.image.setImageResource(0);
        int adapterPosition = holder.getAdapterPosition();
        if (adapterPosition >= object_ids.length) return;
        holder.handle = object_ids[adapterPosition];

        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                int id = object_ids[adapterPosition];
                byte[] jpegByteArray = Backend.cPtpGetThumb(id);
                if (jpegByteArray == null) {
                    Backend.reportError(Backend.PTP_IO_ERR, "Failed to get image thumbnail, stopping connection\n");
                    return;
                } else if (jpegByteArray.length == 0) {
                    // Unable to find thumbnail - assume it's a folder or non-jpeg
                    holder.itemView.setOnClickListener(null);
                    Backend.print("Failed to get image #" + id + "\n");
                    // TODO: reset the image to unknown or default
                    // Maybe run getobjinfo to see what it is
                    return;
                }
                try {
                    BitmapFactory.Options opt = new BitmapFactory.Options();
                    opt.inScaled = true;
                    opt.inDensity = 320;
                    opt.inTargetDensity = 160;

                    Bitmap bitmap = BitmapFactory.decodeByteArray(jpegByteArray, 0, jpegByteArray.length, opt);
                    if (bitmap == null) {
                        Backend.print("Image decode error\n");
                        return;
                    }
                    holder.itemView.post(new Runnable() {
                        @Override
                        public void run() {
                            holder.image.setImageBitmap(bitmap);
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                                holder.image.setForeground(context.getDrawable(R.drawable.ripple));
                            }
                        }
                    });
                } catch (OutOfMemoryError e) {
                    Backend.print("Out of memory\n");
                    return;
                }
            }
        });

        try {
            thread.join();
            thread.start();
        } catch (Exception e) {

        }
    }

    @Override
    public int getItemCount() {
        return object_ids.length;
    }

    public static class ImageViewHolder extends RecyclerView.ViewHolder {
        public ImageView image;
        public int handle;

        // public void setSource() {}

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
