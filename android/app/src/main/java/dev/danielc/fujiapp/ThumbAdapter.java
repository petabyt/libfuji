// recyclerview image adapter to load images LIFO style
// Copyright 2023 Daniel C - https://github.com/petabyt/fujiapp
package dev.danielc.fujiapp;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;

public abstract class ThumbAdapter extends RecyclerView.Adapter<ThumbAdapter.ImageViewHolder> {
    Context context;
    public ThumbAdapter(Context ctx) {
        this.context = ctx;
    }

    abstract void imageClickHandler(ImageViewHolder holder);

    // Set up click event - navigate to viewer and send object handle
    @Override
    public ImageViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        ImageViewHolder holder = ImageViewHolder.create(parent);
        holder.itemView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                imageClickHandler(holder);
            }
        });
        return holder;
    }

    void invalidThumb(Context ctx, ImageViewHolder holder) {
        holder.isLoaded = false;
        holder.itemView.post(new Runnable() {
            @SuppressLint("UseCompatLoadingForDrawables")
            @Override
            public void run() {
                // TODO: Open a popup with some file information
                holder.itemView.setOnClickListener(null);
                holder.image.setBackground(ctx.getDrawable(R.drawable.baseline_question_mark_24));
            }
        });
    }

    void loadThumb(ImageViewHolder holder, byte[] data) {
        try {
            BitmapFactory.Options opt = new BitmapFactory.Options();
            opt.inScaled = true;
            opt.inDensity = 320;
            opt.inTargetDensity = 160;

            Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length, opt);
            if (bitmap == null) {
                invalidThumb(holder.image.getContext(), holder);
                return;
            }
            holder.itemView.post(new Runnable() {
                @SuppressLint("UseCompatLoadingForDrawables")
                @Override
                public void run() {
                    holder.image.setImageBitmap(bitmap);
                    holder.isLoaded = true;
                }
            });
        } catch (OutOfMemoryError e) {
            Backend.reportError(Backend.PTP_RUNTIME_ERR, "Out of memory");
        }
    }

    abstract void queueImage(ImageViewHolder holder, int position);
    abstract void cancelRequest(ImageViewHolder holder);

    @SuppressLint("UseCompatLoadingForDrawables")
    @Override
    public void onBindViewHolder(ImageViewHolder holder, int position) {
        cancelRequest(holder); // Cancel any queued requests on this holder
        holder.image.setForeground(context.getDrawable(R.drawable.ripple));
        holder.image.setBackground(context.getDrawable(R.drawable.light_button));
        holder.image.setImageBitmap(null);
        position = holder.getAdapterPosition();
        queueImage(holder, position);
    }

    public static class ImageViewHolder extends RecyclerView.ViewHolder {
        public ImageView image;
        public FrameLayout frame;
        public int handle;
        public String filename;
        public boolean isLoaded;
        public TextView label;

        public ImageViewHolder(FrameLayout frame, View itemView, TextView label) {
            super(frame);
            this.image = (ImageView)itemView;
            this.handle = -1;
            this.isLoaded = false;
            this.label = label;
        }

        public static ImageViewHolder create(ViewGroup parent) {
            FrameLayout frame = new FrameLayout(parent.getContext());
            ImageView view = new ImageView(parent.getContext());
            view.setLayoutParams(new ViewGroup.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 300));
            view.setScaleType(ImageView.ScaleType.CENTER_CROP);
            frame.addView(view);
            TextView text = new TextView(parent.getContext());
            frame.addView(text);
            return new ImageViewHolder(frame, view, text);
        }
    }
}
