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
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;

public class ImageAdapter extends RecyclerView.Adapter<ImageAdapter.ImageViewHolder> {
    private Context context; // TODO: leak
    private static int[] object_ids;
    public static RecyclerView recyclerView;

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

    public static class Request {
        Context ctx;
        ImageViewHolder holder;
        int position;
        int object_id;
    };

    // LIFO stack
    static ArrayList<Request> requests = new ArrayList<Request>();

    static void invalidThumb(Request req, int drawable_id) {
        req.holder.itemView.post(new Runnable() {
            @SuppressLint("UseCompatLoadingForDrawables")
            @Override
            public void run() {
                // TODO: Open a popup with some file information
                req.holder.itemView.setOnClickListener(null);
                req.holder.image.setBackground(req.ctx.getDrawable(drawable_id));
            }
        });
    }

    static boolean stop_downloading = false;

    static void requestThread() {
        while (stop_downloading == false) {
            if (requests.size() == 0) {
                try {
                    Thread.sleep(100);
                } catch (Exception e) {}
                continue;
            }
            Request req = requests.remove(requests.size() - 1);

            int id = req.object_id;
            byte[] jpegByteArray = Backend.cPtpGetThumb(id);
            if (jpegByteArray == null) {
                Backend.reportError(Backend.PTP_IO_ERR, "Failed to get thumbnail");
                return;
            } else if (jpegByteArray.length == 0) {
                // Unable to find thumbnail - assume it's a folder or non-jpeg
                invalidThumb(req, R.drawable.baseline_question_mark_24);
                continue;
            }

            try {
                BitmapFactory.Options opt = new BitmapFactory.Options();
                opt.inScaled = true;
                opt.inDensity = 320;
                opt.inTargetDensity = 160;

                Bitmap bitmap = BitmapFactory.decodeByteArray(jpegByteArray, 0, jpegByteArray.length, opt);
                if (bitmap == null) {
                    // Invalid JPEG
                    invalidThumb(req, R.drawable.baseline_question_mark_24);
                    continue;
                }
                req.holder.itemView.post(new Runnable() {
                    @SuppressLint("UseCompatLoadingForDrawables")
                    @Override
                    public void run() {
                        req.holder.image.setImageBitmap(bitmap);
                    }
                });
            } catch (OutOfMemoryError e) {
                Backend.reportError(Backend.PTP_RUNTIME_ERR, "Out of memory");
            }
        }
    }

    @Override
    public void onBindViewHolder(ImageViewHolder holder, int position) {
        holder.image.setForeground(context.getDrawable(R.drawable.ripple));
        holder.image.setBackground(context.getDrawable(R.drawable.light_button));
        holder.image.setImageBitmap(null);
        position = holder.getAdapterPosition();
        Request req = new Request();
        req.ctx = context;
        req.holder = holder;
        req.position = position;
        req.object_id = object_ids[position];
        holder.handle = req.object_id;

        requests.add(req);
    }

    @Override
    public int getItemCount() {
        return object_ids.length;
    }

    public static class ImageViewHolder extends RecyclerView.ViewHolder {
        public ImageView image;
        public int handle;

        public ImageViewHolder(@NonNull View itemView) {
            super(itemView);
            image = itemView.findViewById(R.id.imageView);
            handle = -1;
        }

        public static ImageViewHolder inflate(ViewGroup parent) {
            View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_image, parent, false);
            return new ImageViewHolder(view);
        }
    }
}
